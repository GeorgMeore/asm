CXXFLAGS=-g -fsanitize=address,undefined -Wall -Wextra

all:V: test examples/fib examples/link
	./test # run tests

examples/%: examples/%.cc arena.o asm.o amd64.o
	c++ -I . $CXXFLAGS -o $target $prereq

test: test.cc arena.o asm.o amd64.o
	c++ $CXXFLAGS -o $target $prereq

%.o: %.cc
	c++ -c $CXXFLAGS $prereq
