#define _DARWIN_C_SOURCE
#define _XOPEN_SOURCE 700
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "catalog.h"

static char root[PATH_MAX];
static volatile sig_atomic_t interrupted;

typedef struct { int code, tle, ole; } Process;
typedef struct { char verdict[16], sha[65]; uint64_t hash; } Progress;

_Noreturn static void fail(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
    exit(2);
}

static void pathf(char *out, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(out, PATH_MAX, fmt, args);
    va_end(args);
    if (n < 0 || n >= PATH_MAX) fail("Path too long.");
}

static void mkdir_ok(const char *path) {
    if (mkdir(path, 0755) && errno != EEXIST) fail("Cannot create %s: %s", path, strerror(errno));
}

static void remove_tree(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) { unlink(path); return; }
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        char child[PATH_MAX];
        struct stat st;
        pathf(child, "%s/%s", path, entry->d_name);
        if (!lstat(child, &st) && S_ISDIR(st.st_mode)) remove_tree(child);
        else unlink(child);
    }
    closedir(dir);
    rmdir(path);
}

static double now_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts)) fail("Cannot read clock.");
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

static void on_signal(int sig) { interrupted = sig; }

_Noreturn static void child_error(const char *operation) {
    dprintf(STDERR_FILENO, "%s: %s\n", operation, strerror(errno));
    _exit(126);
}

/* No shell. Each command gets a process group so a timeout also kills children. */
static Process run(char *const cmd[], const char *cwd, int input, int output,
                   int errors, int timeout_ms, int limits) {
    fflush(NULL);
    pid_t pid = fork();
    if (pid < 0) fail("Cannot launch process: %s", strerror(errno));
    if (!pid) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        if (input >= 0 && dup2(input, STDIN_FILENO) < 0) child_error("Redirect input");
        if (output >= 0 && dup2(output, STDOUT_FILENO) < 0) child_error("Redirect output");
        if (errors >= 0 && dup2(errors, STDERR_FILENO) < 0) child_error("Redirect errors");
        /* The parent may have already made us the group leader (macOS can return EPERM). */
        if (setpgid(0, 0) && getpgrp() != getpid()) child_error("Create process group");
        if (cwd && chdir(cwd)) child_error("Open working directory");
        if (limits) {
            struct rlimit file_limit = {OUTPUT_LIMIT, OUTPUT_LIMIT};
            struct rlimit no_core = {0, 0};
            if (setrlimit(RLIMIT_FSIZE, &file_limit)) child_error("Set output limit");
            if (setrlimit(RLIMIT_CORE, &no_core)) child_error("Disable core dumps");
        }
        execvp(cmd[0], cmd);
        dprintf(STDERR_FILENO, "Cannot run %s: %s\n", cmd[0], strerror(errno));
        _exit(127);
    }
    (void)setpgid(pid, pid);
    double deadline = now_ms() + timeout_ms;
    int status = 0;
    Process result = {0, 0, 0};
    for (;;) {
        pid_t finished = waitpid(pid, &status, WNOHANG);
        if (finished == pid) break;
        if (finished < 0 && errno != EINTR) { perror("Wait for process"); result.code = 126; break; }
        if (interrupted || now_ms() >= deadline) {
            result.tle = !interrupted;
            kill(-pid, SIGKILL);
            kill(pid, SIGKILL);
            while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
            break;
        }
        struct timespec pause = {0, 5000000};
        nanosleep(&pause, NULL);
    }
    kill(-pid, SIGKILL);
    if (!result.code) result.code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
    if (limits) {
        struct stat st;
        result.ole = (WIFSIGNALED(status) && WTERMSIG(status) == SIGXFSZ)
            || (output >= 0 && !fstat(output, &st) && st.st_size >= OUTPUT_LIMIT)
            || (errors >= 0 && !fstat(errors, &st) && st.st_size >= OUTPUT_LIMIT);
    }
    return result;
}

static FILE *temp_file(void) {
    FILE *file = tmpfile();
    if (!file) fail("Cannot create temporary file: %s", strerror(errno));
    if (fcntl(fileno(file), F_SETFD, FD_CLOEXEC) < 0) fail("Cannot prepare temporary file.");
    return file;
}

