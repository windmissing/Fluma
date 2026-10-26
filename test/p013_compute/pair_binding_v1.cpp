// Isolated test NAPI. It is never linked into the production renderer/physics.
#include "pair_prepare_v1.h"
#include "pair_executor_v1.h"
#include "pair_owner_v1.h"
#include <napi/native_api.h>
#include <chrono>
#include <memory>
#include <string>
#include <cstdio>

namespace {
using namespace p011gpu;
struct State {ContextSlot gpu;std::unique_ptr<CpuSlot> cpu;uint64_t serial=0;std::string buildSha;
 RequestDiagnostics last;bool ready=false;};
State state;
static bool prop(napi_env e,napi_value o,const char* k,napi_value& v){bool has=false;
 return napi_has_named_property(e,o,k,&has)==napi_ok&&has&&napi_get_named_property(e,o,k,&v)==napi_ok;}
static bool number(napi_env e,napi_value o,const char* k,double& x){napi_value v;
 return prop(e,o,k,v)&&napi_get_value_double(e,v,&x)==napi_ok;}
static bool integer(napi_env e,napi_value o,const char* k,uint32_t& x){double d=0;
 if(!number(e,o,k,d)||d<0||d>4294967295.0||d!=double(uint32_t(d)))return false;x=uint32_t(d);return true;}
static bool stringValue(napi_env e,napi_value v,std::string& out){size_t len=0;
 if(napi_get_value_string_utf8(e,v,nullptr,0,&len)!=napi_ok||len>1024)return false;
 std::string tmp(len+1,'\0');if(napi_get_value_string_utf8(e,v,&tmp[0],len+1,&len)!=napi_ok)return false;
 tmp.resize(len);out=tmp;return true;}
static bool namedString(napi_env e,napi_value o,const char* k,std::string& out){napi_value v;
 return prop(e,o,k,v)&&stringValue(e,v,out);}
static bool typed(napi_env e,napi_value o,const char* k,napi_typedarray_type want,size_t count,
 void*& ptr,size_t& bytes,size_t elementSize){napi_value value;if(!prop(e,o,k,value))return false;
 napi_typedarray_type type;size_t length=0,offset=0;napi_value buffer;
 if(napi_get_typedarray_info(e,value,&type,&length,&ptr,&buffer,&offset)!=napi_ok||type!=want||length!=count)return false;
 bytes=length*elementSize;return ptr!=nullptr;
}
static napi_value reply(napi_env e,const char* status,int32_t a,int32_t b,const char* code){napi_value out,v;
 napi_create_object(e,&out);napi_create_string_utf8(e,status,NAPI_AUTO_LENGTH,&v);napi_set_named_property(e,out,"status",v);
 napi_create_int32(e,a,&v);napi_set_named_property(e,out,"slotA",v);napi_create_int32(e,b,&v);napi_set_named_property(e,out,"slotB",v);
 napi_create_string_utf8(e,code,NAPI_AUTO_LENGTH,&v);napi_set_named_property(e,out,"errorCode",v);return out;}
static bool snapshot(napi_env e,napi_value o,SnapshotView& p){uint32_t n=0,c=0,live=0,free=0;
 if(!integer(e,o,"nmax",n)||!integer(e,o,"cols",c)||!integer(e,o,"liveCount",live)||
  !integer(e,o,"freeCount",free)||!number(e,o,"nextId",p.nextId))return false;
 p.nmax=n;p.cols=c;p.liveCount=live;p.freeCount=free;
 void* ptr=nullptr;size_t bytes=0;
#define READ_ARRAY(I,NAME,TYPE,COUNT,ELEM,FIELD,CAST) \
 if(!typed(e,o,NAME,TYPE,COUNT,ptr,bytes,ELEM))return false; \
 p.FIELD=static_cast<CAST>(ptr);p.byteLength[I]=bytes;
 READ_ARRAY(0,"pMass",napi_float64_array,n,8,mass,double*)
 READ_ARRAY(1,"pX",napi_float64_array,n,8,x,double*)
 READ_ARRAY(2,"pY",napi_float64_array,n,8,y,double*)
 READ_ARRAY(3,"pVx",napi_float64_array,n,8,vx,double*)
 READ_ARRAY(4,"pVy",napi_float64_array,n,8,vy,double*)
 READ_ARRAY(5,"pPhase",napi_uint8_array,n,1,phase,uint8_t*)
 READ_ARRAY(6,"pSource",napi_uint8_array,n,1,source,uint8_t*)
 READ_ARRAY(7,"pState",napi_uint8_array,n,1,state,uint8_t*)
 READ_ARRAY(8,"pId",napi_uint32_array,n,4,id,uint32_t*)
 READ_ARRAY(9,"pSourceId",napi_uint32_array,n,4,sourceId,uint32_t*)
 READ_ARRAY(10,"freeSlots",napi_int32_array,n,4,freeSlots,int32_t*)
 READ_ARRAY(11,"colParticleMassH",napi_float64_array,c,8,colMassH,double*)
 READ_ARRAY(12,"colParticleMassL",napi_float64_array,c,8,colMassL,double*)
#undef READ_ARRAY
 return true;}
static bool identity(napi_env e,napi_value work,RequestIdentity& id){uint32_t owner=0,run=0,step=0,attempt=0,substep=0;
 if(!integer(e,work,"ownerId",owner)||!integer(e,work,"runToken",run)||
  !integer(e,work,"stepEpoch",step)||!integer(e,work,"attemptToken",attempt)||
  !integer(e,work,"substepId",substep)||!number(e,work,"paramsRevision",id.paramsRevision)||
  !number(e,work,"inputRevision",id.inputRevision)||!number(e,work,"seed",id.seed))return false;
 id.runId=std::to_string(run);id.stateId=std::to_string(step);
 id.configId=std::to_string(id.paramsRevision)+":"+std::to_string(id.inputRevision);
 id.buildManifestSha=state.buildSha;id.ownerGeneration=owner;id.contextEpoch=state.gpu.contextEpoch();
 if(state.serial==UINT64_MAX||state.serial>=UINT32_MAX)return false;
 id.requestSerial=++state.serial;id.payloadSerial=id.requestSerial;id.poolMutationSerial=id.requestSerial;
 id.attemptSerial=attempt;id.substepSerial=substep;id.candidateRole=4;
 id.candidateOrdinal=uint32_t(id.requestSerial);id.slotIndex=0;return true;
}
static napi_value configure(napi_env e,napi_callback_info info){size_t argc=3;napi_value argv[3];
 if(napi_get_cb_info(e,info,&argc,argv,nullptr,nullptr)!=napi_ok||argc<3)
  return reply(e,"ERROR",-1,-1,"CONFIG_ARGUMENTS");
 double n=0;if(napi_get_value_double(e,argv[0],&n)!=napi_ok||n<2||n>4096||n!=double(uint32_t(n)))
  return reply(e,"ERROR",-1,-1,"CAPACITY_UNSUPPORTED");
 std::string shaderDir,buildSha;if(!stringValue(e,argv[1],shaderDir)||!stringValue(e,argv[2],buildSha)||buildSha.empty())
  return reply(e,"ERROR",-1,-1,"CONFIG_ARGUMENTS");
 state.ready=false;state.cpu.reset();state.gpu.reset();state.serial=0;state.buildSha=buildSha;
 state.cpu.reset(new CpuSlot(uint32_t(n)));
 auto status=state.gpu.initialize(uint32_t(n),shaderDir,state.last);
 if(status!=ExecStatus::OK)return reply(e,"ERROR",-1,-1,state.last.stage.c_str());
 state.ready=true;return reply(e,"READY",-1,-1,"NONE");}
static napi_value select(napi_env e,napi_callback_info info){size_t argc=3;napi_value argv[3];
 if(napi_get_cb_info(e,info,&argc,argv,nullptr,nullptr)!=napi_ok||argc<3||!state.ready)
  return reply(e,"ERROR",-1,-1,"BINDING_UNAVAILABLE");
 SnapshotView p;double radius=0;RequestIdentity id;
 if(!snapshot(e,argv[0],p)||napi_get_value_double(e,argv[1],&radius)!=napi_ok||
   !identity(e,argv[2],id))return reply(e,"ERROR",-1,-1,"INPUT_INVALID");
 PreparedView prepared;auto ps=prepareExactRanges(p,radius,id,*state.cpu,prepared);
 if(ps!=PrepareStatus::OK&&ps!=PrepareStatus::NO_PAIR)return reply(e,"ERROR",-1,-1,"PREPARE_FAILED");
 Result result;auto es=state.gpu.executeIntegerPair(prepared,0,
  {std::chrono::steady_clock::now()+std::chrono::seconds(1)},result,state.last);
 if(es!=ExecStatus::OK){state.ready=false;return reply(e,"ERROR",-1,-1,state.last.stage.c_str());}
 CurrentOwnerState owner;owner.identity=id;owner.current=&p;owner.poolMutationSerial=id.poolMutationSerial;
 owner.active=true;owner.diagnosticOracle=true;VerifiedPair pair;
 auto cs=verifyAndConsume(prepared,result,owner,pair);
 if(cs==ConsumeStatus::NO_PAIR)return reply(e,"NO_PAIR",-1,-1,"NONE");
 if(cs!=ConsumeStatus::OK){state.ready=false;return reply(e,"ERROR",-1,-1,
   cs==ConsumeStatus::ORACLE_MISMATCH?"ORACLE_MISMATCH":"PROTOCOL_OR_STALE");}
 return reply(e,"PAIR",int32_t(pair.slotA),int32_t(pair.slotB),"NONE");}
static napi_value reset(napi_env e,napi_callback_info){state.ready=false;state.gpu.reset();state.cpu.reset();
 return reply(e,"RESET",-1,-1,"NONE");}
static void cleanup(void*){state.ready=false;state.gpu.reset();state.cpu.reset();}
static napi_value init(napi_env e,napi_value exports){napi_property_descriptor d[]={
 {"configure",nullptr,configure,nullptr,nullptr,nullptr,napi_default,nullptr},
 {"selectPair",nullptr,select,nullptr,nullptr,nullptr,napi_default,nullptr},
 {"reset",nullptr,reset,nullptr,nullptr,nullptr,napi_default,nullptr}};
 napi_define_properties(e,exports,3,d);napi_add_env_cleanup_hook(e,cleanup,nullptr);return exports;}
static napi_module module={1,0,nullptr,init,"p011_gpu_pair_prototype",nullptr,{0}};
}
#ifdef _MSC_VER
struct AutoRegister {AutoRegister(){napi_module_register(&module);}};
static AutoRegister autoRegister;
#else
extern "C" __attribute__((constructor)) void RegisterP011GpuPairPrototype(){napi_module_register(&module);}
#endif
