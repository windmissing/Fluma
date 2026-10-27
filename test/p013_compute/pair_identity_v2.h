// T159: exact T138 canonical order, explicit encoding revision. Wire remains V1.
#pragma once
#include "pair_prepare_v1.h"
namespace p011gpu {
constexpr const char* kIdentityEncodingV2="T138_CANONICAL_IDENTITY_LE_V2";
inline bool decodeSha256(const std::string& text,std::array<uint8_t,32>& bytes){
 if(text.size()!=64)return false;auto digit=[](char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;};
 for(size_t i=0;i<32;i++){int a=digit(text[i*2]),b=digit(text[i*2+1]);if(a<0||b<0)return false;bytes[i]=uint8_t(a*16+b);}return true;
}
inline bool canonicalIdentityV2(const Header& header,const RequestIdentity& id,double radius,const std::array<uint8_t,32>& snapshot,const std::array<uint8_t,32>& payload,std::vector<uint8_t>& out){
 if(id.runId.size()>1024||id.stateId.size()>1024||id.configId.size()>8192)return false;
 std::array<uint8_t,32> build{};if(!decodeSha256(id.buildManifestSha,build))return false;
 out.clear();auto u32=[&](uint32_t v){for(int i=0;i<4;i++)out.push_back(uint8_t(v>>(8*i)));};
 auto u64=[&](uint64_t v){u32(uint32_t(v));u32(uint32_t(v>>32));};
 auto text=[&](const std::string& s){u32(uint32_t(s.size()));out.insert(out.end(),s.begin(),s.end());};
 for(const char* revision:{"GPU_SEGMENTED_ID_PAIR_V1","CPU_RN64_MERGE_RANGE_V1","P011_GPU_PAIR_WIRE_V1","P011_GPU_PAIR_PROTOTYPE_V1"})text(revision);
 text(id.runId);text(id.stateId);text(id.configId);u64(bits64(id.paramsRevision));u64(bits64(id.inputRevision));u64(bits64(id.seed));
 u64(id.ownerGeneration);u64(id.contextEpoch);u64(id.payloadSerial);u64(id.poolMutationSerial);u64(id.attemptSerial);u64(id.substepSerial);
 u32(id.candidateRole);u32(id.candidateOrdinal);u32(id.slotIndex);
 for(uint32_t i:{8u,9u,10u,11u,27u,7u})u32(header.w[i]);for(uint32_t i=28;i<32;i++)u32(header.w[i]);u64(bits64(radius));
 out.insert(out.end(),snapshot.begin(),snapshot.end());out.insert(out.end(),payload.begin(),payload.end());out.insert(out.end(),build.begin(),build.end());return true;
}
}
