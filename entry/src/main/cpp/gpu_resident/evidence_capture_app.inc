// Included in App, after WorkPermit. All methods require App::mutex.
std::unique_ptr<EvidenceCapture> evidence;
uint64_t captureSerial=0,sampleSerial=0,closedSourceBoundary=0,countersEpoch=0,countersSession=0;U64 countersRun{};bool countersV2=false;
void CounterIdentityLocked(){countersRun=identity.run;countersEpoch=epoch.load();countersSession=progress.session;countersV2=progressV2;}
bool captureRootBusy=false; // Existing source-close snapshot only, never an arm gate.
uint32_t nativeViewportWidth=0,nativeViewportHeight=0;
RenderCadence30 renderCadence;
EvidenceControl observedPulse{},swapEvidence{};bool checkingPulse=false;
std::array<bool,2> downRecorded{};
EvidenceControl EvidenceControlLocked()const{
 auto out=observedPulse;
 if(!checkingPulse){out.observedSeq=0;out.observedUiNs=0;out.observedReceiveNs=Stamp();}out.seq=progress.sequence;out.uiNs=progress.uiNs;out.receiveNs=progress.receiveNs;
 out.swapCallStarted=swapEvidence.swapCallStarted;out.swapReturned=swapEvidence.swapReturned;out.nativeSwapSucceeded=swapEvidence.nativeSwapSucceeded;out.swapStartNs=swapEvidence.swapStartNs;out.swapEndNs=swapEvidence.swapEndNs;
 out.deadline=permit.deadline;out.permitRevision=permit.revision;out.planId=permitPlan;out.sourceBoundary=ui.boundary;
 auto value=[](U64 n){return (uint64_t(n.hi)<<32)|n.lo;};
 out.owner=value(permit.identity.owner);out.context=value(permit.identity.context);out.surface=value(permit.identity.surface);
 out.geometry=value(permit.identity.geometry);out.config=value(permit.identity.config);out.input=value(permit.input);
 out.permitEpoch=permit.epoch;out.permitSession=permitSession;out.rootKind=permitKind;return out;
}
BridgeTimeRow EvidenceRowLocked(uint64_t root=0)const{
 BridgeTimeRow r;r.ns=r.now=Stamp();r.tid=BridgeTid();r.epoch=epoch.load();r.run=(uint64_t(identity.run.hi)<<32)|identity.run.lo;r.root=root?root:permit.nonce;return r;
}
void CaptureReadyLocked(){if(evidence&&evidence->armed&&!evidence->readySeen&&AllowedLocked()){evidence->readySeen=true;EvidenceKeyLocked("READY","");}}
void EvidenceKeyLocked(const char* kind,const char* why){
 if(evidence&&evidence->armed)evidence->Push(kind,why,EvidenceRowLocked(),EvidenceLedger::Of(progress),EvidenceControlLocked(),true);
}
void FreezeEvidenceLocked(const char* why){
 if(evidence&&evidence->armed){evidence->lifetimeNativeEnd=progress.maxNativeGap;evidence->lifetimeUiEnd=progress.maxUiGap;evidence->Freeze(why,Stamp(),EvidenceLedger::Of(progress),EvidenceControlLocked());}
}
void CheckEvidenceLocked(){
 if(!evidence||!evidence->armed)return;
 const auto& c=evidence->config;
 if(c.epoch!=epoch.load()||c.runLo!=identity.run.lo||c.runHi!=identity.run.hi||c.pageSession!=progress.session||
  c.expectedRevision!=(progressV2?"GRS_M1_PROGRESS_CLOCK_V2":"GRS_M1_PROGRESS_CLOCK_V1"))FreezeEvidenceLocked("IDENTITY_CHANGED");
 else if(Stamp()>=evidence->deadlineNs)FreezeEvidenceLocked("DURATION_REACHED");
}
#include "evidence_capture_request.inc"
std::string EndEvidenceLocked(uint64_t id){
 CheckEvidenceLocked();if(!evidence||evidence->id!=id)return "{\"state\":\"REJECTED\",\"reason\":\"STALE_CAPTURE_ID\"}";
 FreezeEvidenceLocked("EXPLICIT_END");return evidence->State();
}
void CaptureDetailLocked(const BridgeTimeRow& row){
 CheckEvidenceLocked();if(evidence&&evidence->armed){evidence->lifetimeNativeEnd=progress.maxNativeGap;evidence->lifetimeUiEnd=progress.maxUiGap;evidence->Detail(row,EvidenceLedger::Of(progress),EvidenceControlLocked());}
}
void CaptureFaultLocked(const char* why){
 if(!evidence||!evidence->armed)return;
 if(!evidence->faultSeen){evidence->faultSeen=true;EvidenceKeyLocked("FIRST_FAILURE",why);}
 FreezeEvidenceLocked(why);
}
void CaptureRevokeLocked(const char* why){
 if(!evidence||!evidence->armed)return;
 if(!evidence->faultSeen){evidence->faultSeen=true;EvidenceKeyLocked("FIRST_FAILURE",why);}
 if(!evidence->revokeSeen){evidence->revokeSeen=true;EvidenceKeyLocked("REVOKE",why);}
 FreezeEvidenceLocked(why);
}
void CaptureDownLocked(EvidenceDown down,const char* why){
 CheckEvidenceLocked();if(!evidence||!evidence->armed||down.kind>1)return;
 evidence->sourceAttemptSeen=true;if(downRecorded[down.kind])return;downRecorded[down.kind]=true;
 down.nativeWidth=nativeViewportWidth;down.nativeHeight=nativeViewportHeight;down.ready=AllowedLocked();EvidenceCapture::Copy(down.permission,permission);
 if(evidence->Push("SOURCE_ATTEMPT",why,EvidenceRowLocked(),EvidenceLedger::Of(progress),EvidenceControlLocked(),true))evidence->rows[evidence->count-1].down=down;
}
void CaptureOfferLocked(const std::array<uint32_t,32>& words,const char* result){
 CheckEvidenceLocked();if(!evidence||!evidence->armed||words[2]>1)return;
 const auto phase=words[2];++evidence->offerCount[phase];if(!std::strcmp(result,"ENQUEUED_NOT_ACK"))++evidence->enqueuedCount[phase];
 if(evidence->offerRecorded[phase])return;evidence->offerRecorded[phase]=true;
 auto row=EvidenceRowLocked(0);row.root=0;row.code=phase;
 row.probeA=(uint64_t(words[17])<<32)|words[16];row.probeB=(uint64_t(words[19])<<32)|words[18];row.probeC=(uint64_t(words[21])<<32)|words[20];
 evidence->Push("FIRST_OFFER",result,row,EvidenceLedger::Of(progress),EvidenceControlLocked(),true);
}
void CaptureAcceptedLocked(const AcceptedRoot& a){
 if(!evidence||!evidence->armed)return;
 if(!evidence->acceptedSeen){evidence->acceptedSeen=true;EvidenceKeyLocked("FIRST_ACCEPT","");}
 const uint32_t kind=a.completion[27];
 uint32_t phase=kind==1?permitSourcePhase:evidence->lastSourcePhase;
 if(kind==1)evidence->lastSourcePhase=phase;
 const uint32_t role=kind==1?(phase?3:1):(phase==0?2:phase==1?4:0);
 // Full accepted-root identity, not a fabricated scene/presentation token.
 const auto presentRole=phase==0?5:phase==1?6:7;
 if(evidence->tokenRoles[role]&&(kind!=2||evidence->tokenRoles[presentRole]))return;
 std::array<char,1024> token{};size_t at=0;
 auto append=[&](uint64_t v){if(at>=token.size())return;const int n=std::snprintf(token.data()+at,token.size()-at,"|%llu",static_cast<unsigned long long>(v));if(n<0||size_t(n)>=token.size()-at)at=token.size();else at+=size_t(n);};
 const auto prefix="GRS_ACCEPTED_ROOT_Q20_M1_V1";std::memcpy(token.data(),prefix,std::strlen(prefix));at=std::strlen(prefix);
 for(size_t i=4;i<=50;i++)append(a.completion[i]);
 for(const auto* h:{&permit.identity.shaderSha,&permit.identity.angleSha,&permit.identity.layoutSha})for(auto w:*h)append(w);
 append(permit.input.hi);append(permit.input.lo);append(permitSession);append(permitPlan);append(permit.revision);
 if(at>=token.size()){evidence->incomplete=true;++evidence->dropped;return;}
 const bool saved=evidence->SaveRoot(role,(uint64_t(a.nonce.hi)<<32)|a.nonce.lo,(uint64_t(a.physics.hi)<<32)|a.physics.lo,
  (uint64_t(a.source.hi)<<32)|a.source.lo,a.publication,(uint64_t(a.publicationGeneration.hi)<<32)|a.publicationGeneration.lo,kind,phase,token.data());
 auto& t=evidence->pendingToken;t={};t.root=(uint64_t(a.nonce.hi)<<32)|a.nonce.lo;t.physics=(uint64_t(a.physics.hi)<<32)|a.physics.lo;t.source=(uint64_t(a.source.hi)<<32)|a.source.lo;
 t.publicationGeneration=(uint64_t(a.publicationGeneration.hi)<<32)|a.publicationGeneration.lo;t.slot=a.publication;t.kind=kind;t.phase=phase;EvidenceCapture::Copy(t.rootToken,token.data());
 t.sourceId=a.completion[m1::completion256::sourceId];t.requestSeq=a.completion[m1::completion256::requestSeq];t.admittedQ=a.completion[m1::completion256::admittedUnits];t.rejectedQ=a.completion[m1::completion256::rejectedUnits];t.receiptStatus=a.completion[m1::completion256::receiptStatus];t.outcome=a.completion[m1::completion256::outcome];
 if(saved)evidence->tokens[evidence->tokenCount-1]=t;
}