static void rewind_checked(FILE *file) {
    if (fseek(file, 0, SEEK_SET)) fail("Cannot read process output: %s", strerror(errno));
}

static int capture(char *const cmd[], char *buffer, size_t size) {
    FILE *out = temp_file(), *err = temp_file(), *in = temp_file();
    Process result = run(cmd, root, fileno(in), fileno(out), fileno(err), 15000, 0);
    rewind_checked(out);
    size_t n = fread(buffer, 1, size - 1, out);
    buffer[n] = '\0';
    while (n && (buffer[n - 1] == '\n' || buffer[n - 1] == '\r')) buffer[--n] = '\0';
    fclose(out); fclose(err); fclose(in);
    return !result.code && !result.tle && !interrupted;
}

static uint64_t hash_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return 0;
    uint64_t hash = UINT64_C(14695981039346656037);
    int ch;
    while ((ch = fgetc(file)) != EOF) { hash ^= (unsigned char)ch; hash *= UINT64_C(1099511628211); }
    if (ferror(file)) hash = 0;
    fclose(file);
    return hash;
}

static void source_path(char *path, int lang, int assignment) {
    pathf(path, "%s/assignments/%s/%s/%s", root, languages[lang].name,
          assignments[assignment].name, languages[lang].source);
}

static void state_path(char *path, int lang, int assignment) {
    char dir[PATH_MAX];
    pathf(dir, "%s/.getting_started", root); mkdir_ok(dir);
    pathf(dir, "%s/.getting_started/%s", root, languages[lang].name); mkdir_ok(dir);
    pathf(path, "%s/%s", dir, assignments[assignment].name);
}

static Progress read_progress(int lang, int assignment) {
    Progress p = {"NEW", "-", 0};
    char path[PATH_MAX];
    state_path(path, lang, assignment);
    FILE *file = fopen(path, "r");
    if (file) {
        unsigned long long hash;
        if (fscanf(file, "%15s %64s %llx", p.verdict, p.sha, &hash) == 3) p.hash = (uint64_t)hash;
        else p = (Progress){"NEW", "-", 0};
        fclose(file);
    }
    return p;
}

static void save_progress(int lang, int assignment, const Progress *p) {
    char path[PATH_MAX], temporary[PATH_MAX];
    state_path(path, lang, assignment);
    pathf(temporary, "%s.tmp.%ld", path, (long)getpid());
    FILE *file = fopen(temporary, "w");
    if (!file) fail("Cannot save progress.");
    fprintf(file, "%s %s %016llx\n", p->verdict, p->sha, (unsigned long long)p->hash);
    if (fclose(file) || rename(temporary, path)) fail("Cannot save progress.");
}

static const char *unflag(const char *s) { while (*s == '-') s++; return s; }

static int available(const char *command) {
    const char *env = getenv("PATH");
    char *paths = strdup(env ? env : "");
    if (!paths) fail("Out of memory.");
    int found = 0;
    char *cursor = NULL;
    for (char *dir = strtok_r(paths, ":", &cursor); dir; dir = strtok_r(NULL, ":", &cursor)) {
        char path[PATH_MAX];
        pathf(path, "%s/%s", dir, command);
        if (!access(path, X_OK)) { found = 1; break; }
    }
    free(paths);
    return found;
}

static int find_language(const char *name) {
    name = unflag(name);
    if (!strcmp(name, "py") || !strcmp(name, "p") || !strcmp(name, "python")) return PYTHON;
    if (!strcmp(name, "c")) return C;
    if (!strcmp(name, "cpp") || !strcmp(name, "cc") || !strcmp(name, "c++")) return CPP;
    if (!strcmp(name, "java")) return JAVA;
    return -1;
}

static int find_assignment(const char *name) {
    for (int i = 0; i < ASSIGNMENT_COUNT; i++) if (!strcmp(name, assignments[i].name)) return i;
    return -1;
}

