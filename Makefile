all: ImmutableString.a test.out
	size -t ImmutableString.a test.out

CFLAGS = -Wall -Wextra -Werror -Wpedantic -fdiagnostics-color -std=c++14

ImmutableString.a: build/impl.o
	ar rcu $@ $<

build/impl.o: src/impl.cpp include/ImmutableString.hpp
	g++ $(CFLAGS) -o $@ -Os -s -c $<

test.out: test.cpp src/impl.cpp include/ImmutableString.hpp
	g++ $(CFLAGS) -o $@ -Os -g -fsanitize=address test.cpp src/impl.cpp
