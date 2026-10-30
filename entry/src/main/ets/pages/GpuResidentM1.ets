import process from '@ohos.process';
import { common } from '@kit.AbilityKit';
import { GpuBridgeTimelineCold } from '../physics/GpuBridgeTimelineCold';
import { DiagnosticColdFiles } from '../physics/DiagnosticColdFiles';
import m1 from 'libgrsm1.so';
import sensor from '@ohos.sensor';
import systemDateTime from '@ohos.systemDateTime';
import { FrameCallback } from '@ohos.arkui.UIContext';
import { P051GravityConfig, P051GravityInputAdapter } from '../physics/P051GravityInputAdapter';
import { GpuM1DemoController } from '../physics/GpuM1DemoController';
class M1DemoFrame extends FrameCallback {owner:GpuResidentM1|null=null;onFrame(ns:number):void{this.owner?.tick(ns);}}
@Entry @Component
struct GpuResidentM1 {
 private readonly cold:GpuBridgeTimelineCold=new GpuBridgeTimelineCold();
 @State private coldStatus:string='';@State private packageSha:string='';
 private async exportTimeline():Promise<void> {
  if(this.cold.busy)return;this.coldStatus='导出中';
  try{const context=getContext(this) as common.UIAbilityContext;
   const out=await this.cold.collect(m1,new DiagnosticColdFiles(context.filesDir),{module:'libgrsm1.so',route:'GpuResidentM1',packageSha256:this.packageSha.trim(),pid:process.pid},()=>new Promise<void>((resolve:()=>void)=>{setTimeout(resolve,0);}));
   this.coldStatus=out.state+' '+out.reason;
  }catch(e){this.coldStatus='INCOMPLETE '+String(e);}
 }