static int submission(const char *message, int *lang, int *assignment) {
    char language[16], name[64], extra;
    if (sscanf(message, "Submit: %15[^/]/%63s %c", language, name, &extra) != 2) return 0;
    *lang = find_language(language);
    *assignment = find_assignment(name);
    return *lang >= 0 && *assignment >= 0 && !strcmp(language, languages[*lang].name);
}

/* Recover submissions from this branch, including after a fresh clone. */
static void restore_history(int lang, Progress progress[]) {
    char *history = malloc(1024 * 1024);
    if (!history) fail("Out of memory.");
    char *cmd[] = {"git", "log", "--format=%H %s", "--grep=^Submit: ", NULL};
    if (!capture(cmd, history, 1024 * 1024)) { free(history); return; }
    int seen[ASSIGNMENT_COUNT] = {0};
    char *cursor = NULL;
    for (char *line = strtok_r(history, "\n", &cursor); line; line = strtok_r(NULL, "\n", &cursor)) {
        char *message = strchr(line, ' ');
        if (!message) continue;
        *message++ = '\0';
        int found_lang, assignment;
        if (!submission(message, &found_lang, &assignment) || found_lang != lang || seen[assignment]) continue;
        seen[assignment] = 1;
        if (strlen(line) >= sizeof(progress[assignment].sha) || !strcmp(progress[assignment].sha, line)) continue;
        Progress p = {"PENDING", "", 0};
        strcpy(p.sha, line);
        char object[PATH_MAX];
        pathf(object, "%s:assignments/%s/%s/%s", line, languages[lang].name,
              assignments[assignment].name, languages[lang].source);
        char *show[] = {"git", "show", object, NULL};
        /* Hash the exact committed bytes, including trailing newlines. */
        FILE *out = temp_file(), *err = temp_file(), *in = temp_file();
        Process r = run(show, root, fileno(in), fileno(out), fileno(err), 15000, 0);
        if (!r.code && !r.tle) {
            rewind_checked(out);
            p.hash = UINT64_C(14695981039346656037);
            int ch;
            while ((ch = fgetc(out)) != EOF) { p.hash ^= (unsigned char)ch; p.hash *= UINT64_C(1099511628211); }
        }
        fclose(out); fclose(err); fclose(in);
        progress[assignment] = p;
        save_progress(lang, assignment, &p);
    }
    free(history);
}

static void load_progress(int lang, Progress progress[]) {
    for (int i = 0; i < ASSIGNMENT_COUNT; i++) progress[i] = read_progress(lang, i);
    restore_history(lang, progress);
    char repository[PATH_MAX] = "";
    char *remote[] = {"git", "remote", "get-url", "origin", NULL};
    (void)capture(remote, repository, sizeof(repository));
    /* Pin queries to the student's origin; gh can otherwise default to a fork's parent. */
    char *start = strstr(repository, "://");
    start = start ? start + 3 : repository;
    char *at = strchr(start, '@');
    if (at) start = at + 1;
    memmove(repository, start, strlen(start) + 1);
    char *colon = strchr(repository, ':');
    if (colon) *colon = '/';
    size_t len = strlen(repository);
    if (len > 4 && !strcmp(repository + len - 4, ".git")) repository[len - 4] = '\0';
    for (int i = 0; i < ASSIGNMENT_COUNT && !interrupted; i++) {
        if (!strcmp(progress[i].sha, "-")) continue;
        char response[256];
        char *cmd[] = {"gh", "run", "list", "--repo", repository, "--workflow", "autograde.yml", "--commit", progress[i].sha,
            "--limit", "1", "--json", "status,conclusion", "--jq", ".[0] | [.status, .conclusion] | @tsv", NULL};
        if (!capture(cmd, response, sizeof(response))) {
            fprintf(stderr, "Results unavailable; showing saved results. Check gh auth login and your Actions tab.\n");
            break;
        }
        char status[32] = "", conclusion[32] = "";
        sscanf(response, "%31s %31s", status, conclusion);
        if (!strcmp(status, "completed")) {
            strcpy(progress[i].verdict, !strcmp(conclusion, "success") ? "AC" : "FAIL");
        } else if (status[0]) strcpy(progress[i].verdict, "PENDING");
        save_progress(lang, i, &progress[i]);
    }
}

