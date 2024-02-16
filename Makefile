all: ImmutableString.o test.out asan.out
	size -t ImmutableString.o test.o test.out

CFLAGS = -Wall -Wextra -Werror -Wpedantic -fdiagnostics-color -std=c++14

ImmutableString.o: ImmutableString.cpp ImmutableString.hpp
	g++ $(CFLAGS) -Os -s -c ImmutableString.cpp -o $@

test.o: ImmutableString.cpp ImmutableString.hpp
	g++ $(CFLAGS) -Os -g -c ImmutableString.cpp -o $@

asan.o: ImmutableString.cpp ImmutableString.hpp
	g++ $(CFLAGS) -Os -g -fsanitize=address -c ImmutableString.cpp -o $@


test.out: test.cpp test.o ImmutableString.hpp
	g++ $(CFLAGS) -Os -g test.cpp test.o -o $@

asan.out: test.cpp asan.o ImmutableString.hpp
	g++ $(CFLAGS) -Os -g -fsanitize=address test.cpp asan.o -o $@
