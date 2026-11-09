import { p013NowMs } from './P013RenderController';

export class CaptureReply {
 revision:string='T413_CAPTURE_REPLY_V2';state:string='REJECTED';reason:string='';
 attemptId:string='';holdId:string='';requestId:number=0;phaseSequence:number=0;
}
export class CaptureGeometry {
 revision:string='T413_GEOMETRY_V2';globalX:number=0;globalY:number=0;widthVp:number=0;heightVp:number=0;
 vp2px:number=0;geometry:number=0;visible:boolean=false;configuration:string='';
}
export class HoldStatus {
 revision:string='T413_HOLD_STATUS_V2';state:string='IDLE';reason:string='';attemptId:string='';holdId:string='';
 requestId:number=0;phaseSequence:number=0;token:string='';frame:number=0;run:string='';owner:string='';
 geometry:CaptureGeometry=new CaptureGeometry();fixtureKey:string='';roi:number[]=[];maxHoldMs:number=0;
 startMonoMs:number=0;endMonoMs:number=0;remainingMs:number=0;archiveState:string='NATIVE_ARCHIVE_PENDING';
 manifestSha256:string='';terminalSha256:string='';actionSerial:number=0;clockId:string='OHOS_ACTIVE_NS_DIV_1E6';
 bootSession:string='';endFrame:number=0;endToken:string='';displayStatus:string='DISPLAY_NOT_JUDGABLE';
}
export class ClockSample {
 revision:string='T413_CLOCK_SAMPLE_V2';clockId:string='';bootSession:string='';attemptId:string='';holdId:string='';
 sampleSerial:number=0;appMonoMs:number=0;unit:string='ms';continuousForeground:boolean=true;
}
// Cold isolation transaction. No timer may turn expiry into a successful seal.
export class T413Capture {
 private serial:number=0;private sampleSerial:number=0;
 private readonly session:string='app-'+Date.now()+'-'+p013NowMs();
 private current:HoldStatus=new HoldStatus();private sealText:string='';private failureText:string='';
 private currentGeometry:CaptureGeometry=new CaptureGeometry();
 reply(reason:string='',state:string='REJECTED'):CaptureReply {
  const s=this.current,r=new CaptureReply();r.state=state;r.reason=reason;r.attemptId=s.attemptId;
  r.holdId=s.holdId;r.requestId=s.requestId;r.phaseSequence=s.phaseSequence;return r;
 }
 configurable():boolean {return this.current.state==='IDLE'||this.current.state==='RELEASED';}
 blocksFrames():boolean {return !this.configurable()&&this.current.state!=='RESERVED';}
 geometry():CaptureGeometry {return JSON.parse(JSON.stringify(this.currentGeometry)) as CaptureGeometry;}
 setGeometry(g:CaptureGeometry):void {
  if(JSON.stringify(g)!==JSON.stringify(this.currentGeometry))this.invalidate('GEOMETRY_CHANGED');
  this.currentGeometry=JSON.parse(JSON.stringify(g)) as CaptureGeometry;
 }
 canReserve(roi:Int32Array,maxHoldMs:number,width:number,height:number):string {
  if(!this.configurable())return 'HOLD_ACTIVE';
  if(this.serial>=Number.MAX_SAFE_INTEGER)return 'ATTEMPT_ID_EXHAUSTED';
  if(!Number.isSafeInteger(maxHoldMs)||maxHoldMs<1||maxHoldMs>300000)return 'MAX_HOLD_INVALID';
  const g=this.currentGeometry;
  if(!g.visible||g.geometry<1||!Number.isFinite(g.globalX)||!Number.isFinite(g.globalY)||
    !Number.isFinite(g.widthVp)||!Number.isFinite(g.heightVp)||!Number.isFinite(g.vp2px)||
    g.widthVp<=0||g.heightVp<=0||g.vp2px<=0||width<1||height<1)return 'GEOMETRY_UNSTABLE';
  if(roi.length!==12)return 'THREE_ROIS_REQUIRED';
  let bytes=0;
  for(let i=0;i<roi.length;i+=4){const x=roi[i],y=roi[i+1],w=roi[i+2],h=roi[i+3];
   if(x<0||y<0||w<1||h<1||w>64||h>64||x+w>width||y+h>height)return 'ROI_INVALID';bytes+=w*h*8;}
  return bytes>98304?'ROI_BUDGET':'';
 }
 reserve(roi:Int32Array,fixtureKey:string,maxHoldMs:number,requestId:number,sequence:number):CaptureReply {
  const s=new HoldStatus();s.state='RESERVED';s.attemptId=this.session+'-a'+(++this.serial);
  s.bootSession=this.session;s.requestId=requestId;s.phaseSequence=sequence;s.fixtureKey=fixtureKey;
  s.roi=Array.from(roi);s.maxHoldMs=maxHoldMs;s.geometry=this.geometry();s.actionSerial=this.current.actionSerial+1;
  this.current=s;this.sealText='';this.failureText='';return this.reply('','RESERVED');
 }
 open(request:number,sequence:number,token:string,frame:number,run:string,owner:string,start:number):void {
  const s=this.current;if(s.state!=='RESERVED')return;
  if(request!==s.requestId||sequence!==s.phaseSequence||!token||!owner||!run){this.invalidate('CAPTURE_IDENTITY');return;}
  s.holdId=s.attemptId+'-h1';s.state='HOLD_OPEN';s.token=token;s.frame=frame;s.run=run;s.owner=owner;
  s.startMonoMs=start;s.remainingMs=s.maxHoldMs;s.actionSerial++;
 }
 invalidate(reason:string):void {
  const s=this.current;if(s.state!=='RESERVED'&&s.state!=='HOLD_OPEN'&&s.state!=='SEALED')return;
  if(s.state==='SEALED'){// The first seal remains immutable; later owner loss prevents release as success.
   s.state='INVALID';s.reason=reason;s.actionSerial++;s.terminalSha256='';return;
  }
  s.state='INVALID';s.reason=reason;s.endMonoMs=p013NowMs();s.endFrame=s.frame;s.endToken=s.token;s.actionSerial++;
 }
 status():HoldStatus {
  const s=this.current,now=p013NowMs();
  if(s.state==='HOLD_OPEN'){
   if(!Number.isFinite(now)||now<s.startMonoMs){this.invalidate('CLOCK_ROLLBACK');}
   else if(now-s.startMonoMs>=s.maxHoldMs){s.state='EXPIRED';s.reason='MAX_HOLD_EXPIRED';s.endMonoMs=now;s.endFrame=s.frame;s.endToken=s.token;s.actionSerial++;}
   s.remainingMs=Math.max(0,s.maxHoldMs-(now-s.startMonoMs));
  }
  return JSON.parse(JSON.stringify(s)) as HoldStatus;
 }
 sample(id:string):ClockSample|null {
  const s=this.status();if(s.state!=='HOLD_OPEN'||id!==s.holdId||this.sampleSerial>=Number.MAX_SAFE_INTEGER)return null;
  const c=new ClockSample();c.clockId=s.clockId;c.bootSession=s.bootSession;c.attemptId=s.attemptId;c.holdId=s.holdId;
  c.sampleSerial=++this.sampleSerial;c.appMonoMs=p013NowMs();return c;
 }
 seal(id:string):CaptureReply {
  const s=this.status();if(id!==s.holdId||!id)return this.reply('HOLD_IDENTITY');
  if(this.sealText.length>0&&s.state==='SEALED')return this.reply('','SEALED');
  if(s.state!=='HOLD_OPEN')return this.reply(s.reason||'HOLD_NOT_OPEN');
  this.current.state='SEALED';this.current.endMonoMs=p013NowMs();this.current.endFrame=s.frame;this.current.endToken=s.token;this.current.actionSerial++;
  this.sealText=JSON.stringify({revision:'T413_DISPLAY_HOLD_V2',valid:true,firstFailure:'',hold:this.current,
   geometry:this.current.geometry,startMonoMs:s.startMonoMs,endMonoMs:this.current.endMonoMs,endFrame:s.frame,endToken:s.token});
  return this.reply('','SEALED');
 }
 terminal():string {
  const s=this.status();if(s.state==='SEALED')return this.sealText;
  if(s.state!=='INVALID'&&s.state!=='EXPIRED')return '';
  if(this.failureText)return this.failureText;
  this.failureText=JSON.stringify({revision:'T413_DISPLAY_HOLD_V2',valid:false,firstFailure:s.reason,hold:s,
   geometry:s.geometry,startMonoMs:s.startMonoMs,endMonoMs:s.endMonoMs,endFrame:s.endFrame,endToken:s.endToken});return this.failureText;
 }
 ack(attempt:string,request:number,sha:string):CaptureReply {
  const s=this.status();if(attempt!==s.attemptId||request!==s.requestId||!attempt)return this.reply('ARCHIVE_IDENTITY');
  if(s.state==='RESERVED'||this.configurable()||!/^[a-f0-9]{64}$/.test(sha))return this.reply('ARCHIVE_NOT_READY');
  if(s.archiveState==='NATIVE_ARCHIVE_SAVED')return this.reply(s.manifestSha256===sha?'':'ARCHIVE_HASH_CONFLICT',s.manifestSha256===sha?s.state:'REJECTED');
  this.current.archiveState='NATIVE_ARCHIVE_SAVED';this.current.manifestSha256=sha;this.current.actionSerial++;
  return this.reply('',s.state);
 }
 savedTerminal(attempt:string,hold:string,sha:string):CaptureReply {
  const s=this.status();if(s.attemptId!==attempt||s.holdId!==hold||!attempt||!this.terminal()||!/^[a-f0-9]{64}$/.test(sha))return this.reply('TERMINAL_IDENTITY');
  if(s.terminalSha256)return this.reply(s.terminalSha256===sha?'':'TERMINAL_HASH_CONFLICT',s.terminalSha256===sha?s.state:'REJECTED');
  this.current.terminalSha256=sha;this.current.actionSerial++;return this.reply('',s.state);
 }
 cancel(attempt:string,reason:string):CaptureReply {
  if(attempt!==this.current.attemptId||!attempt)return this.reply('ATTEMPT_IDENTITY');
  if(this.current.state==='INVALID'||this.current.state==='EXPIRED')return this.reply('',this.current.state);
  if(this.current.state!=='RESERVED'&&this.current.state!=='HOLD_OPEN')return this.reply('CANCEL_STATE');
  this.invalidate(reason||'EXPLICIT_CANCEL');return this.reply('',this.current.state);
 }
 release(attempt:string,hold:string):CaptureReply {
  const s=this.status();if(attempt!==s.attemptId||hold!==s.holdId||!attempt)return this.reply('RELEASE_IDENTITY');
  if(s.state==='RELEASED')return this.reply('','RELEASED');
  if((s.state!=='SEALED'&&s.state!=='INVALID'&&s.state!=='EXPIRED')||!s.terminalSha256||
    (s.requestId>0&&s.archiveState!=='NATIVE_ARCHIVE_SAVED'))return this.reply('ARCHIVE_OR_TERMINAL_PENDING');
  this.current.state='RELEASED';this.current.actionSerial++;return this.reply('','RELEASED');
 }
}
