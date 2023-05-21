# Simple Branch Coverage Analysis Based on LLVM IR

## Requirement
LLVM, Clang

## how to use
1. place a c file that you want to analyze in test folder

2. run make

3. run below command (grep, for example)
```
clang --ld-path=ld.lld -fno-experimental-new-pass-manager test/grep/grep.c lib/ccov-rt.o -g -O0 -Xclang -load -Xclang lib/libccov.so -o ./test/grep/grep
```

4. run target file, then you will get coverage.dat file, which contains branch coverage information
```
./grep "if" grep.c
```


coverage.dat
```
br: 147.0 -> 0, 0
br: 175.0 -> 2, 0
br: 175.1 -> 2, 0
br: 188.0 -> 0, 0
br: 192.0 -> 0, 0

...

sw: 657.3 -> 0, 0
sw: 657.4 -> 0, 0
sw: 657.5 -> 0, 0
sw: 657.6 -> 0, 0

...

br: 9726.0 -> 1249, 15240

...

br: 10906.2 -> 0, 0
br: 10921.0 -> 0, 0
br: 10926.0 -> 0, 0
br: 10926.1 -> 0, 0
Total: 3419 branches, Covered: 350 branches

```

Here, the br means br instrument in LLVM IR and sw means switch in LLVM IR

br: 9726.0 -> 1249, 15240

means first branch at line 9726 taken 1249 time, not take 15240 times

