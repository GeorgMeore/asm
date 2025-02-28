CXXFLAGS=-g -fsanitize=address,undefined -Wall -Wextra

all:V: test examples/fib examples/link
	./test # run tests

examples/%: examples/%.cc libasm.a
	c++ -L . -I . $CXXFLAGS -o $target $prereq

test: test.cc libasm.a
	c++ $CXXFLAGS -o $target $prereq

libasm.a: arena.o asm.o amd64.o
	ar crs $target $prereq

%.o: %.cc
	c++ -c $CXXFLAGS $prereq
