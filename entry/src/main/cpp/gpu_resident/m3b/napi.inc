// T550 (problem T546 §2): m5Words delegates over the shared strict decode
// contract with Route::M3B — same ordered observation as M3A/G9.
template<size_t N>bool m5Words(napi_env env,napi_callback_info info,std::array<uint32_t,N>& out){
 auto result=grs::words::ReadWords<N>(env,info,grs::words::Route::M3B,0);
 if(result.ok)out=result.words;
 return result.ok;
}
template<size_t N>grs::m3b::ControlIdentityV5 m5IdentityOf(const std::array<uint32_t,N>& words){
 static_assert(N>=18);auto pair=[&](size_t at){return uint64_t(words[at])|(uint64_t(words[at+1])<<32);};return {pair(0),pair(2),pair(4),pair(6),pair(8),pair(10),pair(12),pair(14),pair(16)};
}

std::array<uint32_t,8> m5ConfigDigest(const std::array<uint32_t,32>& words){
 p011gpu::Sha256 hash;const char tag[]="T517_LIFT_RESEARCH_CONFIG_V1";hash.update(reinterpret_cast<const uint8_t*>(tag),sizeof(tag)-1);
 for(size_t i=0;i<24;++i){const auto v=words[i];const uint8_t b[]={uint8_t(v),uint8_t(v>>8),uint8_t(v>>16),uint8_t(v>>24)};hash.update(b,4);}
 const auto digest=hash.finish();std::array<uint32_t,8> result{};for(size_t i=0;i<8;++i)result[i]=uint32_t(digest[4*i])|(uint32_t(digest[4*i+1])<<8)|(uint32_t(digest[4*i+2])<<16)|(uint32_t(digest[4*i+3])<<24);return result;
}
bool m5ConfigDigestValid(const std::array<uint32_t,32>& words){const auto digest=m5ConfigDigest(words);for(size_t i=0;i<8;++i)if(words[24+i]!=digest[i])return false;return true;}
napi_value m5ResearchConfig(napi_env e,napi_callback_info info){
 std::array<uint32_t,2> request{};if(!m5Words(e,info,request)||!grs::words::Semantic(request[0]<=2,"LIFT_MODE_INVALID",7)||!grs::words::Semantic(request[1]==0,"RESERVED_NONZERO",8))return text(e,"RESEARCH_CONFIG_REJECTED");
 // Explicit candidate 1, not a product default or calibrated sensor law.
 std::array<uint32_t,32> words={5,31,192,64,7680,8388608,1048576,65536,1048576,0,0,120,524288,1048576,1,0,request[0],65536,1048576,65536,65536,1048576,1048576,1};
 const auto digest=m5ConfigDigest(words);for(size_t i=0;i<8;++i)words[24+i]=digest[i];
 std::array<char,2048> data{};grs::cold503::Writer writer(data.data(),data.size());writer.Raw("{\"revision\":\"T517_EXPLICIT_RESEARCH_CANDIDATE_1\",\"productionDefault\":false,\"words\":[");for(size_t i=0;i<words.size();++i){if(i)writer.Raw(",");writer.Number(words[i]);}writer.Raw("]}");return text(e,std::string(writer.Data(),writer.Size()));
}
napi_value m5InputClock(napi_env e,napi_callback_info){const auto stamp=now();return text(e,"{\"source\":\"SYNTHETIC\",\"clockDomain\":2,\"words\":["+std::to_string(uint32_t(stamp))+","+std::to_string(uint32_t(stamp>>32))+"]}");}
napi_value m5OfferInput64(napi_env e,napi_callback_info info){
 grs::m3b::LiftInput64 words{};if(!m5Words(e,info,words))return text(e,"M5_INPUT64_REJECTED");auto& a=app();std::shared_ptr<grs::m3b::ControllerV5> controller;{std::lock_guard<std::mutex> lock(a.mutex);controller=a.m5Controller;}
 if(!controller)return text(e,"M5_NOT_READY");const auto result=controller->OfferLiftInput(words,now());a.wake.notify_one();return text(e,std::to_string(uint32_t(result)));
}
napi_value queryLiftClockCapability(napi_env e,napi_callback_info){return text(e,"{\"revision\":\"T512_LIFT_CLOCK_V1\",\"sensorClockVerified\":false,\"sensorState\":\"SENSOR_CLOCK_UNVERIFIED\",\"syntheticClockDomain\":2,\"syntheticSourceKind\":2}");}
napi_value verifyLiftClockEpoch(napi_env e,napi_callback_info){return text(e,"{\"state\":\"SENSOR_CLOCK_UNVERIFIED\",\"reason\":\"DEVICE_SDK_CLOCK_SEMANTICS_CERTIFICATE_REQUIRED\"}");}
napi_value m5ResumeInput(napi_env e,napi_callback_info){
 auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);if(!a.m5Controller||a.m5Identity.inputEpoch==UINT64_MAX||a.m5Identity.pageSession==UINT64_MAX)return text(e,"M5_NOT_READY");
 auto id=a.m5Identity;++id.inputEpoch;++id.pageSession;const auto result=a.m5Controller->ResumeLiftSession(id);if(result==grs::m3b::ControlResult::OK){a.m5Identity=id;a.m5Generation=std::max(a.m5Generation,id.pageSession);}a.wake.notify_one();return text(e,std::to_string(uint32_t(result)));
}

