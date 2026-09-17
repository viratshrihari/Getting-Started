import sys
from functools import reduce

op = @OP@
if op == -1:
    print('moo')
    sys.exit(0)
if op >= 11:
    if op == 11:
        n = int(input())
        sequence = [n]
        while n != 1:
            n = n // 2 if n % 2 == 0 else 3 * n + 1
            sequence.append(n)
        print(*sequence)
    elif op == 12:
        print(input())
    elif op == 13:
        nohj, john = input(), input()
        print('-1' if len(nohj) == len(john) else 'nohj' if len(nohj) > len(john) else 'john')
    elif op == 14:
        n = int(input())
        names = [input() for _ in range(n)]
        longest = max(names, key=len)
        print(len(longest), longest, sep='\n')
    elif op == 15:
        print(sys.stdin.read(), end='')
    else:
        copies = int(input())
        chant = sys.stdin.read().removesuffix('\n')
        if copies and chant:
            print(chant * copies)
    sys.exit(0)

values = list(map(int, sys.stdin.read().split()))
if op in (2, 4, 6, 8, 10):
    values = values[1:]

if op in (1, 2):
    print(*values[::-1])
elif op in (3, 4):
    print(sum(values))
elif op in (5, 6):
    print(reduce(lambda a, b: a % b, values))
elif op in (7, 8):
    print(reduce(lambda a, b: a * b % 1000000007, values))
elif op in (9, 10):
    print(reduce(lambda a, b: a // b, values))
else:
    print(values[0])
