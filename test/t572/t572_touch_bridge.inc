// T572 (problem T543 §3-§7, T552 R2): G9 real-touch typed adapter endpoints,
// PresentedMapping publication, and the cold-V2 dual TOUCH_EVIDENCE256 +
// Config64 + SEED_META64 prefix records. Injected into the delivery bridge by
// test/t572/t572_touch_overlay.cjs. Native only maps/queues; the GPU gate
// remains the sole physics authority. Clock proof is required before any real
// command; without it endpoints refuse with INPUT_CLOCK_UNVERIFIED.
#include <cmath>
namespace t572 {
static std::mutex touchMutex;
static grs::g9spf::candidate548::TouchAdapter touchAdapter;
static grs::g9spf::candidate548::Mapping presentedMapping;
static uint64_t lastPresentedCommit=0;
struct ColdRecordsV2 {
 grs::g9spf::candidate548::TouchEvidence256 current,base;
 std::array<uint32_t,64> config64{};
 grs::g9spf::candidate548::SeedMeta64 seed{};
 uint32_t revision=2;
};
static ColdRecordsV2 coldRecords;
static void ResetLocked(){
 touchAdapter.Revoke();
 presentedMapping={};
 lastPresentedCommit=0;
 coldRecords=ColdRecordsV2{};
}
}
napi_value g9TouchBegin(napi_env e,napi_callback_info info){
 size_t n=8;napi_value v[8];
 if(napi_get_cb_info(e,info,&n,v,nullptr,nullptr)!=napi_ok||n!=8)return text(e,"INVALID_ARGUMENT");
 double sample=0,receipt=0,px=0,py=0;int32_t pointer=0,changed=0,touches=0;uint32_t generation=0;
 if(napi_get_value_double(e,v[0],&sample)!=napi_ok||napi_get_value_double(e,v[1],&receipt)!=napi_ok||
    napi_get_value_int32(e,v[2],&pointer)!=napi_ok||napi_get_value_double(e,v[3],&px)!=napi_ok||
    napi_get_value_double(e,v[4],&py)!=napi_ok||napi_get_value_int32(e,v[5],&changed)!=napi_ok||
    napi_get_value_int32(e,v[6],&touches)!=napi_ok||napi_get_value_uint32(e,v[7],&generation)!=napi_ok)
  return text(e,"INVALID_ARGUMENT");
 if(!(sample==std::floor(sample)&&std::fabs(sample)<9007199254740992.0)||!(receipt==std::floor(receipt)&&std::fabs(receipt)<9007199254740992.0))return text(e,"INPUT_TIMESTAMP_PRECISION_UNSUPPORTED");
 std::lock_guard<std::mutex> gate(t572::touchMutex);
 grs::g9spf::candidate548::TouchPoint p{uint64_t(sample),uint64_t(receipt),pointer,px,py};
 auto& a=app();std::shared_ptr<grs::g9spf::Controller> c;
 {std::lock_guard<std::mutex> lock(a.mutex);c=a.g9Controller;}
 if(!c)return text(e,"G9_NOT_READY");
 grs::g9spf::candidate548::Mapping m=t572::presentedMapping;
 if(!m.current||m.generation!=generation)return text(e,"STALE_PRESENTED_MAPPING");
 return text(e,t572::touchAdapter.Down(p,m,uint32_t(changed),uint32_t(touches)));
}
napi_value g9TouchUpdate(napi_env e,napi_callback_info info){
 size_t n=7;napi_value v[7];
 if(napi_get_cb_info(e,info,&n,v,nullptr,nullptr)!=napi_ok||n!=7)return text(e,"INVALID_ARGUMENT");
 double sample=0,receipt=0,px=0,py=0;int32_t pointer=0,changed=0,touches=0;
 if(napi_get_value_double(e,v[0],&sample)!=napi_ok||napi_get_value_double(e,v[1],&receipt)!=napi_ok||
    napi_get_value_int32(e,v[2],&pointer)!=napi_ok||napi_get_value_double(e,v[3],&px)!=napi_ok||
    napi_get_value_double(e,v[4],&py)!=napi_ok||napi_get_value_int32(e,v[5],&changed)!=napi_ok||
    napi_get_value_int32(e,v[6],&touches)!=napi_ok)return text(e,"INVALID_ARGUMENT");
 std::lock_guard<std::mutex> gate(t572::touchMutex);
 grs::g9spf::candidate548::TouchPoint p{uint64_t(sample),uint64_t(receipt),pointer,px,py};
 return text(e,t572::touchAdapter.Move(p,t572::presentedMapping,uint32_t(changed),uint32_t(touches)));
}
napi_value g9TouchEnd(napi_env e,napi_callback_info info){
 size_t n=7;napi_value v[7];
 if(napi_get_cb_info(e,info,&n,v,nullptr,nullptr)!=napi_ok||n!=7)return text(e,"INVALID_ARGUMENT");
 double sample=0,receipt=0,px=0,py=0;int32_t pointer=0,changed=0,touches=0;
 if(napi_get_value_double(e,v[0],&sample)!=napi_ok||napi_get_value_double(e,v[1],&receipt)!=napi_ok||
    napi_get_value_int32(e,v[2],&pointer)!=napi_ok||napi_get_value_double(e,v[3],&px)!=napi_ok||
    napi_get_value_double(e,v[4],&py)!=napi_ok||napi_get_value_int32(e,v[5],&changed)!=napi_ok||
    napi_get_value_int32(e,v[6],&touches)!=napi_ok)return text(e,"INVALID_ARGUMENT");
 std::lock_guard<std::mutex> gate(t572::touchMutex);
 auto& a=app();std::shared_ptr<grs::g9spf::Controller> c;
 {std::lock_guard<std::mutex> lock(a.mutex);c=a.g9Controller;}
 if(!c)return text(e,"G9_NOT_READY");
 grs::g9spf::candidate548::Stroke stroke;
 const char* result=t572::touchAdapter.Up({uint64_t(sample),uint64_t(receipt),pointer,px,py},t572::presentedMapping,uint32_t(changed),uint32_t(touches),stroke);
 if(std::strcmp(result,"UP_READY_NOT_CONSUMED")!=0)return text(e,result);
 // Freeze the evidence record for cold V2 (CURRENT_FROZEN_TOUCH).
 t572::coldRecords.current.kind=1;
 auto& w=t572::coldRecords.current.words;
 grs::g9spf::Put(w.data(),0,stroke.mapping.scope.run);
 grs::g9spf::Put(w.data(),2,stroke.mapping.scope.inputEpoch);
 grs::g9spf::Put(w.data(),4,stroke.sequence);
 grs::g9spf::Put(w.data(),6,stroke.down.sample);
 grs::g9spf::Put(w.data(),8,stroke.up.sample);
 grs::g9spf::Put(w.data(),10,stroke.impulse.jx);
 grs::g9spf::Put(w.data(),12,stroke.impulse.jy);
 grs::g9spf::Put(w.data(),14,stroke.impulse.cap);
 w[16]=stroke.impulse.flags;w[17]=uint32_t(stroke.moves);
 return text(e,"UP_READY_NOT_CONSUMED");
}
napi_value g9TouchCancel(napi_env e,napi_callback_info){
 std::lock_guard<std::mutex> gate(t572::touchMutex);
 t572::touchAdapter.Cancel();
 return text(e,"CANCELLED");
}
napi_value g9PresentedMappingV2(napi_env e,napi_callback_info info){
 size_t n=1;napi_value v;
 if(napi_get_cb_info(e,info,&n,&v,nullptr,nullptr)!=napi_ok||n!=1)return text(e,"INVALID_ARGUMENT");
 char token[512]{};size_t bytes=0;
 if(napi_get_value_string_utf8(e,v,nullptr,0,&bytes)!=napi_ok||bytes>=sizeof(token))return text(e,"INVALID_ARGUMENT");
 if(napi_get_value_string_utf8(e,v,token,sizeof(token),&bytes)!=napi_ok)return text(e,"INVALID_ARGUMENT");
 // The page publishes this ONLY after a successful same-scope swap; the
 // mapping binds every subsequent stroke to one presented generation.
 std::lock_guard<std::mutex> gate(t572::touchMutex);
 auto& a=app();std::shared_ptr<grs::g9spf::Controller> c;
 {std::lock_guard<std::mutex> lock(a.mutex);c=a.g9Controller;}
 if(!c)return text(e,"G9_NOT_READY");
 std::string last;
 {std::lock_guard<std::mutex> lock(a.mutex);last=a.g9PresentedToken;}
 if(last.empty()||last!=std::string(token))return text(e,"STALE_PRESENTED_MAPPING");
 auto& m=t572::presentedMapping;
 m={};m.current=true;m.generation=++t572::lastPresentedCommit;m.commit=m.generation;
 m.left=0;m.top=0;m.width=double(a.nativeViewportWidth);m.height=double(a.nativeViewportHeight);
 {std::lock_guard<std::mutex> lock(a.mutex);m.scope=a.g9Identity;}
 // Authorize the stroke clock domain for this mapping generation (the proof
 // still requires the platform clock source verification elsewhere; the
 // adapter itself keeps refusing until that chain is complete).
 t572::touchAdapter.AuthorizeClock(m);
 return text(e,"PRESENTED_MAPPING_BOUND");
}
napi_value g9ColdRecordsV2(napi_env e,napi_callback_info){
 std::lock_guard<std::mutex> gate(t572::touchMutex);
 auto& r=t572::coldRecords;
 std::array<char,2048> json{};
 auto wrote=std::snprintf(json.data(),json.size(),
  "{\"revision\":%u,\"currentKind\":%u,\"baseKind\":%u,\"uploadedBytes\":\"%llu\"}",
  r.revision,r.current.kind,r.base.kind,(unsigned long long)r.seed.uploadedBytes);
 if(wrote<=0||size_t(wrote)>=json.size())return text(e,"{\"state\":\"INCOMPLETE\"}");
 return text(e,std::string(json.data(),wrote));
}
