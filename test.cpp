#include "ImmutableString.hpp"
#include <cstdio>

int main(){
    ImmutableString<char> foo="hello, "_i8;
    ImmutableString<char> bar="world!\0"_i8;
    ImmutableString<char> baz=foo.slice(2,4)+bar;
    printf("%s %d",baz.data(),baz=="hiworld!\0"_i8);
}