static const char *symbol(int lang, int assignment, const Progress *p) {
    char source[PATH_MAX], original[PATH_MAX];
    source_path(source, lang, assignment);
    uint64_t current = hash_file(source);
    if (current && current == p->hash) {
        if (!strcmp(p->verdict, "AC")) return "✓";
        if (!strcmp(p->verdict, "FAIL")) return "x";
    }
    pathf(original, "%s/grader/templates/%s/%s", root, languages[lang].name,
          assignment == ECHO ? "echo" : "starter");
    if (strcmp(p->verdict, "NEW") || !current || current != hash_file(original)) return "*";
    return ".";
}

static void print_file(const char *path, size_t limit) {
    FILE *file = fopen(path, "r");
    if (!file) fail("Cannot read %s.", path);
    int ch;
    size_t count = 0;
    while (count++ < limit && (ch = fgetc(file)) != EOF) putchar(ch);
    if (!feof(file)) puts("\n...");
    fclose(file);
}

static int start_assignment(int lang, int assignment) {
    Progress p = read_progress(lang, assignment);
    if (!strcmp(p.verdict, "NEW")) {
        strcpy(p.verdict, "STARTED");
        save_progress(lang, assignment, &p);
    }
    char readme[PATH_MAX], source[PATH_MAX];
    source_path(source, lang, assignment);
    pathf(readme, "%s/assignments/%s/%s/README.md", root, languages[lang].name, assignments[assignment].name);
    print_file(readme, 16000);
    printf("\nEdit: %s\n", source);
    return 0;
}

static int submit_assignment(int lang, int assignment) {
    char source[PATH_MAX], relative[PATH_MAX], response[PATH_MAX], message[128];
    source_path(source, lang, assignment);
    if (access(source, R_OK)) fail("Missing solution: %s", source);
    char *branch[] = {"git", "symbolic-ref", "--quiet", "--short", "HEAD", NULL};
    if (!capture(branch, response, sizeof(response))) fail("Switch to a Git branch before submitting.");
    char *remote[] = {"git", "remote", "get-url", "origin", NULL};
    if (!capture(remote, response, sizeof(response))) fail("Add your fork as the origin remote before submitting.");
    pathf(relative, "assignments/%s/%s/%s", languages[lang].name, assignments[assignment].name, languages[lang].source);
    snprintf(message, sizeof(message), "Submit: %s/%s", languages[lang].name, assignments[assignment].name);
    char *add[] = {"git", "add", "--", relative, NULL};
    Process r = run(add, root, -1, -1, -1, 120000, 0);
    if (r.code || r.tle || interrupted) return 1;
    char *commit[] = {"git", "commit", "--only", "--allow-empty", "-m", message, "--", relative, NULL};
    r = run(commit, root, -1, -1, -1, 120000, 0);
    if (r.code || r.tle || interrupted) return 1;
    Progress p = {"PENDING", "", hash_file(source)};
    char *head[] = {"git", "rev-parse", "HEAD", NULL};
    if (!capture(head, p.sha, sizeof(p.sha))) fail("Cannot read submission commit.");
    save_progress(lang, assignment, &p);
    /* Explicit origin/HEAD avoids push.default pushing other branches. */
    char *push[] = {"git", "push", "--set-upstream", "origin", "HEAD", NULL};
    r = run(push, root, -1, -1, -1, 120000, 0);
    if (r.code || r.tle || interrupted) {
        fputs("Push failed. Your commit is saved. Retry: git push --set-upstream origin HEAD\n", stderr);
        return 1;
    }
    printf("Submitted %s/%s. Grading on GitHub.\ngetting_started -%s list\n",
           languages[lang].name, assignments[assignment].name, languages[lang].flag);
    return 0;
}

