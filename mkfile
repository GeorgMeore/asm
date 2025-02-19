CXXFLAGS=-g -fsanitize=address,undefined -Wall -Wextra

all:V: test example
	./test # run tests

example: example.cc asm.o amd64.o
	c++ $CXXFLAGS -o $target $prereq

test: test.cc asm.o amd64.o
	c++ $CXXFLAGS -o $target $prereq

%.o: %.cc
	c++ -c $CXXFLAGS $prereq
