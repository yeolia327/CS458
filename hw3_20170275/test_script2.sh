COVERAGE=1 make
./kcov-branch-identify grep.i
gcov -b -i kcov-branch-identify