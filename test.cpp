#include "ImmutableString.hpp"
#include <cstdio>
void inside(){
    ImmutableString<char> foo="hello, "_imm;
    ImmutableString<char> bar="world!\0"_imm;
    ImmutableString<char> baz=foo.slice(2,4)+bar;
//    ImmutableString<char> bup=copy(baz);
    printf("%s %d \n",baz.data(),baz=="hiworld!\0"_imm);
}
int main(){
    inside();
    return 0;
}
