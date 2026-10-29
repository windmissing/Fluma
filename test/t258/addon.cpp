// Actual Node N-API execution of the product C V3 reader and atomic validator.
#include "clear_water_identity.h"
#include "clear_water_packet.h"
#include <cstring>
using namespace p013;
namespace {
ClearWaterAtomic atomic;struct Boundary:ClearWaterGpu {int calls=0;Status Begin(const ClearWaterPacket&)override{calls++;return Status::OK;}Status Layer(int,const ClearWaterPacket&)override{calls++;return Status::OK;}Status Present()override{calls++;return Status::OK;}Status Swap(uint64_t)override{calls++;return Status::OK;}void Commit()override{calls++;}} gpu;
void text(napi_env e,napi_value obj,const char* name,const std::string& value){napi_value s;napi_create_string_utf8(e,value.data(),value.size(),&s);napi_set_named_property(e,obj,name,s);}
void number(napi_env e,napi_value obj,const char* name,int value){napi_value n;napi_create_int32(e,value,&n);napi_set_named_property(e,obj,name,n);}
napi_value call(napi_env e,napi_callback_info info){size_t argc=2;napi_value args[2]{};napi_get_cb_info(e,info,&argc,args,nullptr,nullptr);napi_value out;napi_create_object(e,&out);ClearWaterPacket p;const char* stage="CW_V3_PARSE";bool ok=argc>=1&&ReadClearWaterIdentity(e,args[0],p.token,p.configDigest,stage);number(e,out,"acceptedIdentity",ok);text(e,out,"token",p.token);text(e,out,"configDigest",p.configDigest);int before=gpu.calls;Status code=ok?Status::OK:Status::INVALID_ARGUMENT;
 if(ok&&argc==2){void* data=nullptr;size_t size=0;ok=napi_get_arraybuffer_info(e,args[1],&data,&size)==napi_ok;const unsigned char* b=static_cast<const unsigned char*>(data);size_t offset=0;auto read=[&](void* dest,size_t n){if(!ok||offset>size||n>size-offset){ok=false;return;}std::memcpy(dest,b+offset,n);offset+=n;};double v[24]{};read(v,sizeof(v));auto& f=p.frame;f.sessionId=v[0];f.frameId=v[1];f.snapshotId=v[2];f.surfaceEpoch=v[3];f.fieldEpoch=v[4];f.geometryEpoch=v[5];f.continuityEpoch=v[6];f.appearanceConfigRevision=v[7];f.lightConfigRevision=v[8];f.frameTimeNs=v[9];f.surfaceWidthPx=v[10];f.surfaceHeightPx=v[11];f.boxXPx=v[12];f.boxYPx=v[13];f.boxWidthPx=v[14];f.boxHeightPx=v[15];f.capacity=v[16];f.T_dataLayoutVersion=v[17];f.zOriginVersion=v[18];f.visualInverted=v[19]!=0;f.diagnosticEnabled=v[20]!=0;p.context=uint64_t(v[21]);p.count=size_t(v[22]);p.capacity=size_t(v[23]);
  if(p.capacity>512)ok=false;read(p.header.data(),512);read(p.ledger.data(),512);read(p.optics.data(),128);read(p.profile.data(),8192);read(p.proof.data(),p.capacity*64);read(p.origin.data(),p.capacity*8);read(p.wait.data(),p.capacity);read(p.xy.data(),p.capacity*8);read(p.meta.data(),p.capacity*8);
  napi_value value;std::string run,payload;if(napi_get_named_property(e,args[0],"runId",&value)!=napi_ok||ReadClearWaterUtf8(e,value,254,run)!=ClearWaterIdentityRead::OK)ok=false;if(napi_get_named_property(e,args[0],"payloadDigest",&value)!=napi_ok||ReadClearWaterUtf8(e,value,254,payload)!=ClearWaterIdentityRead::OK)ok=false;p.run=run;p.payloadDigest=payload;
  if(ok&&offset==size){auto receipt=atomic.Draw(p,gpu);code=receipt.code;stage=receipt.stage;text(e,out,"renderedToken",receipt.rendered);text(e,out,"lastGoodToken",atomic.lastGood);}else{code=Status::INVALID_ARGUMENT;stage="ADDON_PACKET";}
 }
 number(e,out,"code",int(code));number(e,out,"gpuBoundaryCalls",gpu.calls-before);text(e,out,"stage",stage);return out;
}
}
extern "C" __declspec(dllexport) napi_value napi_register_module_v1(napi_env e,napi_value exports){napi_property_descriptor d={"read",nullptr,call,nullptr,nullptr,nullptr,napi_default,nullptr};napi_define_properties(e,exports,1,&d);return exports;}
