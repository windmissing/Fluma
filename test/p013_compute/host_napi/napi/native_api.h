// T159 HOST UNIT TEST DOUBLE ONLY. Models OH byte-length typed-array convention.
// Never used by generated CMake or production/native builds.
#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstring>
#include <algorithm>
#include <cstdint>
enum napi_status{napi_ok,napi_invalid_arg};
enum napi_valuetype{napi_undefined,napi_object,napi_number,napi_boolean,napi_string};
enum napi_typedarray_type{napi_uint8_array,napi_int32_array,napi_uint32_array,napi_float64_array};
struct MockEnv{};using napi_env=MockEnv*;
struct MockValue;using napi_value=MockValue*;
struct MockValue{
 napi_valuetype type=napi_object;double number=0;bool boolean=false,isBuffer=false,isTyped=false,detached=false;
 std::string string;std::map<std::string,napi_value> properties;std::vector<uint64_t> storage;
 size_t bytes=0,offset=0,count=0; napi_typedarray_type arrayType=napi_uint8_array;napi_value buffer=nullptr;
};
struct MockCall{void* data=nullptr;std::vector<napi_value> args;};using napi_callback_info=MockCall*;
using napi_callback=napi_value(*)(napi_env,napi_callback_info);
constexpr size_t NAPI_AUTO_LENGTH=SIZE_MAX;constexpr int napi_default=0;
struct napi_property_descriptor{const char* utf8name;void* name;napi_callback method;void* getter;void* setter;void* value;int attributes;void* data;};
struct napi_module{int version;unsigned flags;const char* filename;napi_value(*init)(napi_env,napi_value);const char* name;void* privateData;void* reserved[4];};
inline std::vector<std::unique_ptr<MockValue>>& mockArena(){static std::vector<std::unique_ptr<MockValue>> arena;return arena;}
inline napi_value mockValue(){mockArena().emplace_back(new MockValue());return mockArena().back().get();}
inline napi_status napi_create_object(napi_env,napi_value* out){*out=mockValue();return napi_ok;}
inline napi_status napi_get_cb_info(napi_env,napi_callback_info c,size_t* n,napi_value* args,napi_value*,void** data){if(!c)return napi_invalid_arg;*data=c->data;size_t count=std::min(*n,c->args.size());for(size_t i=0;i<count;i++)args[i]=c->args[i];*n=count;return napi_ok;}
inline napi_status napi_has_named_property(napi_env,napi_value o,const char* k,bool* out){if(!o)return napi_invalid_arg;*out=o->properties.count(k)!=0;return napi_ok;}
inline napi_status napi_get_named_property(napi_env,napi_value o,const char* k,napi_value* out){if(!o||!o->properties.count(k))return napi_invalid_arg;*out=o->properties[k];return napi_ok;}
inline napi_status napi_set_named_property(napi_env,napi_value o,const char* k,napi_value v){o->properties[k]=v;return napi_ok;}
inline napi_status napi_typeof(napi_env,napi_value v,napi_valuetype* t){if(!v)return napi_invalid_arg;*t=v->type;return napi_ok;}
inline napi_status napi_create_double(napi_env,double n,napi_value* v){*v=mockValue();(*v)->type=napi_number;(*v)->number=n;return napi_ok;}
inline napi_status napi_get_value_double(napi_env,napi_value v,double* n){if(!v||v->type!=napi_number)return napi_invalid_arg;*n=v->number;return napi_ok;}
inline napi_status napi_get_boolean(napi_env,bool b,napi_value* v){*v=mockValue();(*v)->type=napi_boolean;(*v)->boolean=b;return napi_ok;}
inline napi_status napi_get_value_bool(napi_env,napi_value v,bool* b){if(!v||v->type!=napi_boolean)return napi_invalid_arg;*b=v->boolean;return napi_ok;}
inline napi_status napi_create_string_utf8(napi_env,const char* str,size_t n,napi_value* v){*v=mockValue();(*v)->type=napi_string;(*v)->string=std::string(str,n==SIZE_MAX?std::strlen(str):n);return napi_ok;}
inline napi_status napi_get_value_string_utf8(napi_env,napi_value v,char* out,size_t n,size_t* size){if(!v||v->type!=napi_string)return napi_invalid_arg;*size=v->string.size();if(out&&n){size_t copied=std::min(n-1,*size);std::memcpy(out,v->string.data(),copied);out[copied]=0;}return napi_ok;}
inline napi_status napi_create_arraybuffer(napi_env,size_t n,void** ptr,napi_value* out){*out=mockValue();(*out)->isBuffer=true;(*out)->bytes=n;(*out)->storage.resize((n+7)/8);*ptr=(*out)->storage.data();return napi_ok;}
inline size_t mockStride(napi_typedarray_type t){return t==napi_float64_array?8:t==napi_uint8_array?1:4;}
inline napi_status napi_create_typedarray(napi_env,napi_typedarray_type t,size_t count,napi_value b,size_t offset,napi_value* out){*out=mockValue();(*out)->isTyped=true;(*out)->arrayType=t;(*out)->count=count;(*out)->buffer=b;(*out)->offset=offset;return napi_ok;}
inline napi_status napi_is_typedarray(napi_env,napi_value v,bool* out){*out=v&&v->isTyped;return napi_ok;}
inline napi_status napi_is_arraybuffer(napi_env,napi_value v,bool* out){*out=v&&v->isBuffer;return napi_ok;}
inline napi_status napi_is_detached_arraybuffer(napi_env,napi_value v,bool* out){*out=v&&v->detached;return napi_ok;}
inline napi_status napi_get_arraybuffer_info(napi_env,napi_value v,void** ptr,size_t* bytes){*ptr=v->storage.data();*bytes=v->bytes;return napi_ok;}
inline napi_status napi_get_typedarray_info(napi_env,napi_value v,napi_typedarray_type* type,size_t* byteLength,void** ptr,napi_value* buffer,size_t* offset){*type=v->arrayType;*byteLength=v->count*mockStride(v->arrayType);*offset=v->offset;*buffer=v->buffer;*ptr=reinterpret_cast<uint8_t*>(v->buffer->storage.data())+v->offset;return napi_ok;}
inline napi_status napi_define_properties(napi_env,napi_value,size_t,const napi_property_descriptor*){return napi_ok;}
inline napi_status napi_add_env_cleanup_hook(napi_env,void(*)(void*),void*){return napi_ok;}
inline void napi_module_register(napi_module*){}
