template<size_t N>bool m3Words(napi_env env,napi_callback_info info,std::array<uint32_t,N>& out){
 size_t count=1;napi_value value,backing;bool typed=false,detached=false;napi_typedarray_type type;size_t length=0,offset=0,bytes=0;void* pointer=nullptr;void* base=nullptr;
 if(napi_get_cb_info(env,info,&count,&value,nullptr,nullptr)!=napi_ok||count!=1||napi_is_typedarray(env,value,&typed)!=napi_ok||!typed||napi_get_typedarray_info(env,value,&type,&length,&pointer,&backing,&offset)!=napi_ok||type!=napi_uint32_array||length!=N||offset||!pointer)return false;
 if(napi_is_detached_arraybuffer(env,backing,&detached)!=napi_ok||detached||napi_get_arraybuffer_info(env,backing,&base,&bytes)!=napi_ok||base!=pointer||bytes!=N*4)return false;
 std::memcpy(out.data(),pointer,N*4);return true;
}
template<size_t N>grs::m3a::ControlIdentityV4 m3IdentityOf(const std::array<uint32_t,N>& words){
 static_assert(N>=18);auto pair=[&](size_t at){return uint64_t(words[at])|(uint64_t(words[at+1])<<32);};return {pair(0),pair(2),pair(4),pair(6),pair(8),pair(10),pair(12),pair(14),pair(16)};
}
napi_value m3Configure(napi_env e,napi_callback_info info){
 std::array<uint32_t,16> words{};if(!m3Words(e,info,words)||words[0]!=4||words[1]!=15||!grs::m3a::SelectProfile(words[2],words[3])||words[15]>1)return text(e,"M3_CONFIG_REJECTED");
 grs::m3a::PhysicalConfigV4 cold;cold.omegaProvided=true;cold.fullBranchTrace=words[15]==1;cold.omega=words[4];cold.waterRate=words[5];cold.gravity=words[6];cold.settle=words[7];cold.emit=words[8];cold.kAir=words[9];cold.kFlight=words[10];cold.kWet=words[11];cold.soft=words[12];cold.flux=words[13];if(words[14]!=1||!cold.Valid())return text(e,"M3_CONFIG_REQUIRED");
 auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);if(a.m3Controller){a.m3Controller->Stop();a.m3Controller.reset();}a.m3Selected=true;a.m3Config=cold;a.K=words[2];a.Kw=words[3];
#ifdef GRS_APP_M1
 a.requiresReset=false;
