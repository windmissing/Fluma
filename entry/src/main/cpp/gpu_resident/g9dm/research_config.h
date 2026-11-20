#pragma once
#include "schema_generated.h"
#include "dm_contract_generated.h"
#include "control_types.h"
#include "../program_cache/sha256.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>
namespace grs::g9dm {
using Input128=std::array<uint32_t,128>;
using Completion256=std::array<uint32_t,256>;
inline uint64_t Pair(const uint32_t* words,size_t at){return uint64_t(words[at])|(uint64_t(words[at+1])<<32);}
inline void Put(uint32_t* words,size_t at,uint64_t value){words[at]=uint32_t(value);words[at+1]=uint32_t(value>>32);}
inline std::array<uint32_t,8> HashWords(const uint32_t* words,size_t count){
 p011gpu::Sha256 sha;for(size_t i=0;i<count;++i){uint32_t v=words[i];const uint8_t b[]={uint8_t(v),uint8_t(v>>8),uint8_t(v>>16),uint8_t(v>>24)};sha.update(b,4);}
 auto digest=sha.finish();std::array<uint32_t,8> result{};for(size_t i=0;i<8;++i)result[i]=uint32_t(digest[4*i])|(uint32_t(digest[4*i+1])<<8)|(uint32_t(digest[4*i+2])<<16)|(uint32_t(digest[4*i+3])<<24);return result;
}
struct Config {
 uint32_t k=0,g=0,muStart=0,muStop=0,band=0,entrainRate=0,shear=0,settleTicks=0,revision=0;
 uint32_t depthGravityGainQ16=0,depthShearGainQ16=0,regularizationDepthQ=0,omegaBaseQ16=0,omegaGravityGainQ16=0,omegaShearGainQ16=0,omegaMaxQ16=0,mobilizationPolicy=0;
 bool research=false,synthetic=false,fullTrialTrace=false;
 std::array<uint32_t,64> capsule64{};std::array<uint32_t,8> parameterSha{},seedSha{};
 bool Valid()const{return research&&synthetic&&(capsule64==kConfig192||capsule64==kConfig512)&&k==capsule64[3]&&g==capsule64[4]&&muStart==capsule64[5]&&muStop==capsule64[6]&&band==capsule64[7]&&entrainRate==capsule64[8]&&shear==capsule64[9]&&settleTicks==capsule64[10]&&revision==capsule64[11]&&depthGravityGainQ16==capsule64[48]&&depthShearGainQ16==capsule64[49]&&regularizationDepthQ==capsule64[50]&&omegaBaseQ16==capsule64[51]&&omegaGravityGainQ16==capsule64[52]&&omegaShearGainQ16==capsule64[53]&&omegaMaxQ16==capsule64[54]&&mobilizationPolicy==capsule64[55]&&parameterSha==HashWords(capsule64.data(),64);}
};
inline std::array<uint32_t,8> ParameterHash(const Config& c){return HashWords(c.capsule64.data(),64);}
inline bool DecodeConfig(const std::array<uint32_t,64>& raw,Config& c){if(raw!=kConfig192&&raw!=kConfig512)return false;c={};c.capsule64=raw;c.k=raw[3];c.g=raw[4];c.muStart=raw[5];c.muStop=raw[6];c.band=raw[7];c.entrainRate=raw[8];c.shear=raw[9];c.settleTicks=raw[10];c.revision=raw[11];c.depthGravityGainQ16=raw[48];c.depthShearGainQ16=raw[49];c.regularizationDepthQ=raw[50];c.omegaBaseQ16=raw[51];c.omegaGravityGainQ16=raw[52];c.omegaShearGainQ16=raw[53];c.omegaMaxQ16=raw[54];c.mobilizationPolicy=raw[55];c.research=c.synthetic=true;c.parameterSha=ParameterHash(c);return c.Valid();}
struct SeedFileIdentity {std::array<uint32_t,8> sha{};std::vector<uint32_t> state;};
struct Permit {
 ControlIdentity identity;uint64_t nonce=0,plan=0,frame=0,control=0,geometry=0,deadline=0,baseCommit=0,quanta=0,arenaGeneration=0;
 uint64_t rootBeginNs=0,clockGeneration=0,lifecycleGeneration=0;
 uint32_t kind=0,ordinal=0,baseBank=0,candidateBank=1,arena=0;
 std::array<uint32_t,8> parameterSha{},layoutSha{},shaderSha{},inputSha{};
 bool operator==(const Permit& b)const{return identity==b.identity&&nonce==b.nonce&&plan==b.plan&&frame==b.frame&&control==b.control&&geometry==b.geometry&&deadline==b.deadline&&baseCommit==b.baseCommit&&quanta==b.quanta&&arenaGeneration==b.arenaGeneration&&rootBeginNs==b.rootBeginNs&&kind==b.kind&&ordinal==b.ordinal&&baseBank==b.baseBank&&candidateBank==b.candidateBank&&arena==b.arena&&parameterSha==b.parameterSha&&layoutSha==b.layoutSha&&shaderSha==b.shaderSha&&inputSha==b.inputSha;}
};
enum class Result {OK,IN_PROGRESS,WOULD_BLOCK,INVALID,UNSUPPORTED,STALE,DEADLINE,GL_FAILED,SHADER_FAILED,CONTEXT_LOST,INPUT_CLOCK_UNVERIFIED,CALIBRATION_CONFIG_REQUIRED,BOOT_AUDIT_OK=0x666};
}
