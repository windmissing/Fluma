import p013renderer from 'libp013renderer.so';
import { P011ClearWaterEngine } from '../physics/clearwater/P011ClearWaterEngine';
import { ClearWaterPublication } from '../physics/clearwater/P011ClearWaterPublication';
import { ClearWaterSourceController } from '../physics/clearwater/P011ClearWaterSourceController';
import hilog from '@ohos.hilog';
import { ClearWaterGravityPort } from '../physics/clearwater/P011ClearWaterGravityPort';
import { ClearWaterStepResult,ClearWaterStatus as S } from '../physics/clearwater/P011ClearWaterTypes';
import { ClearWaterBAttachResult } from '../physics/clearwater/P011ClearWaterBTypes';
import { clearWaterCConfig,clearWaterCBConfig } from '../physics/clearwater/P011ClearWaterCConfig';
import { P051GravityInput } from '../physics/P051GravityInputAdapter';
import { ClearWaterNative,ClearWaterNativeFrame,ClearWaterNativeReceipt } from './P013ClearWaterTypes';
export class P013ClearWaterController {
 readonly engine:P011ClearWaterEngine;readonly publication:ClearWaterPublication;readonly source:ClearWaterSourceController;readonly gravity:ClearWaterGravityPort=new ClearWaterGravityPort();readonly frame:ClearWaterNativeFrame=new ClearWaterNativeFrame();readonly step:ClearWaterStepResult=new ClearWaterStepResult();
 readonly counters:Float64Array=new Float64Array(32);readonly trace:Float64Array=new Float64Array(256*24);private traceRows:number=0;
 status:string='WAIT_SURFACE';lastReceipt:ClearWaterNativeReceipt|null=null;private native:ClearWaterNative|null=null;private active:boolean=false;private serial:number=0;private surface:number=0;private context:number=0;private width:number=0;private height:number=0;
 constructor(){const cfg=clearWaterCConfig();this.frame.configDigest=JSON.stringify(cfg)+';'+JSON.stringify(clearWaterCBConfig());this.engine=new P011ClearWaterEngine(cfg);const attach=new ClearWaterBAttachResult();if(this.engine.attachBInto(clearWaterCBConfig(),attach)!==S.OK)throw new Error('CW_B_ATTACH_'+attach.status);this.publication=new ClearWaterPublication(cfg.particleCapacity);this.source=new ClearWaterSourceController(this.engine,cfg.configRev,cfg.rateSand,cfg.rateWater);}
 onLoad():void {this.native=p013renderer as ClearWaterNative;this.frame.sessionId=0;}
 setActive(value:boolean):void {this.active=value;this.source.stop(value?'RESUME':'BACKGROUND');if(this.native!==null)this.native.setActive(value);}
 release():void {this.setActive(false);this.native?.release();this.native=null;}
 reset():void {this.source.reset();this.gravity.reset();}
 invalidateGeometry():void {this.source.stop('INPUT_GEOMETRY',this.engine.geometryEpoch+1);}
 tick(ns:number,nowMs:number,input:P051GravityInput|null,dpi:number):void {
  const n=this.native;if(!this.active||n===null)return;this.counters[0]++;
  let state=n.getState();if(state.state==='WAIT_WINDOW'){this.source.stop('NO_SURFACE');this.status='WAIT_SURFACE';return;}
  if(this.frame.sessionId===0){const init=n.init('p013_clear_water_surface');if(init.code!==0){this.status='INIT_'+init.code;return;}this.frame.sessionId=init.sessionId;}
  const ready=n.prepareDualFieldSurface(this.frame.sessionId,state.surfaceEpoch);if(ready.code!==0||ready.sizeSource!=='EGL_QUERY_SURFACE'){this.source.stop('SURFACE_INVALID');this.status='SURFACE_'+ready.code;return;}
  state=n.getState();if(state.surfaceEpoch!==ready.surfaceEpoch||state.contextEpoch!==ready.contextEpoch||state.widthPx!==ready.widthPx||state.heightPx!==ready.heightPx){this.source.stop('SURFACE_CHANGED');this.status='FAULT_SURFACE';return;}
  if(this.surface!==state.surfaceEpoch||this.context!==state.contextEpoch||this.width!==ready.widthPx||this.height!==ready.heightPx){this.invalidateGeometry();this.surface=state.surfaceEpoch;this.context=state.contextEpoch;this.width=ready.widthPx;this.height=ready.heightPx;}
  if(this.width<128||!Number.isFinite(dpi)||dpi<=0){this.source.stop('UNSUPPORTED_GEOMETRY');this.status='UNSUPPORTED_GEOMETRY';return;}
  let physicsFailure:number=0;
  const fresh=this.gravity.freeze(input,nowMs);
  if(!fresh){this.source.stop('GRAVITY_STALE');this.status='GRAVITY_STALE';}else {
   const tick=this.source.tick(ns,'NS','ARKUI_FRAME_MONOTONIC');
   if(tick){
    const frameStatus=this.engine.beginBFrameInto(++this.serial);if(frameStatus!==S.OK){this.status='FRAME_'+frameStatus;return;}
    try{
     this.counters[1]++;const sourceStatus=this.source.sample();this.counters[2]+=this.source.batch.processed;if(sourceStatus!==S.OK&&sourceStatus!==S.PARTIAL_CAPACITY){this.status='FAULT_SOURCE_'+sourceStatus;return;}
     const count=Math.ceil(this.source.clock.seconds/(1/60)),h=this.source.clock.seconds/count;
     const t=this.gravity.token;t.ownerId=this.engine.ownerId;t.runGeneration=this.engine.runGeneration;t.inputGeneration=this.engine.currentInputGeneration;t.configRev=1;t.tick=this.serial;
     for(let i=0;i<count;i++){const angle=this.gravity.trial(h,i),code=this.engine.advanceSubstepInto(h,angle,this.step);this.counters[3]++;this.counters[code===S.OK?4:5]++;this.gravity.complete(code===S.OK&&this.step.stepped,this.step.lastAcceptedCommit,this.step.stepReason);if(code!==S.OK){physicsFailure=code;this.status='PHYSICS_'+code;break;}}
    }finally{this.engine.endBFrameInto(this.serial);}
   }
  }
  this.counters[6]++;const capture=this.publication.capture(this.engine,1);if(capture!==S.OK){this.status='CAPTURE_'+capture;return;}
  const slot=this.publication.slots[this.publication.publishedIndex],f=this.frame;f.frameId++;f.surfaceEpoch=state.surfaceEpoch;f.contextEpoch=state.contextEpoch;f.surfaceWidthPx=this.width;f.surfaceHeightPx=this.height;f.boxXPx=0;f.boxYPx=0;f.boxWidthPx=this.width;f.boxHeightPx=this.height;f.dpi=dpi;f.frameTimeNs=ns;f.bind(slot,this.engine.runId);
  const r=n.drawClearWaterSceneV3(f,slot.profile64,slot.particleProof64,slot.xy32,slot.meta32,slot.optics64);this.lastReceipt=r;this.counters[7]++;this.counters[r.currentFrameSucceeded?8:9]++;
  if(!r.currentFrameSucceeded)hilog.error(0,'T252DBG','CW draw fail code=%{public}s stage=%{public}s sess=%{public}s ctx=%{public}s surf=%{public}s w=%{public}s h=%{public}s cnt=%{public}s cap=%{public}s tok=%{public}s',String(r.code),r.firstFailureStage,String(f.sessionId),String(f.contextEpoch),String(f.surfaceEpoch),String(f.surfaceWidthPx),String(f.surfaceHeightPx),String(f.particleCount),String(f.particleCapacity),String(f.frameToken.slice(0,40)));
  const row=(this.traceRows%256)*24,h=slot.header64;this.trace.fill(0,row,row+24);this.trace[row]=f.frameId;this.trace[row+1]=ns;this.trace[row+2]=h[0];this.trace[row+3]=h[1];this.trace[row+4]=h[2];this.trace[row+5]=h[5];this.trace[row+6]=h[6];this.trace[row+7]=h[10];this.trace[row+8]=h[11];this.trace[row+9]=h[15];this.trace[row+10]=h[18];this.trace[row+11]=f.surfaceEpoch;this.trace[row+12]=f.contextEpoch;this.trace[row+13]=this.width;this.trace[row+14]=this.height;this.trace[row+15]=r.code;this.trace[row+16]=r.currentFrameSucceeded?1:0;this.trace[row+17]=fresh?1:0;this.trace[row+18]=this.source.clock.seconds;this.trace[row+19]=this.source.feedbackCount;this.trace[row+20]=this.engine.pendingSourceCount;this.traceRows++;
  if(!r.currentFrameSucceeded)this.status='FAULT_'+r.firstFailureStage;else if(fresh)this.status=physicsFailure===0?'CURRENT':'CURRENT_CANONICAL; PHYSICS_FAILED_'+physicsFailure;
 }
}
