#pragma once
#include <napi/native_api.h>
#include <array>
#include <cstdint>
#include <cstring>
namespace grs {
struct BridgeDecode {const char* stage="GET_CB_INFO";napi_status status=napi_ok;size_t argc=0,length=0,offset=0,backingBytes=0;int type=-1;bool detached=false;std::array<uint32_t,32> words{};};
inline BridgeDecode DecodeBridgeCommand(napi_env e,napi_callback_info info){
 BridgeDecode r;napi_value args[2]{};r.argc=2;r.status=napi_get_cb_info(e,info,&r.argc,args,nullptr,nullptr);if(r.status!=napi_ok)return r;if(r.argc!=1){r.stage="ARGC";return r;}
 bool buffer=false;r.stage="IS_ARRAYBUFFER";r.status=napi_is_arraybuffer(e,args[0],&buffer);if(r.status!=napi_ok)return r;
 napi_value backing=args[0];void* data=nullptr;
 if(!buffer){bool typed=false;r.stage="IS_TYPEDARRAY";r.status=napi_is_typedarray(e,args[0],&typed);if(r.status!=napi_ok||!typed)return r;napi_typedarray_type type;r.stage="TYPEDARRAY_INFO";r.status=napi_get_typedarray_info(e,args[0],&type,&r.length,&data,&backing,&r.offset);if(r.status!=napi_ok)return r;r.type=int(type);if(type!=napi_uint32_array){r.stage="TYPEDARRAY_TYPE";return r;}if(r.length!=32){r.stage="TYPEDARRAY_LENGTH";return r;}if(r.offset!=0){r.stage="TYPEDARRAY_OFFSET";return r;}}
 r.stage="DETACHED";r.status=napi_is_detached_arraybuffer(e,backing,&r.detached);if(r.status!=napi_ok||r.detached)return r;
 void* raw=nullptr;r.stage="ARRAYBUFFER_INFO";r.status=napi_get_arraybuffer_info(e,backing,&raw,&r.backingBytes);if(r.status!=napi_ok)return r;
 if(r.backingBytes!=128){r.stage="BACKING_LENGTH";return r;}if(!raw||(!buffer&&data!=raw)){r.stage="BACKING_POINTER";return r;}
 std::memcpy(r.words.data(),raw,128);r.stage="OK";return r;
}
}
