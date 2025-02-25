CXXFLAGS=-g -fsanitize=address,undefined -Wall -Wextra

all:V: test example
	./test # run tests

example: example.cc arena.o asm.o amd64.o
	c++ $CXXFLAGS -o $target $prereq

test: test.cc arena.o asm.o amd64.o
	c++ $CXXFLAGS -o $target $prereq

%.o: %.cc
	c++ -c $CXXFLAGS $prereq
