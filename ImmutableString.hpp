#include <cstring>
#include <new>
#include <cstdlib>
#include <climits>
class ImmutableStringImpl{
    struct Buf{mutable size_t refcount;char data[1];};
    struct Node{
        struct Cat{const Node*const a;const Node*const b;};
        struct Slice{const Buf*const buf;const size_t start;const char*data()const{return &buf->data[start];}};
        
        mutable enum:bool{CAT,SLICE} type : 1;
        const size_t len:CHAR_BIT*sizeof(size_t)-1;
        mutable size_t refcount;
        union{const Cat cat;mutable Slice slice;};
        
        static const Buf*Node_impl(const char*str,size_t len) noexcept;
        const Buf*flatten_impl()const noexcept;
        static Slice wrap_impl(const Buf*buf){
            if(buf==nullptr){throw std::bad_alloc();}
            buf->refcount=1;return Slice{buf,0};
        }
        Node(const char*str,size_t l):type(SLICE),len(l),refcount(1),slice(wrap_impl(Node_impl(str,l))){}
        Node(const Node*a,const Node*b)noexcept:type(CAT),len((a->len)+(b->len)),refcount(1),cat(Cat{a,b}){}
        Node(const Slice&toSlice,size_t start,size_t l)noexcept:type(SLICE),len(l),refcount(1),slice(Slice{toSlice.buf,toSlice.start+start}){
            toSlice.buf->refcount++;
        }
        const Node* incref()const{refcount++;return this;}
        void decref()const{if(0==--refcount){
            switch(type){
            case CAT:cat.a->decref();cat.b->decref();break;
            case SLICE:if(0==--(slice.buf->refcount)){free((Buf*)slice.buf);}break;
            }
            delete this;
        }}
        __attribute__((always_inline)) const Slice&flatten()const{
            if(type!=SLICE){
                new(&slice) Slice(wrap_impl(flatten_impl()));
                type=SLICE;
            }
            return slice;
        }
        void write(char*)const noexcept;
    };
    unsigned char len;
    union{
        const Node* longStr;
        char shortStr[sizeof(Node*)];
    }value;
    ImmutableStringImpl(Node*node):len(0xff){value.longStr=node;}
public:
    ImmutableStringImpl():len(0){};
    ImmutableStringImpl(const char*str, size_t l){
        if(l<=sizeof(Node*)){
            len=l;
            memcpy(value.shortStr,str,sizeof(Node*));//l
        }else{
            len=0xff;
            value.longStr=new Node(str,l);
        }
    }
    ImmutableStringImpl(const ImmutableStringImpl& that)noexcept:len(that.len),value(that.value){// copy
        if(len==0xff){value.longStr->refcount++;}
    }
    ImmutableStringImpl(ImmutableStringImpl&& that):len(that.len),value(that.value){that.len=0;}//move
    __attribute__((always_inline)) ~ImmutableStringImpl(){if(len==0xff){value.longStr->decref();}}
    ImmutableStringImpl& operator=(ImmutableStringImpl) = delete;// assign
    friend void swap(ImmutableStringImpl& a,ImmutableStringImpl& b){
        using std::swap;// enable ADL
        swap(a.len,b.len);
        swap(a.value,b.value);
    }
    size_t length() const{return len==0xff?value.longStr->len:len;}
    explicit operator bool()const{return len!=0;}
    
