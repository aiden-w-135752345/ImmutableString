all: ImmutableString.o a.out

CFLAGS = -g -Wall -pedantic -Wextra -Werror

ImmutableString.o: ImmutableString.cpp ImmutableString.hpp
	g++ -c $(CFLAGS) ImmutableString.cpp -o $@


a.out: test.cpp ImmutableString.o ImmutableString.hpp
	g++ $(CFLAGS) test.cpp ImmutableString.o -o $@
