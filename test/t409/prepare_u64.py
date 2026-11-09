from pathlib import Path
s=Path('entry/src/main/cpp/gpu_resident/app_bridge.cpp').read_text()
a=s.index('bool PageU64(');b=s.index('#include "evidence_capture_napi.inc"',a)
parser=s[a:b]
head=r'''
#include <string>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <algorithm>
#include <iostream>
using napi_env=void*;using napi_value=const std::string*;constexpr int napi_ok=0;
int napi_get_value_string_utf8(napi_env,napi_value value,char* buffer,size_t size,size_t* result){
 if(!buffer){*result=value->size();return napi_ok;}
 const auto n=std::min(value->size(),size-1);std::memcpy(buffer,value->data(),n);buffer[n]=0;*result=n;return napi_ok;
}
'''
tail=r'''
int main(){uint64_t n;for(const std::string s:{"0","1","18446744073709551615"})assert(PageU64(nullptr,&s,n));
 for(const std::string s:{"","00","01","-1","+1","1.0"," 1","1 ","18446744073709551616","100000000000000000000","12345678901234567890trailing"})assert(!PageU64(nullptr,&s,n));
 std::cout<<"T409 production PageU64 body: exact max, overflow and full-length-before-truncation negative controls PASS\n";}
'''
Path('.codex/local-test/t409-host/u64.cpp').write_text(head+parser+tail)