static int copy_file(const char *source, const char *destination) {
    FILE *in = fopen(source, "rb");
    if (!in) return 0;
    FILE *out = fopen(destination, "wb");
    if (!out) { fclose(in); return 0; }
    char buffer[8192];
    size_t n;
    int ok = 1;
    for (;;) {
        n = fread(buffer, 1, sizeof(buffer), in);
        if (n && fwrite(buffer, 1, n, out) != n) { ok = 0; break; }
        if (n < sizeof(buffer)) { if (ferror(in)) ok = 0; break; }
    }
    fclose(in);
    if (fclose(out)) ok = 0;
    return ok;
}

/* Compare tokens exactly, ignoring whitespace only (no float tolerance). */
static int same_output(FILE *actual, FILE *expected) {
    rewind_checked(actual); rewind_checked(expected);
    for (;;) {
        int a, b;
        do { a = fgetc(actual); } while (a != EOF && isspace((unsigned char)a));
        do { b = fgetc(expected); } while (b != EOF && isspace((unsigned char)b));
        if (a == EOF || b == EOF) return a == b && !ferror(actual) && !ferror(expected);
        for (;;) {
            if (a != b) return 0;
            a = fgetc(actual); b = fgetc(expected);
            int end_a = a == EOF || isspace((unsigned char)a);
            int end_b = b == EOF || isspace((unsigned char)b);
            if (end_a || end_b) { if (end_a != end_b) return 0; break; }
        }
    }
}

static void diagnostics(FILE *file) {
    rewind_checked(file);
    int ch;
    size_t n = 0;
    while (n++ < 2000 && (ch = fgetc(file)) != EOF) putchar(ch);
    if (!feof(file)) puts("\n...");
}

static int contains_text(FILE *file, const char *text) {
    rewind_checked(file);
    char line[4096];
    while (fgets(line, sizeof(line), file)) if (strstr(line, text)) return 1;
    return 0;
}

/* Text exercises keep spaces and line boundaries. Normalize CRLF only. */
static char *read_text(FILE *file, size_t *length) {
    char *text = malloc(OUTPUT_LIMIT + 1);
    if (!text) fail("Out of memory.");
    rewind_checked(file);
    size_t n = fread(text, 1, OUTPUT_LIMIT + 1, file);
    if (ferror(file) || n > OUTPUT_LIMIT) { free(text); return NULL; }
    *length = 0;
    for (size_t i = 0; i < n; i++) {
        if (text[i] == '\r' && i + 1 < n && text[i + 1] == '\n') continue;
        text[(*length)++] = text[i];
    }
    return text;
}

static int same_text(FILE *actual, FILE *expected) {
    size_t a_length = 0, b_length = 0;
    char *a = read_text(actual, &a_length), *b = read_text(expected, &b_length);
    /* Answers have one display terminator; the solution may include or omit it. */
    if (b && b_length && b[b_length - 1] == '\n') b_length--;
    int equal = a && b && ((a_length == b_length && !memcmp(a, b, b_length))
        || (a_length == b_length + 1 && a[a_length - 1] == '\n' && !memcmp(a, b, b_length)));
    free(a); free(b);
    return equal;
}

