#pragma once
#include "schema_generated.h"
#include "control_types.h"
#include "../program_cache/sha256.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>
namespace grs::g9spf {
struct Config {
 uint32_t k=0,g=0,muStart=0,muStop=0,band=0,entrainRate=0,shear=0,settleTicks=0,revision=0;
 bool research=false,synthetic=false,fullTrialTrace=false;
 std::array<uint32_t,8> parameterSha{},seedSha{};
 bool Valid()const{
  bool named=false;for(auto v:parameterSha)named|=v!=0;
  return research&&named&&SelectProfile(k)&&g>=120&&g<=1073741824&&muStart<=262144&&muStop<=muStart&&band&&band<=268435456&&entrainRate<=62914560&&shear<=262144&&settleTicks&&revision;
 }
};
struct SeedFileIdentity {std::array<uint32_t,8> sha{};std::vector<uint32_t> state;};
using Input128=std::array<uint32_t,128>;
using Completion256=std::array<uint32_t,256>;
inline uint64_t Pair(const uint32_t* words,size_t at){return uint64_t(words[at])|(uint64_t(words[at+1])<<32);}
inline void Put(uint32_t* words,size_t at,uint64_t value){words[at]=uint32_t(value);words[at+1]=uint32_t(value>>32);}
inline std::array<uint32_t,8> HashWords(const uint32_t* words,size_t count){
 p011gpu::Sha256 sha;for(size_t i=0;i<count;++i){uint32_t v=words[i];const uint8_t b[]={uint8_t(v),uint8_t(v>>8),uint8_t(v>>16),uint8_t(v>>24)};sha.update(b,4);}
 auto digest=sha.finish();std::array<uint32_t,8> result{};for(size_t i=0;i<8;++i)result[i]=uint32_t(digest[4*i])|(uint32_t(digest[4*i+1])<<8)|(uint32_t(digest[4*i+2])<<16)|(uint32_t(digest[4*i+3])<<24);return result;
}
inline std::array<uint32_t,8> ParameterHash(const Config& c){const uint32_t words[]={kProfileCode,c.k,c.g,c.muStart,c.muStop,c.band,c.entrainRate,c.shear,c.settleTicks,c.revision,c.research?1u:0u,c.synthetic?1u:0u};return HashWords(words,sizeof(words)/4);}
struct Permit {
 ControlIdentity identity;uint64_t nonce=0,plan=0,frame=0,control=0,geometry=0,deadline=0,baseCommit=0,quanta=0,arenaGeneration=0;
 uint32_t kind=0,ordinal=0,baseBank=0,candidateBank=1,arena=0;
 std::array<uint32_t,8> parameterSha{},layoutSha{},shaderSha{},inputSha{};
 bool operator==(const Permit& b)const{return identity==b.identity&&nonce==b.nonce&&plan==b.plan&&frame==b.frame&&control==b.control&&geometry==b.geometry&&deadline==b.deadline&&baseCommit==b.baseCommit&&quanta==b.quanta&&arenaGeneration==b.arenaGeneration&&kind==b.kind&&ordinal==b.ordinal&&baseBank==b.baseBank&&candidateBank==b.candidateBank&&arena==b.arena&&parameterSha==b.parameterSha&&layoutSha==b.layoutSha&&shaderSha==b.shaderSha&&inputSha==b.inputSha;}
};
enum class Result {OK,IN_PROGRESS,WOULD_BLOCK,INVALID,UNSUPPORTED,STALE,DEADLINE,GL_FAILED,SHADER_FAILED,CONTEXT_LOST,INPUT_CLOCK_UNVERIFIED,CALIBRATION_CONFIG_REQUIRED};
}
