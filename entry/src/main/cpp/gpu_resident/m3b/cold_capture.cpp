#include "transport.h"
#include "cold_contract.h"
#include <algorithm>
#include <cstring>
namespace grs::m3b {
bool TransportV5::CaptureColdV5(uint32_t* out,size_t capacity,const QuotaSnapshotV3&,size_t& stateWords,size_t& eventWords,uint64_t request,uint64_t trace,uint64_t created,const cold503::Scope* scope){
 stateWords=eventWords=0;captureFailure_="CAPTURE_ARGUMENT_INVALID";cold::Layout layout;
 const bool full=config_.fullBranchTrace;
 if(!ready_||!profile_||!out||!scope||!request||!trace||!created)return false;
 if(busy_){captureFailure_="V5_FULL_PLAN_IN_FLIGHT";return false;}
 if(!Current()){captureFailure_="CAPTURE_CONTEXT_LOST";return false;}
 if(!acceptedRoot_.complete){captureFailure_="NO_COMMITTED_ROOT";return false;}
 if(!cold::Layout::Make(*profile_,alignment_,layout,full)||capacity<layout.words){captureFailure_="CAPTURE_SPAN_TOO_SMALL";return false;}
 if(full&&(!captureArmed_||!fullPlan_||(enteredMask_&baseMask_)!=enteredMask_)){captureFailure_="MISSING_REQUIRED_ANCHOR";return false;}
 std::memset(out,0,layout.words*4);
 auto* header=out;header[0]=826488150u;header[1]=2;header[2]=256;header[3]=3235775746u;header[4]=5;header[5]=31;header[6]=15;header[7]=full?2:1;header[8]=profile_->k;header[9]=profile_->kw;header[10]=full?5:1;header[11]=uint32_t(layout.count);header[14]=uint32_t(layout.Pages()-1);header[15]=1;
 cold::Put(header,16,request);cold::Put(header,18,trace);cold::Put(header,20,created);if(created>UINT64_MAX-60000000000ULL)return false;cold::Put(header,22,created+60000000000ULL);
 cold::Put(header,24,scope->pid);cold::Put(header,26,scope->control.epoch);cold::Put(header,28,scope->control.run);cold::Put(header,30,scope->control.context);cold::Put(header,32,scope->control.surface);cold::Put(header,34,scope->control.session);cold::Put(header,36,scope->control.reset);cold::Put(header,38,scope->control.permission);cold::Put(header,40,acceptedRoot_.permit.identity.inputEpoch);
 cold::Put(header,42,layout.words*4);cold::Put(header,44,layout.words*4);cold::Put(header,46,10224000);cold::Put(header,48,full?fullPlan_:acceptedRoot_.permit.plan);cold::Put(header,50,captureGrant_.sourceBoundary);
 std::copy(config_.lift.sha.begin(),config_.lift.sha.end(),header+52);std::copy(acceptedRoot_.permit.layoutSha.begin(),acceptedRoot_.permit.layoutSha.end(),header+60);std::copy(acceptedRoot_.permit.shaderSha.begin(),acceptedRoot_.permit.shaderSha.end(),header+68);
 // Operator package/ELF declarations stay absent (zero), never guessed.
 auto good=[&](){auto error=glGetError();if(error&&!rawGl_)rawGl_=error;if(rawGl_)captureFailure_="CAPTURE_GL_FAILED";return !rawGl_;};
 auto copy=[&](GLuint buffer,size_t at,size_t count,size_t destination){
  if(!count)return true;
  if(!buffer||destination>layout.words||count>layout.words-destination||at>size_t(PTRDIFF_MAX)/4||count>size_t(PTRDIFF_MAX)/4)return false;
  glBindBuffer(GL_COPY_READ_BUFFER,buffer);const auto* mapped=glMapBufferRange(GL_COPY_READ_BUFFER,GLintptr(at*4),GLsizeiptr(count*4),GL_MAP_READ_BIT);
  if(!mapped){captureFailure_="CAPTURE_MAP_FAILED";good();return false;}std::memcpy(out+destination,mapped,count*4);const bool unmapped=glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE;if(!unmapped)captureFailure_="CAPTURE_UNMAP_FAILED";return unmapped&&good();
 };
 struct Counts {uint32_t planned=0,sealed=0,flags=1,pFirst=0,pLast=0,aFirst=0,aLast=0;uint64_t branch=0;};std::array<Counts,64> count{};count[0]={8192,8192,2};
 auto fixed=[&](size_t i){const auto& segment=layout.segments[i];count[i].planned=count[i].sealed=uint32_t(segment.bytes/(4*segment.stride));count[i].flags=2;};
 glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);if(!good())return false;
 if(!copy(buffers_[currentBank_],0,profile_->stateBytes/4,layout.segments[1].begin/4))return false;fixed(1);
 {size_t destination=layout.segments[2].begin/4;const size_t at[]={0,96,old_->sourceLedger.begin,profile_->source[0].begin/4,profile_->source[1].begin/4},words[]={96,4096*64,4096*16,4096*512,4096*32};for(size_t i=0;i<5;++i){if(!copy(buffers_[15],at[i],words[i],destination))return false;destination+=words[i];}fixed(2);}
 const auto* state=out+layout.segments[1].begin/4+headerWord_;
 auto frame=[&](size_t segment,uint32_t arena,size_t base,size_t at,uint32_t rows){const auto& spec=layout.segments[segment];if(arena>=3||rows>spec.bytes/(4*spec.stride))return false;if(!copy(buffers_[base+arena],at,size_t(rows)*spec.stride,spec.begin/4))return false;count[segment]={rows,rows,2};return true;};
 if(!frame(3,state[144],22,0,state[157])||!frame(4,state[37],8,0,state[38])||!frame(5,state[180],8,size_t(profile_->quantumFrameRows)*64,state[179]))return false;
 if(state[38]){count[4].pFirst=1;count[4].pLast=state[38];}if(state[179]){count[5].aFirst=1;count[5].aLast=state[179];}
 const uint32_t roots=full?5:1;
 const CaptureRoot* identityRoot=full?nullptr:&acceptedRoot_;if(full)for(const auto& root:captureRoots_)if(root.entered){identityRoot=&root;break;}if(!identityRoot){captureFailure_="MISSING_REQUIRED_ROOT_IDENTITY";return false;}std::copy(identityRoot->scope.begin(),identityRoot->scope.end(),header+24);
 auto* capsule=out+6688;identity619::Capsule(capsule,profile_->k,profile_->kw,config_,identityRoot->permit);if(!identity619::CapsuleValid(capsule)){captureFailure_="CONFIG_IDENTITY_INVALID";return false;}uint32_t capsuleSha[8];cold::Digest(capsule,1024,capsuleSha);

