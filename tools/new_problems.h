/* Included by generate.c: uses the same deterministic PRNG and file helpers. */
static void sample_block(FILE *file, const char *input, const char *output) {
    fprintf(file, "Example Input:\n```text\n%s%s```\n\nExample Output:\n```text\n%s%s```\n",
            input, *input && input[strlen(input) - 1] != '\n' ? "\n" : "",
            output, *output && output[strlen(output) - 1] != '\n' ? "\n" : "");
}

static void new_readme(FILE *file, int lang, int id) {
    fprintf(file, "# %s\n\n", assignments[id].description);
    if (id == MOO_I) {
        fputs("Farmer John needs you to start his chant.\n\nPrint `moo`. There is no input.\n\n", file);
    } else if (id == COLLATZ) {
        fputs("The Collatz Conjecture hypothesizes that this sequence always reaches 1 when starting with a positive integer n.\n\n"
              "1. If n = 1, stop.\n2. If n is even, set n = n / 2 and return to step 1.\n"
              "3. Otherwise, set n = 3 * n + 1 and return to step 1.\n"
              "4. You will never reach this step.\n\n"
              "Read n (1 ≤ n ≤ 10^9). Print every value, including the starting n and the final 1. Separate values with spaces.\n\n", file);
        if (lang == C || lang == CPP) fputs("Use `long long`: intermediate values can exceed 10^9.\n\n", file);
        if (lang == JAVA) fputs("Use `long`: intermediate values can exceed 10^9.\n\n", file);
        if (lang == PYTHON) fputs("Use `//` for integer division.\n\n", file);
    } else if (id == MOO_II || id == MOO_III || id == MOO_IV) {
        fputs("Farmer John has a special process that improves milk extraction from his cows by 0.001%. "
              "He dances around the cow while shouting a specific string. His neighbors think he's strange, "
              "so he requires that you make a fool out of yourself too! Farmer John supplies the chant; you say it.\n\n", file);
        if (id == MOO_II) fputs("Read one line. Print that line.\n\n", file);
        if (id == MOO_III) fputs("Read the entire chant until end of input (EOF). Print it. The chant can span several lines.\n\n", file);
        if (id == MOO_IV) {
            fputs("Read K from the first line, then the entire chant until EOF. Print the chant K times, with no added separators. "
                  "K can be zero; then print nothing.\n\n"
                  "Ignore one final line ending in the input chant. Keep every other newline. "
                  "For example, repeating `moomoo\\nm` three times joins each final `m` directly to the next `moomoo`.\n\n"
                  "0 ≤ K ≤ 100000. K × chant length ≤ 1000000.\n", file);
        }
        fputs("Chant length: 0 to 100000 characters, excluding one final line ending. "
              "Characters are printable ASCII", file);
        fputs(id == MOO_II ? ".\n" : " or newlines.\n", file);
        fputs("Preserve spaces and blank lines. A final output newline is optional.\n\n", file);
    } else if (id == LONG_COW_I) {
        fputs("Farmer John has a lot of cows, but Farmer nohJ thinks his cows are longer. "
              "They compare their longest cow names instead. Each farmer has made a cow pick a name for you to compare.\n\n"
              "Read nohJ's cow name, then John's, one per line. Print `nohj` if the first is longer, "
              "`john` if the second is longer, or `-1` for a tie.\n\n"
              "Each name has 1 to 100000 lowercase English letters.\n\n", file);
    } else {
        fputs("Farmer John and Farmer nohJ compare cows by the lengths of their names. "
              "In a twist of fate, Farmer John has made you, a cow, pick his longest name. "
              "Finish quickly so you can get back to grazing and spreading news of Farmer John's evil deeds!\n\n"
              "Read N, then N names, one per line. Print the longest name's length, then the name on the next line. "
              "If lengths tie, use the first name.\n\n"
              "1 ≤ N ≤ 100000. Each name has 1 to 100000 lowercase English letters. Total name length ≤ 1000000.\n\n", file);
    }
    fprintf(file, "Time limit: %d seconds per test.\n\n", languages[lang].time_ms / 1000);
    sample_block(file, assignments[id].example_input, assignments[id].example_output);
    if (id == LONG_COW_I) {
        fputc('\n', file); sample_block(file, "nohjjcow\njohncow\n", "nohj\n");
        fputc('\n', file); sample_block(file, "nohjcow\njohnncow\n", "john\n");
    }
}

static char *random_text(size_t length, int multiline, int names) {
    char *text = malloc(length + 1);
    if (!text) exit(1);
    for (size_t i = 0; i < length; i++) {
        uint32_t value = random_value();
        text[i] = names ? (char)('a' + value % 26) : (char)(' ' + value % 95);
        if (multiline && value % 13 == 0) text[i] = '\n';
    }
    text[length] = '\0';
    return text;
}

static void collatz_case(FILE *in, FILE *out, int tc) {
    static const long long special[] = {1, 2, 3, 27, 1000000000, 999999999, 536870912, 837799, 670617279};
    long long value = tc <= 10 ? special[tc - 2] : 1 + random_value() % UINT32_C(1000000000);
    fprintf(in, "%lld\n", value);
    for (int steps = 0;; steps++) {
        if (steps > 10000 || value <= 0 || value > INT64_MAX / 3 - 1) exit(1);
        fprintf(out, "%lld%c", value, value == 1 ? '\n' : ' ');
        if (value == 1) break;
        value = value % 2 ? 3 * value + 1 : value / 2;
    }
}

