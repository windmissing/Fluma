#include <cstddef>
#include <cassert>
#include <iostream>
#include <cstring>
enum napi_typedarray_type {napi_int32_array=5,napi_float32_array=7};
struct Fake {
 bool typed=true,detached=false,rangeOK=true,typedOK=true;
 napi_typedarray_type type=napi_int32_array;
 size_t length=16,offset=0,backing=64,delta=0;
 int32_t data[16]{0,0,16,16,16,16,16,16,32,32,16,16};
};
using napi_env=void*;using napi_value=Fake*;
constexpr int napi_ok=0;
int napi_is_typedarray(napi_env,napi_value v,bool* x){*x=v->typed;return 0;}
int napi_get_typedarray_info(napi_env,napi_value v,napi_typedarray_type* t,size_t* n,void** data,napi_value* b,size_t* o){
 *t=v->type;*n=v->length;*o=v->offset;*b=v;*data=reinterpret_cast<uint8_t*>(v->data)+(v->offset<=64?v->offset:0)+v->delta;
 return v->typedOK?0:42;
}
int napi_is_detached_arraybuffer(napi_env,napi_value v,bool* d){*d=v->detached;return 0;}
int napi_get_arraybuffer_info(napi_env,napi_value v,void** b,size_t* n){*b=v->data;*n=v->backing;return v->rangeOK?0:43;}
#include "roi_decode.h"
int main(){
 Fake f;int checked=0;
 assert(f.length>12); // old upper bound reproduces default ROI rejection
 auto read=[&](size_t bytes,int n){f.length=bytes;auto r=T385DecodeRoi(nullptr,&f);
  assert(!std::strcmp(r.stage,"ROI_DECODED")&&r.elements==n&&r.rawBytes==bytes);checked++;return r;};
 assert(read(16,4).words[2]==16);read(32,8);read(48,12);
 f.offset=4;assert(read(16,4).words[1]==16);
 const auto reject=[&](const char* stage){
  auto r=T385DecodeRoi(nullptr,&f);assert(!std::strcmp(r.stage,stage)&&r.elements==0);checked++;f={};};
 for(auto bad:{0,4,8,12,64}){f.length=bad;reject("ROI_BYTE_LENGTH");}
 f.type=napi_float32_array;reject("ROI_TYPE");
 f.typed=false;reject("ROI_NOT_TYPED_ARRAY");
 f.typedOK=false;reject("ROI_TYPED_INFO");
 f.detached=true;reject("ROI_BUFFER_DETACHED");
 f.offset=2;reject("ROI_BUFFER_RANGE");
 f.offset=52;reject("ROI_BUFFER_RANGE");
 f.delta=4;reject("ROI_BUFFER_RANGE");
 f.rangeOK=false;reject("ROI_BUFFER_RANGE");
 std::cout<<"T385 Harmony byte decoder "<<checked<<"/"<<checked<<" PASS\n";
}
