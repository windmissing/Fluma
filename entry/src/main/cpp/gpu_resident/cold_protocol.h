#pragma once
#include "execution_diagnostics.h"
#include <charconv>
#include <memory>
#include <mutex>
namespace grs {namespace cold503 {
inline constexpr size_t MaxTraceBytes=27705344,TraceBudget=55410688,PageBytes=16384,PageWords=1024,DiagnosticBytes=2097152;
inline constexpr uint64_t TtlNs=60000000000ULL;
struct Scope {
 diagnostic492::Scope control{};uint64_t pid=0;
 std::array<char,129> route{},source{};
 bool operator==(const Scope& b)const{
  const auto& a=control;const auto& c=b.control;return a.epoch==c.epoch&&a.run==c.run&&a.context==c.context&&a.surface==c.surface&&a.session==c.session&&a.reset==c.reset&&a.permission==c.permission&&pid==b.pid&&route==b.route&&source==b.source;
 }
 bool operator!=(const Scope& b)const{return !(*this==b);}
};
// A single exact-capacity buffer. No growing stream or intermediate strings.
class Writer {
 char* data_;size_t capacity_,used_=0;bool ok_=true;
public:
 Writer(char* data,size_t capacity):data_(data),capacity_(capacity){}
 bool Append(const char* p,size_t n){if(!ok_||n>capacity_-used_){ok_=false;return false;}std::memcpy(data_+used_,p,n);used_+=n;return true;}
 bool Raw(const char* p){return Append(p,std::strlen(p));}
 bool Number(uint64_t n){char value[21];const auto end=std::to_chars(value,value+sizeof(value),n);return Append(value,size_t(end.ptr-value));}
 bool U64(uint64_t n){return Raw("\"")&&Number(n)&&Raw("\"");}
 bool Bool(bool b){return Raw(b?"true":"false");}
 static size_t Utf8(const char* p,size_t n){if(!n)return 0;const auto c=uint8_t(p[0]);const size_t length=c>=0xc2&&c<=0xdf?2:c>=0xe0&&c<=0xef?3:c>=0xf0&&c<=0xf4?4:0;if(!length||length>n)return 0;for(size_t i=1;i<length;++i)if((uint8_t(p[i])&0xc0)!=0x80)return 0;if((c==0xe0&&uint8_t(p[1])<0xa0)||(c==0xed&&uint8_t(p[1])>=0xa0)||(c==0xf0&&uint8_t(p[1])<0x90)||(c==0xf4&&uint8_t(p[1])>=0x90))return 0;return length;}
 bool String(const char* p,size_t size){
  size_t need=2;for(size_t i=0;i<size;++i){const auto c=uint8_t(p[i]);const size_t utf=c>=128?Utf8(p+i,size-i):0;const size_t n=utf?utf:c=='"'||c=='\\'?2:c<32||c>=127?6:1;if(n>SIZE_MAX-need){ok_=false;return false;}need+=n;if(utf)i+=utf-1;}
  if(!ok_||need>capacity_-used_){ok_=false;return false;}
  Raw("\"");constexpr char hex[]="0123456789abcdef";
  for(size_t i=0;i<size;++i){const auto c=uint8_t(p[i]);const size_t utf=c>=128?Utf8(p+i,size-i):0;if(utf){Append(p+i,utf);i+=utf-1;}else if(c=='"'||c=='\\'){char escape[]={'\\',char(c)};Append(escape,2);}else if(c<32||c>=127){char escape[]={'\\','u','0','0',hex[c>>4],hex[c&15]};Append(escape,6);}else Append(p+i,1);}return Raw("\"");
 }
 template<size_t N>bool String(const std::array<char,N>& s){size_t n=0;while(n<N&&s[n])++n;return String(s.data(),n);}
 bool Hex(const void* p,size_t n){if(n>(capacity_-used_)/2||2*n+2>capacity_-used_){ok_=false;return false;}Raw("\"");const auto* b=static_cast<const uint8_t*>(p);constexpr char h[]="0123456789abcdef";for(size_t i=0;i<n;++i){char v[]={h[b[i]>>4],h[b[i]&15]};Append(v,2);}return Raw("\"");}
 void Reset(){used_=0;ok_=true;}
 bool Good()const{return ok_;}char* Data(){return data_;}size_t Size()const{return used_;}size_t Capacity()const{return capacity_;}
};
inline void ControlJson(Writer& w,const diagnostic492::Scope& s){w.Raw("{\"epoch\":");w.U64(s.epoch);w.Raw(",\"run\":");w.U64(s.run);w.Raw(",\"context\":");w.U64(s.context);w.Raw(",\"surface\":");w.U64(s.surface);w.Raw(",\"session\":");w.U64(s.session);w.Raw(",\"resetSerial\":");w.U64(s.reset);w.Raw(",\"permissionRevision\":");w.U64(s.permission);w.Raw("}");}
inline void ScopeJson(Writer& w,const Scope& s){w.Raw("{\"pid\":");w.U64(s.pid);w.Raw(",\"route\":");w.String(s.route);w.Raw(",\"source\":");w.String(s.source);w.Raw(",\"control\":");ControlJson(w,s.control);w.Raw("}");}
inline void Envelope(Writer& w,const char* state){w.Raw("{\"revision\":\"T503_BOUNDED_COLD_V1\",\"state\":");w.String(state,std::strlen(state));w.Raw("}");}
inline constexpr const char* Upgrade="{\"state\":\"UNSUPPORTED_COLD_REVISION\",\"required\":\"T503_BOUNDED_COLD_V1\"}";
inline size_t ProfileWords(uint32_t k,uint32_t kw){if((k!=192&&k!=512)||(kw!=64&&kw!=128))return 0;const uint64_t state=4ull*(16*k+16*kw+2304);const uint64_t quantum=256ull*(4*(48*k+16*kw+254)+k);const uint64_t source=512ull*(2*k+1);return size_t((1024+state+std::max(quantum,source))/4);}
struct Gate {bool active=false;uint64_t copied=0,busy=0;
 const char* Begin(const Scope& expected,const Scope& actual,size_t remaining){if(expected!=actual)return "STALE_SCOPE";if(active){++busy;return "DIAGNOSTIC_BUSY";}if(remaining<PageBytes)return "DIAGNOSTIC_BUDGET";active=true;return "OK";}
 void End(){active=false;}
};
struct GateLease {Gate& gate;std::mutex& mutex;~GateLease(){std::lock_guard<std::mutex> lock(mutex);gate.End();}};
}}
