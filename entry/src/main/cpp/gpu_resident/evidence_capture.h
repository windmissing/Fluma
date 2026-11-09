#pragma once
// T409/T407: fixed native capture storage; JSON is produced only by cold APIs.
#include "bridge_timeline.h"
#include "m1/progress_clock_v2.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
namespace grs {
struct EvidenceConfig {
 std::string packageSha256,expectedRevision;
 uint64_t epoch=0,pageSession=0,durationNs=0;
 uint32_t runLo=0,runHi=0,maxRows=4096,maxBytes=1048576;
 bool Valid()const{
  if(packageSha256.size()!=64||!(expectedRevision=="GRS_M1_PROGRESS_CLOCK_V1"||expectedRevision=="GRS_M1_PROGRESS_CLOCK_V2")||
   durationNs<2000000000ULL||durationNs>5000000000ULL||maxRows==0||maxRows>4096||maxBytes==0||maxBytes>1048576)return false;
  for(char c:packageSha256)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))return false;
  return true;
 }
};
struct EvidenceLedger {
 uint64_t generated=0,accepted=0,pending=0,activeRemaining=0,overload=0,revoked=0,carry=0,clearedCarry=0;
 static EvidenceLedger Of(const m1::ProgressClockV2& p){return {p.generated,p.accepted,p.pending,p.activeRemaining,p.overload,p.revoked,p.carry,p.clearedCarry};}
 void Json(std::ostream& o)const{
  o<<"{\"generatedQuanta\":\""<<generated<<"\",\"acceptedQuantumCount\":\""<<accepted
   <<"\",\"pendingQuanta\":\""<<pending<<"\",\"activePlanRemaining\":\""<<activeRemaining
   <<"\",\"overloadDroppedQuanta\":\""<<overload<<"\",\"revokedDroppedQuanta\":\""<<revoked
   <<"\",\"quotaCarry\":\""<<carry<<"\",\"clearedCarry\":\""<<clearedCarry<<"\"}";
 }
};
struct EvidenceControl {
 uint64_t seq=0,uiNs=0,receiveNs=0,observedSeq=0,observedUiNs=0,observedReceiveNs=0;
 uint64_t deadline=0,permitRevision=0,planId=0,sourceBoundary=0;
 bool swapCallStarted=false,swapReturned=false,nativeSwapSucceeded=false;uint64_t swapStartNs=0,swapEndNs=0;
 uint64_t owner=0,context=0,surface=0,geometry=0,config=0,input=0,permitEpoch=0,permitSession=0;uint32_t rootKind=0;
};
struct EvidenceDown {bool valid=false,accepted=false,ready=false,coordinatesFinite=true;double x=0,y=0,width=0,height=0;uint64_t sampleSerial=0,epoch=0,session=0;uint32_t runLo=0,runHi=0,kind=0,nativeWidth=0,nativeHeight=0;std::array<char,48> permission{};};
struct EvidenceEvent {
 EvidenceDown down{};BridgeTimeRow row{};EvidenceLedger ledger{};EvidenceControl control{};
 std::array<char,40> kind{};std::array<char,96> reason{};
};
struct RenderCadence30 {
 uint64_t anchorNs=0,nextDueNs=0,attemptedNs=0,skipped=0,newPhysicalFrames=0,sourceOnlyFrames=0,lastSwapNs=0,lastSwapGapNs=0,maxSwapGapNs=0,staleNs=0,staleSince=0,lastPhysical=0;
 bool attempted=false,hasPhysical=false;
 bool Due(uint64_t ns,bool v2){
  if(!v2)return true;
  if(attempted&&ns>=anchorNs&&ns<nextDueNs){++skipped;return false;}
  if(!attempted||ns<anchorNs)anchorNs=ns;
  const auto elapsed=ns-anchorNs;
  const auto periods=(elapsed/1000000000)*30+(elapsed%1000000000)*30/1000000000+1;
  const auto offset=(periods/30)*1000000000+((periods%30)*1000000000+29)/30;
  nextDueNs=anchorNs>UINT64_MAX-offset?UINT64_MAX:anchorNs+offset;
  attempted=true;attemptedNs=ns;return true;
 }
 void Stale(uint64_t ns){if(!staleSince)staleSince=ns;}
 void Presented(uint64_t ns,uint64_t physical,uint32_t kind){
  if(lastSwapNs&&ns>=lastSwapNs){lastSwapGapNs=ns-lastSwapNs;maxSwapGapNs=std::max(maxSwapGapNs,lastSwapGapNs);}
  lastSwapNs=ns;if(staleSince&&ns>=staleSince){staleNs+=ns-staleSince;staleSince=0;}
  if(kind==1)++sourceOnlyFrames;
  else if(!hasPhysical||physical!=lastPhysical){++newPhysicalFrames;lastPhysical=physical;hasPhysical=true;}
 }
};
class EvidenceCapture {
public:
 static constexpr uint32_t Capacity=4096,ReservedRows=16,RowBudget=2304,EnvelopeBudget=98304;
 const char* evidenceSource="DEVICE_NATIVE",*clockDomain="STEADY_NS",*uiDomain="FRAME_CALLBACK_NS",*probeDomain="STEADY_NS";
 EvidenceConfig config;uint64_t id=0,startNs=0,endNs=0,deadlineNs=0,firstRoot=0;
 bool armed=false,frozen=false,incomplete=false;
 std::array<char,96> reason{};
 EvidenceLedger initial{},terminal{};
 std::array<EvidenceEvent,Capacity> rows{};uint32_t count=0,criticalCount=0,detailCount=0;
 uint64_t dropped=0,suppressedDetail=0,rejectedPulses=0,pulses=0,maxNativeGap=0,maxUiGap=0;
 std::array<uint64_t,8> nativeGapHistogram{},uiGapHistogram{};
 std::array<uint64_t,73> stageCounts{},stageWallTotalNs{},stageWallMaxNs{},stageMaxStart{},stageMaxEnd{},stageMaxRoot{},stageMaxTid{};
 std::array<bool,73> detailedSeen{};std::array<uint32_t,73> detailedCode{};
 std::array<uint64_t,2> offerCount{},enqueuedCount{};std::array<bool,2> offerRecorded{};
 bool readySeen=false,acceptedSeen=false,sourceAttemptSeen=false,faultSeen=false,revokeSeen=false;
 uint64_t lifetimeNativeStart=0,lifetimeNativeEnd=0,lifetimeUiStart=0,lifetimeUiEnd=0,acceptedPulses=0;
 uint64_t lastPulseSeq=0,lastPulseUi=0,lastPulseNative=0;
 struct Token {uint64_t root=0,physics=0,source=0,publicationGeneration=0;uint32_t slot=0,kind=0,phase=0;uint32_t sourceId=0,requestSeq=0,admittedQ=0,rejectedQ=0,receiptStatus=0,outcome=0;bool presented=false,swap=false;std::array<char,1024> rootToken{},requested{},rendered{};};
 Token pendingToken{};std::array<Token,8> tokens{};uint32_t tokenCount=0;
 RenderCadence30 display;
 std::array<bool,8> tokenRoles{};uint32_t lastSourcePhase=2;
 template<size_t N> static void Copy(std::array<char,N>& to,const char* from){std::snprintf(to.data(),N,"%s",from?from:"");}
 void Begin(const EvidenceConfig& c,uint64_t captureId,uint64_t ns,const EvidenceLedger& ledger){
  config=c;id=captureId;startNs=ns;deadlineNs=ns+c.durationNs;initial=terminal=ledger;armed=true;Copy(reason,"ARMED");
 }
 uint32_t DetailLimit()const{if(config.maxRows<=ReservedRows||config.maxBytes<=EnvelopeBudget)return 0;return std::min(config.maxRows-ReservedRows,(config.maxBytes-EnvelopeBudget)/RowBudget);}
 bool Push(const char* kind,const char* why,const BridgeTimeRow& row,const EvidenceLedger& ledger,const EvidenceControl& ctl,bool critical){
  if(!armed)return false;
  if(count>=config.maxRows||(!critical&&detailCount>=DetailLimit())||(critical&&criticalCount>=ReservedRows)){
   ++dropped;incomplete=true;if(!critical)Freeze("CAPTURE_BUDGET_EXCEEDED",row.ns,ledger,ctl);return false;
  }
  auto& e=rows[count++];e.row=row;e.row.serial=count;e.ledger=ledger;e.control=ctl;Copy(e.kind,kind);Copy(e.reason,why);
  if(critical)++criticalCount;else ++detailCount;return true;
 }
 void Detail(const BridgeTimeRow& r,const EvidenceLedger& ledger,const EvidenceControl& ctl){
  if(!armed)return;
  if(r.stage<stageCounts.size()){
   ++stageCounts[r.stage];const auto duration=(r.probeStart||r.probeEnd)?(r.probeEnd>=r.probeStart?r.probeEnd-r.probeStart:0):r.delta;
   stageWallTotalNs[r.stage]+=duration;if(duration>=stageWallMaxNs[r.stage]){stageWallMaxNs[r.stage]=duration;stageMaxStart[r.stage]=r.probeStart;stageMaxEnd[r.stage]=r.probeEnd;stageMaxRoot[r.stage]=r.root;stageMaxTid[r.stage]=r.tid;}
  }
  // Wait/Wake/normal tick and later-root details are intentionally aggregated.
  if(r.stage==63&&firstRoot==0)firstRoot=r.root;
  const bool selected=r.stage==35||r.stage==40||r.stage==41||r.stage==43||r.stage==44||r.stage==46||r.stage==47||r.stage==48||r.stage==58||r.stage==59;
  const bool polling=r.stage==47||(r.stage==58&&r.carry==5);
  const uint32_t repeatKey=r.stage==58&&r.carry==5?72:r.stage;
  const bool repeat=polling&&detailedSeen[repeatKey]&&detailedCode[repeatKey]==r.code;
  if(r.root&&r.root==firstRoot&&selected&&!repeat){
   detailedSeen[repeatKey]=true;detailedCode[repeatKey]=r.code;
   Push("GL_DETAIL","",r,ledger,ctl,false);
  }else ++suppressedDetail;
 }
 void Pulse(uint64_t seq,uint64_t ui,uint64_t native,bool accepted,bool newCurrentSequence=true){
  if(!armed)return;
  if(!accepted)++rejectedPulses;
  if(!accepted&&(!newCurrentSequence||seq<=lastPulseSeq))return;
  if(lastPulseSeq){
   auto bucket=[](uint64_t gap){return uint32_t(std::min<uint64_t>(7,gap/8000000));};
   if(native>=lastPulseNative){const auto gap=native-lastPulseNative;maxNativeGap=std::max(maxNativeGap,gap);++nativeGapHistogram[bucket(gap)];}
   if(ui>=lastPulseUi){const auto gap=ui-lastPulseUi;maxUiGap=std::max(maxUiGap,gap);++uiGapHistogram[bucket(gap)];}
  }
  ++pulses;if(accepted)++acceptedPulses;lastPulseSeq=seq;lastPulseUi=ui;lastPulseNative=native;
 }
 void Freeze(const char* why,uint64_t ns,const EvidenceLedger& ledger,const EvidenceControl& ctl){
  if(!armed)return;
  BridgeTimeRow r;r.ns=r.now=ns;r.tid=BridgeTid();r.epoch=config.epoch;r.run=(uint64_t(config.runHi)<<32)|config.runLo;
  terminal=ledger;endNs=ns;Copy(reason,why);
  Push("END",why,r,ledger,ctl,true);armed=false;frozen=true;
 }
 uint32_t TokenIndex(uint64_t root)const{for(uint32_t i=0;i<tokenCount;i++)if(tokens[i].root==root)return i;return 8;}
 bool SaveRoot(uint32_t role,uint64_t root,uint64_t physics,uint64_t source,uint32_t slot,uint64_t generation,uint32_t kind,uint32_t phase,const char* token){
  if(!armed||role>=tokenRoles.size()||tokenRoles[role])return false;
  if(tokenCount==tokens.size()||std::strlen(token)>=1024){incomplete=true;++dropped;return false;}
  tokenRoles[role]=true;auto& t=tokens[tokenCount++];t.root=root;t.physics=physics;t.source=source;t.slot=slot;
  t.publicationGeneration=generation;t.kind=kind;t.phase=phase;Copy(t.rootToken,token);return true;
 }
 void Present(uint64_t root,const std::string& requested,const std::string& rendered,bool current,bool swap){
  if(!armed)return;auto i=TokenIndex(root);
  if(i==8&&pendingToken.root==root&&pendingToken.kind==2){
   const auto role=pendingToken.phase==0?5:pendingToken.phase==1?6:7;
   if(tokenRoles[role])return;if(tokenCount==tokens.size()){incomplete=true;++dropped;return;}
   tokenRoles[role]=true;i=tokenCount++;tokens[i]=pendingToken;
  }
  if(i==8)return;
  if(tokens[i].kind==2)tokenRoles[tokens[i].phase==0?5:tokens[i].phase==1?6:7]=true;
  if(requested.size()>=1024||rendered.size()>=1024){incomplete=true;++dropped;return;}
  auto& t=tokens[i];Copy(t.requested,requested.c_str());Copy(t.rendered,rendered.c_str());t.presented=current;t.swap=swap;
 }
 void Requested(uint64_t root,const std::string& token){Present(root,token,"",false,false);}
 void IdentityJson(std::ostream& o)const{
  o<<"\"captureId\":\""<<id<<"\",\"activeRevision\":\""<<config.expectedRevision<<"\",\"packageSha256\":\""<<config.packageSha256
   <<"\",\"epoch\":\""<<config.epoch<<"\",\"runLo\":"<<config.runLo<<",\"runHi\":"<<config.runHi
   <<",\"pageSession\":\""<<config.pageSession<<"\",\"startNs\":\""<<startNs<<"\",\"endNs\":\""<<endNs
   <<"\",\"evidenceSource\":\""<<evidenceSource<<"\",\"clock\":\""<<clockDomain<<"\",\"uiClock\":\""<<uiDomain<<"\",\"probeClock\":\""<<probeDomain<<"\"";
 }
 std::string State()const{std::ostringstream o;o<<"{\"revision\":\"T409_EVIDENCE_CAPTURE_V1\",\"state\":\""<<(frozen?"FROZEN":"ARMED")<<"\",\"reason\":\""<<reason.data()<<"\",";IdentityJson(o);o<<'}';return o.str();}
 void Summary(std::ostream& o)const{
  o<<"{\"renderTargetHz\":"<<(config.expectedRevision=="GRS_M1_PROGRESS_CLOCK_V2"?30:0)<<",\"renderSkippedByCadence\":\""<<display.skipped<<"\",\"newPhysicalFrames\":\""<<display.newPhysicalFrames<<"\",\"sourceOnlyFrames\":\""<<display.sourceOnlyFrames<<"\",\"lastSwapGapNs\":\""<<display.lastSwapGapNs<<"\",\"maxSwapGapNs\":\""<<display.maxSwapGapNs<<"\",\"staleNs\":\""<<(display.staleNs+(display.staleSince&&endNs>=display.staleSince?endNs-display.staleSince:0))<<"\",\"systemPresentVerified\":false,\"initialLedger\":";initial.Json(o);o<<",\"terminalLedger\":";terminal.Json(o);
  o<<",\"maxRows\":"<<config.maxRows<<",\"maxBytes\":"<<config.maxBytes<<",\"durationNs\":\""<<config.durationNs
   <<"\",\"dropped\":\""<<dropped<<"\",\"suppressedDetailedRows\":\""<<suppressedDetail<<"\",\"detailedRoot\":\""<<firstRoot
   <<"\",\"detailPolicy\":\"FIRST_ROOT_SELECTED_CALLS_V1\",\"pulseCount\":\""<<pulses<<"\",\"pulseApiCalls\":\""<<(acceptedPulses+rejectedPulses)<<"\",\"rejectedNewSequencePulses\":\""<<(pulses-acceptedPulses)<<"\",\"rejectedOtherPulses\":\""<<(rejectedPulses-(pulses-acceptedPulses))<<"\",\"acceptedPulses\":\""<<acceptedPulses<<"\",\"lifetimeNativeGapMaxAtStartNs\":\""<<lifetimeNativeStart<<"\",\"lifetimeNativeGapMaxAtEndNs\":\""<<lifetimeNativeEnd<<"\",\"lifetimeUiGapMaxAtStartNs\":\""<<lifetimeUiStart<<"\",\"lifetimeUiGapMaxAtEndNs\":\""<<lifetimeUiEnd<<"\",\"rejectedPulses\":\""<<rejectedPulses<<"\",\"maxNativeGapNs\":\""<<maxNativeGap
   <<"\",\"maxUiGapNs\":\""<<maxUiGap<<"\",\"readySeen\":"<<(readySeen?"true":"false")
   <<",\"sourceOfferSeen\":"<<((offerCount[0]||offerCount[1])?"true":"false")<<",\"sourceOfferCount\":[\""<<offerCount[0]<<"\",\""<<offerCount[1]<<"\"],\"sourceEnqueuedCount\":[\""<<enqueuedCount[0]<<"\",\""<<enqueuedCount[1]<<"\"],\"sourceAttemptSeen\":"<<(sourceAttemptSeen?"true":"false")<<",\"acceptedSeen\":"<<(acceptedSeen?"true":"false")
   <<",\"faultSeen\":"<<(faultSeen?"true":"false")<<",\"revokeSeen\":"<<(revokeSeen?"true":"false")<<",\"nativeGapHistogram\":[";
  for(size_t i=0;i<8;i++){if(i)o<<',';o<<'"'<<nativeGapHistogram[i]<<'"';}o<<"],\"uiGapHistogram\":[";
  for(size_t i=0;i<8;i++){if(i)o<<',';o<<'"'<<uiGapHistogram[i]<<'"';}o<<"],\"stageAggregates\":[";
  bool comma=false;for(size_t i=0;i<stageCounts.size();i++)if(stageCounts[i]){if(comma)o<<',';comma=true;o<<"{\"stage\":"<<i<<",\"count\":\""<<stageCounts[i]<<"\",\"totalNs\":\""<<stageWallTotalNs[i]<<"\",\"maxNs\":\""<<stageWallMaxNs[i]<<"\",\"maxStartNs\":\""<<stageMaxStart[i]<<"\",\"maxEndNs\":\""<<stageMaxEnd[i]<<"\",\"maxRoot\":\""<<stageMaxRoot[i]<<"\",\"maxTid\":\""<<stageMaxTid[i]<<"\"}";}o<<"]}";
 }
 void EventJson(std::ostream& o,const EvidenceEvent& e)const{
  const auto& r=e.row;const auto& c=e.control;
  o<<"{\"serial\":\""<<r.serial<<"\",\"kind\":\""<<e.kind.data()<<"\",\"reason\":\""<<e.reason.data()<<"\",\"captureId\":\""<<id
   <<"\",\"ns\":\""<<r.ns<<"\",\"tid\":\""<<r.tid<<"\",\"root\":\""<<r.root<<"\",\"stage\":"<<r.stage<<",\"ordinal\":"<<r.carry<<",\"quanta\":"<<r.quanta<<",\"deltaNs\":\""<<r.delta<<"\",\"code\":"<<r.code
   <<",\"probeStart\":\""<<r.probeStart<<"\",\"probeEnd\":\""<<r.probeEnd<<"\",\"probeA\":\""<<r.probeA<<"\",\"probeB\":\""<<r.probeB<<"\",\"probeC\":\""<<r.probeC
   <<"\",\"seq\":\""<<c.seq<<"\",\"uiNs\":\""<<c.uiNs<<"\",\"receiveNs\":\""<<c.receiveNs
   <<"\",\"observedSeq\":\""<<c.observedSeq<<"\",\"observedUiNs\":\""<<c.observedUiNs<<"\",\"observedReceiveNs\":\""<<c.observedReceiveNs
   <<"\",\"deadlineNs\":\""<<c.deadline<<"\",\"permitRevision\":\""<<c.permitRevision<<"\",\"planId\":\""<<c.planId<<"\",\"sourceBoundary\":\""<<c.sourceBoundary
   <<"\",\"permitOwner\":\""<<c.owner<<"\",\"permitContext\":\""<<c.context<<"\",\"permitSurface\":\""<<c.surface<<"\",\"permitGeometry\":\""<<c.geometry<<"\",\"permitConfig\":\""<<c.config
   <<"\",\"permitInput\":\""<<c.input<<"\",\"permitEpoch\":\""<<c.permitEpoch<<"\",\"permitSession\":\""<<c.permitSession<<"\",\"rootKind\":"<<c.rootKind
   <<",\"swapCallStarted\":"<<(c.swapCallStarted?"true":"false")<<",\"swapReturned\":"<<(c.swapReturned?"true":"false")<<",\"nativeSwapSucceeded\":"<<(c.nativeSwapSucceeded?"true":"false")<<",\"swapStartNs\":\""<<c.swapStartNs<<"\",\"swapEndNs\":\""<<c.swapEndNs<<"\",\"ledger\":";e.ledger.Json(o);
  if(e.down.valid){const auto& d=e.down;o<<",\"down\":{\"coordinatesFinite\":"<<(d.coordinatesFinite?"true":"false")<<",\"x\":"<<d.x<<",\"y\":"<<d.y<<",\"width\":"<<d.width<<",\"height\":"<<d.height
   <<",\"sampleSerial\":\""<<d.sampleSerial<<"\",\"epoch\":\""<<d.epoch<<"\",\"pageSession\":\""<<d.session<<"\",\"runLo\":"<<d.runLo<<",\"runHi\":"<<d.runHi
   <<",\"nativeWidth\":"<<d.nativeWidth<<",\"nativeHeight\":"<<d.nativeHeight<<",\"phase\":"<<d.kind<<",\"acceptedByUi\":"<<(d.accepted?"true":"false")<<",\"nativeReady\":"<<(d.ready?"true":"false")<<",\"nativePermission\":\""<<d.permission.data()<<"\"}";}
  o<<'}';
 }
 uint32_t PageCount()const{return 1+(count+3)/4+tokenCount;}
 std::string Page(uint32_t cursor)const{
  if(!frozen||cursor>=PageCount())return "{\"state\":\"REJECTED\",\"reason\":\"CAPTURE_NOT_FROZEN_OR_CURSOR\"}";
  std::ostringstream o;o<<"{\"revision\":\"T409_EVIDENCE_CAPTURE_PAGE_V1\",\"state\":\"FROZEN\",\"incomplete\":"<<(incomplete?"true":"false")<<",\"reason\":\""<<reason.data()<<"\",";
  IdentityJson(o);o<<",\"cursor\":"<<cursor<<",\"next\":"<<cursor+1<<",\"complete\":"<<(cursor+1==PageCount()?"true":"false")<<",\"pageCount\":"<<PageCount()<<",\"rowCount\":"<<count<<",\"tokenCount\":"<<tokenCount;
  if(cursor==0){o<<",\"type\":\"SUMMARY\",\"summary\":";Summary(o);}
  else if(cursor<=((count+3)/4)){
   o<<",\"type\":\"EVENTS\",\"rows\":[";const auto begin=(cursor-1)*4,end=std::min(count,begin+4);
   for(auto i=begin;i<end;i++){if(i!=begin)o<<',';EventJson(o,rows[i]);}o<<']';
  }else{
   const auto& t=tokens[cursor-1-(count+3)/4];
   o<<",\"type\":\"TOKEN\",\"token\":{\"root\":\""<<t.root<<"\",\"physicsCommit\":\""<<t.physics<<"\",\"sourceCommit\":\""<<t.source
    <<"\",\"publicationGeneration\":\""<<t.publicationGeneration<<"\",\"publicationSlot\":"<<t.slot<<",\"kind\":"<<t.kind<<",\"phase\":"<<t.phase
    <<",\"sourceId\":"<<t.sourceId<<",\"requestSeq\":"<<t.requestSeq<<",\"admittedQ\":"<<t.admittedQ<<",\"rejectedQ\":"<<t.rejectedQ<<",\"receiptStatus\":"<<t.receiptStatus<<",\"outcome\":"<<t.outcome<<",\"rootToken\":\""<<t.rootToken.data()<<"\",\"requestedToken\":\""<<t.requested.data()<<"\",\"renderedToken\":\""<<t.rendered.data()
    <<"\",\"current\":"<<(t.presented?"true":"false")<<",\"swapSucceeded\":"<<(t.swap?"true":"false")<<",\"presentationEvidence\":\""<<(t.requested[0]?"ATTEMPTED":"NOT_ATTEMPTED_NO_SCENE_TOKEN")<<"\"}";
  }
  o<<'}';return o.str();
 }
};
}
