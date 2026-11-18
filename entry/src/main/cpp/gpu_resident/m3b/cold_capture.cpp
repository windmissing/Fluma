// T571 (problem T527/T535/T552): V5 cold capture — ROOT and FULL modes.
// ROOT captures the current successful prefix; FULL captures the whole plan:
// the 20 physical/provenance scope pairs from the full-trace buffers (when a
// pair actually executed in this plan), the frozen per-root completion copies
// from fullBuffers_[20], the final state/source-context, and the current
// SOURCE frame. Executed roots require their retained BASE pair; absent
// required anchors are failures, never evidence that a root did not execute.
// The prefix follows the
// T535 fixed self-digest mask (its own SHA bytes are virtually zeroed at
// digest time by cold::Digest) and segment descriptors follow the T552 wire
// (kind=index+1, container segmentRevision=1, executed/not-executed).
#include "transport.h"
#include "cold_contract.h"
#include <cstring>
namespace grs::m3b {
bool TransportV5::CaptureColdV5(uint32_t* out,size_t capacity,const QuotaSnapshotV3& quota,size_t& stateWords,size_t& eventWords){
 stateWords=eventWords=0;
 captureFailure_="CAPTURE_ARGUMENT_INVALID";
 cold::Layout layout;
 if(!ready_||!profile_||!out)return false;
 if(busy_){captureFailure_="V5_FULL_PLAN_IN_FLIGHT";return false;}
 if(!Current()){captureFailure_="CAPTURE_CONTEXT_LOST";return false;}
 if(!cold::Layout::Make(*profile_,alignment_,layout,config_.fullBranchTrace))return false;
 if(config_.fullBranchTrace&&((enteredMask_&baseMask_)!=enteredMask_)){captureFailure_="MISSING_REQUIRED_ANCHOR";return false;}
 if(capacity<layout.words){captureFailure_="CAPTURE_SPAN_TOO_SMALL";return false;}
 captureFailure_="CAPTURE_LAYOUT_INVALID";
 auto good=[&](){const auto g=glGetError();if(g&&!rawGl_)rawGl_=g;if(rawGl_)captureFailure_="CAPTURE_GL_FAILED";return !rawGl_;};
 std::memset(out,0,layout.words*4);
 // Prefix header (T535): magic/rev/policy; word 7 marks the archive mode.
 out[0]=826488150u;out[1]=2u;out[2]=256u;out[4]=5u;out[5]=31u;out[6]=15u;out[11]=uint32_t(layout.count);
 out[7]=config_.fullBranchTrace?2u:1u; // T527: ROOT_COMPLETE=1 / FULL_PLAN_BRANCH=2
 cold::Put(out,44,layout.words*4);
 // FULL-plan identity: plan id + executed scope/quantum masks (observed
 // facts only — masks record which streams actually ran in this plan).
 if(config_.fullBranchTrace){cold::Put(out,48,fullPlan_);out[50]=fullMask_;out[51]=fullQuantumMask_;}
 out[3]=3235775746u;out[15]=1;out[8]=profile_->k;out[9]=profile_->kw;
 // Fixed identity row: run/owner/context/surface from the active permit.
 
 auto copy=[&](GLuint buffer,size_t offsetWords,size_t countWords,size_t destination){
  if(!countWords)return true;
  if(destination>layout.words||countWords>layout.words-destination||offsetWords>SIZE_MAX/4)return false;
  glBindBuffer(GL_COPY_READ_BUFFER,buffer);
  auto* mapped=glMapBufferRange(GL_COPY_READ_BUFFER,GLintptr(offsetWords*4),GLsizeiptr(countWords*4),GL_MAP_READ_BIT);
  if(!mapped){captureFailure_="CAPTURE_MAP_FAILED";good();return false;}
  std::memcpy(out+destination,mapped,countWords*4);
  const bool unmapped=glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE;
  if(!unmapped)captureFailure_="CAPTURE_UNMAP_FAILED";
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
 {size_t dst=layout.segments[contextSegment].begin/4;const size_t offsets[]={0,96,old_->sourceLedger.begin,profile_->source[0].begin/4,profile_->source[1].begin/4};const size_t sizes[]={96,4096*64,4096*16,4096*512,4096*32};for(size_t j=0;j<5;++j){if(!copy(buffers_[15],offsets[j],sizes[j],dst))return false;dst+=sizes[j];}}
 executed[contextSegment]=true;
 // Frame rows/arenas come from the accepted state, not SOURCE ledger size.
 const auto* accepted=out+layout.segments[stateSegment].begin/4+headerWord_;
 auto frame=[&](const char* name,uint32_t arena,size_t bufferBase,size_t offset,uint32_t rows,size_t stride){
  const size_t at=layout.Find(name);if(at==layout.count||arena>=3)return false;
  const auto& segment=layout.segments[at];if(rows>segment.bytes/(4*stride))return false;
  if(!copy(buffers_[bufferBase+arena],offset,size_t(rows)*stride,segment.begin/4))return false;
  executed[at]=true;return true;
 };
 if(!frame("SOURCE_FRAME128",accepted[144],22,0,accepted[157],128)||
    !frame("PHYSICAL_FRAME64",accepted[37],8,0,accepted[38],64)||
    !frame("PROVENANCE_FRAME32",accepted[180],8,size_t(profile_->quantumFrameRows)*64,accepted[179],32))return false;
 if(!config_.fullBranchTrace){
  // An absent frozen root-control mirror cannot be replaced by scratch.
 }else{
  // FULL mode (T527 §1): 20 scope pairs from the full-trace buffers. A pair
  // is archived only when its bit is set in the plan's executed scope mask;
  // unexecuted pairs stay NOT_EXECUTED — planned rows may exceed sealed rows
  // and the archive only publishes the sealed prefix (T552 wire semantics).
  for(uint32_t slot=0;slot<20;++slot){
   if(!(fullMask_&(1u<<slot)))continue; // NOT_EXECUTED in this plan
   for(unsigned part=0;part<2;++part){const size_t at=21+slot*2+part;const auto& seg=layout.segments[at];if(!copy(fullBuffers_[slot],part?size_t(profile_->scopeRows)*64:0,seg.bytes/4,seg.begin/4))return false;executed[at]=true;}
  }
  for(unsigned root=0;root<5;++root)if(enteredMask_&(1u<<root)){if(!(baseMask_&(1u<<root)))return false;for(unsigned part=0;part<2;++part){const auto at=11+root*2+part;const auto& seg=layout.segments[at];if(!copy(fullBuffers_[21+root*2+part],0,seg.bytes/4,seg.begin/4))return false;executed[at]=true;}}
  // Frozen per-root completion copies: fullBuffers_[20] holds 6x128 words per
  // root ordinal (the frozen plan-completion block); CONTROL_ROOT_x segments
  // are archived from it for the quanta that actually executed.
  for(uint32_t q=0;q<4;++q){
   if(!(fullQuantumMask_&(1u<<q)))continue;
   char name[24];
   std::snprintf(name,sizeof(name),"CONTROL_ROOT_%u",q+1);
   const size_t at=layout.Find(name);
   if(at==layout.count)return false;
   const size_t segWords=layout.segments[at].bytes/4;
   const size_t available=40960/4; // fullBuffers_[20] capacity in words
   const size_t offset=size_t(q+1)*2048; // per-ordinal 6x128 frozen block
   if(offset>=available)continue;      // ordinal beyond the frozen block
   const size_t words=std::min(segWords,available-offset);
   if(words&&copy(fullBuffers_[20],offset,words,layout.segments[at].begin/4))executed[at]=true;
  }
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
  d[21]=executed[i]?2u:1u;d[22]=i?1u:2u;
  if(i)cold::Digest(out+s.begin/4,s.bytes/4,d+23);
  d[31]=3235775745u;
 }
 // Mandatory prefix itself is always sealed, using the fixed T535 mask.
 auto* prefix=out+256;prefix[14]=prefix[15]=prefix[16]=8192;prefix[21]=2;prefix[22]=2;cold::PrefixDigest(out,prefix+23);
 // Required producer metadata is not yet all retained: never publish a
 // purported COMPLETE archive from capacity/masks alone.
 captureFailure_="META_INCOMPLETE";
 return false;

}
}