static int grade(int lang, int assignment, int sample) {
    char work[PATH_MAX], source[PATH_MAX], copied[PATH_MAX], executable[PATH_MAX];
    const char *tmp = getenv("TMPDIR");
    pathf(work, "%s/getting-started.XXXXXX", tmp && *tmp ? tmp : "/tmp");
    if (!mkdtemp(work)) fail("Cannot create grading directory.");
    source_path(source, lang, assignment);
    pathf(copied, "%s/%s", work, languages[lang].source);
    pathf(executable, "%s/solution", work);
    const char *verdict = "AC";
    int passed = 0, failed_test = 0;
    int test_count = sample ? (assignment == LONG_COW_I ? 3 : 1) : TEST_COUNT;
    FILE *out = temp_file(), *err = temp_file(), *empty = temp_file();
    if (!copy_file(source, copied)) { verdict = "CE"; puts("Missing solution file."); goto done; }
    char *c_compiler = available("gcc-16") ? "gcc-16" : "gcc";
    char *cpp_compiler = available("g++-16") ? "g++-16" : "g++";
    char *c_cmd[] = {c_compiler, "-std=c2y", "-O2", "-Wall", "-Wextra", copied, "-lm", "-o", executable, NULL};
    char *cpp_cmd[] = {cpp_compiler, "-std=gnu++26", "-O2", "-Wall", "-Wextra", copied, "-o", executable, NULL};
    char *java_cmd[] = {"javac", "--release", "26", "--enable-preview", "-encoding", "UTF-8", "-d", work, copied, NULL};
    char *python_exe = getenv("GETTING_STARTED_PYTHON");
    if (!python_exe || !*python_exe) python_exe = "python3";
    char *py_cmd[] = {python_exe, "-I", "-m", "py_compile", copied, NULL};
    char **compile[] = {py_cmd, c_cmd, cpp_cmd, java_cmd};
    Process result = run(compile[lang], work, fileno(empty), fileno(out), fileno(err), 30000, 1);
    if (result.code || result.tle || result.ole || interrupted) {
        verdict = "CE";
        if (result.tle) puts("Compilation timed out.");
        diagnostics(err);
        goto done;
    }
    for (int tc = 1; tc <= test_count && !interrupted; tc++) {
        char input[PATH_MAX], answer[PATH_MAX], cwd[PATH_MAX];
        pathf(input, "%s/testcases/%s/%02d.in", root, assignments[assignment].name, tc);
        pathf(answer, "%s/testcases/%s/%02d.out", root, assignments[assignment].name, tc);
        pathf(cwd, "%s/test-%02d", work, tc); mkdir_ok(cwd);
        FILE *in = fopen(input, "r"), *expected = fopen(answer, "r");
        if (!in || !expected) {
            if (in) fclose(in);
            if (expected) fclose(expected);
            verdict = "ERROR"; printf("Missing test case %02d.\n", tc); goto done;
        }
        fclose(out); fclose(err);
        out = temp_file(); err = temp_file();
        char *native[] = {executable, NULL};
        char *python[] = {python_exe, "-I", copied, NULL};
        char *java[] = {"java", "--enable-preview", "-Xmx256m", "-cp", work, "Main", NULL};
        char **command = lang == PYTHON ? python : lang == JAVA ? java : native;
        result = run(command, cwd, fileno(in), fileno(out), fileno(err), languages[lang].time_ms, 1);
        fclose(in);
        if (sample) {
            if (test_count > 1) printf("Sample %d/%d:\n", tc, test_count);
            printf("Sample input:\n"); print_file(input, 200);
            printf("Expected output:\n"); print_file(answer, 200);
            printf("Your output:\n"); diagnostics(out); putchar('\n');
        }
        if (result.tle) verdict = "TLE";
        else if (result.ole) verdict = "OLE";
        else if (result.code) verdict = "RE";
        else if (!(assignments[assignment].text_output ? same_text(out, expected) : same_output(out, expected))) verdict = "WA";
        fclose(expected);
        if (strcmp(verdict, "AC")) {
            failed_test = tc;
            if (!strcmp(verdict, "RE")) {
                printf("Input: %s\n", input);
                printf("Program exited with code %d.\n", result.code);
                diagnostics(err);
                if (lang == PYTHON && contains_text(err, "EOFError:")) {
                    puts("Input ended. input() reads one line, not one number. Check the README's input format.");
                }
            }
            if (!sample && !strcmp(verdict, "WA")) {
                printf("Expected: "); print_file(answer, 200);
                printf("Received: "); diagnostics(out); putchar('\n');
            }
            break;
        }
        passed++;
    }
done:
    if (interrupted) verdict = "INTERRUPTED";
    if (sample) {
        if (!strcmp(verdict, "AC")) {
            if (test_count > 1) printf("Samples correct (%d/%d).\n", passed, test_count);
            else puts("Sample correct.");
        }
        else printf("Sample failed: %s.\n", verdict);
    } else {
        printf("%s %s/%s — %d/%d tests", verdict, languages[lang].name, assignments[assignment].name, passed, test_count);
        if (failed_test) printf(" (test %02d)", failed_test);
        putchar('\n');
    }
    const char *summary_path = getenv("GITHUB_STEP_SUMMARY");
    if (!sample && summary_path && *summary_path) {
        FILE *summary = fopen(summary_path, "a");
        if (summary) {
            fprintf(summary, "| Assignment | Result | Tests |\n| --- | --- | --- |\n| %s/%s | %s | %d/%d |\n",
                    languages[lang].name, assignments[assignment].name, verdict, passed, TEST_COUNT);
            fclose(summary);
        }
    }
    fclose(out); fclose(err); fclose(empty);
    remove_tree(work);
    return strcmp(verdict, "AC") ? 1 : 0;
}

