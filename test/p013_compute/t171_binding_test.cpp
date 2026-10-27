// Real CAS/identity/consume/binding, mock NAPI+executor only. NOT device GL evidence.
#define P011_GPU_TEST_INJECTION 1
#define main t159_regression_main
#include "t159_binding_test.cpp"
#undef main
static napi_value armPlan(napi_env e,State& s,const char* kind){auto a=obj(e);put(e,a,"revision","P011_GPU_TEST_INJECTION_V3");put(e,a,"runId","run");put(e,a,"kind",kind);MockCall call{&s,{a}};return armTestInjectionV3(e,&call);}
int main(){
 t159_regression_main();fault=0;nextOwner=100;MockEnv env;auto e=&env;
 for(const char* kind:{"STALE_REQUEST_HI","STALE_CONSUME_HI"}){
  State s(e);MockCall start{&s,{config(e)}};assert(str(configure(e,&start),"status")=="READY");assert(str(armPlan(e,s,kind),"status")=="ARMED");
  const auto owner=s.generation;auto p=pool(e),w=work(e);napi_value radius;napi_create_double(e,.5,&radius);auto c=context(e,s,1,1,0);MockCall request{&s,{p,radius,c,w}};
  auto r=select(e,&request);assert(str(r,"status")=="ERROR"&&s.testConsumed);assert((s.originalHiIdentity^s.modifiedHiIdentity)==(1ULL<<32));assert(uint32_t(s.originalHiIdentity)==uint32_t(s.modifiedHiIdentity));
  assert(str(r,"consumeStatus")== (std::string(kind)=="STALE_CONSUME_HI"?"STALE":"NOT_RUN"));
  assert(s.generation==owner);assert(str(armPlan(e,s,kind),"status")=="ERROR");
  MockCall stop{&s,{}};assert(str(reset(e,&stop),"status")=="RESET");assert(str(configure(e,&start),"status")=="READY");
  c=context(e,s,2,2,0);request.args={p,radius,c,w};assert(str(select(e,&request),"status")=="PAIR");reset(e,&stop);
 }
 {
  State s(e);auto global=nextOwner.load();assert(str(armPlan(e,s,"OWNER_EXHAUSTION"),"status")=="ARMED");
  MockCall start{&s,{config(e)}};assert(str(configure(e,&start),"status")=="READY");assert(s.generation==UINT64_MAX-1);assert(s.testAllocator.load()==UINT64_MAX);assert(nextOwner==global);
  MockCall stop{&s,{}};reset(e,&stop);auto r=configure(e,&start);assert(str(r,"errorCode")=="OWNER_GENERATION_EXHAUSTED");assert(!s.ready&&s.testAllocator==UINT64_MAX&&nextOwner==global);
  reset(e,&stop);assert(str(configure(e,&start),"errorCode")=="OWNER_GENERATION_EXHAUSTED");assert(s.testAllocator==UINT64_MAX);
 }
 {
  State s(e);s.programLogs[0].compileStatus="OK";s.programLogs[0].compileLogStatus="CAPTURED";s.programLogs[0].compileLog=std::string("a\0b",3);s.programLogs[0].compileReturnedBytes=3;
  auto r=reply(e,&s,"ERROR","NOT_RUN");auto p=r->properties.at("program0");assert(p->properties.at("compileLog")->string==std::string("a\0b",3));assert(num(p,"compileReturnedBytes")==3);assert(std::isnan(num(r,"nativeValidationCpuMs")));
 }
 std::puts("PASS T171 real cold CAS near UINT64_MAX, no reset/rollover; real request-hi reject vs completed-result current-owner-hi STALE; one-shot owner plan; explicit-byte NUL logs; missing timing NaN->JSON null. GPU GL injection DEVICE_NOT_RUN.");
}
