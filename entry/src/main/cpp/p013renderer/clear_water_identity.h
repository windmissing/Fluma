// T258 / T254: C V3 identity only. Other routes retain their existing String limits.
#pragma once
#include <napi/native_api.h>
#include <string>
#include <vector>
#include <cstring>
#include <limits>
namespace p013 {
enum class ClearWaterIdentityRead {OK,TYPE,LENGTH,CONTENT,NAPI_ERROR};
inline ClearWaterIdentityRead ReadClearWaterUtf8(napi_env e,napi_value value,size_t limit,std::string& out){
 out.clear();napi_valuetype type;if(napi_typeof(e,value,&type)!=napi_ok)return ClearWaterIdentityRead::NAPI_ERROR;
 if(type!=napi_string)return ClearWaterIdentityRead::TYPE;
 size_t length=0;if(napi_get_value_string_utf8(e,value,nullptr,0,&length)!=napi_ok)return ClearWaterIdentityRead::NAPI_ERROR;
 if(length>limit||length==std::numeric_limits<size_t>::max())return ClearWaterIdentityRead::LENGTH;
 if(length==0)return ClearWaterIdentityRead::CONTENT;
 std::vector<char> bytes(length+1,0);size_t copied=0;
 if(napi_get_value_string_utf8(e,value,bytes.data(),bytes.size(),&copied)!=napi_ok||copied!=length)return ClearWaterIdentityRead::NAPI_ERROR;
 if(bytes[length]!=0||std::memchr(bytes.data(),0,length))return ClearWaterIdentityRead::CONTENT;
 out.assign(bytes.data(),length);return ClearWaterIdentityRead::OK;
}
inline bool ReadClearWaterIdentity(napi_env e,napi_value object,std::string& token,std::string& digest,const char*& stage){
 token.clear();digest.clear();const char* names[]={"frameToken","configDigest"};const size_t limits[]={8192,4096};std::string* outputs[]={&token,&digest};
 for(size_t i=0;i<2;i++){napi_value value=nullptr;auto code=napi_get_named_property(e,object,names[i],&value)==napi_ok?ReadClearWaterUtf8(e,value,limits[i],*outputs[i]):ClearWaterIdentityRead::NAPI_ERROR;
  if(code!=ClearWaterIdentityRead::OK){token.clear();digest.clear();stage=code==ClearWaterIdentityRead::LENGTH?"CW_V3_PARSE_IDENTITY_LENGTH":"CW_V3_PARSE_IDENTITY";return false;}}
 return true;
}
}
