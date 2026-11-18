#pragma once
#include "touch_adapter.h"
namespace grs::g9spf::candidate548 {
inline void Bits(uint32_t* out,size_t at,double value){uint64_t bits=0;std::memcpy(&bits,&value,8);Put(out,at,bits);}
inline Input128 TouchBody(const Stroke& s){
 Input128 input{};input[40]=2;Put(input.data(),41,s.sequence);Put(input.data(),43,s.up.sample);input[45]=s.impulse.flags;
 Bits(input.data(),46,s.upX);Bits(input.data(),48,4.0);volatile double dx=s.upX-s.downX,dy=s.upY-s.downY;Bits(input.data(),50,dx);Bits(input.data(),52,dy);Put(input.data(),54,s.up.sample-s.down.sample);Put(input.data(),56,uint64_t(s.impulse.jx));Put(input.data(),58,s.impulse.jy);for(size_t i=0;i<4;++i)input[60+i]=uint32_t(s.impulse.cap>>(32*i));input[77]=53401;return input;
}
inline std::array<uint32_t,8> StableBodySha(const Input128& input){
 std::array<uint32_t,100> words{};const size_t envelope[]={4,5,12,13,26,27,28,29,30,31,32,33};for(size_t i=0;i<12;++i)words[i]=input[envelope[i]];std::copy(input.begin()+40,input.end(),words.begin()+12);return HashWords(words.data(),words.size());
}
inline bool TouchRecord(const Stroke& s,const Input128& input,const Config& config,TouchEvidence256& record){
 if(!s.down.raw.complete||!s.up.raw.complete||!s.proofGeneration||!s.mapping.current||s.mapping.parameter!=config.parameterSha||input[40]!=2||input[77]!=53401||Pair(input.data(),41)!=s.sequence||Pair(input.data(),43)!=s.up.sample)return false;
 record={};record.kind=1;auto* w=record.words.data();w[0]=844380487u;w[1]=1;w[2]=256;w[3]=3235776770u;Put(w,4,s.sequence);w[6]=uint32_t(s.down.pointer);w[7]=1;
 const auto& id=s.mapping.scope;const uint64_t scope[]={id.run,id.owner,id.context,id.inputEpoch,id.pageSession,id.permission,id.window,id.config,id.surface};for(size_t i=0;i<9;++i)Put(w,8+2*i,scope[i]);Put(w,26,s.sequence);Put(w,28,s.down.sample);Put(w,30,s.up.sample);Put(w,32,s.down.receipt);Put(w,34,s.up.receipt);Put(w,38,s.proofGeneration);
 const auto& down=s.down.raw;const auto& up=s.up.raw;
 const double raw[]={down.localX,down.localY,up.localX,up.localY,down.windowX,down.windowY,up.windowX,up.windowY,down.displayX,down.displayY,up.displayX,up.displayY,down.scale,down.originX,down.originY,s.mapping.left,s.mapping.top,s.mapping.width,s.mapping.height,s.downX,s.downY,s.upX,s.upY};for(size_t i=0;i<23;++i)Bits(w,40+2*i,raw[i]);std::copy(input.begin()+50,input.begin()+54,w+86);Put(w,90,s.up.sample-s.down.sample);Put(w,92,s.down.sample);Put(w,94,s.up.sample);
 w[96]=1;w[97]=1;w[98]=1;w[99]=53401;w[100]=w[101]=uint32_t(s.mapping.angle);w[102]=1;w[103]=s.moves+2;w[104]=s.moves;w[106]=input[45]&3u;w[107]=1;Put(w,108,s.mapping.generation);Put(w,110,s.mapping.commit);Put(w,112,s.mapping.geometry);Put(w,114,s.mapping.viewport);w[116]=uint32_t(s.mapping.width);w[117]=uint32_t(s.mapping.height);
 std::copy(config.parameterSha.begin(),config.parameterSha.end(),w+118);std::copy(config.capsule64.begin()+24,config.capsule64.begin()+32,w+126);std::copy(config.capsule64.begin()+32,config.capsule64.begin()+40,w+134);std::copy(s.mapping.seed.begin(),s.mapping.seed.end(),w+142);const auto stable=StableBodySha(input);std::copy(stable.begin(),stable.end(),w+150);std::copy(s.mapping.tokenSha.begin(),s.mapping.tokenSha.end(),w+158);std::copy(s.proofSha.begin(),s.proofSha.end(),w+166);const auto coordinate=HashWords(w+40,46);std::copy(coordinate.begin(),coordinate.end(),w+174);std::copy(s.platformSha.begin(),s.platformSha.end(),w+190);
 w[198]=1;w[199]=1;w[200]=1;w[201]=2;Put(w,202,s.mapping.generation);Put(w,204,s.mapping.commit);std::copy(s.mapping.tokenSha.begin(),s.mapping.tokenSha.end(),w+208);std::copy(input.begin()+56,input.begin()+64,w+216);Bits(w,232,double(s.down.sample));Bits(w,234,double(s.up.sample));Put(w,236,s.up.receipt);const auto checked=ClockProducer::Now();if(!checked||checked<s.up.receipt)return false;Put(w,238,checked);std::copy(s.moveSha.begin(),s.moveSha.end(),w+240);return true;
}
inline void FreezeTouchRecord(TouchEvidence256& record,const Input128& frozen){
 if(!record.kind)return;Put(record.words.data(),36,Pair(frozen.data(),20));const auto hash=HashWords(frozen.data(),frozen.size());std::copy(hash.begin(),hash.end(),record.words.begin()+182);
}
}
