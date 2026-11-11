export interface RecoveryPort {
 recoveryState():string;newRun():string;
 beginRecoverySession(serial:string,epoch:string,lo:string,hi:string,page:string):string;
 pagePulse(session:string,sequence:string,ns:string):string;
}
interface RecoveryReceipt {resetSerial:string;epoch:string;runLo:number;runHi:number;phase:string;reason?:string;windowAvailable:boolean;}
// Each actual page frame owns a new instance. Stop is permanent, including
// callbacks already queued by ArkUI. No timer or generated heartbeat exists.
export class GpuRecoveryPulse {
 session:string='0';sequence:number=0;private ticket:string='';private stopped:boolean=false;
 private epoch:string='';private generation:string;
 private port:RecoveryPort;
 private terminal:string='';
 constructor(port:RecoveryPort,generation:string){this.port=port;this.generation=generation;}
 stop():void{this.stopped=true;}
 bindRequested():void{const r=JSON.parse(this.port.recoveryState()) as RecoveryReceipt;this.ticket=r.resetSerial;this.epoch=r.epoch;}
 pulse(ns:number):string{
  if(this.stopped)return this.terminal||'STALE_FRAME';
  if(!Number.isSafeInteger(ns)||ns<0)return 'PAGE_PULSE_INVALID';
  const r=JSON.parse(this.port.recoveryState()) as RecoveryReceipt;
  if(this.ticket===''){
   if(!r.windowAvailable)return 'WAIT_WINDOW';
   this.port.newRun();this.bindRequested();return 'RESET_REQUESTED';
  }
  if(r.resetSerial!==this.ticket||r.epoch!==this.epoch){this.stop();return 'STALE_RESET_RECEIPT';}
  if(this.session!=='0'&&r.phase==='FAULTED_REVOKED'){
   this.terminal=r.reason||'NEW_RUN_REQUIRED';this.stop();return this.terminal;
  }
  if(this.session==='0'){
   if(r.phase!=='WAIT_NEW_SESSION')return r.phase;
   this.session=this.port.beginRecoverySession(this.ticket,this.epoch,String(r.runLo),String(r.runHi),this.generation);
   if(this.session==='0')return 'SESSION_HANDOFF_REJECTED';
  }
  if(this.sequence===Number.MAX_SAFE_INTEGER){this.stop();return 'PAGE_SEQUENCE_OVERFLOW';}
  const code=this.port.pagePulse(this.session,String(++this.sequence),String(ns));
  if(code==='CLOCK_GAP'||code==='CONTROL_CLOCK_REVERSED'||code==='PULSE_CLOCK_REVERSED'){
   this.terminal=code;this.stop();
  }else if(code==='STALE_PAGE_SESSION'){
   const current=JSON.parse(this.port.recoveryState()) as RecoveryReceipt;
   this.terminal=current.resetSerial===this.ticket&&current.epoch===this.epoch&&current.phase==='FAULTED_REVOKED'?
    current.reason||'NEW_RUN_REQUIRED':code;this.stop();
  }
  return this.terminal||code;
 }
}
