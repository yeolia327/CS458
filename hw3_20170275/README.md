# Simple Branch Coverage Analysis

## Requirement
Clang

## how to use
1. you need a preprocessed single C file sample.i

ex: gcc -E grep.c -o grep.i

2. run make Then you will get kcov-branch-identify executable file.

3. run below command
```
./kcov-branch-identify grep.i
```

then you will get sampel-cov.i file and branch information like below.

```
...
        For      ID: 1659   Line: 12693                Col: 2          Filename: grep.i
        If      ID: 1660   Line: 12695                Col: 6          Filename: grep.i
        If      ID: 1661   Line: 12705                Col: 6          Filename: grep.i
        If      ID: 1662   Line: 12728                Col: 3          Filename: grep.i
        While      ID: 1663   Line: 12733                Col: 3          Filename: grep.i
Total number of branches: 3079
```

4. Compile sample-cov.i file
```
gcc grep-cov.i -o grep-cov
```

5. execute sample-cov file, then you will get coverage.dat file, which contains branch coverage information

```
./grep-cov -n "if" grep.c
```

coverage.dat
```
Line# | # then | # else | conditional expression

...

12728       0       0           (end = memchr(beg + len, '
', (buf + size) - (beg + len))) != 0
12733       0       0           beg > buf && beg[-1] != '
'
Covered: 330 / Total: 3079 = 10.717766%
```