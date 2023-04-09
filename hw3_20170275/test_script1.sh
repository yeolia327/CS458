COVERAGE=1 make
./kcov-branch-identify_comment grep.c
gcov -b -i kcov-branch-identify_comment