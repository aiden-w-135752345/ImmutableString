#include <cstring>
#include <memory>
#include "../deps/backports/include/variant.hpp"
class ImmutableStringImpl{
    struct Node;
    struct Cat{std::shared_ptr<Node>a;std::shared_ptr<Node>b;};
    static std::shared_ptr<char> alloc(size_t len){return{new char[len-1],std::default_delete<char[]>()};}
    struct Slice{
        std::shared_ptr<char>buf;const size_t start;const char*data()const{return start+buf.get();}
    };
    struct Node{
        size_t len;
        backports::variant<Cat,std::shared_ptr<char>>value;
        Node(const char*str,size_t l):len(l),value(alloc(l)){
            memcpy(backports::get<std::shared_ptr<char>>(value).get(),str,len);
        }
        Node(std::shared_ptr<Node>a,std::shared_ptr<Node>b)noexcept:len(a->len+b->len),value(Cat{a,b}){}
        Node(std::shared_ptr<char>toSlice,size_t start,size_t l)noexcept:len(l),value(std::shared_ptr<char>(toSlice,toSlice.get()+start)){}
        __attribute__((always_inline)) const std::shared_ptr<char>&flatten(){
            if(backports::holds_alternative<Cat>(value)){
                std::shared_ptr<char>buf=alloc(len);
                write(buf.get());
                value=buf;
            }
            return backports::get<std::shared_ptr<char>>(value);
        }
        void write(char*)const noexcept;
    };
    struct ShortStr{char value[sizeof(std::shared_ptr<Node>)-1];char space;};
    backports::variant<ShortStr,std::shared_ptr<Node>>value;
    ImmutableStringImpl(std::shared_ptr<Node>node):value(node){}
public:
    ImmutableStringImpl():value(ShortStr{{0},sizeof(std::shared_ptr<Node>)}){}
    ImmutableStringImpl(const char*str, size_t len){
        if(len<sizeof(std::shared_ptr<Node>)){
            value.emplace<ShortStr>();
            ShortStr&shortStr=backports::get<ShortStr>(value);
            memcpy(shortStr.value,str,len);
            shortStr.space=sizeof(std::shared_ptr<Node>)-len;
        }else{
            value=std::make_shared<Node>(str,len);
        }
    }
private:
    struct length_visitor{
        size_t operator()(const ShortStr&str){return sizeof(std::shared_ptr<Node>)-str.space;}
        size_t operator()(const std::shared_ptr<Node>&str){return str->len;}
    };
public:
    size_t length() const{return backports::visit(length_visitor{},value);}
    explicit operator bool()const{return length()!=0;}
    friend bool operator==(const ImmutableStringImpl& a, const ImmutableStringImpl& b){
        if(a.length()!=b.length()){return false;}
        return memcmp(a.data(),b.data(),a.length())==0;
    }
private:
    struct data_visitor{
        const char* operator()(const ShortStr&str){return str.value;}
        const char* operator()(const std::shared_ptr<Node>&str){return str->flatten().get();}
    };
    struct ToNode_visitor{
        std::shared_ptr<Node> operator()(const ShortStr&str){return std::make_shared<Node>(str.value,sizeof(std::shared_ptr<Node>)-str.space);}
        std::shared_ptr<Node> operator()(const std::shared_ptr<Node>&str){return str;}
    };
public:
    const char*data()const{return backports::visit(data_visitor{},value);}
    friend ImmutableStringImpl operator+(const ImmutableStringImpl&a,const ImmutableStringImpl&b){
        size_t alen=a.length(),blen=b.length(),len=alen+blen;
        if(len<=sizeof(std::shared_ptr<Node>)){
            char str[sizeof(std::shared_ptr<Node>)];
            memcpy(str,a.data(),sizeof(std::shared_ptr<Node>));// alen
            memcpy(str+alen,b.data(),blen);
            return ImmutableStringImpl(str,len);
        }else{
            std::shared_ptr<Node>aStr=backports::visit(ToNode_visitor{},a.value);
            std::shared_ptr<Node>bStr=backports::visit(ToNode_visitor{},b.value);
            return ImmutableStringImpl(std::make_shared<Node>(aStr,bStr));
        }
    }
    ImmutableStringImpl slice(size_t start, size_t end)const{
        size_t len=length();
        if(end>len){end=len;}
        len=end-start;
        if(len<=sizeof(std::shared_ptr<Node>)){
            return ImmutableStringImpl(data()+start,len);
        }else{
            return ImmutableStringImpl(std::make_shared<Node>(backports::get<std::shared_ptr<Node>>(value)->flatten(),start,len));
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
    size_t length() const{return value.length()/sizeof(T);}
    explicit operator bool()const{return (bool)value;}
    friend bool operator==(const ImmutableString& a, const ImmutableString& b){return a.value==b.value;}
    friend bool operator!=(const ImmutableString& a, const ImmutableString& b){return !(a==b);}
    template<class Cmp>
    static int compare(const ImmutableString& a, const ImmutableString& b,Cmp cmp){
        size_t alen=a.length(),blen=b.length();
        const T*adat=a.data();const T*bdat=b.data();
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
template class ImmutableString<char16_t>;//template <char16_t> ImmutableString<char16_t> operator+(const ImmutableString<char16_t>&,const ImmutableString<char16_t>&);
template class ImmutableString<char>;    //template <char> ImmutableString<char> operator+(const ImmutableString<char>&,const ImmutableString<char>&);
template class ImmutableString<char32_t>;//template <char32_t> ImmutableString<char32_t> operator+(const ImmutableString<char32_t>&,const ImmutableString<char32_t>&);

size_t UTF8toUTF16(const char* utf8_s, size_t utf8_l, char16_t*utf16_s);
ImmutableString<char16_t> UTF8toImmUTF16(const char *utf8_s, size_t utf8_l);
inline ImmutableString<char16_t> operator""_imm(const char16_t *str, size_t len){
    return ImmutableString<char16_t>(str,len);
}
inline ImmutableString<char> operator""_imm(const char *str, size_t len){
    return ImmutableString<char>(str,len);
}
#include <ostream>

template<class T>struct std::hash<ImmutableString<T>>{
    using result_type = size_t;using argument_type = ImmutableString<T>;
    size_t operator()(const ImmutableString<T>& t) const noexcept{
        return std::_Hash_impl::hash(t.data(),t.length()*sizeof(T));
        //return std::hash<std::string>()(std::string((const char*)t.data(),(const char*)(t.data()+t.length())));
    }
};
template<typename T, typename Traits> inline std::basic_ostream<T, Traits>&
    operator<<(std::basic_ostream<T, Traits>& stream,ImmutableString<T> str)
{ return stream.write(str.data(),str.length()); }
