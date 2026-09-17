# Moo Chant IV

Farmer John has a special process that improves milk extraction from his cows by 0.001%. He dances around the cow while shouting a specific string. His neighbors think he's strange, so he requires that you make a fool out of yourself too! Farmer John supplies the chant; you say it.

Read K from the first line, then the entire chant until EOF. Print the chant K times, with no added separators. K can be zero; then print nothing.

Ignore one final line ending in the input chant. Keep every other newline. For example, repeating `moomoo\nm` three times joins each final `m` directly to the next `moomoo`.

0 ≤ K ≤ 100000. K × chant length ≤ 1000000.
Chant length: 0 to 100000 characters, excluding one final line ending. Characters are printable ASCII or newlines.
Preserve spaces and blank lines. A final output newline is optional.

Time limit: 2 seconds per test.

Example Input:
```text
3
moomoo
m
```

Example Output:
```text
moomoo
mmoomoo
mmoomoo
m
```
