#include "contract.h"
#include "table_generated.h"
#include <algorithm>
#include <limits>
namespace grs::accept647 {
namespace {
bool Add(uint64_t a,uint64_t b,uint64_t& out){if(b>UINT64_MAX-a)return false;out=a+b;return true;}
bool Present(const Digest& d){for(auto w:d)if(w)return true;return false;}
bool Power(uint32_t j){return j==1||j==2||j==4||j==8||j==16||j==32;}
void HashWord(p011gpu::Sha256& h,uint32_t w){const uint8_t b[]={uint8_t(w),uint8_t(w>>8),uint8_t(w>>16),uint8_t(w>>24)};h.update(b,4);}
Digest Finish(p011gpu::Sha256& h){auto bytes=h.finish();Digest out{};for(size_t i=0;i<32;++i)out[i/4]|=uint32_t(bytes[i])<<(8*(i%4));return out;}
bool Composite(Family family){return family==Family::G11_R1||family==Family::G11_DM;}
constexpr Row PcSplitSource(Row base,uint32_t count){base.count=count;return base;}
constexpr Row kPcDmSourceV2=PcSplitSource(kRows[2],14);
constexpr Row kPcR1SourceV2=PcSplitSource(kRows[4],27);
static_assert(kPcDmSourceV2.family==Family::DM_DEFAULT&&kPcR1SourceV2.family==Family::G11_R1);
Digest SourceScheduleHash(const Row& row){
 p011gpu::Sha256 hash;
 const auto base=StaticContractHash();
 for(auto word:base)HashWord(hash,word);
 constexpr char label[]="WIN32_T913_SOURCE_SPLIT_V2";
 hash.update(reinterpret_cast<const uint8_t*>(label),sizeof(label)-1);
 HashWord(hash,uint32_t(row.family));HashWord(hash,row.count);
 return Finish(hash);
}
}
uint64_t Pair(const uint32_t* w,size_t at){return uint64_t(w[at])|(uint64_t(w[at+1])<<32);}
Digest HashWords(const uint32_t* words,size_t n){p011gpu::Sha256 hash;for(size_t i=0;i<n;++i)HashWord(hash,words[i]);return Finish(hash);}
Digest StaticContractHash(){Digest out{};auto digit=[](char c){return uint32_t(c<='9'?c-'0':c-'a'+10);};for(size_t i=0;i<32;++i)out[i/4]|=((digit(kContractSHA[2*i])<<4)|digit(kContractSHA[2*i+1]))<<(8*(i%4));return out;}
const Row* ContractRow(Family family,uint32_t kind){for(const auto& row:kRows)if(row.family==family&&row.kind==kind)return &row;return nullptr;}
std::optional<ExpectedContract> ExpectedForActivePipeline(const ActiveProgramIdentity& active,PermitView permit,uint32_t parentKind){
 if(parentKind!=1&&parentKind!=2)return {};const auto* row=ContractRow(active.family_,permit.Word(3));if(!row||active.family_==Family::R1_DEFAULT||active.family_==Family::G9_RX)return {};
 if((active.family_==Family::G11_R1)!=(parentKind==1))return {};
 if(active.pcSourceScheduleV2_){
  if(row->kind!=1)return {};
  if(active.family_==Family::DM_DEFAULT)row=&kPcDmSourceV2;
  else if(active.family_==Family::G11_R1)row=&kPcR1SourceV2;
  else return {};
 }
 const auto& f=active.facts_;if(!f.producer||!f.program||!f.context||f.owner==std::thread::id{}||(f.k!=192&&f.k!=512)||!f.parameterRevision||!f.stateBytes||!f.workBytes||!f.eventCapacity||!f.pathCapacity)return {};
 if(!Present(f.parameter)||!Present(f.physicalSeed)||!Present(f.physicalLayout)||!Present(f.actualShader)||!Present(f.physicalSpec)||!Present(f.activePipeline))return {};
 if(Composite(active.family_)){if(!Present(f.compositeLayout)||!Present(f.visualConfig)||!Present(f.visualRecipe)||!Present(f.actualVisualSeed))return {};if(f.v!=2048&&f.v!=8192)return {};}else if(f.v||f.baseVisualGeneration||f.baseVisualTicks)return {};
 if(!permit.U64(18)||!permit.U64(20)||!permit.U64(22)||!permit.U64(26)||permit.U64(30)>permit.U64(28)||permit.Word(38)>=5||permit.Word(39)>=5||permit.Word(38)==permit.Word(39)||permit.Word(40)>1||!permit.U64(36))return {};
 for(size_t i=44;i<62;i+=2)if(!permit.U64(i))return {};
 for(size_t i=0;i<8;++i)if(permit.Word(72+i)!=f.parameter[i]||permit.Word(88+i)!=f.physicalLayout[i]||permit.Word(96+i)!=f.actualShader[i])return {};
 uint64_t nextGeneration=0,nextTicks=0;if(Composite(active.family_)){if(!Add(f.baseVisualGeneration,1,nextGeneration)||!Add(f.baseVisualTicks,row->kind==2?256:0,nextTicks))return {};uint64_t quanta=permit.U64(34);if(quanta>UINT64_MAX/256||f.baseVisualTicks!=quanta*256)return {};}
 return ExpectedContract(active,permit,*row,nextGeneration,nextTicks);
}
uint32_t ExpectedContract::Word(size_t at)const{
 const auto& r=*row_;const auto& f=active_->facts_;auto pair=[&](size_t start,uint64_t value){return at==start?uint32_t(value):uint32_t(value>>32);};
 const uint32_t fixed[]={1480938055u,1u,uint32_t(r.family),r.kind,r.magic,r.revision,r.tailProfile,r.physicalProfile,r.seal,256u,r.count,r.mask};if(at<12)return fixed[at];
 if(at>=12&&at<14)return pair(12,f.stateBytes);if(at>=14&&at<16)return pair(14,f.workBytes);if(at==16)return f.eventCapacity;if(at==17)return f.pathCapacity;
 if(at>=18&&at<42||at>=44&&at<62||at>=160&&at<168)return permit_.Word(at);if(at==42)return f.k;if(at==43)return f.v;
 if(at>=62&&at<64)return pair(62,f.baseVisualGeneration);if(at>=64&&at<66)return pair(64,f.baseVisualTicks);if(at>=66&&at<68)return pair(66,nextGeneration_);if(at>=68&&at<70)return pair(68,nextTicks_);
 if(at>=72&&at<80)return f.parameter[at-72];if(at>=80&&at<88)return f.physicalSeed[at-80];if(at>=88&&at<96)return f.physicalLayout[at-88];if(at>=96&&at<104)return f.actualShader[at-96];if(at>=104&&at<112)return f.physicalSpec[at-104];if(at>=112&&at<120)return Composite(r.family)?f.compositeLayout[at-112]:0;if(at>=120&&at<128)return f.activePipeline[at-120];if(at>=128&&at<136)return Composite(r.family)?f.visualConfig[at-128]:0;if(at>=136&&at<144)return Composite(r.family)?f.visualRecipe[at-136]:0;if(at>=144&&at<152)return Composite(r.family)?f.actualVisualSeed[at-144]:0;if(at>=152&&at<160)return (active_->pcSourceScheduleV2_?SourceScheduleHash(r):StaticContractHash())[at-152];
 if(at==181)return f.parameterRevision;if(at==182)return 1;if(at==183)return r.physicalProfile==826691667?1:2;return 0;
}
Digest ExpectedContract::Hash()const{p011gpu::Sha256 hash;for(size_t i=0;i<224;++i)HashWord(hash,Word(i));return Finish(hash);}
bool SemanticCore::CompositeExtension(const ExpectedContract& expected,const Tail& w,bool unchanged){
 if(!Composite(expected.row_->family)){for(size_t i=156;i<256;++i)if(w[i])return false;return true;}
 if(w[156]!=1||w[157]!=expected.Word(183)||w[158]!=expected.Word(7)||w[159]!=expected.Word(6))return false;
 const size_t tailAt[]={160,168,176,184,192,200,216,224};const size_t expectedAt[]={152,128,136,104,112,120,80,144};
 for(size_t n=0;n<8;++n)for(size_t i=0;i<8;++i)if(w[tailAt[n]+i]!=expected.Word(expectedAt[n]+i))return false;
 if(Pair(w.data(),208)!=expected.U64(62)||Pair(w.data(),210)!=expected.U64(unchanged?62:66)||Pair(w.data(),212)!=expected.U64(64)||Pair(w.data(),214)!=expected.U64(unchanged?64:68))return false;
 const auto hash=expected.Hash();for(size_t i=0;i<8;++i)if(w[232+i]!=hash[i])return false;for(size_t i=240;i<256;++i)if(w[i])return false;return true;
}
bool SemanticCore::Common(const ExpectedContract& x,const NativeMappedObservation& observation){
 if(observation.active_!=x.active_)return false;const auto& f=x.active_->facts_;const auto& e=observation.evidence_;const auto& w=*observation.tail_;const auto& r=*x.row_;
 if(!e.current||!e.liveGuard||!e.mapped||!e.fenceSignaled||e.firstGl||e.firstEgl||e.currentContext!=f.context||e.actualProgram!=f.program||e.owner!=f.owner||e.dispatches!=r.count||e.barriers!=r.count||e.fenceNs<x.U64(30)||e.fenceNs>x.U64(28)||e.observedNs<e.fenceNs||e.observedNs>x.U64(28))return false;
 if(w[0]!=r.magic||w[1]!=r.revision||w[2]!=256||w[3]!=r.seal||w[4]!=r.tailProfile||w[5]!=r.kind||w[39]!=r.count||w[40]!=r.count||w[41]!=e.barriers||w[42]!=r.mask||w[52]||w[53]||w[63])return false;
 const uint64_t ids[]={x.U64(18),x.U64(44),x.U64(46),x.U64(48),x.U64(60),x.U64(26),x.U64(58),x.U64(50),x.U64(52),x.U64(20),x.U64(32)};
 for(size_t i=0;i<11;++i)if(Pair(w.data(),8+2*i)!=ids[i])return false;
 if(w[32]!=x.Word(38)||w[33]!=x.Word(39)||w[34]!=x.Word(40)||Pair(w.data(),35)!=x.U64(36)||w[43]!=w[44]||w[45]!=w[46]||w[44]>f.eventCapacity||w[46]>f.pathCapacity||(w[47]&~63u)||(w[37]&&!Power(w[37]))||(w[38]&&!Power(w[38])))return false;
 for(size_t i=0;i<8;++i)if(w[64+i]!=x.Word(72+i)||w[72+i]!=x.Word(88+i)||w[80+i]!=x.Word(96+i)||w[88+i]!=x.Word(160+i))return false;
 if(r.kind==1){if(w[37]||w[38]||w[47])return false;for(size_t i=120;i<156;++i)if(w[i])return false;}
 else{if(w[62])return false;for(size_t i=0;i<6;++i){size_t at=120+6*i;if(w[47]&(1u<<i)){if(w[at]!=(1u<<i)||(w[at+1]!=1&&w[at+1]!=4))return false;}else for(size_t n=0;n<6;++n)if(w[at+n])return false;}}
 return CompositeExtension(x,w,w[6]==2||w[6]==4);
}
std::optional<ValidatedAccepted> SemanticCore::Accepted(const ExpectedContract& x,const NativeMappedObservation& observation){
 if(!Common(x,observation))return {};const auto& w=*observation.tail_;const auto& r=*x.row_;if(w[7]||w[48]||w[49]||w[50]||w[51]||w[6]<1||w[6]>3||r.kind==2&&w[6]!=1)return {};
 if(w[6]==2){if(r.kind!=1||Pair(w.data(),30)!=x.U64(32)||Pair(w.data(),54)!=x.U64(34)||w[62])return {};}
 else{uint64_t commit=0,quanta=0;if(!Add(x.U64(32),1,commit)||!Add(x.U64(34),r.kind==2?1:0,quanta)||Pair(w.data(),30)!=commit||Pair(w.data(),54)!=quanta)return {};}
 if(r.kind==1){if(w[6]==1&&(w[62]==0||w[62]>244666)||w[6]==3&&w[62])return {};}
 else{if(!Power(w[37])||!Power(w[38])||w[38]>w[37]||w[47]!=w[37]*2-1)return {};uint32_t selected=0;for(size_t i=0;i<6;++i){size_t at=120+6*i;if(w[at+1]==1){if(selected||w[at+2]||w[at+3]||w[at+4]||w[at+5])return {};selected=1u<<i;}else if(w[at]&&w[at+2]==0)return {};}if(selected!=w[37])return {};}
 return ValidatedAccepted(observation,x);
}
std::optional<ValidatedBlocked> SemanticCore::Blocked(const ExpectedContract& x,const NativeMappedObservation& observation){
 if(!Common(x,observation))return {};const auto& w=*observation.tail_;if(x.row_->kind!=2||w[6]!=4||w[37]||w[47]!=63||w[7]<1||w[7]>18||Pair(w.data(),30)!=x.U64(32)||Pair(w.data(),54)!=x.U64(34))return {};
 for(size_t i=0;i<6;++i)if(w[120+6*i+1]!=4||w[120+6*i+2]==0)return {};if(w[48]!=w[152]||w[49]!=w[153]||w[50]!=w[154]||w[51]!=w[155])return {};
 return ValidatedBlocked(observation,x);
}
}
