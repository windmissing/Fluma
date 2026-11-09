import process from '@ohos.process';
import { common } from '@kit.AbilityKit';
import { GpuEvidenceCaptureCold, EvidenceCaptureReply, EvidenceArmConfig } from '../physics/GpuEvidenceCaptureCold';
import { GpuBridgeTimelineCold } from '../physics/GpuBridgeTimelineCold';
import { DiagnosticColdFiles } from '../physics/DiagnosticColdFiles';
import m1 from 'libgrsm1.so';
import sensor from '@ohos.sensor';
import systemDateTime from '@ohos.systemDateTime';
import { FrameCallback } from '@ohos.arkui.UIContext';
import { P051GravityConfig, P051GravityInputAdapter } from '../physics/P051GravityInputAdapter';
import { GpuM1DemoController } from '../physics/GpuM1DemoController';
class M1DemoFrame extends FrameCallback {
 owner:GpuResidentM1|null=null;session:string='0';sequence:number=0;
 onFrame(ns:number):void{
  if(this.owner===null||this.session==='0'||!Number.isSafeInteger(ns)||ns<0)return;
  const code:string=m1.pagePulse(this.session,String(++this.sequence),String(ns));
  this.owner.tick(ns,code);
 }
}
@Entry @Component
struct GpuResidentM1 {
 private readonly captureCold:GpuEvidenceCaptureCold=new GpuEvidenceCaptureCold();
 private captureRequestId:string='';private captureSha:string='';private archivedSha:string='';private archivedRequestId:string='';private archivedRequest:EvidenceCaptureReply|null=null;private capturePending:boolean=false;private captureExporting:boolean=false;
 @State private captureId:string='';@State private captureText:string='未采集';
 private armCapture():void {
  if(this.captureCold.busy||this.captureExporting)return;
  const s=this.model.snapshot;if(s===null){this.captureText='STATUS_FAULT';return;}
  const config:EvidenceArmConfig={packageSha256:this.packageSha.trim(),expectedRevision:s.progressClockRevision??'',epoch:String(s.epoch),runLo:s.runLo,runHi:s.runHi,pageSession:s.pageSession??'0',durationNs:'2000000000',maxRows:4096,maxBytes:1048576};
  const reply:EvidenceCaptureReply=JSON.parse(m1.beginEvidenceCapture(JSON.stringify(config))) as EvidenceCaptureReply;
  if(reply.state==='RESERVED'&&reply.requestId!==undefined){this.captureRequestId=reply.requestId;this.captureSha=config.packageSha256;}
  this.updateCapture(reply);
 }
 private updateCapture(reply:EvidenceCaptureReply):void {
  if(reply.state!=='REJECTED')this.capturePending=reply.state==='RESERVED';
  const label=reply.state+' '+(reply.reason||reply.lastBlocker||'')+' #'+(reply.requestId??'');
  if(label!==this.captureText)this.captureText=label;
  if((reply.state==='ARMED'||reply.state==='FROZEN')&&reply.captureId!==undefined&&reply.captureId!=='0'){
   this.captureId=reply.captureId;this.archivedSha=this.captureSha;this.archivedRequestId=reply.requestId??'';this.archivedRequest=reply;
  }
 }
 private cancelCaptureRequest():void {
  if(!this.captureRequestId)return;
  this.updateCapture(JSON.parse(m1.cancelEvidenceCaptureRequest(this.captureRequestId)) as EvidenceCaptureReply);
 }
 private async exportCapture():Promise<void> {
  if(this.captureCold.busy||this.captureExporting||!this.captureId||this.capturePending)return;
  this.captureExporting=true;
  this.captureText=m1.endEvidenceCapture(this.captureId);
  try{const context=getContext(this) as common.UIAbilityContext;const files=new DiagnosticColdFiles(context.filesDir);
   const request:string=m1.evidenceCaptureRequest(this.archivedRequestId);
   const r=JSON.parse(request) as EvidenceCaptureReply;
   const receipt:string=r.state==='FROZEN'&&r.captureId===this.captureId?request:JSON.stringify(this.archivedRequest);
   if(this.archivedRequest!==null)await files.save('gpu-capture-request-'+this.archivedRequestId+'-'+this.captureId+'.json',receipt);
   const result=await this.captureCold.collect(m1,files,this.captureId,this.archivedSha,process.pid,()=>new Promise<void>((resolve:()=>void)=>{setTimeout(resolve,0);}));this.captureText=result.state+' '+result.reason;}catch(e){this.captureText='INCOMPLETE '+String(e);}finally{this.captureExporting=false;}
 }
 private readonly cold:GpuBridgeTimelineCold=new GpuBridgeTimelineCold();
 @State private progressV2:boolean=false;
 @State private coldStatus:string='';@State private tailProbe:boolean=false;@State private coldTwoBatches:boolean=false;@State private packageSha:string='';
 private async exportTimeline():Promise<void> {
  if(this.cold.busy)return;this.coldStatus='导出中';
  try{const context=getContext(this) as common.UIAbilityContext;
   const out=await this.cold.collect(m1,new DiagnosticColdFiles(context.filesDir),{module:'libgrsm1.so',route:'GpuResidentM1',packageSha256:this.packageSha.trim(),pid:process.pid},()=>new Promise<void>((resolve:()=>void)=>{setTimeout(resolve,0);}));
   this.coldStatus=out.state+' '+out.reason;
  }catch(e){this.coldStatus='INCOMPLETE '+String(e);}
 }

