#define _XOPEN_SOURCE 700
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "../grader/catalog.h"

/* Instructor-only generator. Stable PRNG; never called by student commands/CI. */
static uint64_t seed = UINT64_C(20260910);
static uint32_t random_value(void) {
    seed ^= seed >> 12; seed ^= seed << 25; seed ^= seed >> 27;
    return (uint32_t)((seed * UINT64_C(2685821657736338717)) >> 32);
}

static void directory(const char *path) {
    if (mkdir(path, 0755) && errno != EEXIST) { perror(path); exit(1); }
}

static FILE *create(const char *path) {
    FILE *file = fopen(path, "w");
    if (!file) { perror(path); exit(1); }
    return file;
}

static void write_text(const char *path, const char *text, int overwrite) {
    if (!overwrite) {
        FILE *existing = fopen(path, "r");
        if (existing) { fclose(existing); return; }
    }
    FILE *file = create(path);
    fputs(text, file);
    if (fclose(file)) { perror(path); exit(1); }
}

static const char *echo_source[] = {
    "value = int(input())\nprint(value)\n",
    "#include <stdio.h>\n\nint main(void) {\n    int value;\n    scanf(\"%d\", &value);\n    printf(\"%d\\n\", value);\n    return 0;\n}\n",
    "#include <iostream>\nusing namespace std;\n\nint main() {\n    int value;\n    cin >> value;\n    cout << value << '\\n';\n    return 0;\n}\n",
    "import java.util.Scanner;\n\npublic class Main {\n    public static void main(String[] args) {\n        Scanner scanner = new Scanner(System.in);\n        int value = scanner.nextInt();\n        System.out.println(value);\n    }\n}\n"
};

static const char *starter_source[] = {
    "# Write your solution here.\n",
    "#include <stdio.h>\n\nint main(void) {\n    // Write your solution here.\n    return 0;\n}\n",
    "#include <iostream>\nusing namespace std;\n\nint main() {\n    // Write your solution here.\n    return 0;\n}\n",
    "import java.util.Scanner;\n\npublic class Main {\n    public static void main(String[] args) {\n        Scanner scanner = new Scanner(System.in);\n        // Write your solution here.\n    }\n}\n"
};

#include "new_problems.h"

static void readme(FILE *file, int lang, int id) {
    if (id == MOO_I || id >= COLLATZ) { new_readme(file, lang, id); return; }
    const Assignment *a = &assignments[id];
    fprintf(file, "# %s\n\n", a->name);
    if (id == ECHO) {
        fprintf(file, "This assignment has already been filled for you!\n\nTo submit, run `getting_started -%s submit %s`.\n\n", languages[lang].flag, a->name);
    }
    fprintf(file, "%s\n\n", a->description);
    if (id == REVERSE_TWO) fputs("HINT: Look at the previous assignment!\n\n", file);
    if (a->n_values) fputs("1 ≤ N ≤ 100000.\n", file);
    if (id == DIVIDE_TWO || id == MODULO_TWO) fputs("Integers: 0 ≤ first ≤ 10^9; 1 ≤ second ≤ 10^9.\n", file);
    else if (id == DIVIDE_N || id == MODULO_N) fputs("Integers: 0 ≤ first ≤ 10^9; 1 ≤ remaining values ≤ 10^9.\n", file);
    else fputs("Integers: 0 to 10^9.\n", file);
    fprintf(file, "Time limit: %d seconds per test.\n\n", languages[lang].time_ms / 1000);
    if (id == SUM_TWO || id == SUM_N || id == PRODUCT_TWO || id == PRODUCT_N) {
        if (lang == C) fputs("Use `long long` for arithmetic (`%lld` with scanf/printf).\n\n", file);
        if (lang == CPP) fputs("Use `long long` for arithmetic.\n\n", file);
        if (lang == JAVA) fputs("Use `long` for arithmetic (`scanner.nextLong()`).\n\n", file);
    }
    if (id == MODULO_TWO || id == MODULO_N) fputs("`a % b` gives the remainder. Example: `17 % 5 = 2`.\n\n", file);
    if (id == MODULO_N) fputs("Compute `((a1 % a2) % a3) ...`. For N = 1, print a1.\n\n", file);
    if (id == PRODUCT_TWO || id == PRODUCT_N) {
        fputs("Use `% 1000000007` after every multiplication.\n\n", file);
        if (id == PRODUCT_N) fputs("Start with `answer = 1`.\n\n", file);
        const char *expression = id == PRODUCT_N ? "answer * value" : "a * b";
        fprintf(file, "```%s\n", languages[lang].name);
        if (lang == PYTHON) fprintf(file, "answer = (%s) %% 1000000007\n", expression);
        else if (id == PRODUCT_N) fprintf(file, "answer = (%s) %% 1000000007%s;\n", expression, lang == JAVA ? "L" : "LL");
        else fprintf(file, "%s answer = (%s) %% 1000000007%s;\n", lang == JAVA ? "long" : "long long", expression, lang == JAVA ? "L" : "LL");
        fputs("```\n\n", file);
    }
    if (id == DIVIDE_TWO || id == DIVIDE_N) {
        if (lang == PYTHON) fputs("Use `a // b`. Example: `7 // 2 = 3`.\n\n", file);
        else fputs("Use integer `a / b`. Example: `7 / 2 = 3`.\n\n", file);
    }
    if (id == DIVIDE_N) fputs("Compute `floor(floor(a1 / a2) / a3) ...`. For N = 1, print a1.\n\n", file);
    fprintf(file, "Example Input:\n```text\n%s```\n\nExample Output:\n```text\n%s```\n", a->example_input, a->example_output);
}

