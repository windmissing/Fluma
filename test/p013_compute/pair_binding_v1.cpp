// T159 isolated Worker/env-owned binding. Never linked into the production app.
#include "pair_prepare_v1.h"
#include "pair_executor_v1.h"
#include "pair_owner_v1.h"
#include "pair_identity_v2.h"
#include <napi/native_api.h>
#include <atomic>
#include <algorithm>
#include <iterator>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <cstdio>
#include <cmath>
#ifndef T159_SOURCE_SHA
#define T159_SOURCE_SHA "UNBOUND_TEST_BUILD"
#endif
namespace {
using namespace p011gpu;
std::atomic<uint64_t> nextOwner{1};
static uint64_t allocateOwner(){uint64_t value=nextOwner.load();while(value!=UINT64_MAX){if(nextOwner.compare_exchange_weak(value,value+1))return value;}return 0;}
struct State {
 napi_env env;std::thread::id thread=std::this_thread::get_id();ContextSlot gpu;
 std::unique_ptr<CpuSlot> slots[2];uint64_t request=0,payload=0,mutation=0,generation=0;
 std::array<uint32_t,8> lastSnapshotSha{};std::array<const void*,13> lastPointers{};
 uint32_t ordinal[6]={},uses[2]={};std::string buildSha,runId,configId;
 RequestDiagnostics last,cleanup,firstFailure;bool ready=false,oracle=true,hasSnapshot=false;
 explicit State(napi_env e):env(e){}
 void record(){if(firstFailure.stage=="NONE"&&last.stage!="NONE"&&!last.valid)firstFailure=last;}
 bool owner(napi_env e)const{return env==e&&thread==std::this_thread::get_id();}
};
static bool prop(napi_env e,napi_value o,const char* k,napi_value& v){bool has=false;return napi_has_named_property(e,o,k,&has)==napi_ok&&has&&napi_get_named_property(e,o,k,&v)==napi_ok;}
static bool number(napi_env e,napi_value o,const char* k,double& x){napi_value v;napi_valuetype type;return prop(e,o,k,v)&&napi_typeof(e,v,&type)==napi_ok&&type==napi_number&&napi_get_value_double(e,v,&x)==napi_ok&&std::isfinite(x);}
static bool integer(napi_env e,napi_value o,const char* k,uint32_t& x){double d=0;if(!number(e,o,k,d)||d<0||d>4294967295.||std::floor(d)!=d)return false;x=uint32_t(d);return true;}
static bool words(napi_env e,napi_value o,const char* key,uint64_t& out){uint32_t lo=0,hi=0;std::string low=std::string(key)+"Lo",high=std::string(key)+"Hi";if(!integer(e,o,low.c_str(),lo)||!integer(e,o,high.c_str(),hi))return false;out=join64(lo,hi);return true;}
static bool boolean(napi_env e,napi_value o,const char* key,bool& value){napi_value v;napi_valuetype type;return prop(e,o,key,v)&&napi_typeof(e,v,&type)==napi_ok&&type==napi_boolean&&napi_get_value_bool(e,v,&value)==napi_ok;}
static bool text(napi_env e,napi_value o,const char* key,std::string& out,size_t max=1024){napi_value v;napi_valuetype type;size_t length=0;if(!prop(e,o,key,v)||napi_typeof(e,v,&type)!=napi_ok||type!=napi_string||napi_get_value_string_utf8(e,v,nullptr,0,&length)!=napi_ok||length>max)return false;std::string owned(length+1,'\0');if(napi_get_value_string_utf8(e,v,&owned[0],owned.size(),&length)!=napi_ok)return false;owned.resize(length);out=std::move(owned);return true;}
static void put(napi_env e,napi_value o,const char* k,const char* text){napi_value v;napi_create_string_utf8(e,text,NAPI_AUTO_LENGTH,&v);napi_set_named_property(e,o,k,v);}
static void put(napi_env e,napi_value o,const char* k,double x){napi_value v;napi_create_double(e,x,&v);napi_set_named_property(e,o,k,v);}
static void put(napi_env e,napi_value o,const char* k,bool x){napi_value v;napi_get_boolean(e,x,&v);napi_set_named_property(e,o,k,v);}
static void put64(napi_env e,napi_value o,const char* key,uint64_t x){std::string lo=std::string(key)+"Lo",hi=std::string(key)+"Hi";put(e,o,lo.c_str(),double(uint32_t(x)));put(e,o,hi.c_str(),double(uint32_t(x>>32)));}
static void bytes(napi_env e,napi_value o,const char* key,const void* source,size_t n,napi_typedarray_type type,size_t count){napi_value buffer,view;void* own=nullptr;if(napi_create_arraybuffer(e,n,&own,&buffer)!=napi_ok)return;if(n)std::memcpy(own,source,n);if(napi_create_typedarray(e,type,count,buffer,0,&view)==napi_ok)napi_set_named_property(e,o,key,view);}
static napi_value reply(napi_env e,State* s,const char* status,const char* error,ExecStatus exec=ExecStatus::INPUT_INVALID,const char* consume="NOT_RUN",const char* oracle="NOT_EVALUATED",const VerifiedPair* pair=nullptr){
 napi_value out;napi_create_object(e,&out);put(e,out,"status",status);put(e,out,"errorCode",error);put(e,out,"execStatus",execStatusName(exec));put(e,out,"consumeStatus",consume);put(e,out,"oracleVerdict",oracle);put(e,out,"slotA",pair?double(pair->slotA):-1.);put(e,out,"slotB",pair?double(pair->slotB):-1.);put(e,out,"identityEncodingRevision",kIdentityEncodingV2);put(e,out,"nativeSourceBuildSha256",T159_SOURCE_SHA);put(e,out,"gpuRuntime","NOT_MEASURED");
 if(s){put64(e,out,"ownerGeneration",s->generation);put64(e,out,"contextEpoch",s->gpu.contextEpoch());put64(e,out,"requestSerial",s->request);put(e,out,"stage",s->last.stage.c_str());put(e,out,"detail",s->last.detail.c_str());put(e,out,"rawGlError",double(s->last.rawGlError));put(e,out,"rawEglError",double(s->last.rawEglError));put(e,out,"rawWaitReturn",double(s->last.rawWaitReturn));put(e,out,"waitReturn",s->last.waitReturn.c_str());put(e,out,"dispatchCount",double(s->last.dispatchCount));put(e,out,"requestCpuMs",s->last.requestCpuMs);put(e,out,"uploadCpuMs",s->last.uploadCpuMs);put(e,out,"dispatchCpuMs",s->last.dispatchCpuMs);put(e,out,"fenceWaitCpuMs",s->last.fenceWaitCpuMs);put(e,out,"mapCpuMs",s->last.mapCpuMs);put(e,out,"liveBuffers",double(s->gpu.liveBufferCount()));put(e,out,"peakBuffers",double(s->gpu.peakBufferCount()));put(e,out,"slot0State",double(s->gpu.slotState(0)));put(e,out,"slot1State",double(s->gpu.slotState(1)));put(e,out,"slot0Uses",double(s->uses[0]));put(e,out,"slot1Uses",double(s->uses[1]));put(e,out,"quarantined",s->gpu.quarantined());put(e,out,"cleanupStage",s->cleanup.stage.c_str());put(e,out,"cleanupRawGlError",double(s->cleanup.rawGlError));put(e,out,"cleanupRawEglError",double(s->cleanup.rawEglError));put(e,out,"resourcesRetired",s->cleanup.resourcesRetired);put(e,out,"firstFailureStage",s->firstFailure.stage.c_str());put(e,out,"firstFailureRawGlError",double(s->firstFailure.rawGlError));put(e,out,"firstFailureRawEglError",double(s->firstFailure.rawEglError));}
 return out;
}
static State* callState(napi_env e,napi_callback_info info,size_t& argc,napi_value* argv){void* data=nullptr;if(napi_get_cb_info(e,info,&argc,argv,nullptr,&data)!=napi_ok)return nullptr;auto* s=static_cast<State*>(data);return s&&s->owner(e)?s:nullptr;}
static bool typed(napi_env e,napi_value o,const char* key,napi_typedarray_type want,size_t count,size_t stride,void*& ptr,size_t& bytes){napi_value v,buffer;bool array=false,detached=false;napi_typedarray_type type;size_t byteLength=0,offset=0,bufferBytes=0;void* base=nullptr;
 if(!prop(e,o,key,v)||napi_is_typedarray(e,v,&array)!=napi_ok||!array||napi_get_typedarray_info(e,v,&type,&byteLength,&ptr,&buffer,&offset)!=napi_ok||type!=want||count>SIZE_MAX/stride||byteLength!=count*stride||offset%stride)return false;
 if(napi_is_arraybuffer(e,buffer,&array)!=napi_ok||!array||napi_is_detached_arraybuffer(e,buffer,&detached)!=napi_ok||detached||napi_get_arraybuffer_info(e,buffer,&base,&bufferBytes)!=napi_ok||!base||!ptr||offset>bufferBytes||byteLength>bufferBytes-offset||ptr!=static_cast<uint8_t*>(base)+offset)return false;bytes=byteLength;return true;
}
static bool snapshot(napi_env e,napi_value o,SnapshotView& p){uint32_t n=0,c=0;if(!integer(e,o,"nmax",n)||n<2||n>4096||!integer(e,o,"cols",c)||c<1||c>128||!integer(e,o,"liveCount",p.liveCount)||!integer(e,o,"freeCount",p.freeCount)||!number(e,o,"nextId",p.nextId))return false;p.nmax=n;p.cols=c;void* ptr=nullptr;size_t size=0;
#define READ(I,KEY,TYPE,COUNT,STRIDE,FIELD,CAST) if(!typed(e,o,KEY,TYPE,COUNT,STRIDE,ptr,size))return false;p.FIELD=static_cast<CAST>(ptr);p.byteLength[I]=size;
 READ(0,"pMass",napi_float64_array,n,8,mass,double*) READ(1,"pX",napi_float64_array,n,8,x,double*) READ(2,"pY",napi_float64_array,n,8,y,double*) READ(3,"pVx",napi_float64_array,n,8,vx,double*) READ(4,"pVy",napi_float64_array,n,8,vy,double*)
 READ(5,"pPhase",napi_uint8_array,n,1,phase,uint8_t*) READ(6,"pSource",napi_uint8_array,n,1,source,uint8_t*) READ(7,"pState",napi_uint8_array,n,1,state,uint8_t*)
 READ(8,"pId",napi_uint32_array,n,4,id,uint32_t*) READ(9,"pSourceId",napi_uint32_array,n,4,sourceId,uint32_t*) READ(10,"freeSlots",napi_int32_array,n,4,freeSlots,int32_t*) READ(11,"colParticleMassH",napi_float64_array,c,8,colMassH,double*) READ(12,"colParticleMassL",napi_float64_array,c,8,colMassL,double*)
#undef READ
 return true;
}
static bool safeValue(napi_env e,napi_value o,const char* key,uint64_t& out){double d=0;if(!number(e,o,key,d)||d<0||d>9007199254740991.||std::floor(d)!=d)return false;out=uint64_t(d);return true;}
static bool identity(napi_env e,napi_value ctx,napi_value work,State& s,RequestIdentity& id){
 std::string schema;if(!text(e,ctx,"schemaRevision",schema)||schema!="T141_REQUEST_CONTEXT_V2")return false;
 if(!text(e,ctx,"identityEncodingRevision",id.identityEncodingRevision)||id.identityEncodingRevision!=kIdentityEncodingV2||!text(e,ctx,"runId",id.runId)||!text(e,ctx,"stateId",id.stateId)||!text(e,ctx,"configId",id.configId,8192)||!text(e,ctx,"buildManifestSha256",id.buildManifestSha))return false;
 if(!words(e,ctx,"ownerGeneration",id.ownerGeneration)||!words(e,ctx,"contextEpoch",id.contextEpoch)||!words(e,ctx,"payloadSerial",id.payloadSerial)||!words(e,ctx,"poolMutationSerial",id.poolMutationSerial)||!words(e,ctx,"attemptSerial",id.attemptSerial)||!words(e,ctx,"substepSerial",id.substepSerial)||!words(e,ctx,"requestSerial",id.requestSerial)||!integer(e,ctx,"candidateRole",id.candidateRole)||!integer(e,ctx,"candidateOrdinal",id.candidateOrdinal)||!integer(e,ctx,"slotIndex",id.slotIndex)||!number(e,ctx,"paramsRevision",id.paramsRevision)||!number(e,ctx,"inputRevision",id.inputRevision)||!number(e,ctx,"seed",id.seed))return false;
 uint64_t owner=0,run=0,step=0,attempt=0,substep=0;double params=0,input=0,seed=0;
 if(!safeValue(e,work,"ownerId",owner)||!safeValue(e,work,"runToken",run)||!safeValue(e,work,"stepEpoch",step)||!safeValue(e,work,"attemptToken",attempt)||!safeValue(e,work,"substepId",substep)||!number(e,work,"paramsRevision",params)||!number(e,work,"inputRevision",input)||!number(e,work,"seed",seed))return false;
 const auto expectedState="owner:"+std::to_string(owner)+"|step:"+std::to_string(step)+"|attempt:"+std::to_string(attempt)+"|substep:"+std::to_string(substep);
 id.flags=s.oracle?1:0;
 return id.runId==s.runId+"|runToken:"+std::to_string(run)&&id.stateId==expectedState&&id.configId==s.configId&&id.buildManifestSha==s.buildSha&&id.ownerGeneration==s.generation&&id.contextEpoch==s.gpu.contextEpoch()&&id.requestSerial>s.request&&id.payloadSerial>s.payload&&id.poolMutationSerial>=s.mutation&&id.attemptSerial==attempt&&id.substepSerial==substep&&bits64(id.paramsRevision)==bits64(params)&&bits64(id.inputRevision)==bits64(input)&&bits64(id.seed)==bits64(seed)&&id.candidateRole>=1&&id.candidateRole<=5&&id.candidateOrdinal>0&&id.candidateOrdinal>=s.ordinal[id.candidateRole]&&id.slotIndex<2;
}
static napi_value configure(napi_env e,napi_callback_info info){size_t argc=1;napi_value a[1];State* s=callState(e,info,argc,a);if(!s)return reply(e,nullptr,"ERROR","WRONG_OWNER",ExecStatus::STALE);
 uint32_t cap=0;std::string directory,build,run,config,source;bool oracle=false;std::array<std::string,3> shaders;std::array<uint8_t,32> decoded{};
 if(argc!=1||!integer(e,a[0],"nmax",cap)||cap<2||cap>4096||!text(e,a[0],"shaderDirectory",directory)||!text(e,a[0],"buildManifestSha256",build)||!decodeSha256(build,decoded)||!text(e,a[0],"sourceManifestSha256",source)||!decodeSha256(source,decoded)||source!=T159_SOURCE_SHA||!text(e,a[0],"runId",run)||run.empty()||!text(e,a[0],"configId",config,8192)||config.empty()||!boolean(e,a[0],"diagnosticOracle",oracle)||!text(e,a[0],"buildShaderSha256",shaders[0])||!text(e,a[0],"queryShaderSha256",shaders[1])||!text(e,a[0],"reduceShaderSha256",shaders[2]))return reply(e,s,"ERROR","CONFIG_ARGUMENTS");
 for(auto& sha:shaders)if(!decodeSha256(sha,decoded))return reply(e,s,"ERROR","SHADER_SHA_ARGUMENTS");
 s->ready=false;auto reset=s->gpu.reset(&s->cleanup);if(reset!=ExecStatus::OK)return reply(e,s,"ERROR","RESET_UNCONFIRMED",reset);
 s->slots[0].reset();s->slots[1].reset();s->last=RequestDiagnostics();
 try{for(auto& slot:s->slots){slot.reset(new CpuSlot(cap));slot->identityBytes.reserve(12288);}}catch(const std::bad_alloc&){return reply(e,s,"ERROR","CPU_SLOT_ALLOCATION");}
 s->generation=allocateOwner();if(!s->generation)return reply(e,s,"ERROR","OWNER_GENERATION_EXHAUSTED");
 s->request=s->payload=s->mutation=0;s->hasSnapshot=false;s->buildSha=build;s->runId=run;s->configId=config;s->oracle=oracle;std::fill(std::begin(s->ordinal),std::end(s->ordinal),0);s->uses[0]=s->uses[1]=0;
 auto initialized=s->gpu.initialize(cap,directory,s->last,&shaders);if(initialized!=ExecStatus::OK){s->record();s->gpu.reset(&s->cleanup);return reply(e,s,"ERROR",s->last.stage.c_str(),initialized);}
 s->ready=true;return reply(e,s,"READY","NONE",ExecStatus::OK);
}
static napi_value select(napi_env e,napi_callback_info info){size_t argc=4;napi_value a[4];State* s=callState(e,info,argc,a);if(!s)return reply(e,nullptr,"ERROR","WRONG_OWNER",ExecStatus::STALE);if(argc!=4||!s->ready)return reply(e,s,"ERROR","BINDING_UNAVAILABLE",ExecStatus::STALE);
 SnapshotView pool;RequestIdentity id;double radius=0;if(!snapshot(e,a[0],pool)||napi_get_value_double(e,a[1],&radius)!=napi_ok||!identity(e,a[2],a[3],*s,id)||pool.nmax!=s->slots[0]->nmax)return reply(e,s,"ERROR","REQUEST_CONTEXT_OR_INPUT",ExecStatus::INPUT_INVALID);
 s->request=id.requestSerial;s->payload=id.payloadSerial;s->ordinal[id.candidateRole]=id.candidateOrdinal;auto& slot=*s->slots[id.slotIndex];PreparedView p;
 auto before=std::chrono::steady_clock::now();auto prepared=prepareExactRanges(pool,radius,id,slot,p);double prepareMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-before).count();
 if(prepared!=PrepareStatus::OK&&prepared!=PrepareStatus::NO_PAIR)return reply(e,s,"ERROR","PREPARE_FAILED");
 std::array<const void*,13> pointers={pool.mass,pool.x,pool.y,pool.vx,pool.vy,pool.phase,pool.source,pool.state,pool.id,pool.sourceId,pool.freeSlots,pool.colMassH,pool.colMassL};std::array<uint32_t,8> hash{};std::copy(slot.header.w.begin()+32,slot.header.w.begin()+40,hash.begin());
 if(s->hasSnapshot&&id.poolMutationSerial==s->mutation&&(hash!=s->lastSnapshotSha||pointers!=s->lastPointers))return reply(e,s,"ERROR","POOL_MUTATION_UNADVANCED",ExecStatus::STALE);
 s->mutation=id.poolMutationSerial;s->lastSnapshotSha=hash;s->lastPointers=pointers;s->hasSnapshot=true;
 Result result;auto exec=s->gpu.executeIntegerPair(p,id.slotIndex,{std::chrono::steady_clock::now()+std::chrono::seconds(1)},result,s->last);s->uses[id.slotIndex]++;
 if(exec!=ExecStatus::OK){s->ready=false;s->record();auto out=reply(e,s,"ERROR",s->last.stage.c_str(),exec);put(e,out,"slotIndex",double(id.slotIndex));return out;}
 SnapshotView current;if(!snapshot(e,a[0],current)){s->ready=false;return reply(e,s,"ERROR","CURRENT_VIEW_STALE",exec,"STALE");}
 CurrentOwnerState owner;owner.identity=id;owner.current=&current;owner.poolMutationSerial=id.poolMutationSerial;owner.active=s->ready;owner.diagnosticOracle=s->oracle;VerifiedPair pair;auto consumed=verifyAndConsume(p,result,owner,pair);
 const bool accepted=consumed==ConsumeStatus::OK||consumed==ConsumeStatus::NO_PAIR;if(!accepted)s->ready=false;
 auto out=reply(e,s,consumed==ConsumeStatus::OK?"PAIR":consumed==ConsumeStatus::NO_PAIR?"NO_PAIR":"ERROR",accepted?"NONE":consumeStatusName(consumed),exec,consumeStatusName(consumed),s->oracle?(accepted?"AGREE":consumed==ConsumeStatus::ORACLE_MISMATCH?"ORACLE_MISMATCH":"NOT_EVALUATED"):"DISABLED",consumed==ConsumeStatus::OK?&pair:nullptr);
 put(e,out,"slotIndex",double(id.slotIndex));put(e,out,"prepareCpuMs",prepareMs);put(e,out,"cpuEmptyShortcut",p.n<2);put(e,out,"buildManifestSha256",s->buildSha.c_str());bytes(e,out,"headerWords",slot.header.w.data(),256,napi_uint32_array,64);bytes(e,out,"resultWords",result.w.data(),128,napi_uint32_array,32);if(s->oracle)bytes(e,out,"identityBytes",slot.identityBytes.data(),slot.identityBytes.size(),napi_uint8_array,slot.identityBytes.size());return out;
}
static napi_value reset(napi_env e,napi_callback_info info){size_t n=0;State* s=callState(e,info,n,nullptr);if(!s)return reply(e,nullptr,"ERROR","WRONG_OWNER",ExecStatus::STALE);s->ready=false;auto status=s->gpu.reset(&s->cleanup);if(status==ExecStatus::OK){s->slots[0].reset();s->slots[1].reset();s->hasSnapshot=false;}return reply(e,s,status==ExecStatus::OK?"RESET":"ERROR",status==ExecStatus::OK?"NONE":"RESET_UNCONFIRMED",status);}
static void cleanup(void* data){auto* s=static_cast<State*>(data);s->ready=false;auto status=s->gpu.reset(&s->cleanup);if(status!=ExecStatus::OK)std::fprintf(stderr,"T159_CLEANUP_FAILED stage=%s gl=%u egl=%u live=%u\\n",s->cleanup.stage.c_str(),s->cleanup.rawGlError,s->cleanup.rawEglError,s->gpu.liveBufferCount());delete s;}
static napi_value init(napi_env e,napi_value exports){auto* s=new State(e);napi_property_descriptor d[]={
 {"configure",nullptr,configure,nullptr,nullptr,nullptr,napi_default,s},{"selectPair",nullptr,select,nullptr,nullptr,nullptr,napi_default,s},{"reset",nullptr,reset,nullptr,nullptr,nullptr,napi_default,s}};napi_define_properties(e,exports,3,d);napi_add_env_cleanup_hook(e,cleanup,s);return exports;}
static napi_module module={1,0,nullptr,init,"p011_gpu_pair_prototype",nullptr,{0}};
}
#ifdef _MSC_VER
struct AutoRegister {AutoRegister(){napi_module_register(&module);}};static AutoRegister autoRegister;
#else
extern "C" __attribute__((constructor)) void RegisterP011GpuPairPrototype(){napi_module_register(&module);}
#endif
