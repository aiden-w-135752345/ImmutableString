#include "ImmutableString.hpp"
#include <cstdio>
void inside(){
    ImmutableString<char> foo="hello, "_i8;
    ImmutableString<char> bar="world!\0"_i8;
    ImmutableString<char> baz=foo.slice(2,4)+bar;
//    ImmutableString<char> bup=copy(baz);
    printf("%s %d \n",baz.data(),baz=="hiworld!\0"_i8);
}
int main(){
    inside();
    return 0;
}
