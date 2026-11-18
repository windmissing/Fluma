export class PrepareAttemptOwner {
 prepareId:number=0;pageGeneration:number=0;branch:number=0;
 windowId:number=0;windowGeneration:number=0;controller:number=0;session:number=0;
 surface:number=0;context:number=0;run:string='';
 same(other:PrepareAttemptOwner):boolean {
  return this.prepareId===other.prepareId&&this.pageGeneration===other.pageGeneration&&this.branch===other.branch&&
   this.windowId===other.windowId&&this.windowGeneration===other.windowGeneration&&
   this.controller===other.controller&&this.session===other.session&&this.surface===other.surface&&
   this.context===other.context&&this.run===other.run;
 }
 text():string {return JSON.stringify(this);}
}
export type PrepareChangeKind='SOFT'|'HARD';
export class PrepareApiRecordV4 {
 api:string='';startMonoMs:number=0;endMonoMs:number=0;prepareId:number=0;
 code:number=0;name:string='';message:string='';runStatus:string='';
}
export class PrepareWitnessRecord {
 monoMs:number=0;callback:string='';reason:string='';kind:PrepareChangeKind='HARD';
 state:string='';subphase:string='';inputSerial:number=0;observationSerial:number=0;
 before:PrepareAttemptOwner=new PrepareAttemptOwner();after:PrepareAttemptOwner=new PrepareAttemptOwner();
}
export class PrepareCheckTicket {
 valid:boolean=false;prepareId:number=0;pageGeneration:number=0;
 observationSerial:number=0;inputSerial:number=0;serialText:string='';
}
// T570 (problem T570 §2-§3): the v4 prepare transaction — external PREPARING
// with an internal subphase, hard-owner reset on generation change, soft
// resampling before READY, synchronous check→reserve→consume-only-on-RESERVED
// and the first-invalidating witness. The strict 3000ms deadline from the
// user's begin is never restarted.
export class PrepareReadinessV4 {
 readonly revision:string='T570_PREPARE_TRANSACTION_V4';
 state:string='CONFIGURING';subphase:string='DISMISSING';reason:string='';active:boolean=false;
 pageGeneration:number=0;prepareId:number=0;configurationRevision:number=0;
 observationSerial:number=0;inputSerial:number=0;started:number=0;deadline:number=0;
 firstCause:string='';firstCauseSet:boolean=false;
 private witness:PrepareWitnessRecord[]=[];
 private witnessDropped:number=0;
 private witnessOverflow:boolean=false;
 private apiRecords:PrepareApiRecordV4[]=[];
 private currentOwner:PrepareAttemptOwner=new PrepareAttemptOwner();
 private bindingDone:boolean=false;
 private permitConsumed:boolean=false;
 begin(now:number,allowed:boolean,owner:PrepareAttemptOwner):boolean {
  if(!this.active||!allowed||this.state==='PREPARING')return false;
  this.prepareId=owner.prepareId;this.pageGeneration=owner.pageGeneration;
  this.started=now;this.deadline=now+3000;
  this.state='PREPARING';this.subphase='DISMISSING';this.reason='WAIT_OBSERVATIONS';
  this.witness=[];this.witnessDropped=0;this.witnessOverflow=false;
  this.apiRecords=[];this.currentOwner=owner;this.bindingDone=false;
  this.permitConsumed=false;this.firstCause='';this.firstCauseSet=false;
  return true;
 }
 private noteFirstCause(reason:string):void {
  if(!this.firstCauseSet){this.firstCause=reason;this.firstCauseSet=true;}
 }
 private record(kind:PrepareChangeKind,reason:string,before:PrepareAttemptOwner,after:PrepareAttemptOwner,nowMs:number,callback:string):void {
  if(this.witness.length>=64){this.witnessDropped++;this.witnessOverflow=true;return;}
  const r=new PrepareWitnessRecord();
  r.monoMs=nowMs;r.callback=callback;r.reason=reason;r.kind=kind;
  r.state=this.state;r.subphase=this.subphase;
  r.inputSerial=this.inputSerial;r.observationSerial=this.observationSerial;
  r.before=before;r.after=after;
  this.witness.push(r);
 }
 // noteChange: record-then-classify. Hard changes invalidate immediately;
 // soft changes before READY clear the stable candidate and re-sample.
 noteChange(reason:string,before:PrepareAttemptOwner,after:PrepareAttemptOwner,nowMs:number,callback:string):'SOFT'|'HARD' {
  const hard=!(before.same(after));
  const kind:PrepareChangeKind=hard?'HARD':'SOFT';
  this.observationSerial++;
  this.record(kind,reason,before,after,nowMs,callback);
  if(hard){
   if(this.state==='PREPARING'||this.state==='READY'){
    this.noteFirstCause(reason);
    this.state='INVALIDATED';this.reason=reason;
   }
   return 'HARD';
  }
  if(this.state==='PREPARING'){this.subphase='SETTLING';return 'SOFT';}
  if(this.state==='READY'){
   this.noteFirstCause(reason);
   this.state='INVALIDATED';this.reason=reason;
   return 'HARD';
  }
  return 'SOFT';
 }
 // The first valid complete owner in the same page/window binds once; later
 // hard owner changes terminate the attempt (never silently rebind).
 bindOwner(owner:PrepareAttemptOwner):boolean {
  if(this.state!=='PREPARING')return false;
  if(this.bindingDone)return this.currentOwner.same(owner);
  this.bindingDone=true;this.currentOwner=owner;this.subphase='SETTLING';
  return true;
 }
 sampleFrame(owner:PrepareAttemptOwner,nowMs:number,frameOk:boolean):boolean {
  if(this.state!=='PREPARING')return false;
  if(!this.currentOwner.same(owner)){this.noteChange('OWNER_CHANGED',this.currentOwner,owner,nowMs,'frame');return false;}
  if(!frameOk){this.subphase='VERIFY_PAIR';return false;}
  return true;
 }
 // Synchronous check: never consumes. The caller passes the same ticket to
 // consume() only after the existing capture.reserve returned RESERVED.
 checkForReserve(snapshotOk:boolean,nowMs:number):PrepareCheckTicket {
  const t=new PrepareCheckTicket();
  if(this.state!=='READY'){t.serialText=this.reason||'PREPARE_NOT_READY';return t;}
  if(nowMs-this.started>=3000){this.state='INVALIDATED';this.reason='PREPARE_TIMEOUT';t.serialText='PREPARE_TIMEOUT';return t;}
  if(!snapshotOk){this.state='INVALIDATED';this.reason='PREPARE_STALE';t.serialText='PREPARE_STALE';return t;}
  t.valid=true;t.prepareId=this.prepareId;t.pageGeneration=this.pageGeneration;
  t.observationSerial=this.observationSerial;t.inputSerial=this.inputSerial;
  t.serialText=this.prepareId+'|'+this.pageGeneration+'|'+this.observationSerial+'|'+this.inputSerial;
  return t;
 }
 consume(ticket:PrepareCheckTicket):boolean {
  if(!ticket.valid||!this.stateIsReady())return false;
  this.permitConsumed=true;this.state='INVALIDATED';this.reason='PREPARE_PERMIT_CONSUMED';
  return true;
 }
 private stateIsReady():boolean {return this.state==='READY'&&!this.permitConsumed;}
 ready():void {
  if(this.state==='PREPARING')this.state='READY';
 }
 elapsed(now:number):boolean {
  if(this.state!=='PREPARING')return true;
  if(!Number.isFinite(now)||now<this.started){this.fail('PREPARE_CLOCK_INVALID');return false;}
  if(now-this.started>=3000){this.noteFirstCause('PREPARE_TIMEOUT:'+this.reason);this.state='INVALIDATED';this.reason='PREPARE_TIMEOUT:'+this.reason;return false;}
  return true;
 }
 fail(reason:string):void {
  this.noteFirstCause(reason);
  this.state='FAILED';this.reason=reason;
 }
 report():string {
  return JSON.stringify({revision:this.revision,prepareId:this.prepareId,pageGeneration:this.pageGeneration,
   state:this.state,subphase:this.subphase,reason:this.reason,firstCause:this.firstCause,
   startedMonoMs:this.started,deadlineMonoMs:this.deadline,
   observationSerial:this.observationSerial,inputSerial:this.inputSerial,
   witnessRecords:this.witness.length,witnessDropped:this.witnessDropped,
   witnessOverflow:this.witnessOverflow,owner:this.currentOwner,
   api:this.apiRecords,witness:this.witness});
 }
}
