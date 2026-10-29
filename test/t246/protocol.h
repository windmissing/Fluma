#pragma once
#include <array>
#include <cstdint>
namespace t246 {
struct Identity {uint64_t owner=1,context=0,surface=0,lease=1,token=0;};
struct Tail {std::array<uint32_t,256> words{};};
inline Tail makeTail(const Identity& id){Tail t; t.words[0]=0x54323436;t.words[1]=1;t.words[2]=256;
 const uint64_t values[]={id.owner,id.context,id.surface,id.lease,id.token};for(int i=0;i<5;i++){t.words[4+2*i]=uint32_t(values[i]);t.words[5+2*i]=uint32_t(values[i]>>32);}t.words[254]=0x444f4e45;return t;}
inline bool accepts(const Tail& t,const Identity& current,bool cancelled,bool leased){return !cancelled&&leased&&t.words==makeTail(current).words;}
inline bool rangeValid(uint64_t offset,uint64_t size,uint64_t bytes,uint64_t alignment){return alignment&&offset%alignment==0&&offset<=bytes&&size<=bytes-offset&&size==1024;}
}
