import sensor from '@ohos.sensor';
import hilog from '@ohos.hilog';
import { P051GravityConfig,P051GravityInputAdapter } from '../physics/P051GravityInputAdapter';
import { p013NowMs } from '../render/P013RenderController';
import { FrameCallback } from '@ohos.arkui.UIContext';
import { P051DerivedDemoV2 } from '../physics/derived/P051DerivedDemoV2';
import { DerivedPublishedSlotV2 } from '../physics/derived/P051DerivedFacadeV2';
import { P013SceneController } from '../render/P013SceneController';
import { P012FrameContext,P012AppearanceTier,P012Ablation,P012FillBox } from '../render/P012RenderTypes';
import { P011ParticleSnapshot } from '../physics/P011ParticleField';
class DerivedFrameV2 extends FrameCallback {owner:P051DerivedV2Demo|null=null;onFrame(ns:number):void{this.owner?.tick(ns);}}
@Entry
@Component
struct P051DerivedV2Demo {
 private readonly model=new P051DerivedDemoV2();private readonly renderer=new P013SceneController();private readonly callback=new DerivedFrameV2();private publication:DerivedPublishedSlotV2=this.model.createPublicationCold();private previous=0;private serial=0;private active=false;private queued=false;private surfaceWidth:number=320;private surfaceHeight:number=480;private stepDbgFrames:number=0;
 private readonly gravityConfig:P051GravityConfig=new P051GravityConfig();private readonly gravity:P051GravityInputAdapter=new P051GravityInputAdapter(this.gravityConfig);private sensorActive:boolean=false;
 private readonly accelerometer=(data:sensor.AccelerometerResponse):void=>{if(this.active)this.gravity.consumeAccelerometer(data.x,data.y,data.z,p013NowMs());};
 private updateSensor():void{if(this.active&&!this.sensorActive){try{sensor.on(sensor.SensorId.ACCELEROMETER,this.accelerometer,{interval:this.gravityConfig.sensorIntervalNs});this.sensorActive=true;}catch(e){this.status='传感器不可用';}}else if(!this.active&&this.sensorActive){try{sensor.off(sensor.SensorId.ACCELEROMETER,this.accelerometer);}catch(e){}this.sensorActive=false;this.gravity.reset();}}
 @State private status:string='等待新管线';
 aboutToAppear():void{this.callback.owner=this;}
 onPageShow():void{this.active=true;this.model.setActive(true);this.renderer.setActive(true);this.updateSensor();this.schedule();}
 onPageHide():void{this.active=false;this.model.setActive(false);this.renderer.setActive(false);this.updateSensor();this.previous=0;}
 aboutToDisappear():void{this.onPageHide();this.callback.owner=null;this.renderer.release();}
 private schedule():void{if(this.active&&!this.queued){this.queued=true;this.getUIContext().postFrameCallback(this.callback);}}
 tick(ns:number):void{this.queued=false;if(!this.active)return;const dt=this.previous===0?0:Math.min(1/30,(ns-this.previous)/1e9);this.previous=ns;
  const input=this.gravity.latest(p013NowMs());if(input!==null)this.model.applyDefinedInput(input);
  if(!this.model.tick(ns,dt)){this.status=this.model.status;this.schedule();return;}const copy=this.model.copyPublicationInto(this.publication);if(copy!=='OK'){this.status=copy;this.schedule();return;}
  const p=this.publication,c:P012FrameContext={physicsCommitId:p.physicsCommitId,particleLayerEnabled:true,fieldEpoch:p.stateEpoch,continuityEpoch:1,geometryEpoch:1,frameSerial:++this.serial,sampleTimeMs:ns/1e6,frameDeltaMs:dt*1000,inputMode:'EXPERIENCE',sourceKind:'gesture',physicalRouteId:'dual-field-particle-v1',runIdentity:p.runId,boxX:0,boxY:0,boxW:this.surfaceWidth,boxH:this.surfaceHeight,pixelRatio:1,visualInverted:false,appearanceTier:P012AppearanceTier.OFF,renderSeed:51,configRevision:1,ablation:P012Ablation.OFF,fillBox:P012FillBox.TIGHT,canonicalEvidence:false,gravityDirX:Math.sin(p.effectiveTiltDeg*Math.PI/180),gravityDirY:Math.cos(p.effectiveTiltDeg*Math.PI/180),sandTracerEnabled:false};
  const receipt=p.drawNewPipeline(this.renderer,c,ns,this.getUIContext().vp2px(1));this.status=receipt?.currentFrameSucceeded?'运行中':receipt?.presentationState==='STALE'?'上一有效帧（STALE）':'绘制故障：'+this.renderer.failure;
  // T347 forensic instrumentation (tester, registered in test-report-T347): T345 §3 device
  // readout — every 120 frames emit pYmin/pYmean/live/motion + bed sums from the publication
  // payload cold copy. Observation only; no physics/audit semantics touched.
  this.stepDbgFrames++;
  if(this.stepDbgFrames>=120){this.stepDbgFrames=0;const cp=p.exportPayloadCold();const auth=cp.authority;const ps:P011ParticleSnapshot|null=auth.particleState??null;if(ps!==null){let live=0,pYmin=Infinity,pYsum=0,motion=0;for(let i=0;i<ps.nmax;i++)if(ps.pState[i]!==0){live++;pYmin=Math.min(pYmin,ps.pY[i]);pYsum+=ps.pY[i];if(Math.abs(ps.pVy[i])>0)motion++;}let mh=0,ml=0;for(let cc=0;cc<128;cc++){mh+=auth.mh[cc];ml+=auth.ml[cc];}hilog.info(0,'T347STEP','frame=%{public}s commit=%{public}s live=%{public}s pYmin=%{public}s pYmean=%{public}s motion=%{public}s mh=%{public}s ml=%{public}s',String(this.serial),String(p.physicsCommitId),String(live),String(pYmin===Infinity?-1:pYmin),String(live>0?pYsum/live:-1),String(motion),String(mh),String(ml));}}
  this.schedule();
 }
 private touch(event:TouchEvent):void{if(event.type===TouchType.Up||event.type===TouchType.Cancel){this.model.stop();return;}if(event.touches.length<1)return;const p=event.touches[0],x=p.x/this.surfaceWidth*128,y=(1-p.y/this.surfaceHeight)*256;if(event.type===TouchType.Down)this.model.start(x,y);else this.model.move(x,y);}
 build(){
  Column({space:8}) {
    Text('空容器落沙 · 数值V2隔离').fontSize(20)
    Text(this.status).fontSize(12)
    Row({space:8}) {
      Button('左倾').onClick(()=>this.model.setTilt(-15))
      Button('正向').onClick(()=>this.model.setTilt(0))
      Button('右倾').onClick(()=>this.model.setTilt(15))
      Button('重置').onClick(()=>{this.model.reset();this.publication=this.model.createPublicationCold();})
    }
    Stack() {
      XComponent({id:'p013_lmat_surface',type:XComponentType.TEXTURE,libraryname:'p013renderer'})
        .width('100%').height('100%').onLoad(()=>this.renderer.onLoad()).hitTestBehavior(HitTestMode.None)
      Stack(){}.width('100%').height('100%').onTouch((event:TouchEvent)=>this.touch(event))
    }.width('92%').layoutWeight(1).onAreaChange((_old:Area,area:Area)=>{this.surfaceWidth=Number(area.width);this.surfaceHeight=Number(area.height);})
  }.width('100%').height('100%').padding({top:16,bottom:24})
 }
}
