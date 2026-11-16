#include "completion_gate.h"
namespace grs::g9spf {
bool CompletionGate::Common(const Profile& p,const Permit& permit,const Completion256& w,const NativeEvidence& evidence){
 if(!permit.nonce||!permit.plan||permit.kind<1||permit.kind>2||permit.baseBank>=5||permit.candidateBank>=5||permit.baseBank==permit.candidateBank||permit.arena>1||!permit.arenaGeneration)return false;
 const uint32_t count=permit.kind==1?12:2024;
 if(w[0]!=kCompletionMagic||w[1]!=1||w[2]!=256||w[3]!=kSeal||w[4]!=kProfileCode||w[5]!=permit.kind||w[39]!=count||w[40]!=count||w[41]!=evidence.barriers||evidence.dispatches!=count||evidence.gl||evidence.egl||!evidence.fenceNs||evidence.fenceNs>permit.deadline||w[42]!=0xffff||w[52]||w[53])return false;
 const uint64_t ids[]={permit.nonce,permit.identity.run,permit.identity.owner,permit.identity.context,permit.identity.surface,permit.geometry,permit.identity.config,permit.identity.inputEpoch,permit.identity.pageSession,permit.plan,permit.baseCommit};
 for(size_t i=0;i<11;++i)if(Pair(w.data(),8+2*i)!=ids[i])return false;
 if(w[32]!=permit.baseBank||w[33]!=permit.candidateBank||w[34]!=permit.arena||Pair(w.data(),35)!=permit.arenaGeneration||w[43]!=w[44]||w[45]!=w[46]||w[44]>p.eventCapacity||w[46]>p.pathCapacity||(w[47]&~63u))return false;
 for(size_t i=0;i<8;++i)if(w[64+i]!=permit.parameterSha[i]||w[72+i]!=permit.layoutSha[i]||w[80+i]!=permit.shaderSha[i]||w[88+i]!=permit.inputSha[i])return false;
 for(size_t i=156;i<w.size();++i)if(w[i])return false;
 if(permit.kind==2){
  if(w[37]&&w[37]!=1&&w[37]!=2&&w[37]!=4&&w[37]!=8&&w[37]!=16&&w[37]!=32)return false;
  for(uint32_t i=0;i<6;++i){const uint32_t at=120+6*i,j=1u<<i;if(w[47]&(1u<<i)){if(w[at]!=j||(w[at+1]!=1&&w[at+1]!=4))return false;}else for(uint32_t f=0;f<6;++f)if(w[at+f])return false;}
 }
 return true;
}
std::optional<ValidatedRecord> CompletionGate::Accepted(const Profile& p,const Permit& permit,const Completion256& w,const NativeEvidence& evidence){
 if(!Common(p,permit,w,evidence)||w[7]||w[48]||w[49]||w[50]||w[51])return {};
 const uint32_t outcome=w[6];if(outcome<1||outcome>3||permit.kind==2&&outcome!=1)return {};
 if(outcome==2){if(permit.kind!=1||Pair(w.data(),30)!=permit.baseCommit||Pair(w.data(),54)!=permit.quanta||w[62]||w[63])return {};}
 else{uint64_t commit;if(!CheckedAdd(permit.baseCommit,1,commit)||Pair(w.data(),30)!=commit)return {};uint64_t quanta;if(!CheckedAdd(permit.quanta,permit.kind==2?1:0,quanta)||Pair(w.data(),54)!=quanta)return {};}
 if(permit.kind==2){if(!w[37]||!w[38]||w[38]>w[37]||w[47]!=(w[37]*2u-1u))return {};uint32_t selected=0;for(uint32_t i=0;i<6;++i)if(w[120+6*i+1]==1){if(selected)return {};selected=1u<<i;}if(selected!=w[37])return {};}
 if(permit.kind==1&&(w[37]||w[38]||w[47]||w[63]||(outcome==1&&(w[62]==0||w[62]>244666))||(outcome==3&&w[62])))return {};
 return ValidatedRecord(permit,w);
}
std::optional<ValidatedBlockedControlRecord> CompletionGate::Blocked(const Profile& p,const Permit& permit,const Completion256& w,const NativeEvidence& evidence){
 if(!Common(p,permit,w,evidence)||permit.kind!=2||w[6]!=4||w[37]||Pair(w.data(),30)!=permit.baseCommit||Pair(w.data(),54)!=permit.quanta||w[47]!=63||w[7]==0||w[7]>18)return {};
 for(uint32_t i=0;i<6;++i)if(w[120+6*i+1]!=4)return {};
 return ValidatedBlockedControlRecord(ValidatedRecord(permit,w));
}
}
