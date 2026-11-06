#pragma once
#include <array>
#include <cstdint>
#include <cstring>

// T385: the Harmony 32-bit TypedArray information reports bytes.
struct T385RoiDecode {
 const char* stage="NOT_DECODED";
 int status=-1,type=-1;
 size_t rawBytes=0,offset=0,backingBytes=0;
 int elements=0;
 std::array<int32_t,12> words{};
};
inline T385RoiDecode T385DecodeRoi(napi_env e,napi_value value){
 T385RoiDecode r;
 bool typed=false,detached=false;
 r.status=napi_is_typedarray(e,value,&typed);
 if(r.status!=napi_ok||!typed){r.stage="ROI_NOT_TYPED_ARRAY";return r;}
 napi_typedarray_type type{};
 napi_value backing=nullptr;void *data=nullptr,*base=nullptr;
 r.status=napi_get_typedarray_info(e,value,&type,&r.rawBytes,&data,&backing,&r.offset);
 if(r.status!=napi_ok){r.stage="ROI_TYPED_INFO";return r;}
 r.type=int(type);
 if(type!=napi_int32_array){r.stage="ROI_TYPE";return r;}
 if(r.rawBytes!=16&&r.rawBytes!=32&&r.rawBytes!=48){r.stage="ROI_BYTE_LENGTH";return r;}
 if(!backing){r.stage="ROI_BUFFER";return r;}
 r.status=napi_is_detached_arraybuffer(e,backing,&detached);
 if(r.status!=napi_ok||detached){r.stage="ROI_BUFFER_DETACHED";return r;}
 r.status=napi_get_arraybuffer_info(e,backing,&base,&r.backingBytes);
 if(r.status!=napi_ok||!base||!data||r.offset%sizeof(int32_t)||
    r.offset>r.backingBytes||r.rawBytes>r.backingBytes-r.offset||
    data!=static_cast<const uint8_t*>(base)+r.offset){r.stage="ROI_BUFFER_RANGE";return r;}
 r.elements=int(r.rawBytes/sizeof(int32_t));
 std::memcpy(r.words.data(),data,r.rawBytes);
 r.stage="ROI_DECODED";return r;
}
