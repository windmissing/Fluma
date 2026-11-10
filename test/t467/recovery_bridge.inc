 recovery467::Transaction recovery;
 void RecoveryNote(const char* kind,uint32_t role,uint64_t a=0,uint64_t b=0,uint64_t c=0,uint32_t code=0){
  recovery.Note({kind,Stamp(),BridgeTid(),0,0,0,permissionRevision,permit.nonce,a,b,c,role,code});
 }
 void RecoveryRequestLocked(bool explicitReset){
  // Revoke and freeze before clearing run counters. Old callbacks stay closed.
  RecoveryNote("OLD_PERMIT_TOMBSTONE",1,permit.epoch,(uint64_t(identity.run.hi)<<32)|identity.run.lo,permit.nonce);
  FreezeEvidenceLocked("RUN_RESET");
#ifdef GRS_PROGRESS_V2_EXPERIMENT
  CancelCaptureScopeLocked("RUN_RESET",Stamp());
#endif
  progress.Revoke("RUN_RESET");
  recovery.Archive({recovery.epoch,progress.session,progress.generated,progress.accepted,progress.overload,progress.revoked,progress.clearedCarry});
  BridgeState::RevokeLocked("WAIT_RETIRED");jobLive=false;permit={};v2ResourcesReady=false;
  if(explicitReset&&active&&window&&recovery.Request(epoch,window)){
   requiresReset=false;phase=permission="WAIT_RETIRED";RecoveryNote("RESET_REQUESTED",1);
  }else{
   recovery.Fault(explicitReset?"WAIT_WINDOW_OR_ACTIVE":"WINDOW_OR_PAGE_CHANGED");requiresReset=true;
  }
 }
 uint64_t RecoverySessionLocked(uint64_t serial,uint64_t e,U64 run,uint64_t page){
  if(!CurrentLocked(e,window)||!page||!recovery.CanSession(serial,e,run,window))return 0;
  if(!progress.BeginSession()){recovery.Fault("SESSION_OVERFLOW");requiresReset=true;return 0;}
  if(!recovery.Bind(progress.session,page)){progress.Revoke("SESSION_HANDOFF_FAILED");return 0;}
  RecoveryNote("SESSION_HANDOFF",1,page,serial,e);return progress.session;
 }
 void RecoveryJson(std::ostream& out,bool full){
  std::ostringstream state;recovery.Json(state,full);auto value=state.str();value.pop_back();
  out<<value<<",\"windowAvailable\":"<<(window&&active?"true":"false")<<",\"actualClock\":\""<<(progressV2?"GRS_M1_PROGRESS_CLOCK_V2":"GRS_M1_PROGRESS_CLOCK_V1")<<"\",\"readyEpoch\":\""<<readyEpoch<<"\"}";
 }
