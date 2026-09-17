# Collatz

The Collatz Conjecture hypothesizes that this sequence always reaches 1 when starting with a positive integer n.

1. If n = 1, stop.
2. If n is even, set n = n / 2 and return to step 1.
3. Otherwise, set n = 3 * n + 1 and return to step 1.
4. You will never reach this step.

Read n (1 ≤ n ≤ 10^9). Print every value, including the starting n and the final 1. Separate values with spaces.

Use `long long`: intermediate values can exceed 10^9.

Time limit: 2 seconds per test.

Example Input:
```text
10
```

Example Output:
```text
10 5 16 8 4 2 1
```
