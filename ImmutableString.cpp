#include "ImmutableString.hpp"

size_t UTF8toUTF16(const char* utf8_s, size_t utf8_l, char16_t*utf16_s) {
    size_t utf8_i = 0,utf16_i = 0;
    while (utf8_i < utf8_l) {
        unsigned char utf8_c=utf8_s[utf8_i++];
        char16_t utf16_c=utf8_c|0xEF80;
        if     (utf8_c<0x80){utf16_c=utf8_c;}// ASCII
        else if(utf8_c<0xC0){}// lone continuations
        else if(utf8_c<0xE0){if(utf8_i<utf8_l){// 2-byte
            char16_t hi=utf8_c&0x1F,lo=(unsigned char)utf8_s[utf8_i];
            char32_t unicode=(hi<<6)|(lo&0x3F);
            if(0x80<=lo&&lo<0xC0&&0x7f<unicode){utf16_c=unicode;utf8_i++;}
        }}
        else if(utf8_c<0xF0){if(utf8_i+1<utf8_l){// 3-byte
            char16_t hi=utf8_c&0x0F,med=(unsigned char)utf8_s[utf8_i],lo=(unsigned char)utf8_s[utf8_i+1];
            char16_t unicode=(hi<<12)|((med&0x3F)<<6)|(lo&0x3F);
            if(0x80<=med&&med<0xC0&&0x80<=lo&&lo<0xC0&&0x07FF<unicode){utf16_c=unicode;utf8_i+=2;}
        }}
        else if(utf8_c<0xFE){if(utf8_i+2<utf8_l){// 4-byte
            char32_t hi=utf8_c&0x07,mhi=(unsigned char)utf8_s[utf8_i],mlo=(unsigned char)utf8_s[utf8_i+1],lo=(unsigned char)utf8_s[utf8_i+2];
            char32_t unicode=(hi<<18)|(mhi&0x3F<<12)|((mlo&0x3F)<<6)|(lo&0x3F);
            char16_t high=((unicode-0x10000)>>10)|0xD800;
            char16_t low =((unicode-0x10000)&0x3ff)|0xDC00;
            if(0x80<=mhi&&mhi<0xC0&&0x80<=mlo&&mlo<0xC0&&0x80<=lo&&lo<0xC0&&0xFFFF<unicode&&unicode<0x110000){
                if(utf16_s){utf16_s[utf16_i]=low;}
                utf16_i++;utf16_c=high;utf8_i+=3;
            }
        }}
        else{}// unused bytes
        if(utf16_s){utf16_s[utf16_i]=utf16_c;}
        utf16_i++;
    }
    return utf16_i;
}
ImmutableString<char16_t> UTF8toImmUTF16(const char *utf8_s, size_t utf8_l){
    size_t utf16_l=UTF8toUTF16(utf8_s,utf8_l,nullptr);
    char16_t *utf16_s=(char16_t*)malloc(utf16_l*sizeof(char16_t));
    UTF8toUTF16(utf8_s,utf8_l,utf16_s);
    return ImmutableString<char16_t>(utf16_s,utf16_l);
}