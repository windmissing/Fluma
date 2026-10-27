// Real binding/prepare/owner code, test-double NAPI+executor. NOT GPU/Worker runtime evidence.
#define T159_SOURCE_SHA "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
#include "pair_binding_v1.cpp"
#include <cassert>
static int fault=0;
namespace p011gpu {
struct ContextSlot::Opaque{std::thread::id thread=std::this_thread::get_id();uint32_t state[2]={};};
ContextSlot::ContextSlot():impl_(new Opaque()){}
ContextSlot::~ContextSlot(){delete impl_;}
uint32_t ContextSlot::slotState(uint32_t i)const{return impl_->state[i];}
ExecStatus ContextSlot::reset(RequestDiagnostics* d){if(impl_->thread!=std::this_thread::get_id())return ExecStatus::STALE;
 if(fault==3){quarantined_=true;if(d){d->stage="MOCK_DESTROY_FAILURE";d->rawEglError=0x3002;d->resourcesRetired=false;}return ExecStatus::GL_ERROR;}
 liveBuffers_=0;initialized_=false;quarantined_=false;impl_->state[0]=impl_->state[1]=0;if(d){*d=RequestDiagnostics();d->resourcesRetired=true;d->resetStatus="RETIRED";}return ExecStatus::OK;}
void ContextSlot::cancelOwner(uint64_t){reset();}
ExecStatus ContextSlot::initialize(uint32_t n,const std::string&,RequestDiagnostics& d,const std::array<std::string,3>*){nmax_=n;epoch_=epoch_?epoch_+1:0x100000001ULL;liveBuffers_=peakBuffers_=12;initialized_=true;d.valid=true;return ExecStatus::OK;}
ExecStatus ContextSlot::executeIntegerPair(const PreparedView& p,uint32_t slot,Deadline,Result& r,RequestDiagnostics& d){
 if(fault==1){quarantined_=true;impl_->state[slot]=4;d.stage="MOCK_WAIT_FAILED";d.rawGlError=0x505;d.rawWaitReturn=0x911d;return ExecStatus::WAIT_FAILED;}
 r=cpuEmptyResult(*p.header);uint32_t a=0,b=0;if(legacyPair(*p.snapshot,p.radius,a,b)){
  r.w[2]=kStatusPair;r.w[4]=p.snapshot->id[a];r.w[5]=p.snapshot->id[b];r.w[6]=a;r.w[7]=b;
  for(uint32_t i=0;i<p.n;i++){if(p.leaves[i].slot==a)r.w[8]=i;if(p.leaves[i].slot==b)r.w[9]=i;}
 }
 if(p.n>=2){uint32_t count=p.K,reduce=0;do{count=(count+63)/64;reduce++;}while(count>1);r.w[10]=p.h+1+reduce;}
 if(fault==2)r.w[23]^=1;d.valid=true;return ExecStatus::OK;
}
}
static napi_value obj(napi_env e){napi_value o;napi_create_object(e,&o);return o;}
static std::string str(napi_value o,const char* key){return o->properties.at(key)->string;}
static double num(napi_value o,const char* key){return o->properties.at(key)->number;}
static napi_value config(napi_env e){auto o=obj(e);put(e,o,"nmax",4.);for(auto key:{"buildManifestSha256","sourceManifestSha256","buildShaderSha256","queryShaderSha256","reduceShaderSha256"})put(e,o,key,T159_SOURCE_SHA);put(e,o,"shaderDirectory","MOCK_ONLY");put(e,o,"runId","run");put(e,o,"configId","config");put(e,o,"diagnosticOracle",true);return o;}
static napi_value pool(napi_env e){auto o=obj(e);put(e,o,"nmax",4.);put(e,o,"cols",1.);put(e,o,"liveCount",2.);put(e,o,"freeCount",2.);put(e,o,"nextId",3.);
 for(const char* k:{"pMass","pX","pY","pVx","pVy","colParticleMassH","colParticleMassL"}){double v[4]={};if(std::string(k)=="pMass"||std::string(k)=="colParticleMassH")v[0]=v[1]=1;if(std::string(k)=="colParticleMassH")v[0]=2;if(std::string(k)=="pX")v[0]=v[1]=.5;if(std::string(k)=="pY"){v[0]=10;v[1]=10.1;}size_t n=std::string(k).find("col")==0?1:4;bytes(e,o,k,v,n*8,napi_float64_array,n);}
 for(const char* k:{"pPhase","pSource","pState"}){uint8_t v[4]={};if(std::string(k)=="pState")v[0]=v[1]=1;bytes(e,o,k,v,4,napi_uint8_array,4);}
 uint32_t ids[4]={1,2,0,0},sources[4]={17,17,0,0};int32_t free[4]={2,3,0,0};bytes(e,o,"pId",ids,16,napi_uint32_array,4);bytes(e,o,"pSourceId",sources,16,napi_uint32_array,4);bytes(e,o,"freeSlots",free,16,napi_int32_array,4);return o;}