 private readonly model:GpuM1DemoController=new GpuM1DemoController(m1);
 private frame:M1DemoFrame=new M1DemoFrame();private queued:boolean=false;private shown:boolean=false;private active:boolean=false;private sensorActive:boolean=false;
 private readonly sensorConfig:P051GravityConfig=new P051GravityConfig();private readonly adapter:P051GravityInputAdapter=new P051GravityInputAdapter(this.sensorConfig);
 @StorageLink('emptySandDemoForeground') @Watch('syncActive') private foreground:boolean=true;
 @State private stateText:string='等待窗口';@State private sourceText:string='沙';@State private receiptText:string='尚无准入';@State private gravityText:string='等待真实传感器';@State private memoryText:string='';
 @State private manual:boolean=false;@State private showOptics:boolean=false;@State private sigma:number=.03;@State private gain:number=.6;@State private widthPx:number=1;
 private readonly accelerometer=(data:sensor.AccelerometerResponse):void=>{if(!this.active||this.manual)return;const t=systemDateTime.getUptime(systemDateTime.TimeType.ACTIVE,true)/1000000;const s=this.adapter.consumeAccelerometer(data.x,data.y,data.z,t);if(s.valid&&s.targetTiltDeg!==null)this.model.sensor(s.targetTiltDeg,s.shakeInput);};
 aboutToAppear():void{this.frame.owner=this;}
 onPageShow():void{this.frame.owner=null;this.frame=new M1DemoFrame();this.frame.owner=this;this.queued=false;this.shown=true;this.syncActive();}
 onPageHide():void{this.cold.cancel();this.shown=false;this.frame.owner=null;this.queued=false;this.syncActive();}
 aboutToDisappear():void{this.onPageHide();this.frame.owner=null;}
 private syncActive():void {const active=this.shown&&this.foreground;if(active===this.active)return;this.active=active;this.model.setActive(active);this.updateSensor();if(active)this.schedule();}
 private updateSensor():void {
  const need=this.active&&!this.manual;
  if(need&&!this.sensorActive){try{sensor.on(sensor.SensorId.ACCELEROMETER,this.accelerometer,{interval:this.sensorConfig.sensorIntervalNs});this.sensorActive=true;}catch(e){this.gravityText='传感器不可用，物理暂停';}}
  else if(!need&&this.sensorActive){try{sensor.off(sensor.SensorId.ACCELEROMETER,this.accelerometer);}catch(e){}this.sensorActive=false;this.adapter.reset();}
 }
 private schedule():void{if(this.active&&!this.queued){this.queued=true;this.getUIContext().postFrameCallback(this.frame);}}
 tick(ns:number):void {
  this.queued=false;if(!this.active)return;
  try{this.model.tick(ns);const s=this.model.snapshot;if(s!==null){
   this.stateText=s.state==='CURRENT'?'画面已更新':(s.state==='STALE'?'物理暂停，保留上一画面':(s.state==='FAULT'?'仿真故障：'+(s.stage??'未知'):'等待仿真就绪'));
   this.receiptText='准入 '+(s.sourceAdmittedQ/1048576).toFixed(4)+' · '+(this.model.lastEnqueue==='ENQUEUED_NOT_ACK'?'请求已排队，准入另计':'按住画面持续添加');
   this.gravityText='目标 '+(s.gravityTargetQ/256).toFixed(1)+'° / GPU接受 '+(s.gravityAcceptedQ/256).toFixed(1)+'° · 摇晃仅观测';
   this.memoryText='W '+((s.settledHi*4294967296+s.settledLo)/1048576).toFixed(3)+' / J '+((s.flightHi*4294967296+s.flightLo)/1048576).toFixed(3)+' · 已知GPU峰值 '+(s.knownGpuBytesPeak/1048576).toFixed(2)+'MiB · 提交 '+s.physicsCommitHi+':'+s.physicsCommitLo;
  }}catch(e){this.model.stop('UI_FAULT');this.stateText='界面故障，已停止新源';}this.schedule();
 }
 private touch(e:TouchEvent):void{if(e.type===TouchType.Cancel){this.model.stop('CANCEL');return;}const points=e.changedTouches;if(points.length===0)return;for(let i=0;i<points.length;i++){const p=points[i];if(e.type===TouchType.Down)this.model.down(p.id,p.x,p.y);else if(e.type===TouchType.Move)this.model.move(p.id,p.x,p.y);else if(e.type===TouchType.Up)this.model.up(p.id);}}
 build(){Column({space:4}){
  Text('沙水 DEMO V4 · GPU M1').fontSize(18)
  Text(this.stateText).fontSize(11).maxLines(2)
  Text(this.receiptText).fontSize(10).maxLines(2)
  Row({space:8}){Button('沙').onClick(()=>{this.model.select(0);this.sourceText='沙';});Button('水').onClick(()=>{this.model.select(1);this.sourceText='水';});Button('停止').onClick(()=>this.model.stop('STOP'));Button('新空场').onClick(()=>this.model.reset());}
  Text('当前：'+this.sourceText+'；抬手停源，已有水沙继续运动。参数待定标。').fontSize(11)
  XComponent({id:'grs-m1',type:XComponentType.SURFACE,libraryname:'grsm1'}).width('100%').layoutWeight(1).onAreaChange((_o:Area,a:Area)=>this.model.geometry(Number(a.width),Number(a.height))).onTouch((e:TouchEvent)=>this.touch(e))
  Row({space:6}){Text('离线手动重力').fontSize(11);Toggle({type:ToggleType.Switch,isOn:this.manual}).onChange((v:boolean)=>{this.manual=v;this.model.gravityMode(!v);this.updateSensor();});Button('光学/诊断').onClick(()=>{this.model.stop('CONFIG');this.showOptics=!this.showOptics;});}
  if(this.manual){Slider({min:-30,max:30,value:0,step:1}).onChange((v:number)=>this.model.manualAngle(v));}
  if(this.showOptics){TextInput({placeholder:'诊断签包 SHA256（外部签包清单）',text:this.packageSha}).onChange((v:string)=>{this.packageSha=v;});Button('导出原生时间线').onClick(()=>this.exportTimeline());Button('取消导出').onClick(()=>this.cold.cancel());Text(this.coldStatus).fontSize(10);Text(this.gravityText).fontSize(10);Text(this.memoryText).fontSize(10);Text('光学应用会新建空场；不热改当前库存。').fontSize(11);Text('透射 sigma '+this.sigma.toFixed(3)).fontSize(10);Slider({min:0,max:.12,value:this.sigma,step:.005}).onChange((v:number)=>{this.sigma=v;});Text('高光 '+this.gain.toFixed(2)+' / 宽度 '+this.widthPx.toFixed(1)+'px').fontSize(10);Slider({min:0,max:1,value:this.gain,step:.05}).onChange((v:number)=>{this.gain=v;});Slider({min:.5,max:8,value:this.widthPx,step:.5}).onChange((v:number)=>{this.widthPx=v;});Button('应用光学并新建空场').onClick(()=>this.model.applyOptics(this.sigma,this.gain,this.widthPx));Row(){Button('K192 / W128').onClick(()=>this.model.profile(192,128));Button('K512 / W128').onClick(()=>this.model.profile(512,128));}}
 }.width('100%').height('100%').padding({top:8,bottom:8});}
}