static void locate_root(const char *argv0) {
    char resolved[PATH_MAX];
    if (strchr(argv0, '/')) {
        if (!realpath(argv0, resolved)) fail("Cannot locate getting_started.");
    } else {
        const char *env = getenv("PATH");
        char *paths = strdup(env ? env : "");
        if (!paths) fail("Out of memory.");
        int found = 0;
        char *cursor = NULL;
        for (char *dir = strtok_r(paths, ":", &cursor); dir; dir = strtok_r(NULL, ":", &cursor)) {
            char candidate[PATH_MAX];
            pathf(candidate, "%s/%s", dir, argv0);
            if (!access(candidate, X_OK) && realpath(candidate, resolved)) { found = 1; break; }
        }
        free(paths);
        if (!found) fail("Cannot locate getting_started on PATH.");
    }
    char *slash = strrchr(resolved, '/');
    if (!slash) fail("Cannot locate repository.");
    *slash = '\0';
    strcpy(root, resolved);
    if (chdir(root)) fail("Cannot open repository.");
}

static void usage(void) {
    puts("getting_started -[py|p|python|c|cpp|cc|c++|java] [list|start NAME|start next|run NAME|submit NAME]");
    puts("✓ completed  . not started  * started/grading/edited  x incorrect");
}

int main(int argc, char **argv) {
    if (argc == 2 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))) { usage(); return 0; }
    if (argc < 2) { usage(); return 2; }
    signal(SIGINT, on_signal); signal(SIGTERM, on_signal);
    locate_root(argv[0]);
    if (!strcmp(argv[1], "--ci")) {
        int lang, assignment;
        if (argc != 3 || !submission(argv[2], &lang, &assignment)) fail("Invalid submission marker.");
        return grade(lang, assignment, 0);
    }
    int lang = find_language(argv[1]);
    if (argv[1][0] != '-' || lang < 0) fail("Choose a language: -py, -c, -cpp, or -java.");
    const char *command = argc > 2 ? unflag(argv[2]) : "list";
    if (!strcmp(command, "list") && argc <= 3) {
        Progress progress[ASSIGNMENT_COUNT];
        load_progress(lang, progress);
        for (int i = 0; i < ASSIGNMENT_COUNT; i++) printf("%s %s\n", symbol(lang, i, &progress[i]), assignments[i].name);
        return interrupted ? 130 : 0;
    }
    if (argc != 4 || (strcmp(command, "start") && strcmp(command, "submit") && strcmp(command, "run"))) { usage(); return 2; }
    int assignment = find_assignment(argv[3]);
    if (!strcmp(command, "start") && !strcmp(argv[3], "next")) {
        Progress progress[ASSIGNMENT_COUNT];
        load_progress(lang, progress);
        if (interrupted) return 130;
        for (int i = 0; i < ASSIGNMENT_COUNT; i++) {
            if (strcmp(symbol(lang, i, &progress[i]), "✓")) return start_assignment(lang, i);
        }
        puts("All assignments completed."); return 0;
    }
    if (assignment < 0) fail("Unknown assignment. Run getting_started -%s list.", languages[lang].flag);
    if (!strcmp(command, "start")) return start_assignment(lang, assignment);
    if (!strcmp(command, "run")) return grade(lang, assignment, 1);
    return submit_assignment(lang, assignment);
}
