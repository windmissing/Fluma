#pragma once
#include "control.h"
namespace grs {
enum class TraceKind:uint32_t {Dispatch,Poll,Completion};
// CPU wall durations only. GPU timer data is unavailable, never a measured zero.
struct TransportTrace {
 Identity identity;RootRequest root;TraceKind kind=TraceKind::Dispatch;
 uint32_t ordinal=0,stage=0,groups=0,barrierBits=0,rawGl=0,result=0;
 uint64_t beginNs=0,endNs=0,stateBytes=0,workspaceBytes=0,outputBytes=0,mappedBytes=0;
 bool gpuTimerAvailable=false;
};
class TransportTraceRing {
 public:
 static constexpr size_t capacity=256;
 void Push(const TransportTrace& row){if(count==capacity){++dropped;return;}rows[count++]=row;}
 void Clear(){count=0;dropped=0;}
 size_t Size()const{return count;}uint64_t Dropped()const{return dropped;}
 bool Complete()const{return dropped==0;}
 const TransportTrace& At(size_t i)const{return rows.at(i);}
 private:std::array<TransportTrace,capacity> rows{};size_t count=0;uint64_t dropped=0;
};
}