napi_value m5Configure(napi_env e,napi_callback_info info){
 std::array<uint32_t,32> words{};if(!m5Words(e,info,words)||!grs::words::Semantic(words[0]==5,"REVISION_MISMATCH",0)||!grs::words::Semantic(words[1]==31,"FEATURES_MISMATCH",1)||!grs::words::Semantic(grs::m3b::SelectProfile(words[2],words[3])!=nullptr,"PROFILE_UNSUPPORTED",2)||!grs::words::Semantic(words[15]<=1,"FULL_FLAG_INVALID",3))return text(e,"M5_CONFIG_REJECTED");
 if(!grs::words::Semantic(words[15]==0,"PREFIX_DIGEST_POLICY_REQUIRED",3))return text(e,"PREFIX_DIGEST_POLICY_REQUIRED");
 if(!grs::words::Semantic(m5ConfigDigestValid(words),"PARAMETER_SHA_MISMATCH",5))return text(e,"M5_PARAMETER_SHA_REJECTED");
 grs::m3b::PhysicalConfigV5 cold;cold.lift.mode=grs::m3b::LiftMode(words[16]);cold.lift.liftRateQ16=words[17];cold.lift.bandQ=words[18];cold.lift.rampQ16=words[19];cold.lift.decayQ16=words[20];cold.lift.liftSpeedQ=words[21];cold.lift.liftCapQ=words[22];cold.lift.revision=words[23];for(size_t i=0;i<8;++i)cold.lift.sha[i]=words[24+i];cold.omegaProvided=true;cold.fullBranchTrace=words[15]==1;cold.omega=words[4];cold.waterRate=words[5];cold.gravity=words[6];cold.settle=words[7];cold.emit=words[8];cold.kAir=words[9];cold.kFlight=words[10];cold.kWet=words[11];cold.soft=words[12];cold.flux=words[13];if(!grs::words::Semantic(words[14]==1,"COLD_REQUIRED",4)||!grs::words::Semantic(cold.Valid(),"COLD_PARAMETERS_INVALID",4))return text(e,"M5_CONFIG_REQUIRED");
 auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);if(a.m5Controller){a.m5Controller->Stop();a.m5Controller.reset();}if(a.m3Controller)a.m3Controller->Stop();a.m3Selected=false;a.m5Selected=true;a.m5Config=cold;a.K=words[2];a.Kw=words[3];
#ifdef GRS_APP_M1
 a.requiresReset=false;