 private readonly model:GpuM1DemoController=new GpuM1DemoController(m1);
 private lastTextNs:number=-1;private lastDiagnosticNs:number=-1;
 private frame:M1DemoFrame=new M1DemoFrame();private queued:boolean=false;private shown:boolean=false;private active:boolean=false;private sensorActive:boolean=false;
 private readonly sensorConfig:P051GravityConfig=new P051GravityConfig();private readonly adapter:P051GravityInputAdapter=new P051GravityInputAdapter(this.sensorConfig);
 @StorageLink('emptySandDemoForeground') @Watch('syncActive') private foreground:boolean=true;
 @State private stateText:string='等待窗口';@State private sourceText:string='沙';@State private receiptText:string='尚无准入';@State private gravityText:string='等待真实传感器';@State private memoryText:string='';
 @State private manual:boolean=false;@State private showOptics:boolean=false;@State private sigma:number=.03;@State private gain:number=.6;@State private widthPx:number=1;
 private readonly accelerometer=(data:sensor.AccelerometerResponse):void=>{if(!this.active||this.manual)return;const t=systemDateTime.getUptime(systemDateTime.TimeType.ACTIVE,true)/1000000;const s=this.adapter.consumeAccelerometer(data.x,data.y,data.z,t);if(s.valid&&s.targetTiltDeg!==null)this.model.sensor(s.targetTiltDeg,s.shakeInput);};
 aboutToAppear():void{this.frame.owner=this;}
 onPageShow():void{this.frame.owner=null;this.frame=new M1DemoFrame();this.frame.owner=this;this.queued=false;this.shown=true;this.syncActive();}
 onPageHide():void{this.captureCold.cancel();this.cold.cancel();this.shown=false;this.frame.owner=null;this.queued=false;this.syncActive();}
 aboutToDisappear():void{this.onPageHide();this.frame.owner=null;}
 private syncActive():void {const active=this.shown&&this.foreground;if(active===this.active)return;this.active=active;this.model.setActive(active);
  if(active){
   this.frame.owner=null;this.frame=new M1DemoFrame();this.frame.owner=this;
   this.frame.session=m1.beginPageSession();this.queued=false;
  }else if(this.frame.session!=='0'){m1.endPageSession(this.frame.session);this.frame.owner=null;}
  this.updateSensor();if(active)this.schedule();}
 private updateSensor():void {
  const need=this.active&&!this.manual;
  if(need&&!this.sensorActive){try{sensor.on(sensor.SensorId.ACCELEROMETER,this.accelerometer,{interval:this.sensorConfig.sensorIntervalNs});this.sensorActive=true;}catch(e){this.gravityText='传感器不可用，物理暂停';}}
  else if(!need&&this.sensorActive){try{sensor.off(sensor.SensorId.ACCELEROMETER,this.accelerometer);}catch(e){}this.sensorActive=false;this.adapter.reset();}
 }
 private schedule():void{if(this.active&&!this.queued){this.queued=true;this.getUIContext().postFrameCallback(this.frame);}}
 tick(ns:number,pulse:string):void {
  this.queued=false;if(!this.active)return;
  try{
   this.model.refresh();const s=this.model.snapshot;
   if(s===null||s.progressClockRevision===undefined){this.model.stop('UNSUPPORTED_PROGRESS_REVISION');}
   else if(pulse!=='PAGE_PULSE_ACCEPTED'&&s.progressClockRevision==='GRS_M1_PROGRESS_CLOCK_V2')this.model.stop(pulse);
   else this.model.tickSnapshot(ns);
   if(this.lastTextNs<0||ns<this.lastTextNs||ns-this.lastTextNs>=33333334){
    this.lastTextNs=ns;
    const capture=s?.captureRequest;
    if(capture!==undefined&&capture.requestId===this.captureRequestId&&!this.captureCold.busy&&!this.captureExporting)this.updateCapture(capture);
    else if(this.captureRequestId===''&&s?.captureStorageReady===false)this.captureText='CAPTURE_STORAGE_NOT_READY';
    const identity=s===null?'无快照':(s.progressClockRevision??'?')+' / run '+s.runHi+':'+s.runLo+' / epoch '+s.epoch+' / session '+(s.pageSession??'?')+' / sample '+(s.sampleSerial??'?');
    const stale=s===null||pulse!=='PAGE_PULSE_ACCEPTED'||!s.ready||s.state!=='CURRENT';
    this.stateText=(stale?'STALE · ':'CURRENT · ')+identity+' · '+(pulse==='PAGE_PULSE_ACCEPTED'?(s?.stage??s?.state??'STATUS_FAULT'):pulse);
    const counterIdentity=s===null?'':(stale?'STALE · ':'')+'账 '+(s.countersProgressRevision??'?')+' / run '+(s.countersRunHi??0)+':'+(s.countersRunLo??0)+' / epoch '+(s.countersEpoch??'?')+' / session '+(s.countersPageSession??'?')+' / sample '+(s.sampleSerial??'?');
    this.receiptText=s===null?'状态不可用':counterIdentity+' · 最近源收据#'+s.sourceRequestSeq+' '+(s.sourceAdmittedQ/1048576).toFixed(4)+' · 物理 '+s.physicsCommitHi+':'+s.physicsCommitLo;
    if(this.showOptics&&(this.lastDiagnosticNs<0||ns<this.lastDiagnosticNs||ns-this.lastDiagnosticNs>=250000000)){
     this.lastDiagnosticNs=ns;
     this.gravityText=s===null?'状态不可用':counterIdentity+' · 目标 '+(s.gravityTargetQ/256).toFixed(1)+'° / GPU '+(s.gravityAcceptedQ/256).toFixed(1)+'°';
     this.memoryText=s===null?'状态不可用':counterIdentity+' · W '+((s.settledHi*4294967296+s.settledLo)/1048576).toFixed(3)+' / J '+((s.flightHi*4294967296+s.flightLo)/1048576).toFixed(3)+' · GPU峰 '+(s.knownGpuBytesPeak/1048576).toFixed(2)+'MiB';
    }
   }
  }catch(e){this.model.stop('UI_FAULT');this.stateText='STALE · STATUS_FAULT';this.receiptText='状态不可用';}
  this.schedule();
 }
 private touch(e:TouchEvent):void{if(e.type===TouchType.Cancel){this.model.stop('CANCEL');return;}const points=e.changedTouches;if(points.length===0)return;for(let i=0;i<points.length;i++){const p=points[i];if(e.type===TouchType.Down)this.model.down(p.id,p.x,p.y);else if(e.type===TouchType.Move)this.model.move(p.id,p.x,p.y);else if(e.type===TouchType.Up)this.model.up(p.id);}}
 build(){Column({space:4}){
  Text('沙水 DEMO V4 · GPU M1').fontSize(18)
  Text(this.stateText).fontSize(11).height(32).maxLines(2)
  Text(this.receiptText).fontSize(10).height(30).maxLines(2)
  Row({space:8}){Button('沙').onClick(()=>{this.model.select(0);this.sourceText='沙';});Button('水').onClick(()=>{this.model.select(1);this.sourceText='水';});Button('停止').onClick(()=>this.model.stop('STOP'));Button('新空场').onClick(()=>this.model.reset());}
  Row({space:8}){Button('开始2秒同窗采集').onClick(()=>this.armCapture());Button('取消预约').onClick(()=>this.cancelCaptureRequest());Button('冻结并导出同窗').onClick(()=>this.exportCapture());}
  Text(this.captureText).fontSize(10).height(14).maxLines(1)
  Text('当前：'+this.sourceText+'；抬手停源，已有水沙继续运动。参数待定标。').fontSize(11)
  XComponent({id:'grs-m1',type:XComponentType.SURFACE,libraryname:'grsm1'}).width('100%').layoutWeight(1).onAreaChange((_o:Area,a:Area)=>this.model.geometry(Number(a.width),Number(a.height))).onTouch((e:TouchEvent)=>this.touch(e))
  Row({space:6}){Text('离线手动重力').fontSize(11);Toggle({type:ToggleType.Switch,isOn:this.manual}).onChange((v:boolean)=>{this.manual=v;this.model.gravityMode(!v);this.updateSensor();});Button('光学/诊断').onClick(()=>{this.model.stop('CONFIG');this.showOptics=!this.showOptics;});}
  if(this.manual){Slider({min:-30,max:30,value:0,step:1}).onChange((v:number)=>this.model.manualAngle(v));}
  if(this.showOptics){TextInput({placeholder:'诊断签包 SHA256（外部签包清单）',text:this.packageSha}).onChange((v:string)=>{this.packageSha=v;});Button(this.tailProbe?'下轮提交探针：开':'下轮提交探针：关').onClick(()=>{this.tailProbe=!this.tailProbe;this.coldStatus=m1.setTailProbe(this.tailProbe);});Button(this.coldTwoBatches?'下轮冷准备：两批':'下轮冷准备：T375').onClick(()=>{this.coldTwoBatches=!this.coldTwoBatches;this.coldStatus=m1.setColdTwoBatches(this.coldTwoBatches);});Button(this.progressV2?'下轮进度时钟：V2':'下轮进度时钟：V1').onClick(()=>{this.progressV2=!this.progressV2;this.coldStatus=m1.setProgressV2(this.progressV2);});Button('导出原生时间线').onClick(()=>this.exportTimeline());Button('取消导出').onClick(()=>this.cold.cancel());Text(this.coldStatus).fontSize(10).height(28).maxLines(2);Text(this.gravityText).fontSize(10).height(28).maxLines(2);Text(this.memoryText).fontSize(10).height(28).maxLines(2);Text('光学应用会新建空场；不热改当前库存。').fontSize(11);Text('透射 sigma '+this.sigma.toFixed(3)).fontSize(10);Slider({min:0,max:.12,value:this.sigma,step:.005}).onChange((v:number)=>{this.sigma=v;});Text('高光 '+this.gain.toFixed(2)+' / 宽度 '+this.widthPx.toFixed(1)+'px').fontSize(10);Slider({min:0,max:1,value:this.gain,step:.05}).onChange((v:number)=>{this.gain=v;});Slider({min:.5,max:8,value:this.widthPx,step:.5}).onChange((v:number)=>{this.widthPx=v;});Button('应用光学并新建空场').onClick(()=>this.model.applyOptics(this.sigma,this.gain,this.widthPx));Row(){Button('K192 / W128').onClick(()=>this.model.profile(192,128));Button('K512 / W128').onClick(()=>this.model.profile(512,128));}}
 }.width('100%').height('100%').padding({top:8,bottom:8});}
}
