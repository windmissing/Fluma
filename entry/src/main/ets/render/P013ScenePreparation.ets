// T56 single bounded preparation: pure numerical providers, one publication token.
import { P011PublishedRenderState } from '../physics/P011PublishedRenderState';
import { P012FrameContext } from './P012RenderTypes';
import { P013DualFieldAdapter,P013DualFieldUploadSlot } from './P013DualFieldAdapter';
import { P013_COLUMN_PROJECTION } from './P013ParticleProjection';
import { P013MaterialDataPreparation } from './preparation/P013MaterialDataPreparation';
import { P013SceneNumericAssets } from './preparation/P013SceneNumericAssets';
import { P012FrameSlot,P012MaterialFrameView } from './P012WaterSandAdapter';
import { P012MaterialMotionCache } from './P012MaterialMotionCache';
import { P013MaterialRejectionStore, P013MaterialRejectionV1 } from './P013MaterialRejection';
import { P013SceneFrame } from './P013SceneTypes';
export class P013ScenePreparation {
  readonly materialRejection: P013MaterialRejectionStore = new P013MaterialRejectionStore();
  private diagnosticFrame: P013SceneFrame | null = null; private diagnosticPipeline: number = 0; private diagnosticScale: number = 1;
  private diagnosticBinding: number = 0; private diagnosticDigest: string = '';
  setDiagnosticFrame(frame: P013SceneFrame, pipeline: number, scale: number, binding: number, digest: string): void {
    this.diagnosticFrame=frame;this.diagnosticPipeline=pipeline;this.diagnosticScale=scale;this.diagnosticBinding=binding;this.diagnosticDigest=digest;
  }
  getLastMaterialRejection(): P013MaterialRejectionV1 | null {return this.materialRejection.exportFrozen();}
  readonly data: Float32Array=new Float32Array(2048);readonly motion: Float64Array=new Float64Array(10);
  readonly numeric: P013SceneNumericAssets=new P013SceneNumericAssets();
  private material: P013MaterialDataPreparation=new P013MaterialDataPreparation();
  private adapter: P013DualFieldAdapter=new P013DualFieldAdapter();
  private slot: P013DualFieldUploadSlot=new P013DualFieldUploadSlot(1);
  private warpH: Float64Array=new Float64Array(128);private warpL: Float64Array=new Float64Array(128);private warpM: Float64Array=new Float64Array(128);private spark: Float64Array=new Float64Array(128);
  private current: P012FrameSlot=new P012FrameSlot(128);private previous: P012FrameSlot=new P012FrameSlot(128);
  private motionCache: P012MaterialMotionCache=new P012MaterialMotionCache(128);
  private run: string='';private epoch: number=0;private geometry: number=0;private continuity: number=0;private commit: number=0;private sampleMs: number=0;private frameSerial: number=0;
  pairValid: boolean=false;
  get xy(): Float32Array{return this.slot.particleXY;}get meta(): Uint32Array{return this.slot.particleMeta;}
  get count(): number{return this.slot.frame.particleCount;}get capacity(): number{return this.slot.capacity;}
  pack(state: P011PublishedRenderState,context: P012FrameContext): string {
    this.materialRejection.begin(state.runId);
    if(state.particleRouteActive){
      if(this.slot.capacity!==state.particleCapacity)this.slot=new P013DualFieldUploadSlot(state.particleCapacity);
      const result=this.adapter.packInto(state,context,this.slot,P013_COLUMN_PROJECTION,false);if(result!=='OK')return result;
    }else{
      if(!state.runId||state.field.physicsCommitId!==state.physicsCommitId||context.physicsCommitId!==state.physicsCommitId)return 'PUBLICATION_MISMATCH';
      if(this.slot.capacity!==1)this.slot=new P013DualFieldUploadSlot(1);this.slot.frame.particleCount=0;
    }
    const f=state.field,cur=this.current,prev=this.previous;let totalH=0,totalL=0,compH=0,compL=0;
    for(let c=0;c<128;c++){
      cur.massH[c]=f.bh[c]+f.mh[c]+f.suspH[c];cur.massL[c]=f.bl[c]+f.ml[c]+f.suspL[c];const yH=cur.massH[c]-compH,tH=totalH+yH;compH=(tH-totalH)-yH;totalH=tH;
      const yL=cur.massL[c]-compL,tL=totalL+yL;compL=(tL-totalL)-yL;totalL=tL;
    }
    this.pairValid=this.run===state.runId&&this.epoch===state.stateEpoch&&this.geometry===context.geometryEpoch&&this.continuity===context.continuityEpoch&&
      state.physicsCommitId>this.commit&&context.frameSerial===this.frameSerial+1&&context.sampleTimeMs>this.sampleMs&&context.sampleTimeMs-this.sampleMs<=100&&state.publicationKind==='STEP'&&
      Math.abs(totalH-prev.totalH)<=64*128*Number.EPSILON*Math.max(1,prev.totalH+totalH)&&Math.abs(totalL-prev.totalL)<=64*128*Number.EPSILON*Math.max(1,prev.totalL+totalL);
    // Original field mass proxy cannot infer pool<->field transfers. Freeze on nonconservation; never manufacture velocity.
    if(this.run!==state.runId||this.epoch!==state.stateEpoch||this.geometry!==context.geometryEpoch||this.continuity!==context.continuityEpoch)this.motionCache.reset();
    if(state.physicsCommitId!==this.commit||this.run!==state.runId)this.motionCache.update(new P012MaterialFrameView(prev),new P012MaterialFrameView(cur),this.pairValid,context.boxW/128,256);
    const offH=this.motionCache.meanDeltaXPx(true),offL=this.motionCache.meanDeltaXPx(false),offM=(offH+offL)*.5;
    this.motion[0]=offH;this.motion[1]=offL;this.motion[2]=offM;this.motion[9]=this.motionCache.globalPhasePx(true);
    for(let c=0;c<128;c++){
      const x=context.boxX+(c+.5)*context.boxW/128;
      this.warpH[c]=this.numeric.warp(x,context.boxX,context.boxW/128,offH);this.warpL[c]=this.numeric.warp(x,context.boxX,context.boxW/128,offL);this.warpM[c]=this.numeric.warp(x,context.boxX,context.boxW/128,offM);this.spark[c]=this.numeric.gain(c);
    }
    try{this.material.pack(f,context.boxW,context.boxH,this.warpH,this.warpL,this.warpM,this.spark);}catch(e){
      try {
        const frame=this.diagnosticFrame;
        if(frame!==null){
          const summary=this.materialRejection.summary(state,context,frame,this.diagnosticPipeline,this.diagnosticScale,this.diagnosticBinding,this.diagnosticDigest);
          summary.sourceFailure='SCENE_MATERIAL_DATA';this.materialRejection.observe(summary);
          const capture=this.materialRejection.needsCapture(),point=this.material.failurePoint;
          const name=e instanceof Error?e.name:null,message=e instanceof Error?e.message:null;
          if(point.reason==='')point.fail('UNEXPECTED_EXCEPTION','MATERIAL_EXCEPTION',-1,-1,-1,0);
          this.materialRejection.capture(summary,point,name,message);
          if(capture){this.materialRejection.capturePublication(state);this.material.captureDiagnostic(this.materialRejection);
            this.materialRejection.copy(14,this.warpH);this.materialRejection.copy(15,this.warpL);this.materialRejection.copy(16,this.warpM);this.materialRejection.copy(17,this.spark);this.materialRejection.copy(18,this.motion);}
        }
      }catch(diagnosticError){}return 'SCENE_MATERIAL_DATA';}
    this.data.set(this.material.data);const min:number[]=[Number.POSITIVE_INFINITY,Number.POSITIVE_INFINITY,Number.POSITIVE_INFINITY],max:number[]=[-1,-1,-1];
    for(let c=0;c<128;c++){
      const sand=this.data[c*4],water=this.data[c*4+1],bh=this.data[c*4+2],mix=this.data[c*4+3];
      const heights:number[]=[bh,mix,sand],bottoms:number[]=[0,bh,mix];
      for(let k=0;k<3;k++){min[k]=Math.min(min[k],context.boxY+(1-heights[k]/f.capacity)*context.boxH);max[k]=Math.max(max[k],context.boxY+(1-bottoms[k]/f.capacity)*context.boxH);}
      // Preserve the signed legacy q helper: no unapproved concentration change during migration.
      const depth=sand-water;this.data[1536+c*4]=depth>1e-6?Math.max(0,Math.min(1,f.suspH[c]/depth)):0;
      this.data[1536+c*4+1]=depth>1e-6?Math.max(0,Math.min(1,f.suspL[c]/depth)):0;this.data[1536+c*4+2]=this.data[1536+c*4+3]=0;
    }
    for(let k=0;k<3;k++){this.motion[k+3]=min[k];this.motion[k+6]=max[k];}
    prev.massH.set(cur.massH);prev.massL.set(cur.massL);prev.totalH=totalH;prev.totalL=totalL;
    this.run=state.runId;this.epoch=state.stateEpoch;this.geometry=context.geometryEpoch;this.continuity=context.continuityEpoch;this.commit=state.physicsCommitId;this.sampleMs=context.sampleTimeMs;this.frameSerial=context.frameSerial;
    try{if(this.diagnosticFrame!==null){const summary=this.materialRejection.summary(state,context,this.diagnosticFrame,this.diagnosticPipeline,this.diagnosticScale,this.diagnosticBinding,this.diagnosticDigest);summary.sourceFailure='';this.materialRejection.observe(summary);}}catch(e){}
    return 'OK';
  }
}
