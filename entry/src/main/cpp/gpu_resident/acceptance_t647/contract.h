#pragma once
#include "../program_cache/sha256.h"
#include <array>
#include <cstdint>
#include <optional>
#include <thread>
namespace grs::g9dm {class Transport;}
namespace grs::g11vis {class VisualTransport;}
namespace grs::pc::sand {class VulkanOwner;}
namespace grs::accept647 {
using Digest=std::array<uint32_t,8>;using Tail=std::array<uint32_t,256>;
enum class Family:uint32_t {R1_DEFAULT=1,DM_DEFAULT=2,G11_R1=3,G11_DM=4,G9_RX=5};
struct Row {Family family;uint32_t kind,physicalProfile,tailProfile,count,mask,magic,revision,seal;};
const Row* ContractRow(Family,uint32_t);
Digest StaticContractHash();
uint64_t Pair(const uint32_t*,size_t);Digest HashWords(const uint32_t*,size_t);
// A typed read-only adapter over the owner's immutable parent permit, not a cast
// between the R1 and DM permit/config types and not an extra Expected224 copy.
class PermitView {
 const void* pointer_=nullptr;uint32_t(*read_)(const void*,size_t)=nullptr;uint64_t begin_=0;
 template<class P>static uint32_t Read(const void* raw,size_t at){const auto& p=*static_cast<const P*>(raw);
  auto pair=[&](size_t start,uint64_t v){return at==start?uint32_t(v):uint32_t(v>>32);};
  const uint64_t fields[]={p.nonce,p.plan,p.frame,p.control,p.geometry,p.deadline,0,p.baseCommit,p.quanta,p.arenaGeneration};
  if(at>=18&&at<38)return pair(18+((at-18)/2)*2,fields[(at-18)/2]);
  if(at==38)return p.baseBank;if(at==39)return p.candidateBank;if(at==40)return p.arena;if(at==41)return p.ordinal;if(at==3)return p.kind;
  const uint64_t scope[]={p.identity.run,p.identity.owner,p.identity.context,p.identity.inputEpoch,p.identity.pageSession,p.identity.permission,p.identity.window,p.identity.config,p.identity.surface};
  if(at>=44&&at<62)return pair(44+((at-44)/2)*2,scope[(at-44)/2]);
  if(at>=72&&at<80)return p.parameterSha[at-72];if(at>=88&&at<96)return p.layoutSha[at-88];if(at>=96&&at<104)return p.shaderSha[at-96];if(at>=160&&at<168)return p.inputSha[at-160];return 0;
 }
public:
 template<class P>static PermitView Of(const P& p,uint64_t begin){PermitView view;view.pointer_=&p;view.read_=&Read<P>;view.begin_=begin;return view;}
 uint32_t Word(size_t at)const{if(at==30)return uint32_t(begin_);if(at==31)return uint32_t(begin_>>32);return read_?read_(pointer_,at):0;}
 uint64_t U64(size_t at)const{return uint64_t(Word(at))|(uint64_t(Word(at+1))<<32);}
};
struct ProgramFacts {
 const void* producer=nullptr;uintptr_t program=0,context=0;std::thread::id owner;
 uint32_t k=0,v=0,parameterRevision=0,eventCapacity=0,pathCapacity=0;
 uint64_t stateBytes=0,workBytes=0,baseVisualGeneration=0,baseVisualTicks=0;
 Digest parameter{},physicalSeed{},physicalLayout{},actualShader{},physicalSpec{},compositeLayout{},activePipeline{},visualConfig{},visualRecipe{},actualVisualSeed{};
};
class ExpectedContract;
class ActiveProgramIdentity {
 ProgramFacts facts_;Family family_;
 ActiveProgramIdentity(const ProgramFacts& facts,Family family):facts_(facts),family_(family){}
 static ActiveProgramIdentity ForDefaultDm(const ProgramFacts& facts){return ActiveProgramIdentity(facts,Family::DM_DEFAULT);}
 static ActiveProgramIdentity ForComposite(const ProgramFacts& facts,uint32_t parentKind){return ActiveProgramIdentity(facts,parentKind==1?Family::G11_R1:Family::G11_DM);}
 friend class grs::g9dm::Transport;friend class grs::g11vis::VisualTransport;
 friend class grs::pc::sand::VulkanOwner;
 friend class ExpectedContract;friend class NativeMappedObservation;friend class SemanticCore;
 friend std::optional<class ExpectedContract> ExpectedForActivePipeline(const ActiveProgramIdentity&,PermitView,uint32_t);
public:
 const void* Producer()const{return facts_.producer;}
};
class ExpectedContract {
 const ActiveProgramIdentity* active_;PermitView permit_;const Row* row_;uint64_t nextGeneration_=0,nextTicks_=0;
 ExpectedContract(const ActiveProgramIdentity& active,PermitView permit,const Row& row,uint64_t nextGen,uint64_t nextTicks):active_(&active),permit_(permit),row_(&row),nextGeneration_(nextGen),nextTicks_(nextTicks){}
 friend std::optional<ExpectedContract> ExpectedForActivePipeline(const ActiveProgramIdentity&,PermitView,uint32_t);
 friend class SemanticCore;
public:
 uint32_t Word(size_t)const;uint64_t U64(size_t at)const{return uint64_t(Word(at))|(uint64_t(Word(at+1))<<32);}
 Digest Hash()const;
 bool Serialize(uint32_t* caller,size_t count)const{if(!caller||count!=224)return false;for(size_t i=0;i<224;++i)caller[i]=Word(i);return true;}
 Family SelectedFamily()const{return row_->family;}const void* Producer()const{return active_->Producer();}
};
using CompositeExpectedContract=ExpectedContract;
std::optional<ExpectedContract> ExpectedForActivePipeline(const ActiveProgramIdentity&,PermitView,uint32_t parentKind);
struct ActualEvidence {
 uint32_t dispatches=0,barriers=0,firstGl=0,firstEgl=0;uint64_t fenceNs=0,observedNs=0;
 uintptr_t currentContext=0,actualProgram=0;std::thread::id owner;bool fenceSignaled=false,mapped=false,current=false,liveGuard=false;
};
// Only the real producer may seal its own mapped tail and actual counters.
class NativeMappedObservation {
 const ActiveProgramIdentity* active_;const Tail* tail_;ActualEvidence evidence_;
 NativeMappedObservation(const ActiveProgramIdentity& active,const Tail& tail,const ActualEvidence& evidence):active_(&active),tail_(&tail),evidence_(evidence){}
 friend class grs::g9dm::Transport;friend class grs::g11vis::VisualTransport;friend class SemanticCore;friend class ValidatedAccepted;friend class ValidatedBlocked;
 friend class grs::pc::sand::VulkanOwner;
};
class ValidatedAccepted {
 Tail tail_;Digest expected_;const void* producer_;
 ValidatedAccepted(const NativeMappedObservation& o,const ExpectedContract& e):tail_(*o.tail_),expected_(e.Hash()),producer_(e.Producer()){}
 friend class SemanticCore;
public:
 const Tail& Words()const{return tail_;}const Digest& ExpectedHash()const{return expected_;}const void* Producer()const{return producer_;}
};
class ValidatedBlocked {
 Tail tail_;Digest expected_;const void* producer_;
 ValidatedBlocked(const NativeMappedObservation& o,const ExpectedContract& e):tail_(*o.tail_),expected_(e.Hash()),producer_(e.Producer()){}
 friend class SemanticCore;
public:
 const Tail& Words()const{return tail_;}const Digest& ExpectedHash()const{return expected_;}const void* Producer()const{return producer_;}
};
class SemanticCore {
 static bool Common(const ExpectedContract&,const NativeMappedObservation&);
 static bool CompositeExtension(const ExpectedContract&,const Tail&,bool unchanged);
public:
 static std::optional<ValidatedAccepted> Accepted(const ExpectedContract&,const NativeMappedObservation&);
 static std::optional<ValidatedBlocked> Blocked(const ExpectedContract&,const NativeMappedObservation&);
};
}
