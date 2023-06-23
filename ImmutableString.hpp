// #include <iostream>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <utility>
#include <climits>

template <class T> class ImmutableString{
    struct Node;
    enum Type{CAT,SLICE};
    struct Cat{
        Node*a;Node*b;
    };
    struct Slice{
        struct LongStr{size_t refcount;T data[1];}*str;
        size_t start;
        T*data(){return &str->data[start];}
        static Node*makeLong(size_t len){
            LongStr*str=(LongStr*)malloc(sizeof(LongStr)+(len-1)*sizeof(T));
            str->refcount=1;
            return new Node(Slice{str,0},len);
        };
    };
    struct Node{
        Type type : 1;
        size_t len:CHAR_BIT*sizeof(size_t)-1;
        size_t refcount;
        union{Cat cat;Slice slice;};
        Node(Cat concat,size_t l):type(CAT),len(l),refcount(1),cat(concat){}
        Node(Slice s,size_t l):type(SLICE),len(l),refcount(1),slice(s){}
        void decref(){if(--refcount==0){
            switch(type){
            case CAT:cat.a->decref();cat.b->decref();break;
            case SLICE:if(slice.str->refcount--==0){free(slice.str);}break;
            }
            delete this;
        }}
        Node*flatten(){
            if(type==SLICE){return this;}
            Node* node=Slice::makeLong(len);
            write(node->slice.data());
            decref();
            return node;
        }
        void write(T* data){
            switch(type){
            case CAT:cat.a->write(data);cat.b->write(data+(cat.a->len));break;
            case SLICE:memcpy(data,slice.data(),len*sizeof(T));break;
            }
        }
    };
    uint8_t len;
    union{
        Node* longStr;
        T shortStr[sizeof(Node*)/sizeof(T)];
    }value;
public:
    ImmutableString():len(0){};
    ImmutableString(const T*str, size_t len){
        if(len<=sizeof(Node*)/sizeof(T)){
            this->len=len;
            memcpy(value.shortStr,str,len);
        }else{
            this->len=0xff;
            value.longStr=Slice::makeLong(len);
            memcpy(value.longStr->slice.data(),str,len);
        }
    }
    ImmutableString(const ImmutableString& other):len(other.len),value(other.value){// copy
        if(len==0xff){value.longStr->refcount++;}
    }
    ImmutableString(ImmutableString&& other):ImmutableString(){swap(*this,other);}//move
    ~ImmutableString(){
        if(len==0xff){value.longStr->decref();}
    }
    ImmutableString& operator=(ImmutableString other){swap(*this,other);return *this;}// assign
    friend void swap(ImmutableString& first,ImmutableString& second) noexcept{
        using std::swap;// enable ADL
        swap(first.len,second.len);
        swap(first.value,second.value);
    }
    size_t length() const{return len==0xff?value.longStr->len:len;}
    explicit operator bool()const{return len!=0;}
    
    friend bool operator==(const ImmutableString& lhs, const ImmutableString& rhs){
        if(lhs.length()!=rhs.length()){return false;}
        return memcmp(lhs.data(),rhs.data(),lhs.length())==0;
    }
    friend bool operator!=(const ImmutableString& lhs, const ImmutableString& rhs) {return !(lhs==rhs);}
    static int compare(const ImmutableString& lhs, const ImmutableString& rhs){
        size_t llen=lhs.length(),rlen=rhs.length();
        const T*ldat=lhs.data();const T*rdat=rhs.data();
        if(llen<rlen){return memcmp(ldat,rdat,llen)||-1;}
        if(llen>rlen){return memcmp(ldat,rdat,rlen)||1;}
        return memcmp(ldat,rdat,rlen);
    }
    
    friend bool operator< (const ImmutableString& lhs, const ImmutableString& rhs) { return compare(lhs,rhs) <  0; }
    friend bool operator> (const ImmutableString& lhs, const ImmutableString& rhs) { return compare(lhs,rhs) >  0; }
    friend bool operator<=(const ImmutableString& lhs, const ImmutableString& rhs) { return compare(lhs,rhs) <= 0; }
    friend bool operator>=(const ImmutableString& lhs, const ImmutableString& rhs) { return compare(lhs,rhs) >= 0; }
    const T*data()const{
        if(this->len!=0xff){return value.shortStr;}
        if(value.longStr->type==CAT){((ImmutableString*)this)->value.longStr=value.longStr->flatten();}
        return value.longStr->slice.data();
    }
    char operator[](size_t i)const{return data()[i];}
    friend ImmutableString operator+(const ImmutableString&a,const ImmutableString&b){
        size_t alen=a.length(),blen=b.length(),len=alen+blen;
        ImmutableString ret;
        if(len<=sizeof(Node*)/sizeof(T)){
            ret.len=len;
            memcpy(ret.value.shortStr,a.data(),alen);
            memcpy(ret.value.shortStr+alen,b.data(),blen);
        }else{
            ret.len=0xff;
            Node*anode;Node*bnode;
            if(a.len==0xff){anode=a.value.longStr;}else{
                anode=Slice::makeLong(alen);
                memcpy(anode->slice.data(),a.data(),alen);
            }
            if(b.len==0xff){bnode=b.value.longStr;}else{
                bnode=Slice::makeLong(blen);
                memcpy(bnode->slice.data(),b.data(),blen);
            }
            ret.value.longStr=new Node(Cat{anode,bnode},len);
        }
        return ret;
    }
    ImmutableString slice(size_t start, size_t end)const{
        size_t len=length();
        if(end>len){end=len;}
        len=end-start;
        ImmutableString ret;
        if(len<=sizeof(Node*)/sizeof(T)){
            ret.len=len;
            memcpy(ret.value.shortStr,data(),len);
        }else{
            if(value.longStr->type==CAT){((ImmutableString*)this)->value.longStr=value.longStr->flatten();}
            ret.len=0xff;
            ret.value.longStr=new Node(value.longStr->slice,len);
            ret.value.longStr->slice.start+=start;
            ret.value.longStr->slice.str->refcount++;
        }
        return ret;
    }
};
size_t UTF8toUTF16(const char* utf8_s, size_t utf8_l, char16_t*utf16_s);
ImmutableString<char16_t> UTF8toImmUTF16(const char *utf8_s, size_t utf8_l);
inline ImmutableString<char16_t> operator""_i16(const char *str, size_t len){
    return UTF8toImmUTF16(str,len);
}
inline ImmutableString<char> operator""_i8(const char *str, size_t len){
    return ImmutableString<char>(str,len);
}