static int is_divisor(int id) {
    return id == DIVIDE_TWO || id == DIVIDE_N || id == MODULO_TWO || id == MODULO_N;
}

static void fixtures(int id) {
    if (id == MOO_I || id >= COLLATZ) { new_fixtures(id); return; }
    char path[512];
    snprintf(path, sizeof(path), "testcases/%s", assignments[id].name); directory(path);
    static const int sizes[TEST_COUNT] = {1, 1, 1, 2, 5, 8, 13, 21, 50, 100, 237, 999, 4096, 8191, 10000, 25000, 50000, 99999, 100000, 100000};
    for (int tc = 1; tc <= TEST_COUNT; tc++) {
        int n = assignments[id].n_values ? sizes[tc - 1] : id == ECHO ? 1 : 2;
        long long *values = malloc((size_t)n * sizeof(*values));
        if (!values) exit(1);
        for (int i = 0; i < n; i++) {
            values[i] = random_value() % UINT32_C(1000000001);
            if (tc == 2) values[i] = 0;
            if (tc == 3) values[i] = 1000000000;
            if (tc == 4) values[i] = 1;
            if (tc == 5) values[i] = (long long)i + 1;
            if (is_divisor(id) && i > 0) {
                if (values[i] == 0) values[i] = 1;
                if (tc == 6 || tc == 19) values[i] = 1;
                if (tc == 7) values[i] = 2;
                if (tc == 8 && id == MODULO_N) values[i] = n - i + 1;
            }
            if (tc == 20 && (id == SUM_N || id == PRODUCT_N)) values[i] = 1000000000;
            if (tc == 18 && id == PRODUCT_N && i == n / 2) values[i] = 0;
        }
        snprintf(path, sizeof(path), "testcases/%s/%02d.in", assignments[id].name, tc);
        FILE *in = create(path);
        snprintf(path, sizeof(path), "testcases/%s/%02d.out", assignments[id].name, tc);
        FILE *out = create(path);
        if (tc == 1) {
            fputs(assignments[id].example_input, in);
            fputs(assignments[id].example_output, out);
        } else {
            if (assignments[id].n_values) fprintf(in, "%d\n", n);
            for (int i = 0; i < n; i++) fprintf(in, "%lld%c", values[i], i == n - 1 ? '\n' : ' ');
            if (id == REVERSE_TWO || id == REVERSE_N) {
                for (int i = n - 1; i >= 0; i--) fprintf(out, "%lld%c", values[i], i ? ' ' : '\n');
            } else {
                long long result = values[0];
                for (int i = 1; i < n; i++) {
                    if (id == SUM_TWO || id == SUM_N) result += values[i];
                    if (id == PRODUCT_TWO || id == PRODUCT_N) result = result * values[i] % 1000000007;
                    if (id == DIVIDE_TWO || id == DIVIDE_N) result /= values[i];
                    if (id == MODULO_TWO || id == MODULO_N) result %= values[i];
                }
                fprintf(out, "%lld\n", result);
            }
        }
        free(values);
        if (fclose(in) || fclose(out)) { perror("test case"); exit(1); }
    }
}

int main(void) {
    directory("assignments"); directory("testcases"); directory("grader/templates");
    for (int lang = 0; lang < LANGUAGE_COUNT; lang++) {
        char path[512];
        snprintf(path, sizeof(path), "assignments/%s", languages[lang].name); directory(path);
        snprintf(path, sizeof(path), "grader/templates/%s", languages[lang].name); directory(path);
        snprintf(path, sizeof(path), "grader/templates/%s/echo", languages[lang].name); write_text(path, echo_source[lang], 1);
        snprintf(path, sizeof(path), "grader/templates/%s/starter", languages[lang].name); write_text(path, starter_source[lang], 1);
        for (int id = 0; id < ASSIGNMENT_COUNT; id++) {
            snprintf(path, sizeof(path), "assignments/%s/%s", languages[lang].name, assignments[id].name); directory(path);
            snprintf(path, sizeof(path), "assignments/%s/%s/README.md", languages[lang].name, assignments[id].name);
            FILE *file = create(path); readme(file, lang, id); if (fclose(file)) exit(1);
            snprintf(path, sizeof(path), "assignments/%s/%s/%s", languages[lang].name, assignments[id].name, languages[lang].source);
            write_text(path, id == ECHO ? echo_source[lang] : starter_source[lang], 0);
        }
    }
    for (int id = 0; id < ASSIGNMENT_COUNT; id++) fixtures(id);
    printf("Generated %d assignments and %d fixed test cases. Existing solutions preserved.\n",
           LANGUAGE_COUNT * ASSIGNMENT_COUNT, ASSIGNMENT_COUNT * TEST_COUNT);
    return 0;
}
