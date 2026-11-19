#include "napi_words_contract.h"
#include <algorithm>
#include <cstdio>
#include <time.h>
namespace grs::words {
namespace {
thread_local InputCall* current=nullptr;
std::mutex configurationMutex;
uint64_t Boot(){timespec t{};if(clock_gettime(CLOCK_MONOTONIC,&t)||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000||uint64_t(t.tv_sec)>(UINT64_MAX-uint64_t(t.tv_nsec))/1000000000ULL)return 0;return uint64_t(t.tv_sec)*1000000000ULL+uint64_t(t.tv_nsec);}
template<size_t N>void Copy(std::array<char,N>& out,const char* value){out={};if(value)std::strncpy(out.data(),value,N-1);}
std::string Quote(const std::string& value){std::string out="\"";for(unsigned char c:value){if(c=='"'||c=='\\'){out+='\\';out+=char(c);}else if(c<32){char b[7];std::snprintf(b,sizeof(b),"\\u%04x",c);out+=b;}else out+=char(c);}return out+'"';}
std::string U64(uint64_t n){return Quote(std::to_string(n));}
bool Decimal(const std::string& text,uint64_t& n){n=0;if(text.empty()||text.size()>20||text[0]=='0')return false;for(char c:text){if(c<'0'||c>'9'||n>(UINT64_MAX-uint64_t(c-'0'))/10)return false;n=n*10+uint64_t(c-'0');}return n!=0;}
const char* Qualification(int32_t status){return status<0?"NOT_EVALUATED":status==napi_ok?"VALID":"API_FAILED";}
std::string Nullable(bool valid,uint64_t n){return valid?std::to_string(n):"null";}
std::string Bool(bool valid,bool value){return valid?(value?"true":"false"):"null";}
int32_t Status(const Observation& o,Stage stage){switch(stage){case Stage::CB_INFO:case Stage::ARG_COUNT:return o.cbInfoStatus;case Stage::IS_TYPEDARRAY:return o.isTypedarrayStatus;case Stage::TYPEDARRAY_INFO:case Stage::ELEMENT_TYPE:case Stage::PROVIDER_PROFILE:return o.isSendableStatus>napi_ok?o.isSendableStatus:o.typedarrayInfoStatus;case Stage::ELEMENT_COUNT:case Stage::LENGTH_UNIT:case Stage::BYTE_OFFSET:case Stage::DATA_POINTER:return o.typedarrayInfoStatus;case Stage::DETACHED_API:case Stage::DETACHED_VALUE:return o.isDetachedStatus;case Stage::BACKING_INFO:case Stage::BASE_POINTER:case Stage::BACKING_BYTES:return o.arraybufferInfoStatus;default:return -1;}}
std::string Identity(const Snapshot& s){return "\"api\":"+Quote(s.api.data())+",\"route\":"+Quote(RouteName(s.route))+",\"bootStamp\":"+U64(s.bootStamp)+",\"diagGeneration\":"+U64(s.diagGeneration)+",\"attemptId\":"+U64(s.attemptId);}
std::string Error(const Snapshot& s,const char* code){return "{\"revision\":\"GPU_NAPI_ERROR_V1\",\"ok\":false,\"state\":\"REJECTED\",\"code\":"+Quote(std::strlen(code)<=128?code:"REPLY_NON_JSON_OR_OVERSIZE")+","+Identity(s)+",\"stage\":"+Quote(StageName(s.firstFailedStage))+",\"reason\":"+Quote(s.reason.data())+",\"napiStatus\":"+(s.rawStatus<0?"null":std::to_string(s.rawStatus))+",\"qualification\":"+Quote(Qualification(s.rawStatus))+",\"diagnosticRevision\":\"GPU_INPUT_DIAGNOSTIC_V2\"}";}
napi_value Text(napi_env env,const std::string& text){napi_value value=nullptr;return napi_create_string_utf8(env,text.c_str(),text.size(),&value)==napi_ok?value:nullptr;}
bool Pending(napi_env env){bool pending=false;return napi_is_exception_pending(env,&pending)!=napi_ok||pending;}
std::string JsonState(const std::string& raw){const auto key=raw.find("\"state\"");if(key==std::string::npos)return {};auto at=raw.find(':',key+7);if(at==std::string::npos)return {};at=raw.find_first_not_of(" \r\n\t",at+1);if(at==std::string::npos||raw[at]!='"')return {};auto end=raw.find('"',at+1);return end==std::string::npos?std::string{}:raw.substr(at+1,end-at-1);}
bool FailedState(const std::string& state){for(auto token:{"REJECT","INVALID","NOT_READY","NOT_FOUND","STALE","FAULT","FAILED","INCOMPLETE","UNSUPPORTED","WOULD_BLOCK","BUSY","EXPIRED","MISSING","EXHAUSTED","REQUIRED","UNVERIFIED"})if(state.find(token)!=std::string::npos)return true;return false;}
}
DiagnosticStore& DiagnosticStore::Instance(){static DiagnosticStore store;return store;}
InputCall* CurrentInputCall(){return current;}
InputCall::InputCall(Route route,const char* api,size_t n){
 previous_=current;current=this;snapshot.route=route;snapshot.expectedN=uint32_t(n);Copy(snapshot.api,api);
 auto& store=DiagnosticStore::Instance();std::lock_guard<std::mutex> lock(store.mutex);if(!store.bootStamp)store.bootStamp=Boot();snapshot.bootStamp=store.bootStamp;snapshot.diagGeneration=store.diagGeneration;
 if(store.bootStamp&&store.serial!=UINT64_MAX)snapshot.attemptId=++store.serial;else Copy(snapshot.reason,"ATTEMPT_ID_EXHAUSTED_OR_CLOCK_UNAVAILABLE");
}
InputCall::~InputCall(){if(!finished_)Finish(false,"INPUT_CALL_UNWOUND");current=previous_;}
void InputCall::Capture(const Observation& observed,Stage stage,bool ok,const uint32_t* words,size_t n){
 snapshot.observed=observed;snapshot.firstFailedStage=stage;snapshot.rawStatus=ok?-1:Status(observed,stage);
 if(observed.copyComplete){snapshot.headerWords=uint32_t(std::min<size_t>(n,16));std::copy(words,words+snapshot.headerWords,snapshot.configHeader.begin());}
 if(!ok)Copy(snapshot.reason,StageName(stage));
}
bool Semantic(bool pass,const char* reason,uint32_t bit){if(auto* call=current){if(bit<12)(pass?call->snapshot.semanticPassed:call->snapshot.semanticFailed)|=1u<<bit;if(!pass&&!call->snapshot.reason[0]){Copy(call->snapshot.reason,reason);call->snapshot.firstFailedStage=Stage::API_SEMANTIC;call->snapshot.rawStatus=-1;}}return pass;}
void InputCall::Finish(bool ok,const char* reason,bool newGeneration){
 if(finished_)return;finished_=true;snapshot.ok=ok;snapshot.used=snapshot.attemptId!=0;if(!ok&&!snapshot.reason[0])Copy(snapshot.reason,reason);
 auto& store=DiagnosticStore::Instance();std::lock_guard<std::mutex> lock(store.mutex);
 if(newGeneration&&ok&&store.diagGeneration!=UINT64_MAX){++store.diagGeneration;for(auto& ledger:store.routes)ledger={};snapshot.diagGeneration=store.diagGeneration;}
 if(!snapshot.used||snapshot.diagGeneration!=store.diagGeneration)return;
 auto& ledger=store.routes[size_t(snapshot.route)];ledger.lastAttempt=snapshot;if(!ok&&!ledger.firstFailure.used)ledger.firstFailure=snapshot;
}
napi_value Invoke(napi_env env,napi_callback_info info,Route route,const char* api,size_t n,napi_callback implementation,bool configuration,bool jsonStatus){
 std::unique_lock<std::mutex> configLock(configurationMutex,std::defer_lock);if(configuration)configLock.lock();
 InputCall call(route,api,n);
 if(!call.snapshot.attemptId){call.Finish(false,"ID_EXHAUSTED");return Pending(env)?nullptr:Text(env,Error(call.snapshot,"ID_EXHAUSTED"));}
 if(configuration&&call.snapshot.diagGeneration==UINT64_MAX){call.Finish(false,"DIAGNOSTIC_GENERATION_EXHAUSTED");return Text(env,Error(call.snapshot,"DIAGNOSTIC_GENERATION_EXHAUSTED"));}
 napi_value value=implementation(env,info);
 if(Pending(env)||!value){call.Finish(false,"PENDING_NAPI_EXCEPTION");return nullptr;}
 size_t bytes=0;auto status=napi_get_value_string_utf8(env,value,nullptr,0,&bytes);
 if(status!=napi_ok||bytes>16384){call.snapshot.rawStatus=int32_t(status);call.Finish(false,"REPLY_STRING_OR_BOUND");return Pending(env)?nullptr:Text(env,Error(call.snapshot,"REPLY_STRING_OR_BOUND"));}
 std::array<char,16385> buffer{};size_t used=0;status=napi_get_value_string_utf8(env,value,buffer.data(),buffer.size(),&used);
 if(status!=napi_ok||used!=bytes){call.snapshot.rawStatus=int32_t(status);call.Finish(false,"REPLY_READ_FAILED");return Pending(env)?nullptr:Text(env,Error(call.snapshot,"REPLY_READ_FAILED"));}
 std::string raw(buffer.data(),used);const bool json=raw.size()>=2&&raw.front()=='{'&&raw.back()=='}';const auto state=json?JsonState(raw):raw;
 const bool ok=!call.snapshot.reason[0]&&((json&&!FailedState(state)&&raw.find("\"error\"")==std::string::npos)||(!json&&(raw=="0"||raw=="M3_NEW_EMPTY_RUN_REQUESTED"||raw=="M5_NEW_EMPTY_RUN_REQUESTED"||raw=="G9_NEW_EMPTY_RESEARCH_RUN_REQUESTED"||raw=="G9_NEW_S40_REAL_TOUCH_R2_REQUESTED")));
 if(json&&raw.size()+Identity(call.snapshot).size()+16>16384){call.Finish(false,"REPLY_UTF8_BOUND");return Text(env,Error(call.snapshot,"REPLY_UTF8_BOUND"));}
 call.Finish(ok,state.empty()?"API_SEMANTIC_REJECTED":state.c_str(),configuration&&ok);
 if(!ok)return Text(env,Error(call.snapshot,state.empty()?"API_SEMANTIC_REJECTED":state.c_str()));
 if(json){raw.pop_back();raw+=",\"inputCall\":{"+Identity(call.snapshot)+"}}";if(raw.size()>16384)return Text(env,Error(call.snapshot,"REPLY_UTF8_BOUND"));return Text(env,raw);}
 if(jsonStatus)return Text(env,"{\"revision\":\"GPU_NAPI_SUCCESS_V1\",\"ok\":true,\"state\":"+Quote(raw)+","+Identity(call.snapshot)+"}");
 return value;
}
bool SnapshotJson(const std::string& routeText,const std::string& attemptText,std::string& out,const std::string& bootText,const std::string& generationText){
 Route route;auto error=[&](const char* reason){out="{\"revision\":\"GPU_INPUT_DIAGNOSTIC_V2\",\"error\":"+Quote(reason)+"}";return true;};
 if(routeText=="M3A")route=Route::M3A;else if(routeText=="M3B")route=Route::M3B;else if(routeText=="G9")route=Route::G9;else return error("UNKNOWN_ROUTE");
 uint64_t id=0,boot=0,generation=0;if(!attemptText.empty()&&!Decimal(attemptText,id))return error("ATTEMPT_ID_INVALID");if((bootText.empty()!=generationText.empty())||(!bootText.empty()&&(!Decimal(bootText,boot)||!Decimal(generationText,generation))))return error("SNAPSHOT_IDENTITY_INVALID");
 Snapshot selected,first;{auto& store=DiagnosticStore::Instance();std::lock_guard<std::mutex> lock(store.mutex);const auto& ledger=store.routes[size_t(route)];first=ledger.firstFailure;
  auto match=[&](const Snapshot& s){return s.used&&(!id||s.attemptId==id)&&(!boot||(s.bootStamp==boot&&s.diagGeneration==generation));};
  if(match(ledger.lastAttempt))selected=ledger.lastAttempt;else if(id&&match(first))selected=first;
 }
 if(!selected.used)return error(id?"SNAPSHOT_EVICTED_OR_NOT_FOUND":"NO_ATTEMPT_RECORDED");
 const auto& s=selected;const auto& o=s.observed;out="{\"revision\":\"GPU_INPUT_DIAGNOSTIC_V2\","+Identity(s)+",\"ok\":"+(s.ok?"true":"false")+",\"expectedN\":"+std::to_string(s.expectedN)+",\"expectedBytes\":"+std::to_string(uint64_t(s.expectedN)*4)+",\"firstFailedStage\":"+(s.ok?"null":Quote(StageName(s.firstFailedStage)))+",\"reason\":"+(s.ok?"null":Quote(s.reason.data()))+",\"napiStatus\":"+(s.rawStatus<0?"null":std::to_string(s.rawStatus))+",\"qualification\":"+Quote(Qualification(s.rawStatus));
 out+=",\"rawReturnedLength\":"+Nullable(o.typedarrayInfoStatus==napi_ok,o.rawReturnedLength)+",\"lengthUnit\":"+Quote(provider629::Name(o.lengthUnit))+",\"normalizedElementCount\":"+Nullable(o.normalizationObserved,o.normalizedElementCount)+",\"providerProfileId\":"+Quote(o.providerProfileId.data())+",\"providerEvidenceSHA\":"+Quote(o.providerEvidenceSHA.data())+",\"providerQualification\":"+provider629::Lookup(o.providerProfileId.data());
 out+=",\"apiStatuses\":{";const char* names[]={"cbInfo","isTypedarray","typedarrayInfo","isDetached","arraybufferInfo","isSendable"};const int32_t statuses[]={o.cbInfoStatus,o.isTypedarrayStatus,o.typedarrayInfoStatus,o.isDetachedStatus,o.arraybufferInfoStatus,o.isSendableStatus};for(size_t i=0;i<6;++i){if(i)out+=',';out+=Quote(names[i])+":{\"qualification\":"+Quote(Qualification(statuses[i]))+",\"status\":"+(statuses[i]<0?"null":std::to_string(statuses[i]))+"}";}out+='}';
 out+=",\"argc\":"+Nullable(o.cbInfoStatus==napi_ok,o.argc)+",\"typed\":"+Bool(o.typed==Tri::VALID,o.typedValue)+",\"elementKind\":"+(o.typedarrayInfoStatus==napi_ok?std::to_string(o.elementKind):"null")+",\"elementLength\":"+Nullable(o.typedarrayInfoStatus==napi_ok,o.elementLength)+",\"byteOffset\":"+Nullable(o.typedarrayInfoStatus==napi_ok,uint64_t(o.byteOffset)) +",\"dataPresent\":"+Bool(o.typedarrayInfoStatus==napi_ok,o.dataPresent)+",\"detached\":"+Bool(o.detached==Tri::VALID,o.detachedValue)+",\"basePresent\":"+Bool(o.arraybufferInfoStatus==napi_ok,o.basePresent)+",\"baseEqualsData\":"+Bool(o.arraybufferInfoStatus==napi_ok,o.baseEqualsData)+",\"backingBytes\":"+Nullable(o.arraybufferInfoStatus==napi_ok,o.backingBytes)+",\"copyComplete\":"+(o.copyComplete?"true":"false")+",\"configHeader\":";
 if(!o.copyComplete)out+="null";else{out+='[';for(size_t i=0;i<s.headerWords;++i){if(i)out+=',';out+=std::to_string(s.configHeader[i]);}out+=']';}
 out+=",\"semanticChecks\":{";const char* checks[]={"REVISION","FEATURES","PROFILE","FULL","COLD","HASH","FLAGS","MODE","RESERVED","CONFIG64","SOURCE","CLOCK"};for(size_t i=0;i<12;++i){if(i)out+=',';out+=Quote(checks[i])+":"+Quote(s.semanticFailed&(1u<<i)?"FAIL":s.semanticPassed&(1u<<i)?"PASS":"NOT_EVALUATED");}out+='}';
 out+=",\"firstFailure\":";if(!first.used)out+="null";else out+="{"+Identity(first)+",\"stage\":"+Quote(StageName(first.firstFailedStage))+"}";out+='}';if(out.size()>4096)return error("SNAPSHOT_ENCODING_INCOMPLETE");return true;
}
napi_value Diagnostics(napi_env env,napi_callback_info info){
 auto fail=[&](const char* reason){return Pending(env)?nullptr:Text(env,"{\"revision\":\"GPU_INPUT_DIAGNOSTIC_V2\",\"error\":"+Quote(reason)+"}");};
 size_t argc=5;napi_value argv[5]{};if(napi_get_cb_info(env,info,&argc,argv,nullptr,nullptr)!=napi_ok||argc<1||argc>4)return fail("ARG_COUNT");
 std::array<std::string,4> strings;for(size_t i=0;i<argc;++i){size_t n=0;if(napi_get_value_string_utf8(env,argv[i],nullptr,0,&n)!=napi_ok||n>(i?20u:3u))return fail("QUERY_ARGUMENT_INVALID");char text[24]{};size_t copied=0;if(napi_get_value_string_utf8(env,argv[i],text,sizeof(text),&copied)!=napi_ok||copied!=n)return fail("QUERY_ARGUMENT_INVALID");strings[i].assign(text,copied);}
 std::string out;SnapshotJson(strings[0],strings[1],out,strings[2],strings[3]);return Text(env,out);
}
} // namespace grs::words
