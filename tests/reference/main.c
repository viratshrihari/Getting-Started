#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    int op = @OP@;
    if (op == -1) { puts("moo"); return 0; }
    if (op == 11) {
        long long n;
        if (scanf("%lld", &n) != 1) return 1;
        for (;;) {
            printf("%lld%c", n, n == 1 ? '\n' : ' ');
            if (n == 1) return 0;
            n = n % 2 ? n * 3 + 1 : n / 2;
        }
    }
    if (op == 12 || op == 15 || op == 16) {
        int copies = 1;
        if (op == 16) { if (scanf("%d", &copies) != 1) return 1; getchar(); }
        char *chant = malloc(100002);
        if (!chant) return 1;
        size_t length = fread(chant, 1, 100002, stdin);
        if (op == 16 && length && chant[length - 1] == '\n') length--;
        for (int i = 0; i < copies; i++) fwrite(chant, 1, length, stdout);
        if (op == 16 && copies && length) putchar('\n');
        free(chant);
        return 0;
    }
    if (op == 13 || op == 14) {
        int count = 2;
        if (op == 14 && scanf("%d", &count) != 1) return 1;
        char *name = malloc(100001), *longest = malloc(100001);
        if (!name || !longest) return 1;
        size_t maximum = 0, first = 0, second = 0;
        for (int i = 0; i < count; i++) {
            if (scanf("%100000s", name) != 1) return 1;
            size_t length = strlen(name);
            if (i == 0) first = length;
            else if (i == 1) second = length;
            if (length > maximum) { maximum = length; strcpy(longest, name); }
        }
        if (op == 13) puts(first == second ? "-1" : first > second ? "nohj" : "john");
        else printf("%zu\n%s\n", maximum, longest);
        free(name); free(longest);
        return 0;
    }
    int many = op == 2 || op == 4 || op == 6 || op == 8 || op == 10;
    int n = op == 0 ? 1 : 2;
    if (many && scanf("%d", &n) != 1) return 1;
    long long *a = malloc((size_t)n * sizeof(*a));
    if (!a) return 1;
    for (int i = 0; i < n; i++) if (scanf("%lld", &a[i]) != 1) return 1;
    if (op == 1 || op == 2) {
        for (int i = n - 1; i >= 0; i--) printf("%lld%c", a[i], i ? ' ' : '\n');
    } else {
        long long answer = a[0];
        for (int i = 1; i < n; i++) {
            switch (op) {
                case 3: case 4: answer += a[i]; break;
                case 5: case 6: answer %= a[i]; break;
                case 7: case 8: answer = answer * a[i] % 1000000007; break;
                case 9: case 10: answer /= a[i]; break;
            }
        }
        printf("%lld\n", answer);
    }
    free(a);
    return 0;
}
