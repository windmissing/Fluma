#include "bridge_napi.h"
#include "bridge_packet.h"
#include <node_api.h>
#include <string>
static napi_value call(napi_env e,napi_callback_info info){auto d=grs::DecodeBridgeCommand(e,info);grs::Identity id;id.run={1,77};id.geometry={3,23};id.config={4,66};const char* stage=d.stage;if(std::strcmp(stage,"OK")==0)stage=grs::ValidateBridgePacket(d.words,id,{2,99});napi_value o,v,a;napi_create_object(e,&o);napi_create_string_utf8(e,stage,NAPI_AUTO_LENGTH,&v);napi_set_named_property(e,o,"stage",v);napi_create_int32(e,int(d.status),&v);napi_set_named_property(e,o,"napiStatus",v);napi_create_array_with_length(e,32,&a);for(uint32_t i=0;i<32;i++){napi_create_uint32(e,d.words[i],&v);napi_set_element(e,a,i,v);}napi_set_named_property(e,o,"words",a);return o;}
extern "C" __declspec(dllexport) napi_value napi_register_module_v1(napi_env e,napi_value out){napi_property_descriptor d={"decode",nullptr,call,nullptr,nullptr,nullptr,napi_default,nullptr};napi_define_properties(e,out,1,&d);return out;}
