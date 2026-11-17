// T550 (problem T546 §2): g9Words delegates over the shared strict decode
// contract with its own Route::G9 — G9 attempts are never labeled M3.
template<size_t N>bool g9Words(napi_env e,napi_callback_info info,std::array<uint32_t,N>& out){
 auto result=grs::words::ReadWords<N>(env,info,grs::words::Route::G9,0);
 if(result.ok)out=result.words;
 return result.ok;
}
template<size_t N>grs::g9spf::ControlIdentity g9IdentityOf(const std::array<uint32_t,N>& w){const auto p=[&](size_t i){return uint64_t(w[i])|(uint64_t(w[i+1])<<32);};return {p(0),p(2),p(4),p(6),p(8),p(10),p(12),p(14),p(16)};}
napi_value g9ResearchConfig(napi_env e,napi_callback_info info){
 std::array<uint32_t,2> request{};if(!g9Words(e,info,request)||!grs::g9spf::SelectProfile(request[0])||request[1]>1)return text(e,"G9_RESEARCH_CONFIG_REJECTED");
 grs::g9spf::Config c;c.k=request[0];c.g=16*1048576;c.muStart=32768;c.muStop=16384;c.band=1048576;c.entrainRate=4*65536;c.shear=65536;c.settleTicks=384;c.revision=1;c.research=c.synthetic=true;c.fullTrialTrace=request[1]==1;c.parameterSha=grs::g9spf::ParameterHash(c);
 std::array<uint32_t,24> words{grs::g9spf::kProfileCode,c.k,c.g,c.muStart,c.muStop,c.band,c.entrainRate,c.shear,c.settleTicks,c.revision,1,1,c.fullTrialTrace?1u:0u};for(size_t i=0;i<8;++i)words[16+i]=c.parameterSha[i];
 std::array<char,2048> data{};grs::cold503::Writer w(data.data(),data.size());w.Raw("{\"revision\":\"T529_EXPLICIT_RESEARCH_CANDIDATE_1\",\"productionDefault\":false,\"words\":[");for(size_t i=0;i<words.size();++i){if(i)w.Raw(",");w.Number(words[i]);}w.Raw("]}");return text(e,std::string(w.Data(),w.Size()));
}
napi_value g9Configure(napi_env e,napi_callback_info info){
 std::array<uint32_t,24> w{};if(!g9Words(e,info,w)||w[0]!=grs::g9spf::kProfileCode||w[10]!=1||w[11]!=1||w[12]>1||w[13]||w[14]||w[15])return text(e,"G9_CONFIG_REJECTED");
 grs::g9spf::Config c;c.k=w[1];c.g=w[2];c.muStart=w[3];c.muStop=w[4];c.band=w[5];c.entrainRate=w[6];c.shear=w[7];c.settleTicks=w[8];c.revision=w[9];c.research=c.synthetic=true;c.fullTrialTrace=w[12]==1;for(size_t i=0;i<8;++i)c.parameterSha[i]=w[16+i];
 if(!c.Valid()||c.parameterSha!=grs::g9spf::ParameterHash(c))return text(e,"CALIBRATION_CONFIG_REQUIRED");
 auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);
 if(a.coldTrace.reading||a.coldTrace.builder>=0||(a.m3Controller&&a.m3Controller.use_count()>2)||(a.g9Controller&&a.g9Controller.use_count()>2))return text(e,"G9_RETIRE_READER_BUSY");
 // Switching routes is an explicit scope retirement. Reclaim idle legacy slot
 // capacity before the new route can reserve its independent two-slot limit.
 for(int i=0;i<2;++i){if(a.coldTrace.slots[i].state!=grs::cold503::TraceManager::FREE)a.coldTrace.Close(i,"ROUTE_CHANGED");auto retired=a.coldTrace.Trim(i);retired.reset();}
 if(a.m3Controller)a.m3Controller->Stop();if(a.g9Controller)a.g9Controller->Stop();a.m3Selected=false;a.g9Selected=true;a.g9Config=c;a.K=c.k;a.Kw=64;a.requiresReset=false;a.InvalidateLocked();a.wake.notify_one();return text(e,"G9_NEW_EMPTY_RESEARCH_RUN_REQUESTED");
}
napi_value g9Session(napi_env e,napi_callback_info){auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);if(!a.g9Controller)return text(e,"{\"state\":\"G9_NOT_READY\"}");auto id=a.G9IdentityWordsLocked();std::array<char,2048> data{};grs::cold503::Writer w(data.data(),data.size());w.Raw("{\"revision\":\"G9_SPF_Q20_V1\",\"identity\":[");for(size_t i=0;i<id.size();++i){if(i)w.Raw(",");w.Number(id[i]);}w.Raw("]}");return text(e,std::string(w.Data(),w.Size()));}
napi_value g9Pulse(napi_env e,napi_callback_info info){std::array<uint32_t,22> w{};if(!g9Words(e,info,w))return text(e,"G9_PULSE_REJECTED");auto& a=app();std::shared_ptr<grs::g9spf::Controller> c;{std::lock_guard<std::mutex> lock(a.mutex);c=a.g9Controller;}if(!c)return text(e,"G9_NOT_READY");const grs::g9spf::PagePulseV3 pulse{g9IdentityOf(w),grs::g9spf::Pair(w.data(),18),grs::g9spf::Pair(w.data(),20)};auto result=c->Pulse(pulse,now());a.wake.notify_one();return text(e,std::to_string(uint32_t(result)));}
napi_value g9Target(napi_env e,napi_callback_info info){std::array<uint32_t,19> w{};if(!g9Words(e,info,w))return text(e,"G9_TARGET_REJECTED");auto& a=app();std::shared_ptr<grs::g9spf::Controller> c;{std::lock_guard<std::mutex> lock(a.mutex);c=a.g9Controller;}return text(e,c?std::to_string(uint32_t(c->Target(g9IdentityOf(w),int32_t(w[18]),now()))):"G9_NOT_READY");}
napi_value g9OfferModel(napi_env e,napi_callback_info info){std::array<uint32_t,148> w{};if(!g9Words(e,info,w))return text(e,"G9_INPUT_REJECTED");auto& a=app();std::shared_ptr<grs::g9spf::Controller> c;{std::lock_guard<std::mutex> lock(a.mutex);if(g9IdentityOf(w)!=a.g9Identity)return text(e,"G9_STALE_IDENTITY");c=a.g9Controller;}if(!c)return text(e,"G9_NOT_READY");grs::g9spf::Input128 body{};std::copy(w.begin()+20,w.end(),body.begin());auto result=c->OfferModelCommand(body,now(),grs::g9spf::Pair(w.data(),18));a.wake.notify_one();return text(e,std::to_string(uint32_t(result)));}
napi_value g9Quota(napi_env e,napi_callback_info){auto& a=app();std::shared_ptr<grs::g9spf::Controller> c;{std::lock_guard<std::mutex> lock(a.mutex);c=a.g9Controller;}if(!c)return text(e,"G9_NOT_READY");auto quota=c->Quota(now());std::array<char,16384> data{};grs::cold503::Writer writer(data.data(),data.size());writer.Raw("{\"revision\":\"GRS_M3_QUOTA_SNAPSHOT_V3\",\"words\":[");for(size_t i=0;i<quota.size();++i){if(i)writer.Raw(",");writer.Number(quota[i]);}writer.Raw("]}");return text(e,std::string(writer.Data(),writer.Size()));}
template<size_t N>std::shared_ptr<grs::g9spf::Controller> g9ColdController(const std::array<uint32_t,N>& words){auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);return g9IdentityOf(words)==a.g9Identity?a.g9Controller:nullptr;}
napi_value g9ColdCapabilities(napi_env e,napi_callback_info){auto& a=app();std::shared_ptr<grs::g9spf::Controller> c;{std::lock_guard<std::mutex> lock(a.mutex);c=a.g9Controller;}return text(e,c?c->ColdCapabilities():"{\"state\":\"G9_NOT_READY\"}");}
napi_value g9ColdBegin(napi_env e,napi_callback_info info){std::array<uint32_t,20> w{};if(!g9Words(e,info,w))return text(e,"G9_COLD_REJECTED");auto c=g9ColdController(w);if(!c)return text(e,"G9_STALE_SCOPE");auto result=c->ColdBegin(grs::g9spf::Pair(w.data(),18),now());app().wake.notify_one();return text(e,result);}
napi_value g9ColdState(napi_env e,napi_callback_info info){std::array<uint32_t,20> w{};if(!g9Words(e,info,w))return text(e,"G9_COLD_REJECTED");auto c=g9ColdController(w);return text(e,c?c->ColdState(grs::g9spf::Pair(w.data(),18),now()):"G9_STALE_SCOPE");}
napi_value g9ColdEnd(napi_env e,napi_callback_info info){std::array<uint32_t,20> w{};if(!g9Words(e,info,w))return text(e,"G9_COLD_REJECTED");auto c=g9ColdController(w);return text(e,c?c->ColdEnd(grs::g9spf::Pair(w.data(),18),now()):"G9_STALE_SCOPE");}
napi_value g9ColdPage(napi_env e,napi_callback_info info){std::array<uint32_t,21> w{};if(!g9Words(e,info,w))return text(e,"G9_COLD_REJECTED");auto c=g9ColdController(w);return text(e,c?c->ColdPage(grs::g9spf::Pair(w.data(),18),w[20],now()):"G9_STALE_SCOPE");}
