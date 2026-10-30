// T228 / GRS_M1_DEMO_UI_V4. Input protocol only, no CPU inventory or angle acceptance.
export interface GpuM1DemoPort {
 status():string;offer(command:ArrayBuffer):string;setSource(kind:number):string;
 setActive(active:boolean):string;setGravityMode(sensor:boolean):string;
 setGravitySample(rawDegrees:number,shake:number,sequence:number):string;
 setAngle(index:number):string;newRun():string;configureM1(k:number,kw:number,features:number,revision:number):string;
 configureOptics(optics:Float32Array):string;
}
export interface GpuM1DemoStatus {
 state:string;stage?:string;ready:boolean;permissionState:string;epoch:number;readyEpoch:number;
 runLo:number;runHi:number;geometryLo:number;geometryHi:number;inputLo:number;inputHi:number;configLo:number;configHi:number;
 sourceUiRevision:string;sourceOpen:boolean;sourceKind:number;sourceControlLo:number;sourceControlHi:number;
 sourceRequestSeq:number;sourceReceiptStatus:number;sourceAdmittedQ:number;sourceRejectedQ:number;
 sourceCommitLo:number;sourceCommitHi:number;physicsCommitLo:number;physicsCommitHi:number;
 gravityRaw:number;gravityTargetQ:number;gravityAcceptedQ:number;shakeDiagnostic:number;
 settledLo:number;settledHi:number;flightLo:number;flightHi:number;waterTotalLo:number;waterTotalHi:number;
 gpuBufferAllocatedBytes:number;frameTargetBytes:number;knownGpuBytesPeak:number;
}
export class GpuM1DemoController {
 readonly revision:string='GRS_M1_DEMO_UI_V4';readonly calibration:string='PENDING_CALIBRATION';
 readonly payloadBytes:number=192; // command128+cold optics64; not JS heap/RSS
 readonly optics:Float32Array=new Float32Array([.03,.04,.13,.18,.6,1,1,0,1,.8,.9,1,.02,16,.6,.8]);
 private readonly command:ArrayBuffer=new ArrayBuffer(128);private readonly words:Uint32Array;private readonly raw:DataView;
 private active:boolean=false;private pointer:number=-1;private x:number=0;private y:number=0;private lastNs:number=-1;
 private width:number=0;private height:number=0;private mirror:boolean=false;private sequence:number=0;private sampleSequence:number=0;
 private runLo:number=0;private runHi:number=0;private geometryLo:number=0;private geometryHi:number=0;
 kind:number=0;sensorMode:boolean=true;rate:number=8; // units/s candidate, not a speed law
 snapshot:GpuM1DemoStatus|null=null;message:string='等待 GPU M1';lastEnqueue:string='NONE';
 private readonly port:GpuM1DemoPort;
 constructor(port:GpuM1DemoPort){this.port=port;this.words=new Uint32Array(this.command);this.raw=new DataView(this.command);}
 private word(v:number):boolean{return Number.isSafeInteger(v)&&v>=0&&v<=4294967295;}
 private allowed(s:GpuM1DemoStatus):boolean{return s.sourceUiRevision===this.revision&&s.ready&&s.permissionState==='READY'&&Number.isSafeInteger(s.epoch)&&s.epoch>0&&s.epoch===s.readyEpoch&&this.word(s.runLo)&&this.word(s.runHi)&&this.word(s.inputLo)&&this.word(s.inputHi)&&this.word(s.geometryLo)&&this.word(s.geometryHi)&&this.word(s.configLo)&&this.word(s.configHi)&&this.word(s.sourceControlLo)&&this.word(s.sourceControlHi);}
 refresh():void {
  try {
   const s:GpuM1DemoStatus=JSON.parse(this.port.status()) as GpuM1DemoStatus;
   if(s.sourceUiRevision!==this.revision){this.stop('UNSUPPORTED_UI_REVISION');this.snapshot=null;return;}
   if(this.runLo!==s.runLo||this.runHi!==s.runHi){this.stop('NEW_EMPTY_RUN');this.sequence=0;this.runLo=s.runLo;this.runHi=s.runHi;}
   if(this.geometryLo!==s.geometryLo||this.geometryHi!==s.geometryHi){this.stop('GEOMETRY_CHANGED');this.geometryLo=s.geometryLo;this.geometryHi=s.geometryHi;}
   this.snapshot=s;if(!this.allowed(s)&&this.pointer>=0)this.stop(s.stage??s.permissionState);
  }catch(e){this.stop('STATUS_FAULT');this.snapshot=null;}
 }
 setActive(value:boolean):void {if(this.active===value)return;this.stop(value?'RESUME_NEEDS_DOWN':'BACKGROUND');this.active=value;if(value)this.port.setGravityMode(this.sensorMode);this.port.setActive(value);}
 select(kind:number):void {if(kind!==0&&kind!==1)return;this.stop('SWITCH_NEEDS_DOWN');this.kind=kind;}
 stop(reason:string):void {this.pointer=-1;this.lastNs=-1;this.port.setSource(-1);this.message=reason;}
 geometry(width:number,height:number,mirror:boolean=false):void {if(this.width===width&&this.height===height&&this.mirror===mirror)return;this.stop('GEOMETRY_CHANGED');this.width=width;this.height=height;this.mirror=mirror;}
 private position(x:number,y:number):boolean {
  if(!Number.isFinite(x)||!Number.isFinite(y)||!Number.isFinite(this.width)||!Number.isFinite(this.height)||this.width<=0||this.height<=0)return false;
  const u=x/this.width,v=y/this.height;this.x=(this.mirror?1-u:u)*128;this.y=(this.mirror?v:1-v)*256;
  return this.x>=0&&this.x<128&&this.y>=0&&this.y<=256;
 }
 down(id:number,x:number,y:number):boolean {this.refresh();if(!this.active||this.pointer>=0||this.snapshot===null||!this.allowed(this.snapshot)||!Number.isSafeInteger(id)||id<0||!this.position(x,y))return false;this.pointer=id;this.lastNs=-1;this.port.setSource(this.kind);return true;}
 move(id:number,x:number,y:number):void {if(id!==this.pointer)return;if(!this.position(x,y))this.stop('OUTSIDE_BOX');}
 up(id:number):void {if(id===this.pointer)this.stop('UP');}
 tick(rawNs:number):void {
  this.refresh();const s=this.snapshot;if(!this.active||this.pointer<0||s===null)return;
  if(!this.allowed(s)||!s.sourceOpen||s.sourceKind!==this.kind){this.lastNs=-1;return;}
  if(!Number.isSafeInteger(rawNs)||rawNs<0){this.stop('INVALID_CLOCK');return;}
  if(this.lastNs<0){this.lastNs=rawNs;return;}
  const delta=rawNs-this.lastNs;this.lastNs=rawNs;
  if(delta===0)return;if(delta<0||delta>33333333){this.stop('CLOCK_GAP_NO_CATCHUP');return;}
  if(!Number.isFinite(this.rate)||this.rate<=0||this.rate>300){this.stop('INVALID_SOURCE_RATE');return;}
  if(this.sequence===4294967295){this.stop('REQUEST_ID_EXHAUSTED');return;}
  const w=this.words;w.fill(0);w[0]=1129534023;w[1]=2;w[2]=this.kind;w[4]=s.runLo;w[5]=s.runHi;w[6]=++this.sequence;
  w[8]=s.inputLo;w[9]=s.inputHi;w[10]=s.sourceControlLo;w[11]=s.sourceControlHi;w[12]=s.geometryLo;w[13]=s.geometryHi;
  w[14]=this.kind+1;w[15]=this.sequence;this.raw.setFloat64(64,this.x,true);this.raw.setFloat64(72,this.y,true);this.raw.setFloat64(80,this.rate*delta/1000000000,true);
  w[22]=rawNs%4294967296;w[23]=Math.floor(rawNs/4294967296);w[24]=this.sequence;w[26]=1;w[27]=1;w[28]=s.configLo;w[29]=s.configHi;
  this.lastEnqueue=this.port.offer(this.command);this.message=this.lastEnqueue;
  if(this.lastEnqueue!=='ENQUEUED_NOT_ACK')this.stop(this.lastEnqueue); // no backlog/retry
 }
 sensor(raw:number,shake:number):string {if(!this.active||!this.sensorMode||!Number.isFinite(raw)||!Number.isFinite(shake)||this.sampleSequence===4294967295)return 'INVALID_SENSOR';return this.port.setGravitySample(raw,shake,++this.sampleSequence);}
 gravityMode(sensor:boolean):void {this.stop('GRAVITY_MODE_NEEDS_DOWN');this.sensorMode=sensor;this.port.setGravityMode(sensor);}
 manualAngle(degrees:number):void {if(this.sensorMode||!Number.isFinite(degrees)||Math.abs(degrees)>30)return;this.port.setAngle(Math.round(degrees*256));}
 reset():void {this.stop('RESET_NEW_EMPTY_RUN');this.port.newRun();this.snapshot=null;}
 profile(k:number,kw:number):void {this.stop('PROFILE_NEW_EMPTY_RUN');this.port.configureM1(k,kw,3,2);this.snapshot=null;}
 applyOptics(sigma:number,gain:number,width:number):string {
  if(!Number.isFinite(sigma)||sigma<0||!Number.isFinite(gain)||gain<0||gain>1||!Number.isFinite(width)||width<=0||width>8)return 'INVALID_OPTICS';
  this.stop('OPTICS_NEW_EMPTY_RUN');this.optics[0]=sigma;this.optics[4]=gain;this.optics[8]=width;this.snapshot=null;return this.port.configureOptics(this.optics);
 }
}
