"""Produce the V5 bridge fragments; old endpoints retain their own route."""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[2]
D=ROOT/'entry/src/main/cpp/gpu_resident/m3b'
def bridge(text):
 return re.sub(r'm3(?!b)','m5',text).replace('M3','M5')
for name in ['bridge_fields.inc','bridge_create.inc','bridge_pump.inc','napi.inc']:
 p=D/name;s=bridge(p.read_text(encoding='utf-8'))
 s=s.replace(r'\"wire\":4',r'\"wire\":5').replace(r'\"features\":15',r'\"features\":31').replace(r'\"revision\":4',r'\"revision\":5')
 p.write_text(s,encoding='utf-8')
p=D/'napi.inc';s=p.read_text(encoding='utf-8')
s=s.replace('std::array<uint32_t,16> words{};if(!m5Words(e,info,words)||words[0]!=4||words[1]!=15',
            'std::array<uint32_t,32> words{};if(!m5Words(e,info,words)||words[0]!=5||words[1]!=31')
anchor='grs::m3b::PhysicalConfigV5 cold;cold.omegaProvided=true;'
assert s.count(anchor)==1
s=s.replace(anchor,'''if(words[15])return text(e,"PREFIX_DIGEST_POLICY_REQUIRED");
 if(!m5ConfigDigestValid(words))return text(e,"M5_PARAMETER_SHA_REJECTED");
 grs::m3b::PhysicalConfigV5 cold;cold.lift.mode=grs::m3b::LiftMode(words[16]);cold.lift.liftRateQ16=words[17];cold.lift.bandQ=words[18];cold.lift.rampQ16=words[19];cold.lift.decayQ16=words[20];cold.lift.liftSpeedQ=words[21];cold.lift.liftCapQ=words[22];cold.lift.revision=words[23];for(size_t i=0;i<8;++i)cold.lift.sha[i]=words[24+i];cold.omegaProvided=true;''')
s=s.replace('a.m5Selected=true;a.m5Config=cold;','if(a.m3Controller)a.m3Controller->Stop();a.m3Selected=false;a.m5Selected=true;a.m5Config=cold;')
anchor='napi_value m5Configure('
assert s.count(anchor)==1
helpers='''
std::array<uint32_t,8> m5ConfigDigest(const std::array<uint32_t,32>& words){
 p011gpu::Sha256 hash;const char tag[]="T517_LIFT_RESEARCH_CONFIG_V1";hash.update(reinterpret_cast<const uint8_t*>(tag),sizeof(tag)-1);
 for(size_t i=0;i<24;++i){const auto v=words[i];const uint8_t b[]={uint8_t(v),uint8_t(v>>8),uint8_t(v>>16),uint8_t(v>>24)};hash.update(b,4);}
 const auto digest=hash.finish();std::array<uint32_t,8> result{};for(size_t i=0;i<8;++i)result[i]=uint32_t(digest[4*i])|(uint32_t(digest[4*i+1])<<8)|(uint32_t(digest[4*i+2])<<16)|(uint32_t(digest[4*i+3])<<24);return result;
}
bool m5ConfigDigestValid(const std::array<uint32_t,32>& words){const auto digest=m5ConfigDigest(words);for(size_t i=0;i<8;++i)if(words[24+i]!=digest[i])return false;return true;}
napi_value m5ResearchConfig(napi_env e,napi_callback_info info){
 std::array<uint32_t,2> request{};if(!m5Words(e,info,request)||request[0]>2||request[1])return text(e,"RESEARCH_CONFIG_REJECTED");
 // Explicit candidate 1, not a product default or calibrated sensor law.
 std::array<uint32_t,32> words={5,31,192,64,7680,8388608,1048576,65536,1048576,0,0,120,524288,1048576,1,0,request[0],65536,1048576,65536,65536,1048576,1048576,1};
 const auto digest=m5ConfigDigest(words);for(size_t i=0;i<8;++i)words[24+i]=digest[i];
 std::array<char,2048> data{};grs::cold503::Writer writer(data.data(),data.size());writer.Raw("{\\"revision\\":\\"T517_EXPLICIT_RESEARCH_CANDIDATE_1\\",\\"productionDefault\\":false,\\"words\\":[");for(size_t i=0;i<words.size();++i){if(i)writer.Raw(",");writer.Number(words[i]);}writer.Raw("]}");return text(e,std::string(writer.Data(),writer.Size()));
}
napi_value m5InputClock(napi_env e,napi_callback_info){const auto stamp=now();return text(e,"{\\"source\\":\\"SYNTHETIC\\",\\"clockDomain\\":2,\\"words\\":["+std::to_string(uint32_t(stamp))+","+std::to_string(uint32_t(stamp>>32))+"]}");}
napi_value m5OfferInput64(napi_env e,napi_callback_info info){
 grs::m3b::LiftInput64 words{};if(!m5Words(e,info,words))return text(e,"M5_INPUT64_REJECTED");auto& a=app();std::shared_ptr<grs::m3b::ControllerV5> controller;{std::lock_guard<std::mutex> lock(a.mutex);controller=a.m5Controller;}
 if(!controller)return text(e,"M5_NOT_READY");const auto result=controller->OfferLiftInput(words,now());a.wake.notify_one();return text(e,std::to_string(uint32_t(result)));
}
napi_value queryLiftClockCapability(napi_env e,napi_callback_info){return text(e,"{\\"revision\\":\\"T512_LIFT_CLOCK_V1\\",\\"sensorClockVerified\\":false,\\"sensorState\\":\\"SENSOR_CLOCK_UNVERIFIED\\",\\"syntheticClockDomain\\":2,\\"syntheticSourceKind\\":2}");}
napi_value verifyLiftClockEpoch(napi_env e,napi_callback_info){return text(e,"{\\"state\\":\\"SENSOR_CLOCK_UNVERIFIED\\",\\"reason\\":\\"DEVICE_SDK_CLOCK_SEMANTICS_CERTIFICATE_REQUIRED\\"}");}
napi_value m5ResumeInput(napi_env e,napi_callback_info){
 auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);if(!a.m5Controller||a.m5Identity.inputEpoch==UINT64_MAX||a.m5Identity.pageSession==UINT64_MAX)return text(e,"M5_NOT_READY");
 auto id=a.m5Identity;++id.inputEpoch;++id.pageSession;const auto result=a.m5Controller->ResumeLiftSession(id);if(result==grs::m3b::ControlResult::OK){a.m5Identity=id;a.m5Generation=std::max(a.m5Generation,id.pageSession);}a.wake.notify_one();return text(e,std::to_string(uint32_t(result)));
}
'''
s=s.replace(anchor,helpers+'\n'+anchor)
# Report the actual resumed producer generation, never a hard-coded old key.
s=s.replace('out<<"],\\"sourceControlGeneration\\":[1,0]}";',
            'const auto generation=a.m5Controller->SourceGeneration();out<<"],\\"sourceControlGeneration\\":["<<uint32_t(generation)<<","<<uint32_t(generation>>32)<<"]}";')
p.write_text(s,encoding='utf-8')
PYGENERATED=True
