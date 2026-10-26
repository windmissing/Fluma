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
}
