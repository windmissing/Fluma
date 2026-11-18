// T572 (problem T543 §3-§7, T552 R2): G9 real-touch typed adapter endpoints,
// PresentedMapping publication, and the cold-V2 dual TOUCH_EVIDENCE256 +
// Config64 + SEED_META64 prefix records. Injected into the delivery bridge by
// test/t572/t572_touch_overlay.cjs. Native only maps/queues; the GPU gate
// remains the sole physics authority. Clock proof is required before any real
// command; without it endpoints refuse with INPUT_CLOCK_UNVERIFIED.
#include <cmath>
namespace t572 {
static std::mutex touchMutex;static grs::g9spf::candidate548::ClockProducer clockProducer;
static grs::g9spf::candidate548::TouchAdapter touchAdapter;
static void ResetLocked(){
 clockProducer.Revoke();touchAdapter.Revoke();



}
void Revoke(){std::lock_guard<std::mutex> lock(touchMutex);ResetLocked();}
void Configure(const grs::g9spf::ControlIdentity& scope){
 grs::g9spf::candidate548::ProducerProof proof;const bool qualified=grs::g9spf::candidate548::ReadQualifiedPlatformClockProof(scope,proof);
 std::lock_guard<std::mutex> lock(touchMutex);ResetLocked();if(qualified)clockProducer.InstallVerifiedProducerProof(proof);
}
}
napi_value g9TouchBegin(napi_env e,napi_callback_info){return text(e,"TOUCH_RAW_PROVENANCE_V2_REQUIRED");}
napi_value g9TouchUpdate(napi_env e,napi_callback_info){return text(e,"TOUCH_RAW_PROVENANCE_V2_REQUIRED");}
napi_value g9TouchEnd(napi_env e,napi_callback_info){return text(e,"TOUCH_RAW_PROVENANCE_V2_REQUIRED");}
// Packet64 is a raw event packet, never a client supplied qualification.
// action/pointer/changed/active; timestamp binary64; local/window/display XY;
// vp2px(1); component window origin px; native publication generation U64;
// point source=changedTouches, unit=ns; words28..63 reserved zero.
napi_value g9TouchEventV2(napi_env e,napi_callback_info info){
 const uint64_t receipt=grs::g9spf::candidate548::ClockProducer::Now();
 std::array<uint32_t,64> words{};if(!g9Words(e,info,words)||!receipt||words[0]>3u)return text(e,"TOUCH_PACKET_INVALID");
 for(size_t i=28;i<64;++i)if(words[i])return text(e,"TOUCH_PACKET_RESERVED_NONZERO");
 if(words[0]==3u){std::lock_guard<std::mutex> lock(t572::touchMutex);t572::touchAdapter.Cancel();return text(e,"CANCELLED");}
 if(words[26]!=1u||words[27]!=1u||words[2]>1u||words[3]>1u)return text(e,"TOUCH_POINT_SOURCE_INVALID");
 double sample=0;std::memcpy(&sample,words.data()+4,8);if(!std::isfinite(sample)||sample<=0||sample>9007199254740991.0||sample!=std::floor(sample))return text(e,"INPUT_TIMESTAMP_PRECISION_UNSUPPORTED");
 grs::g9spf::candidate548::TouchPoint point;point.sample=uint64_t(sample);point.receipt=receipt;point.pointer=int32_t(words[1]);
 double* fields[]={&point.raw.localX,&point.raw.localY,&point.raw.windowX,&point.raw.windowY,&point.raw.displayX,&point.raw.displayY,&point.raw.scale,&point.raw.originX,&point.raw.originY};for(size_t i=0;i<9;++i){std::memcpy(fields[i],words.data()+6+2*i,8);if(!std::isfinite(*fields[i]))return text(e,"TOUCH_POINT_INVALID");}
 if(point.raw.scale<=0)return text(e,"TOUCH_POINT_INVALID");point.px=point.raw.localX*point.raw.scale;point.py=point.raw.localY*point.raw.scale;point.raw.complete=true;
 auto& a=app();std::shared_ptr<grs::g9spf::Controller> c;grs::g9spf::candidate548::Mapping snapshot;
 {std::lock_guard<std::mutex> lock(a.mutex);c=a.g9Controller;if(a.active&&a.g9Identity==a.g9PresentedMapping.scope)snapshot=a.g9PresentedMapping;}
 std::lock_guard<std::mutex> lock(t572::touchMutex);
 if(!c||!snapshot.current||snapshot.generation!=grs::g9spf::Pair(words.data(),24)||!c->CanPresent(snapshot.generation,receipt)){t572::ResetLocked();return text(e,"STALE_PRESENTED_MAPPING");}
 if(!t572::clockProducer.Valid(snapshot.scope)){t572::touchAdapter.Revoke();return text(e,"INPUT_CLOCK_UNVERIFIED");}
 if(words[0]==0u){t572::touchAdapter.AuthorizeClock(snapshot,t572::clockProducer);return text(e,t572::touchAdapter.Down(point,snapshot,words[2],words[3]));}
 if(words[0]==1u)return text(e,t572::touchAdapter.Move(point,snapshot,words[2],words[3]));
 grs::g9spf::candidate548::Stroke stroke;const char* up=t572::touchAdapter.Up(point,snapshot,words[2],words[3],stroke);if(std::strcmp(up,"UP_READY_NOT_CONSUMED"))return text(e,up);
 const auto queued=c->OfferTouch(stroke,t572::clockProducer,grs::g9spf::candidate548::ClockProducer::Now());a.wake.notify_one();
 return text(e,queued==grs::g9spf::Result::OK?"QUEUED_NOT_CONSUMED":queued==grs::g9spf::Result::WOULD_BLOCK?"QUEUE_FULL":queued==grs::g9spf::Result::STALE?"STALE_UP_NOT_REFRESHED":"TOUCH_OFFER_REJECTED");
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
 // A read never publishes mapping, changes generation, or grants clock proof.
 grs::g9spf::candidate548::Mapping candidate;std::string last;std::shared_ptr<grs::g9spf::Controller> c;
 {auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);c=a.g9Controller;if(!c||!a.active)return text(e,"G9_NOT_READY");last=a.g9PresentedToken;if(a.g9Identity==a.g9PresentedMapping.scope)candidate=a.g9PresentedMapping;}
 if(last.empty()||last!=std::string(token)||!candidate.current||!c->CanPresent(candidate.generation,grs::g9spf::candidate548::ClockProducer::Now()))return text(e,"STALE_PRESENTED_MAPPING");
 std::array<char,2048> data{};grs::cold503::Writer w(data.data(),data.size());
 w.Raw("{\"revision\":\"G9_PRESENTED_MAPPING_V2\",\"state\":\"PRESENTED_MAPPING_BOUND\",\"generation\":");w.U64(candidate.generation);w.Raw(",\"commit\":");w.U64(candidate.commit);
 w.Raw(",\"width\":");w.Number(uint32_t(candidate.width));w.Raw(",\"height\":");w.Number(uint32_t(candidate.height));w.Raw(",\"parameterSha\":[");for(size_t i=0;i<8;++i){if(i)w.Raw(",");w.Number(candidate.parameter[i]);}w.Raw("],\"seedSha\":[");for(size_t i=0;i<8;++i){if(i)w.Raw(",");w.Number(candidate.seed[i]);}w.Raw("]}");return text(e,std::string(w.Data(),w.Size()));
}
napi_value g9ColdRecordsV2(napi_env e,napi_callback_info){return text(e,"READ_BOUND_COLD_PREFIX_V2_REQUIRED");}