static void chant_case(FILE *in, FILE *out, int id, int tc) {
    static const char *edge[] = {"", "m", "  moo   moo  ", "moo! 123 % \\", "\n", "moo\n\nmoo", " moo \n\n m ", "moo\n"};
    char *chant;
    if (tc <= 9 && (id != MOO_II || tc <= 5)) {
        size_t n = strlen(edge[tc - 2]);
        chant = malloc(n + 1); if (!chant) exit(1);
        memcpy(chant, edge[tc - 2], n + 1);
    } else {
        size_t length = tc == 10 ? 0 : tc == 18 ? 99999 : tc >= 19 ? 100000 : 1 + random_value() % 2000;
        chant = random_text(length, id != MOO_II, 0);
        /* Keep cases without a final newline unambiguous. */
        if (length && tc % 2 && chant[length - 1] == '\n') chant[length - 1] = 'm';
    }
    size_t length = strlen(chant);
    int copies = 1;
    if (id == MOO_IV) {
        copies = tc == 2 || tc == 5 ? 0 : tc == 3 ? 100000 : tc == 20 ? 10 : (int)(random_value() % 7);
        if (tc == 6 || tc == 7 || tc == 8 || tc == 9 || tc == 10 || tc == 19) copies = 3;
        if (length && (size_t)copies * length > 1000000) copies = (int)(1000000 / length);
        fprintf(in, "%d\n", copies);
    }
    fwrite(chant, 1, length, in);
    /* Add exactly one delimiter, including when the chant ends in a blank line. */
    if (!length || tc % 2 == 0 || chant[length - 1] == '\n') fputc('\n', in);
    for (int i = 0; i < copies; i++) fwrite(chant, 1, length, out);
    /* Keep intentional final blank lines distinguishable from an optional terminator. */
    if (id == MOO_IV ? copies > 0 && length > 0 : length > 0) fputc('\n', out);
    free(chant);
}

static void long_cow_one_case(FILE *in, FILE *out, int tc) {
    if (tc == 2) { fputs("nohjjcow\njohncow\n", in); fputs("nohj\n", out); return; }
    if (tc == 3) { fputs("nohjcow\njohnncow\n", in); fputs("john\n", out); return; }
    size_t a = 1 + random_value() % 1000, b = 1 + random_value() % 1000;
    if (tc == 4) a = b = 1;
    if (tc == 5) a = b;
    if (tc == 18) { a = 100000; b = 99999; }
    if (tc == 19) { a = 99999; b = 100000; }
    if (tc == 20) a = b = 100000;
    char *first = random_text(a, 0, 1), *second = random_text(b, 0, 1);
    fprintf(in, "%s\n%s%s", first, second, tc % 2 ? "" : "\n");
    fprintf(out, "%s\n", a == b ? "-1" : a > b ? "nohj" : "john");
    free(first); free(second);
}

static void long_cow_two_case(FILE *in, FILE *out, int tc) {
    int n = tc == 2 || tc == 6 ? 1 : tc == 3 ? 2 : tc == 4 ? 3 : tc == 18 ? 99999 : tc >= 19 ? 100000 : 1 + (int)(random_value() % 1000);
    fprintf(in, "%d\n", n);
    char *best = NULL;
    size_t best_length = 0;
    for (int i = 0; i < n; i++) {
        size_t length = 1 + random_value() % (tc >= 18 ? 5 : 100);
        if (tc == 2) length = 1;
        if (tc == 3 || tc == 5) length = 4;
        if (tc == 4) length = (size_t)i + 1;
        if (tc == 6 || (tc == 18 && i == n - 1) || (tc == 19 && i == 0)) length = 100000;
        if (tc == 20) length = 10;
        char *name = random_text(length, 0, 1);
        if (tc == 5) memset(name, 'm', length);
        fprintf(in, "%s%s", name, i == n - 1 && tc % 2 ? "" : "\n");
        if (length > best_length) { free(best); best = name; best_length = length; }
        else free(name);
    }
    fprintf(out, "%zu\n%s\n", best_length, best);
    free(best);
}

static void new_fixtures(int id) {
    char path[512];
    snprintf(path, sizeof(path), "testcases/%s", assignments[id].name); directory(path);
    for (int tc = 1; tc <= TEST_COUNT; tc++) {
        snprintf(path, sizeof(path), "testcases/%s/%02d.in", assignments[id].name, tc);
        FILE *in = create(path);
        snprintf(path, sizeof(path), "testcases/%s/%02d.out", assignments[id].name, tc);
        FILE *out = create(path);
        if (id == MOO_I || tc == 1) { fputs(assignments[id].example_input, in); fputs(assignments[id].example_output, out); }
        else if (id == COLLATZ) collatz_case(in, out, tc);
        else if (id == MOO_II || id == MOO_III || id == MOO_IV) chant_case(in, out, id, tc);
        else if (id == LONG_COW_I) long_cow_one_case(in, out, tc);
        else long_cow_two_case(in, out, tc);
        if (fclose(in) || fclose(out)) { perror("test case"); exit(1); }
    }
}