#endif
 a.InvalidateLocked();a.wake.notify_one();return text(e,"M5_NEW_EMPTY_RUN_REQUESTED");
}
napi_value m5Session(napi_env e,napi_callback_info){
 auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);if(!a.m5Selected||!a.m5Controller)return text(e,"{\"state\":\"M5_NOT_READY\"}");
 auto id=a.M5IdentityWordsLocked();std::ostringstream out;out<<"{\"revision\":5,\"features\":31,\"identity\":[";for(size_t i=0;i<id.size();++i){if(i)out<<',';out<<id[i];}const auto generation=a.m5Controller->SourceGeneration();out<<"],\"sourceControlGeneration\":["<<uint32_t(generation)<<","<<uint32_t(generation>>32)<<"]}";return text(e,out.str());
}
napi_value m5Pulse(napi_env e,napi_callback_info info){
 std::array<uint32_t,22> words{};if(!m5Words(e,info,words))return text(e,"M5_PULSE_REJECTED");auto& a=app();std::shared_ptr<grs::m3b::ControllerV5> controller;{std::lock_guard<std::mutex> lock(a.mutex);controller=a.m5Controller;}
 if(!controller)return text(e,"M5_NOT_READY");grs::m3b::PagePulseV3 pulse{m5IdentityOf(words),uint64_t(words[18])|(uint64_t(words[19])<<32),uint64_t(words[20])|(uint64_t(words[21])<<32)};const auto code=controller->Pulse(pulse,now());a.wake.notify_one();return text(e,std::to_string(uint32_t(code)));
}
napi_value m5Target(napi_env e,napi_callback_info info){
 std::array<uint32_t,19> words{};if(!m5Words(e,info,words))return text(e,"M5_TARGET_REJECTED");auto& a=app();std::shared_ptr<grs::m3b::ControllerV5> controller;{std::lock_guard<std::mutex> lock(a.mutex);controller=a.m5Controller;}
 return text(e,controller?std::to_string(uint32_t(controller->Target(m5IdentityOf(words),int32_t(words[18])))):"M5_NOT_READY");
}
napi_value m5Offer(napi_env e,napi_callback_info info){
 std::array<uint32_t,566> words{};if(!m5Words(e,info,words))return text(e,"M5_SOURCE_REJECTED");auto id=m5IdentityOf(words);grs::m3b::SourceIntentKeyV5 key{id.run,id.owner,id.context,id.inputEpoch,id.pageSession,uint64_t(words[18])|(uint64_t(words[19])<<32),uint64_t(words[20])|(uint64_t(words[21])<<32)};grs::m3b::SourcePacketV5 packet;std::copy(words.begin()+22,words.end(),packet.words.begin());
 auto& a=app();std::shared_ptr<grs::m3b::ControllerV5> controller;{std::lock_guard<std::mutex> lock(a.mutex);controller=a.m5Controller;}if(!controller)return text(e,"M5_NOT_READY");auto result=controller->Offer(id,key,packet);a.wake.notify_one();return text(e,std::to_string(uint32_t(result)));
}
napi_value m5Quota(napi_env e,napi_callback_info){
 auto& a=app();std::shared_ptr<grs::m3b::ControllerV5> controller;{std::lock_guard<std::mutex> lock(a.mutex);controller=a.m5Controller;}if(!controller)return text(e,"M5_NOT_READY");auto quota=controller->Quota(now());std::array<char,16384> data{};grs::m3b::cold::Writer writer(data.data(),data.size());if(!grs::m3b::cold::Quota(writer,quota))return text(e,"M5_QUOTA_INVALID");return text(e,std::string(writer.Data(),writer.Size()));
}
template<size_t N>std::shared_ptr<grs::m3b::ControllerV5> m5ColdController(const std::array<uint32_t,N>& words){auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);return m5IdentityOf(words)==a.m5Identity?a.m5Controller:nullptr;}
napi_value m5ColdBegin(napi_env e,napi_callback_info info){std::array<uint32_t,20> words{};if(!m5Words(e,info,words))return text(e,"M5_COLD_SCHEMA_REJECTED");auto controller=m5ColdController(words);if(!controller)return text(e,"M5_STALE_SCOPE");auto result=controller->ColdBegin(uint64_t(words[18])|(uint64_t(words[19])<<32),now());app().wake.notify_one();return text(e,result);}
napi_value m5ColdState(napi_env e,napi_callback_info info){std::array<uint32_t,20> words{};if(!m5Words(e,info,words))return text(e,"M5_COLD_SCHEMA_REJECTED");auto controller=m5ColdController(words);return text(e,controller?controller->ColdState(uint64_t(words[18])|(uint64_t(words[19])<<32),now()):"M5_STALE_SCOPE");}
napi_value m5ColdPage(napi_env e,napi_callback_info info){std::array<uint32_t,22> words{};if(!m5Words(e,info,words)||words[21])return text(e,"M5_COLD_SCHEMA_REJECTED");auto controller=m5ColdController(words);return text(e,controller?controller->ColdPage(uint64_t(words[18])|(uint64_t(words[19])<<32),words[20],now()):"M5_STALE_SCOPE");}
napi_value m5ColdEnd(napi_env e,napi_callback_info info){std::array<uint32_t,20> words{};if(!m5Words(e,info,words))return text(e,"M5_COLD_SCHEMA_REJECTED");auto controller=m5ColdController(words);return text(e,controller?controller->ColdEnd(uint64_t(words[18])|(uint64_t(words[19])<<32),now()):"M5_STALE_SCOPE");}
napi_value m5ColdCapabilities(napi_env e,napi_callback_info){auto& a=app();std::shared_ptr<grs::m3b::ControllerV5> controller;{std::lock_guard<std::mutex> lock(a.mutex);controller=a.m5Controller;}return text(e,controller?controller->ColdCapabilities():"M5_NOT_READY");}
napi_value m5SourceStats(napi_env e,napi_callback_info){auto& a=app();std::shared_ptr<grs::m3b::ControllerV5> controller;{std::lock_guard<std::mutex> lock(a.mutex);controller=a.m5Controller;}return text(e,controller?controller->SourceStats():"M5_NOT_READY");}