static napi_value work(napi_env e){auto w=obj(e);put(e,w,"ownerId",4294967297.);put(e,w,"runToken",4294967298.);put(e,w,"stepEpoch",4294967299.);put(e,w,"attemptToken",4294967300.);put(e,w,"substepId",4294967301.);put(e,w,"paramsRevision",9007199254740991.);put(e,w,"inputRevision",4294967297.);put(e,w,"seed",-0.);return w;}
static napi_value context(napi_env e,State& s,uint64_t request,uint64_t mutation,uint32_t slot){auto c=obj(e);put(e,c,"schemaRevision","T141_REQUEST_CONTEXT_V2");put(e,c,"identityEncodingRevision",kIdentityEncodingV2);put(e,c,"runId","run|runToken:4294967298");put(e,c,"stateId","owner:4294967297|step:4294967299|attempt:4294967300|substep:4294967301");put(e,c,"configId","config");put(e,c,"buildManifestSha256",T159_SOURCE_SHA);
 put64(e,c,"ownerGeneration",s.generation);put64(e,c,"contextEpoch",s.gpu.contextEpoch());put64(e,c,"payloadSerial",request);put64(e,c,"requestSerial",request);put64(e,c,"poolMutationSerial",mutation);put64(e,c,"attemptSerial",4294967300ULL);put64(e,c,"substepSerial",4294967301ULL);put(e,c,"candidateRole",3.);put(e,c,"candidateOrdinal",7.);put(e,c,"slotIndex",double(slot));put(e,c,"paramsRevision",9007199254740991.);put(e,c,"inputRevision",4294967297.);put(e,c,"seed",-0.);return c;}
int main(){MockEnv env1,env2;auto e=&env1;State s(e),other(&env2);nextOwner=0x100000007ULL;MockCall start{&s,{config(e)}};auto ready=configure(e,&start);assert(str(ready,"status")=="READY");assert(num(ready,"ownerGenerationHi")==1);assert(num(ready,"contextEpochHi")==1);assert(other.generation==0);
 auto p=pool(e),w=work(e);napi_value radius;napi_create_double(e,.5,&radius);uint64_t serial=0x200000001ULL,mutation=0x300000001ULL;
 auto run=[&](uint32_t slot){auto c=context(e,s,serial++,mutation,slot);MockCall call{&s,{p,radius,c,w}};return select(e,&call);};
 assert(str(run(0),"status")=="PAIR");assert(str(run(1),"status")=="PAIR");assert(s.uses[0]==1&&s.uses[1]==1&&s.slots[0].get()!=s.slots[1].get());
 auto wrong=configure(&env2,&start);assert(str(wrong,"errorCode")=="WRONG_OWNER");std::string wrongThread;std::thread t([&]{wrongThread=str(configure(e,&start),"errorCode");});t.join();assert(wrongThread=="WRONG_OWNER");
 auto source=p->properties["pX"]->buffer;reinterpret_cast<double*>(source->storage.data())[0]=.6;assert(str(run(0),"errorCode")=="POOL_MUTATION_UNADVANCED");mutation++;assert(str(run(0),"status")=="PAIR");
 double masses[4]={1,1,0,0};bytes(e,p,"pMass",masses,32,napi_float64_array,4);assert(str(run(1),"errorCode")=="POOL_MUTATION_UNADVANCED");mutation++;assert(str(run(1),"status")=="PAIR");
 p->properties["pMass"]->buffer->detached=true;assert(str(run(0),"errorCode")=="REQUEST_CONTEXT_OR_INPUT");p->properties["pMass"]->buffer->detached=false;
 auto c=context(e,s,serial++,mutation,0);put(e,c,"attemptSerialHi",0.);MockCall bad{&s,{p,radius,c,w}};assert(str(select(e,&bad),"status")=="ERROR");
 fault=1;auto failed=run(0);assert(str(failed,"execStatus")=="WAIT_FAILED"&&num(failed,"rawGlError")==0x505&&num(failed,"rawWaitReturn")==0x911d);assert(!s.ready&&s.gpu.slotState(0)==4);assert(str(run(1),"errorCode")=="BINDING_UNAVAILABLE");
 MockCall stop{&s,{}};fault=3;auto unreleased=reset(e,&stop);assert(str(unreleased,"status")=="ERROR"&&num(unreleased,"liveBuffers")==12&&!unreleased->properties["resourcesRetired"]->boolean);
 fault=0;assert(str(reset(e,&stop),"status")=="RESET");auto old=s.generation;assert(str(configure(e,&start),"status")=="READY"&&s.generation>old);mutation++;fault=2;auto protocol=run(1);assert(str(protocol,"consumeStatus")=="PROTOCOL_ERROR"&&str(protocol,"oracleVerdict")=="NOT_EVALUATED");fault=0;reset(e,&stop);
 nextOwner=UINT64_MAX;assert(str(configure(e,&start),"errorCode")=="OWNER_GENERATION_EXHAUSTED");assert(nextOwner==UINT64_MAX);
 std::puts("PASS T159 binding: env/thread isolation, full64 original/context IDs, two slots, actual mutation+alias guard, detached typed view, stale hi-word, WAIT raw code/quarantine, failed retirement count, recovery generation, protocol classification, fail-closed owner exhaustion. NAPI/EXECUTOR=HOST_TEST_DOUBLES GPU=NOT_RUN");
}
