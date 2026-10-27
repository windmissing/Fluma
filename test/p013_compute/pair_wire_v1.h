#pragma once
#include <array>
#include <cstdint>
#include <cstring>

namespace p011gpu {
constexpr uint32_t kHeaderWords=64,kResultWords=32,kLeafWords=4,kQueryWords=8,kCandidateWords=8;
constexpr uint32_t kHeaderMagic=0x47505031u,kResultMagic=0x47505231u,kDone=0x444f4e45u;
constexpr uint32_t kStatusNotReady=0,kStatusPair=1,kStatusNoPair=2;
struct Header {std::array<uint32_t,kHeaderWords> w{};};
struct Result {std::array<uint32_t,kResultWords> w{};};
struct Leaf {uint32_t id=0,slot=0,valid=0,rank=0;};
struct Query {uint32_t id=0,slot=0,aRank=0,L=0,R=0,G0=0,G1=0,valid=0;};
struct Candidate {uint32_t valid=0,idA=0,idB=0,slotA=0,slotB=0,aRank=0,bRank=0,reserved=0;};
static_assert(sizeof(Header)==256&&sizeof(Result)==128&&sizeof(Leaf)==16&&sizeof(Query)==32&&sizeof(Candidate)==32);
inline void split64(uint64_t x,uint32_t& lo,uint32_t& hi){lo=uint32_t(x);hi=uint32_t(x>>32);}
inline uint64_t join64(uint32_t lo,uint32_t hi){return uint64_t(lo)|(uint64_t(hi)<<32);}
inline bool littleEndian(){uint32_t x=1;return *reinterpret_cast<uint8_t*>(&x)==1;}
inline uint64_t bits64(double x){uint64_t b;std::memcpy(&b,&x,8);return b;}
// T157 named scalar word offsets from T138; CPU_EMPTY uses the same V1 envelope.
constexpr uint32_t kHeaderTokenWord=28,kHeaderContextWord=14,kHeaderPayloadWord=16,kHeaderMutationWord=18,kHeaderIdentityShaWord=48;
constexpr uint32_t kResultTokenWord=12,kResultContextWord=16,kResultPayloadWord=18,kResultMutationWord=20,kResultIdentityShaWord=22,kResultDoneWord=30;
inline Result cpuEmptyResult(const Header& header){
 Result r;r.w[0]=kResultMagic;r.w[1]=1;r.w[2]=kStatusNoPair;
 for(uint32_t i=0;i<4;i++)r.w[kResultTokenWord+i]=header.w[kHeaderTokenWord+i];
 for(uint32_t i=0;i<2;i++){r.w[kResultContextWord+i]=header.w[kHeaderContextWord+i];r.w[kResultPayloadWord+i]=header.w[kHeaderPayloadWord+i];r.w[kResultMutationWord+i]=header.w[kHeaderMutationWord+i];}
 for(uint32_t i=0;i<8;i++)r.w[kResultIdentityShaWord+i]=header.w[kHeaderIdentityShaWord+i];r.w[kResultDoneWord]=kDone;return r;
}

}