#endif
 a.InvalidateLocked();a.wake.notify_one();return text(e,"M3_NEW_EMPTY_RUN_REQUESTED");
}
napi_value m3Session(napi_env e,napi_callback_info){
 auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);if(!a.m3Selected||!a.m3Controller)return text(e,"{\"state\":\"M3_NOT_READY\"}");
 auto id=a.M3IdentityWordsLocked();std::ostringstream out;out<<"{\"revision\":4,\"features\":15,\"identity\":[";for(size_t i=0;i<id.size();++i){if(i)out<<',';out<<id[i];}out<<"],\"sourceControlGeneration\":[1,0]}";return text(e,out.str());
}
napi_value m3Pulse(napi_env e,napi_callback_info info){
 std::array<uint32_t,22> words{};if(!m3Words(e,info,words))return text(e,"M3_PULSE_REJECTED");auto& a=app();std::shared_ptr<grs::m3a::ControllerV4> controller;{std::lock_guard<std::mutex> lock(a.mutex);controller=a.m3Controller;}
 if(!controller)return text(e,"M3_NOT_READY");grs::m3a::PagePulseV3 pulse{m3IdentityOf(words),uint64_t(words[18])|(uint64_t(words[19])<<32),uint64_t(words[20])|(uint64_t(words[21])<<32)};const auto code=controller->Pulse(pulse,now());a.wake.notify_one();return text(e,std::to_string(uint32_t(code)));
}
napi_value m3Target(napi_env e,napi_callback_info info){
 std::array<uint32_t,19> words{};if(!m3Words(e,info,words))return text(e,"M3_TARGET_REJECTED");auto& a=app();std::shared_ptr<grs::m3a::ControllerV4> controller;{std::lock_guard<std::mutex> lock(a.mutex);controller=a.m3Controller;}
 return text(e,controller?std::to_string(uint32_t(controller->Target(m3IdentityOf(words),int32_t(words[18])))):"M3_NOT_READY");
}
napi_value m3Offer(napi_env e,napi_callback_info info){
 std::array<uint32_t,566> words{};if(!m3Words(e,info,words))return text(e,"M3_SOURCE_REJECTED");auto id=m3IdentityOf(words);grs::m3a::SourceIntentKeyV4 key{id.run,id.owner,id.context,id.inputEpoch,id.pageSession,uint64_t(words[18])|(uint64_t(words[19])<<32),uint64_t(words[20])|(uint64_t(words[21])<<32)};grs::m3a::SourcePacketV4 packet;std::copy(words.begin()+22,words.end(),packet.words.begin());
 auto& a=app();std::shared_ptr<grs::m3a::ControllerV4> controller;{std::lock_guard<std::mutex> lock(a.mutex);controller=a.m3Controller;}if(!controller)return text(e,"M3_NOT_READY");auto result=controller->Offer(id,key,packet);a.wake.notify_one();return text(e,std::to_string(uint32_t(result)));
}
napi_value m3Quota(napi_env e,napi_callback_info){
 auto& a=app();std::shared_ptr<grs::m3a::ControllerV4> controller;{std::lock_guard<std::mutex> lock(a.mutex);controller=a.m3Controller;}if(!controller)return text(e,"M3_NOT_READY");auto quota=controller->Quota(now());std::array<char,16384> data{};grs::m3a::cold::Writer writer(data.data(),data.size());if(!grs::m3a::cold::Quota(writer,quota))return text(e,"M3_QUOTA_INVALID");return text(e,std::string(writer.Data(),writer.Size()));
}
template<size_t N>std::shared_ptr<grs::m3a::ControllerV4> m3ColdController(const std::array<uint32_t,N>& words){auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);return m3IdentityOf(words)==a.m3Identity?a.m3Controller:nullptr;}
napi_value m3ColdBegin(napi_env e,napi_callback_info info){std::array<uint32_t,20> words{};if(!m3Words(e,info,words))return text(e,"M3_COLD_SCHEMA_REJECTED");auto controller=m3ColdController(words);if(!controller)return text(e,"M3_STALE_SCOPE");auto result=controller->ColdBegin(uint64_t(words[18])|(uint64_t(words[19])<<32),now());app().wake.notify_one();return text(e,result);}
napi_value m3ColdState(napi_env e,napi_callback_info info){std::array<uint32_t,20> words{};if(!m3Words(e,info,words))return text(e,"M3_COLD_SCHEMA_REJECTED");auto controller=m3ColdController(words);return text(e,controller?controller->ColdState(uint64_t(words[18])|(uint64_t(words[19])<<32),now()):"M3_STALE_SCOPE");}
napi_value m3ColdPage(napi_env e,napi_callback_info info){std::array<uint32_t,22> words{};if(!m3Words(e,info,words)||words[21])return text(e,"M3_COLD_SCHEMA_REJECTED");auto controller=m3ColdController(words);return text(e,controller?controller->ColdPage(uint64_t(words[18])|(uint64_t(words[19])<<32),words[20],now()):"M3_STALE_SCOPE");}
napi_value m3ColdEnd(napi_env e,napi_callback_info info){std::array<uint32_t,20> words{};if(!m3Words(e,info,words))return text(e,"M3_COLD_SCHEMA_REJECTED");auto controller=m3ColdController(words);return text(e,controller?controller->ColdEnd(uint64_t(words[18])|(uint64_t(words[19])<<32),now()):"M3_STALE_SCOPE");}
napi_value m3ColdCapabilities(napi_env e,napi_callback_info){auto& a=app();std::shared_ptr<grs::m3a::ControllerV4> controller;{std::lock_guard<std::mutex> lock(a.mutex);controller=a.m3Controller;}return text(e,controller?controller->ColdCapabilities():"M3_NOT_READY");}
napi_value m3SourceStats(napi_env e,napi_callback_info){auto& a=app();std::shared_ptr<grs::m3a::ControllerV4> controller;{std::lock_guard<std::mutex> lock(a.mutex);controller=a.m3Controller;}return text(e,controller?controller->SourceStats():"M3_NOT_READY");}
