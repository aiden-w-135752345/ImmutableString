#include "include/ImmutableString.hpp"
#include <iostream>
int main(){
    ImmutableString<char> foo="hello, "_imm;
    ImmutableString<char> bar="world!\0"_imm;
    ImmutableString<char> baz=foo.slice(2,4)+bar;
    std::cout<<baz<<'\n'<<(baz=="hiworld!\0"_imm)<<'\n';
    return 0;
}
