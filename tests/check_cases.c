#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../grader/catalog.h"

static int maximum_cows, maximum_copies;
static size_t maximum_chant, maximum_name, maximum_names_total, maximum_repeated;
static int zero_copies, empty_chant, long_collatz;

static char *read_all(FILE *file, size_t *length) {
    char *data = malloc(OUTPUT_LIMIT + 1); assert(data);
    *length = fread(data, 1, OUTPUT_LIMIT, file);
    assert(!ferror(file) && fgetc(file) == EOF);
    data[*length] = '\0';
    return data;
}

static void new_case(int id, FILE *input, FILE *output) {
    if (id == MOO_I) {
        char word[16];
        assert(fgetc(input) == EOF);
        assert(fscanf(output, "%15s", word) == 1 && !strcmp(word, "moo"));
    } else if (id == COLLATZ) {
        long long start, previous = 0, value;
        assert(fscanf(input, "%lld", &start) == 1 && start >= 1 && start <= 1000000000);
        int count = 0;
        while (fscanf(output, "%lld", &value) == 1) {
            if (!count) assert(value == start);
            else {
                assert(previous != 1 && previous > 0 && previous < INT64_MAX / 3);
                assert(value == (previous % 2 ? previous * 3 + 1 : previous / 2));
            }
            if (value > INT32_MAX) long_collatz = 1;
            previous = value;
            assert(++count < 10000);
        }
        assert(count > 0 && previous == 1);
    } else if (assignments[id].text_output) {
        int copies = 1;
        if (id == MOO_IV) {
            assert(fscanf(input, "%d", &copies) == 1 && copies >= 0 && copies <= 100000);
            assert(fgetc(input) == '\n');
            if (copies > maximum_copies) maximum_copies = copies;
            if (!copies) zero_copies = 1;
        }
        size_t length, answer_length;
        char *chant = read_all(input, &length), *answer = read_all(output, &answer_length);
        if (length && chant[length - 1] == '\n') length--;
        assert(length <= 100000);
        if (length > maximum_chant) maximum_chant = length;
        if (!length) empty_chant = 1;
        for (size_t i = 0; i < length; i++) {
            assert((chant[i] >= ' ' && chant[i] <= '~') || (id != MOO_II && chant[i] == '\n'));
        }
        assert((size_t)copies * length <= 1000000);
        if ((size_t)copies * length > maximum_repeated) maximum_repeated = (size_t)copies * length;
        if (answer_length && answer[answer_length - 1] == '\n') answer_length--;
        assert(answer_length == (size_t)copies * length);
        for (int i = 0; i < copies; i++) assert(!memcmp(answer + (size_t)i * length, chant, length));
        free(chant); free(answer);
    } else {
        int n = 2;
        if (id == LONG_COW_II) {
            assert(fscanf(input, "%d", &n) == 1 && n >= 1 && n <= 100000);
            assert(fgetc(input) == '\n');
            if (n > maximum_cows) maximum_cows = n;
        }
        char *name = malloc(100002), *best = malloc(100002); assert(name && best);
        size_t first = 0, second = 0, longest = 0, total = 0;
        for (int i = 0; i < n; i++) {
            assert(fgets(name, 100002, input));
            size_t length = strcspn(name, "\n"); name[length] = '\0';
            assert(length >= 1 && length <= 100000);
            for (size_t j = 0; j < length; j++) assert(name[j] >= 'a' && name[j] <= 'z');
            total += length;
            if (!i) first = length;
            if (i == 1) second = length;
            if (length > longest) { longest = length; strcpy(best, name); }
            if (length > maximum_name) maximum_name = length;
        }
        assert(fgetc(input) == EOF);
        if (id == LONG_COW_I) {
            assert(fscanf(output, "%100001s", name) == 1);
            assert(!strcmp(name, first == second ? "-1" : first > second ? "nohj" : "john"));
        } else {
            assert(total <= 1000000);
            if (total > maximum_names_total) maximum_names_total = total;
            size_t answer_length;
            assert(fscanf(output, "%zu", &answer_length) == 1 && answer_length == longest);
            assert(fscanf(output, "%100001s", name) == 1 && !strcmp(name, best));
        }
        free(name); free(best);
    }
    char extra;
    assert(fscanf(input, " %c", &extra) == EOF);
    assert(fscanf(output, " %c", &extra) == EOF);
}

int main(void) {
    int checked = 0;
    for (int id = 0; id < ASSIGNMENT_COUNT; id++) {
        int max_n = 0;
        for (int tc = 1; tc <= TEST_COUNT; tc++) {
            char path[256];
            snprintf(path, sizeof(path), "testcases/%s/%02d.in", assignments[id].name, tc);
            FILE *input = fopen(path, "r"); assert(input);
            snprintf(path, sizeof(path), "testcases/%s/%02d.out", assignments[id].name, tc);
            FILE *output = fopen(path, "r"); assert(output);
            if (id == MOO_I || id >= COLLATZ) {
                new_case(id, input, output);
                fclose(input); fclose(output); checked++;
                continue;
            }
            int n = id == ECHO ? 1 : 2;
            if (assignments[id].n_values) assert(fscanf(input, "%d", &n) == 1);
            assert(n >= 1 && n <= 100000);
            if (n > max_n) max_n = n;
            long long *a = malloc((size_t)n * sizeof(*a)); assert(a);
            for (int i = 0; i < n; i++) {
                assert(fscanf(input, "%lld", &a[i]) == 1);
                assert(a[i] >= 0 && a[i] <= 1000000000);
                if (i && (id == DIVIDE_TWO || id == DIVIDE_N || id == MODULO_TWO || id == MODULO_N)) assert(a[i] > 0);
            }
            char extra;
            assert(fscanf(input, " %c", &extra) == EOF);
            if (id == REVERSE_TWO || id == REVERSE_N) {
                for (int i = 0; i < n; i++) {
                    long long got;
                    assert(fscanf(output, "%lld", &got) == 1 && got == a[n - i - 1]);
                }
            } else {
                long long expected = 0;
                if (id == PRODUCT_TWO || id == PRODUCT_N) {
                    expected = 1;
                    for (int i = 0; i < n; i++) expected = expected * a[i] % 1000000007;
                } else if (id == SUM_TWO || id == SUM_N) {
                    for (int i = 0; i < n; i++) expected += a[i];
                } else {
                    expected = a[0];
                    for (int i = 1; i < n; i++) {
                        if (id == MODULO_TWO || id == MODULO_N) expected %= a[i];
                        if (id == DIVIDE_TWO || id == DIVIDE_N) expected /= a[i];
                    }
                }
                long long got;
                assert(fscanf(output, "%lld", &got) == 1 && got == expected);
            }
            assert(fscanf(output, " %c", &extra) == EOF);
            free(a); fclose(input); fclose(output);
            checked++;
        }
        if (assignments[id].n_values) assert(max_n == 100000);
    }
    assert(maximum_cows == 100000 && maximum_copies == 100000);
    assert(maximum_name == 100000 && maximum_chant == 100000);
    assert(maximum_names_total == 1000000 && maximum_repeated == 1000000);
    assert(zero_copies && empty_chant && long_collatz);
    printf("Validated %d fixed cases, constraints, answers, and N = 100000 coverage.\n", checked);
}
