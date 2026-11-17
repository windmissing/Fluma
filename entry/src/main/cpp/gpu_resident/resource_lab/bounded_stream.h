#pragma once
#include <cstring>
#include <streambuf>
#include <ostream>
#include <memory>
#include <string>
namespace grs::resource538 {
inline constexpr size_t ResourceBytes=1048576;
class Buffer:public std::streambuf {
 std::unique_ptr<char[]> bytes;bool failed=false;
 int_type overflow(int_type c)override{failed=true;return traits_type::eof();}
 std::streamsize xsputn(const char* p,std::streamsize n)override{const auto room=epptr()-pptr();if(n<0||n>room){failed=true;return 0;}std::memcpy(pptr(),p,size_t(n));pbump(int(n));return n;}
public:
 explicit Buffer(size_t n=ResourceBytes):bytes(new char[n]){setp(bytes.get(),bytes.get()+n);}
 bool Good()const{return !failed;}std::string Value()const{return std::string(pbase(),pptr());}
};
class Stream:public std::ostream {
 Buffer buffer;
public:explicit Stream(size_t n=ResourceBytes):std::ostream(nullptr),buffer(n){rdbuf(&buffer);}
 std::string str(){return good()&&buffer.Good()?buffer.Value():"{\"revision\":\"T538_BOUNDED_RESOURCE_V1\",\"state\":\"INCOMPLETE_UTF8_LIMIT\",\"originalExit\":\"PRESERVED_IN_LEDGER\"}";}
};
}
