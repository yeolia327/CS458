ifeq (, $(shell which llvm-config))
$(error "No llvm-config in $$PATH")
endif

LLVMVER  = $(shell llvm-config --version 2>/dev/null | sed 's/git//' | sed 's/svn//' )
LLVM_MAJOR = $(shell llvm-config --version 2>/dev/null | sed 's/\..*//' )
LLVM_MINOR = $(shell llvm-config --version 2>/dev/null | sed 's/.*\.//' | sed 's/git//' | sed 's/svn//' | sed 's/ .*//' )
$(info Detected LLVM VERSION : $(LLVMVER))

CC=clang
CXX=clang++
CFLAGS=`llvm-config --cflags` -fPIC -ggdb -O0
AR=ar

CXXFLAGS=`llvm-config --cxxflags` -fPIC -ggdb -O0

MAKEFILE_PATH=$(abspath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR:=$(dir $(MAKEFILE_PATH))

.PHONY : clean all intwrite intwrite_test ccov

all: intwrite ccov

intwrite: lib/libintwrite.so lib/intwrite-rt.o

lib/libintwrite.so: intwrite/intwrite-pass.cpp
	mkdir -p lib
	$(CXX) $(CXXFLAGS) -shared $< -o $@

lib/intwrite-rt.o: intwrite/intwrite-rt.c
	mkdir -p lib
	$(CC) $(CFLAGS) -c $< -o $@

intwrite_test: test/example.c intwrite 
	$(CC) --ld-path=ld.lld -fno-experimental-new-pass-manager $< lib/intwrite-rt.o -g -O0 -Xclang -load -Xclang lib/libintwrite.so -o ./test/example
	./test/example
	cat log

ccov: lib/libccov.so lib/ccov-rt.o

lib/libccov.so: ccov/ccov-pass.cpp
	mkdir -p lib
	$(CXX) $(CXXFLAGS) -shared $< -o $@

lib/ccov-rt.o: ccov/ccov-rt.c
	mkdir -p lib
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f lib/*.so lib/*.o test/example log