 for(uint32_t root=0;root<roots;++root){
  const auto& raw=full?captureRoots_[root]:acceptedRoot_;const uint32_t kind=full?(root?2u:1u):raw.permit.kind;auto* record=out+6048+128*root;
  record[0]=kind;record[7]=1;record[30]=2304+256*root;record[31]=3584+64*root;record[32]=3904+256*root;record[33]=5184+64*root;record[34]=full?11+2*root:UINT32_MAX;record[35]=full?12+2*root:UINT32_MAX;record[36]=3;record[37]=6+root;record[38]=full&&root?21+10*(root-1):UINT32_MAX;record[39]=full&&root?5:0;record[40]=1;record[41]=2;
  if(!raw.entered){
   const bool provedSkip=full&&((root==0&&!captureSourceExpected_)||(root>captureGrant_.quanta));if(!provedSkip){captureFailure_="MISSING_REQUIRED_ROOT_RECORD";return false;}
   continue;
  }
  const auto& permit=raw.permit;const auto& tail=raw.completion;if(!raw.complete){captureFailure_="ROOT_COMPLETION_NOT_OBSERVED";return false;}
  record[1]=tail[28];record[2]=tail[30];record[3]=tail[31];record[4]=tail[99];record[5]=raw.evidence.submittedPasses;record[6]=raw.accepted?1:0;record[7]=raw.accepted?4:2;
  if(!raw.accepted)header[12]|=1;
  cold::Put(record,8,permit.nonce);cold::Put(record,10,permit.sourceCommit);cold::Put(record,12,permit.physicsCommit);cold::Put(record,14,cold::Pair(tail.data(),18));cold::Put(record,16,cold::Pair(tail.data(),20));cold::Put(record,18,permit.frame);cold::Put(record,20,permit.plan);cold::Put(record,22,raw.started);cold::Put(record,24,permit.deadline);cold::Put(record,26,raw.checked);cold::Put(record,28,raw.sourceGeneration);
  std::copy(config_.lift.sha.begin(),config_.lift.sha.end(),record+42);cold::Digest(raw.input.data(),raw.input.size(),record+50);
  std::copy(tail.begin(),tail.end(),out+record[30]);std::copy(raw.input.begin(),raw.input.end(),out+record[31]);std::copy(raw.quota.begin(),raw.quota.end(),out+record[32]);
  if(raw.cursor[0]!=826488146u||raw.cursor[1]!=1||raw.cursor[2]!=64||raw.cursor[3]!=3235775761u||raw.cursor[15]!=3||raw.scope!=identityRoot->scope||permit.shaderSha!=identityRoot->permit.shaderSha||permit.layoutSha!=identityRoot->permit.layoutSha||permit.angleSha!=identityRoot->permit.angleSha){captureFailure_="FROZEN_ROOT_IDENTITY_MISMATCH";return false;}
  std::copy(raw.cursor.begin(),raw.cursor.end(),out+record[33]);identity619::Token(raw.scope.data(),raw.cursor.data(),raw.input.data(),raw.quota.data(),tail.data(),record,capsule,capsuleSha,record+58);
  if(full){for(uint32_t part=0;part<2;++part){const size_t i=11+2*root+part;if(!(baseMask_&(1u<<root))){captureFailure_="MISSING_REQUIRED_ANCHOR";return false;}if(!copy(fullBuffers_[21+2*root+part],0,layout.segments[i].bytes/4,layout.segments[i].begin/4))return false;fixed(i);}}
  auto& controls=count[6+root];controls.flags=2;
  if(kind==1)continue;
  auto* control=out+layout.segments[6+root].begin/4;
  if(!copy(full?fullBuffers_[20]:buffers_[25+permit.eventArena],full?size_t(root)*2048:0,full?1088:768,layout.segments[6+root].begin/4))return false;
  controls.planned=controls.sealed=full?8:6;
  if(!full)continue;
  for(uint32_t role=1;role<=5;++role){const uint32_t slot=role-1,controlSlot=role<=3?role-1:role;std::array<uint32_t,32> entry{},final{};
   std::copy(control+768+64*slot,control+800+64*slot,entry.begin());std::copy(control+800+64*slot,control+832+64*slot,final.begin());auto* meta=control+768+32*slot;
   const uint32_t flag=cold::BuildScopeMeta(meta,root,role,control+128*controlSlot,entry.data(),final.data());if(!flag){captureFailure_="META_INCOMPLETE";return false;}
   if(flag==1)continue;
   if(cold::Pair(meta,4)!=permit.nonce){captureFailure_="META_ROOT_NONCE_MISMATCH";return false;}
   const size_t physical=21+10*(root-1)+2*slot,aux=physical+1;const uint32_t plannedP=meta[9]-meta[8],sealedP=meta[10]-meta[8],plannedA=meta[12]-meta[11],sealedA=meta[13]-meta[11];
   if(plannedP>profile_->scopeRows||plannedA>SelectLineageProfile(profile_->k,profile_->kw)->ea){captureFailure_="CAPTURE_CURSOR_RANGE";return false;}
   if(!(fullMask_&(1u<<((root-1)*5+slot)))){captureFailure_="MISSING_REQUIRED_PAIRED_STREAM";return false;}
   const uint32_t pFirst=sealedP?meta[8]+1:0,pLast=sealedP?meta[10]:0,aFirst=sealedA?meta[11]+1:0,aLast=sealedA?meta[13]:0;
   count[physical]={plannedP,sealedP,flag,pFirst,pLast,aFirst,aLast,cold::Pair(meta,6)};count[aux]={plannedA,sealedA,flag,pFirst,pLast,aFirst,aLast,cold::Pair(meta,6)};
   const auto buffer=fullBuffers_[(root-1)*5+slot];if(!copy(buffer,0,size_t(plannedP)*64,layout.segments[physical].begin/4)||!copy(buffer,size_t(profile_->scopeRows)*64,size_t(plannedA)*32,layout.segments[aux].begin/4))return false;
   if(flag==4){header[12]|=1;controls.flags=4;controls.sealed=0;}
  }
  std::memset(control+928,0,(2048-928)*4);
 }
 const auto& source=full?captureSource_:acceptedSource_;std::copy(source.words.begin(),source.words.end(),out+5504);
 for(size_t i=0;i<layout.count;++i){const auto& spec=layout.segments[i];const auto& rows=count[i];auto* descriptor=out+256+32*i;const uint32_t root=!i||!full?0:i<=5?UINT32_MAX:i<=10?uint32_t(i-6):i<=20?uint32_t((i-11)/2):uint32_t((i-21)/10+1);const uint32_t role=full&&i>=21?uint32_t((i-21)%10/2+1):0;
  descriptor[0]=uint32_t(i+1);descriptor[1]=root;descriptor[2]=role;cold::Put(descriptor,4,rows.branch);descriptor[6]=uint32_t(i);cold::Put(descriptor,7,i?trace:0);cold::Put(descriptor,9,spec.begin);cold::Put(descriptor,11,spec.bytes);descriptor[13]=spec.stride;descriptor[14]=descriptor[16]=rows.sealed;descriptor[15]=rows.planned;descriptor[17]=rows.pFirst;descriptor[18]=rows.pLast;descriptor[19]=rows.aFirst;descriptor[20]=rows.aLast;descriptor[21]=rows.flags;descriptor[22]=!i||full&&i>=6&&i<=10?2:1;descriptor[31]=3235775745u;if(i)cold::Digest(out+spec.begin/4,spec.bytes/4,descriptor+23);
 }
 cold::PrefixDigest(out,out+279);

 stateWords=layout.words-256;captureFailure_="OK";return true;
}
}