napi_value g9ClockCapabilitiesV1(napi_env e,napi_callback_info){
 auto& a=app();grs::g9spf::ControlIdentity scope;{std::lock_guard<std::mutex> lock(a.mutex);scope=a.g9Identity;}
 std::lock_guard<std::mutex> gate(t572::touchMutex);std::array<char,2048> data{};grs::cold503::Writer w(data.data(),data.size());w.Raw("{\"revision\":\"G9_CLOCK_CAPABILITIES_V1\",\"producerId\":\"ARKUI_BASE_EVENT_TIMESTAMP\",\"profile\":\"OHOS_TOUCH_MONOTONIC_NS_R1\",\"unit\":\"ns\",\"epoch\":\"CLOCK_MONOTONIC\",\"proofGeneration\":");w.U64(t572::clockProducer.Generation());w.Raw(",\"verified\":");w.Bool(t572::clockProducer.Valid(scope));w.Raw(",\"state\":");const auto* state=t572::clockProducer.Valid(scope)?"VERIFIED_NATIVE_PRODUCER":"INPUT_CLOCK_UNVERIFIED";w.String(state,std::strlen(state));auto sha=[&](const std::array<uint32_t,8>& hash){std::array<uint8_t,32> raw{};for(size_t i=0;i<8;++i)for(size_t j=0;j<4;++j)raw[4*i+j]=uint8_t(hash[i]>>(8*j));w.Hex(raw.data(),raw.size());};const auto& proof=t572::clockProducer.Proof();w.Raw(",\"platformBuildSha\":");sha(proof.osBuild);w.Raw(",\"platformSourceSha\":");sha(proof.producerSource);w.Raw(",\"conversionSourceSha\":");sha(proof.conversionSource);w.Raw(",\"nativeClockSourceSha\":");sha(proof.nativeClockSource);w.Raw(",\"requiredEvidence\":\"linked native provider must verify deployed OS build -> event producer -> conversion -> CLOCK_MONOTONIC; no JS proof grant\",\"nativeNow\":");w.U64(grs::g9spf::candidate548::ClockProducer::Now());w.Raw("}");return text(e,std::string(w.Data(),w.Size()));
}
