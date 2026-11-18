// T571 (problem T527/T535/T552): V5 cold capture — ROOT and FULL modes.
// ROOT captures the current successful prefix; FULL captures the whole plan:
// the 20 physical/provenance scope pairs from the full-trace buffers (when a
// pair actually executed in this plan), the frozen per-root completion copies
// from fullBuffers_[20], the final state/source-context, and the current
// SOURCE frame. BASE_STATE_x/BASE_SOURCE_CONTEXT_x anchors have no persistent
// resident object in this transport; per the T527 contract they are recorded
// NOT_EXECUTED rather than invented from CURRENT (an all-zero body still
// participates in the full-length segment digest). The prefix follows the
// T535 fixed self-digest mask (its own SHA bytes are virtually zeroed at
// digest time by cold::Digest) and segment descriptors follow the T552 wire
// (kind=index+1, container segmentRevision=1, executed/not-executed).
#include "transport.h"
#include "cold_contract.h"
#include <cstring>
namespace grs::m3b {
bool TransportV5::CaptureColdV5(uint32_t* out,size_t capacity,const QuotaSnapshotV3& quota,size_t& stateWords,size_t& eventWords){
 stateWords=eventWords=0;
 cold::Layout layout;
 if(!ready_||busy_||!Current()||!profile_||!out)return false;
 if(!cold::Layout::Make(*profile_,alignment_,layout,config_.fullBranchTrace))return false;
 if(config_.fullBranchTrace&&!fullFrozen_&&!busy_)return false; // plan mid-flight: refuse rather than archive a half plan
 if(capacity<layout.words)return false;
 auto good=[&](){const auto g=glGetError();if(g&&!rawGl_)rawGl_=g;return !rawGl_;};
 std::memset(out,0,layout.words*4);
 // Prefix header (T535): magic/rev/policy; word 7 marks the archive mode.
 out[0]=826488150u;out[1]=1u;out[2]=256u;out[4]=5u;out[5]=31u;out[6]=15u;out[11]=uint32_t(layout.count);
 out[7]=config_.fullBranchTrace?2u:1u; // T527: ROOT_COMPLETE=1 / FULL_PLAN_BRANCH=2
 cold::Put(out,44,layout.words*4);
 // FULL-plan identity: plan id + executed scope/quantum masks (observed
 // facts only — masks record which streams actually ran in this plan).
 if(config_.fullBranchTrace){cold::Put(out,48,fullPlan_);out[50]=fullMask_;out[51]=fullQuantumMask_;}
 out[3]=3235775745u;
 // Fixed identity row: run/owner/context/surface from the active permit.
 cold::Put(out,256+0,permit_.identity.run);cold::Put(out,256+2,permit_.identity.context);
 cold::Put(out,256+4,permit_.identity.surface);
 auto copy=[&](GLuint buffer,size_t offsetWords,size_t countWords,size_t destination){
  if(!countWords)return true;
  if(destination>layout.words||countWords>layout.words-destination||offsetWords>SIZE_MAX/4)return false;
  glBindBuffer(GL_COPY_READ_BUFFER,buffer);
  auto* mapped=glMapBufferRange(GL_COPY_READ_BUFFER,GLintptr(offsetWords*4),GLsizeiptr(countWords*4),GL_MAP_READ_BIT);
  if(!mapped){good();return false;}
  std::memcpy(out+destination,mapped,countWords*4);
  const bool unmapped=glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE;
  return unmapped&&good();
 };
 glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);
 if(!good())return false;
 // Executed flags per segment (layout.count descriptors in file order).
 std::array<bool,64> executed{};
 // Final state: the current visible bank (FULL and ROOT alike).
 const size_t stateWordsAvail=profile_->stateBytes/4;
 const GLuint stateBank=buffers_[currentBank_<3?currentBank_:0];
 const size_t stateSegment=layout.Find("FINAL_STATE");
 if(stateSegment==layout.count)return false;
 if(!copy(stateBank,0,stateWordsAvail,layout.segments[stateSegment].begin/4))return false;
 executed[stateSegment]=true;
 stateWords=layout.segments[stateSegment].bytes/4;
 // Final source context: the resident combined semantic stream.
 const size_t contextSegment=layout.Find("FINAL_SOURCE_CONTEXT");
 if(contextSegment==layout.count)return false;
 if(!copy(buffers_[20],0,layout.segments[contextSegment].bytes/4,layout.segments[contextSegment].begin/4))return false;
 executed[contextSegment]=true;
 // SOURCE frame: the live SOURCE128 arena prefix actually accepted so far.
 const size_t sourceSegment=layout.Find("SOURCE_FRAME128");
 if(sourceSegment!=layout.count&&buffers_[5]){
  const size_t frameWords=std::min<size_t>(layout.segments[sourceSegment].bytes/4,size_t(profile_->k+1)*64);
  if(!copy(buffers_[5],0,frameWords,layout.segments[sourceSegment].begin/4))return false;
  executed[sourceSegment]=true;
 }
 if(!config_.fullBranchTrace){
  // ROOT mode: the remaining prefix segments come from the resident event
  // arenas; absent objects stay NOT_EXECUTED with zero bodies.
  struct RootPlan{const char* name;GLuint buffer;};
  const RootPlan plan[]={
   {"PHYSICAL_FRAME64",buffers_[8+0]},
   {"PROVENANCE_FRAME32",buffers_[8+1]},
   {"CONTROL_ROOT_0",buffers_[15]},
  };
  for(const auto& item:plan){
   const size_t at=layout.Find(item.name);
   if(at==layout.count||!item.buffer)continue;
   if(!copy(item.buffer,0,layout.segments[at].bytes/4,layout.segments[at].begin/4))return false;
   executed[at]=true;
  }
 }else{
  // FULL mode (T527 §1): 20 scope pairs from the full-trace buffers. A pair
  // is archived only when its bit is set in the plan's executed scope mask;
  // unexecuted pairs stay NOT_EXECUTED — planned rows may exceed sealed rows
  // and the archive only publishes the sealed prefix (T552 wire semantics).
  for(uint32_t slot=0;slot<20;++slot){
   if(!(fullMask_&(1u<<slot)))continue; // NOT_EXECUTED in this plan
   char name[40];
   std::snprintf(name,sizeof(name),"Q%u_ROLE%u_%s",slot/5,slot%5+1,slot%2?"PROVENANCE":"PHYSICAL");
   const size_t at=layout.Find(name);
   if(at==layout.count)return false;
   if(!copy(fullBuffers_[slot],0,layout.segments[at].bytes/4,layout.segments[at].begin/4))return false;
   executed[at]=true;
  }
  // Frozen per-root completion copies: fullBuffers_[20] holds 6x128 words per
  // root ordinal (the frozen plan-completion block); CONTROL_ROOT_x segments
  // are archived from it for the quanta that actually executed.
  for(uint32_t q=0;q<4;++q){
   if(!(fullQuantumMask_&(1u<<q)))continue;
   char name[24];
   std::snprintf(name,sizeof(name),"CONTROL_ROOT_%u",q);
   const size_t at=layout.Find(name);
   if(at==layout.count)return false;
   const size_t segWords=layout.segments[at].bytes/4;
   const size_t available=32768/4; // fullBuffers_[20] capacity in words
   const size_t offset=size_t(q)*2048; // per-ordinal 6x128 frozen block
   if(offset>=available)continue;      // ordinal beyond the frozen block
   const size_t words=std::min(segWords,available-offset);
   if(words&&copy(fullBuffers_[20],offset,words,layout.segments[at].begin/4))executed[at]=true;
  }
  // BASE_STATE_x / BASE_SOURCE_CONTEXT_x: no persistent resident base
  // snapshots exist in this transport; recorded NOT_EXECUTED (T527: do not
  // invent anchors by reversing CURRENT) — descriptors below seal zero rows.
 }
 // Segment descriptors — field offsets match cold::Validate exactly:
 // d[0]=kind(index+1), Pair(d,9)=begin, Pair(d,11)=bytes, d[13]=stride,
 // d[14]=sealedRows(==validRows; 0 for NOT_EXECUTED), d[21] role/trial bits<=7,
 // d[31]=seal.
 for(size_t i=0;i<layout.count;++i){
  const auto& s=layout.segments[i];
  auto* d=out+256+i*32;
  const size_t rows=s.bytes/(4*s.stride);
  const uint32_t sealed=executed[i]&&rows<=UINT32_MAX?uint32_t(rows):0u;
  d[0]=uint32_t(i+1);
  cold::Put(d,9,s.begin);
  cold::Put(d,11,s.bytes);
  d[13]=s.stride;
  d[14]=sealed;d[15]=executed[i]&&rows<=UINT32_MAX?uint32_t(rows):0u;d[16]=sealed;
  d[21]=0u;
  cold::Digest(out+s.begin,s.bytes/4,d+23);
  d[31]=3235775745u;
 }
 return cold::Validate(out,capacity,layout);
}
}