    friend bool operator==(const ImmutableStringImpl& a, const ImmutableStringImpl& b){
        if(a.length()!=b.length()){return false;}
        return memcmp(a.data(),b.data(),a.length())==0;
    }
    const char*data()const{
        if(len!=0xff){return value.shortStr;}
        return value.longStr->flatten().data();
    }
    friend ImmutableStringImpl operator+(const ImmutableStringImpl&a,const ImmutableStringImpl&b){
        size_t alen=a.length(),blen=b.length(),len=alen+blen;
        if(len<=sizeof(Node*)){
            ImmutableStringImpl ret;
            ret.len=len;
            memcpy(ret.value.shortStr,a.data(),sizeof(Node*));// alen
            memcpy(ret.value.shortStr+alen,b.data(),blen);
            return ret;
        }else{
            const Node* freeA=nullptr;const Node* freeB=nullptr;
            try{
                const Node*aStr=a.len==0xff?a.value.longStr->incref():(freeA=new Node(a.data(),alen));
                const Node*bStr=b.len==0xff?b.value.longStr->incref():(freeB=new Node(b.data(),blen));
                return ImmutableStringImpl(new Node(aStr,bStr));
            }catch(...){delete freeA;delete freeB;throw;}
        }
    }
    ImmutableStringImpl slice(size_t start, size_t end)const{
        size_t len=length();
        if(end>len){end=len;}
        len=end-start;
        if(len<=sizeof(Node*)){
            ImmutableStringImpl ret;
            ret.len=len;
            memcpy(ret.value.shortStr,data(),sizeof(Node*));// len
            return ret;
        }else{
            return ImmutableStringImpl(new Node(value.longStr->flatten(),start,len));
        }
    }
    int indexOf(const ImmutableStringImpl&needleStr,int fromIndex,int step)const{
        size_t haystackLen=length(),needleLen=needleStr.length();
        const char*haystack=data();const char*needle=needleStr.data();
        for(size_t i=fromIndex;i+needleLen<=haystackLen;i+=step){
            if(memcmp(haystack+i,needle,needleLen)==0){return i;}
        }
        return -1;
    }
};
template<class T> class ImmutableString{
    static_assert(std::is_trivially_copyable<T>::value,"must be copyable via memcpy");
    ImmutableStringImpl value;
    ImmutableString(ImmutableStringImpl&& that):value(that){}
public:
    ImmutableString():value(){};
    ImmutableString(const T*str, size_t len):value((const char*)str,len*sizeof(T)){}
    ImmutableString(const ImmutableString& that):value(that.value){}// copy
    ImmutableString(ImmutableString&& that):value((ImmutableStringImpl&&)that.value){}//move
    ImmutableString& operator=(ImmutableString that){swap(*this,that);return *this;}// assign
    friend void swap(ImmutableString& a,ImmutableString& b){swap(a.value,b.value);}
    size_t length() const{return value.length()/sizeof(T);}
    explicit operator bool()const{return (bool)value;}
    friend bool operator==(const ImmutableString& a, const ImmutableString& b){return a.value==b.value;}
    friend bool operator!=(const ImmutableString& a, const ImmutableString& b){return !(a==b);}
    template<class Cmp>
    static int compare(const ImmutableString& a, const ImmutableString& b,Cmp cmp){
        size_t alen=a.length(),blen=b.length();
        const T*adat=a.value.data();const T*bdat=b.value.data();
        size_t i=0,minlen=alen<blen?alen:blen;
        for(; i<minlen;i++){
            auto c=cmp(adat[i],bdat[i]);
            if(c!=0){return c;}
        }
        return (blen<alen)-(alen<blen);
    }
private:
    struct DefaultCmp{int operator()(const T&a,const T&b){return (b<a)-(a<b);}};
public:
    friend bool operator< (const ImmutableString& a, const ImmutableString& b) { return compare(a,b,DefaultCmp()) <  0; }
    friend bool operator> (const ImmutableString& a, const ImmutableString& b) { return compare(a,b,DefaultCmp()) >  0; }
    friend bool operator<=(const ImmutableString& a, const ImmutableString& b) { return compare(a,b,DefaultCmp()) <= 0; }
    friend bool operator>=(const ImmutableString& a, const ImmutableString& b) { return compare(a,b,DefaultCmp()) >= 0; }
    const T*data()const{return (T*)value.data();}
    const T&operator[](size_t i)const{return data()[i];}
    friend ImmutableString operator+(const ImmutableString&a,const ImmutableString&b){return a.value+b.value;}
    ImmutableString slice(size_t start, size_t end)const{return value.slice(start*sizeof(T),end*sizeof(T));}
    int indexOf(const ImmutableString&needleStr,int fromIndex)const{
        return value.indexOf(needleStr.value,fromIndex*sizeof(T),sizeof(T));
    }
};
template class ImmutableString<char16_t>;template <char16_t> ImmutableString<char16_t> operator+(const ImmutableString<char16_t>&,const ImmutableString<char16_t>&);
template class ImmutableString<char>; template <char> ImmutableString<char> operator+(const ImmutableString<char>&,const ImmutableString<char>&);
template class ImmutableString<char32_t>;template <char32_t> ImmutableString<char32_t> operator+(const ImmutableString<char32_t>&,const ImmutableString<char32_t>&);

size_t UTF8toUTF16(const char* utf8_s, size_t utf8_l, char16_t*utf16_s);
ImmutableString<char16_t> UTF8toImmUTF16(const char *utf8_s, size_t utf8_l);
inline ImmutableString<char16_t> operator""_i16(const char *str, size_t len){
    return UTF8toImmUTF16(str,len);
}
inline ImmutableString<char> operator""_i8(const char *str, size_t len){
    return ImmutableString<char>(str,len);
}
