// T56 production scene boundary. No Canvas renderer, no per-layer clock/prepare.
import nativeRenderer from 'libp013renderer.so';
import { P013SceneFrame,P013SceneNative,P013SceneReceipt } from './P013SceneTypes';
import { P013ScenePreparation } from './P013ScenePreparation';
import { P013LightParams } from './P013RenderTypes';
import { P012FrameContext,P012AppearanceParams,p012FactoryAppearance,validateP012AppearanceParams } from './P012RenderTypes';
import { P011PublishedRenderState } from '../physics/P011PublishedRenderState';
import { p013Sha256 } from './preparation/P013CanvasAssetExport';
import { P013BaseByteEvaluator,P013BaseStripDescriptor } from './preparation/P013BaseByteEvaluator';
export class P013SceneController {
  readonly light: P013LightParams=new P013LightParams();
  readonly preparation: P013ScenePreparation=new P013ScenePreparation();
  readonly frame: P013SceneFrame=new P013SceneFrame();
  lastReceipt: P013SceneReceipt|null=null;failure: string='NOT_READY';configurationFailure: string='';
  private native: P013SceneNative|null=null;private active: boolean=true;private loaded: boolean=false;
  private session: number=0;private serial: number=0;private revision: number=0;private generation: number=0;private requestedGeneration: number=0;
  private configuredContext: number=-1;private configured: boolean=false;
  private values: Float64Array=new Float64Array(57);private previous: Float64Array=new Float64Array(57);
  private configuredSurface: number=-1;private lastLogNs: number=0;
  private baseEvaluator: P013BaseByteEvaluator=new P013BaseByteEvaluator();
  private configBindingId: number=0;private configDigest: string='';
  constructor(native: P013SceneNative|null=null){this.native=native;this.loaded=native!==null;}
  onLoad(): void {this.native=nativeRenderer as P013SceneNative;this.loaded=true;this.session=0;this.configured=false;if(!this.active)this.native.setActive(false);}
  setActive(active: boolean): void {this.active=active;if(this.loaded)this.native?.setActive(active);}
  release(): void {if(this.loaded)this.native?.release();this.loaded=false;this.native=null;this.session=0;this.configured=false;}
  drawNewPipelineFrame(state: P011PublishedRenderState,context: P012FrameContext,app: P012AppearanceParams=p012FactoryAppearance(),pipeline: number=0,timeNs: number=0,scale: number=1): P013SceneReceipt|null {
    const native=this.native;if(native===null||!this.loaded||!this.active){this.failure='NOT_READY';return null;}
    try {
      if(this.session===0){const init=native.init('p013_lmat_surface');if(init.code!==0){this.failure='INIT_'+init.code;return null;}this.session=init.sessionId;this.serial=this.revision=this.generation=0;this.configured=false;}
      let target=native.getState();const prepared=native.prepareDualFieldSurface(this.session,target.surfaceEpoch);
      if(prepared.code!==0||prepared.sizeSource!=='EGL_QUERY_SURFACE'||prepared.widthPx<1||prepared.heightPx<1){this.failure='PREPARE_'+prepared.code+'_'+prepared.errorStage;return null;}
      target=native.getState();if(prepared.surfaceEpoch!==target.surfaceEpoch||prepared.contextEpoch!==target.contextEpoch){this.failure='STALE_PREPARED_SURFACE';return null;}
      if(!Number.isFinite(scale)||scale<=0){this.failure='SCENE_SCALE';return null;}
      const appearanceError=validateP012AppearanceParams(app,context.boxH/state.field.capacity);if(appearanceError!==null){this.failure='SCENE_APPEARANCE_'+appearanceError;return null;}
      const v=this.values;v.fill(0);v[0]=pipeline;v[1]=context.appearanceTier;v[2]=context.ablation;v[3]=context.renderSeed;v[4]=context.geometryEpoch;
      v[6]=context.boxX;v[7]=context.boxY;v[8]=context.boxW;v[9]=context.boxH;v[10]=scale;v[11]=app.fogDensityScale;v[12]=app.clearMottleContrast;
      const px=context.boxH/state.field.capacity;v[14]=256/Math.max(1,Math.ceil(256/(app.strataLambdaMaterial*px)));v[13]=v[14]/px;v[15]=.25;
      v[16]=context.ablation!==1&&context.ablation!==7?1:0;v[17]=context.ablation!==4&&context.ablation!==7?1:0;v[18]=context.ablation!==3&&context.ablation!==7?1:0;v[19]=scale;
      v.set([145/255,168/255,111/255,216/255,196/255,150/255,181/255,182/255,131/255],20);v.set([145/255,168/255,111/255,216/255,196/255,150/255],29);
      v.set([this.light.la,this.light.ld,this.light.lmin,this.light.lmax,this.light.lref,this.light.ellLightMaterial,this.light.lightX,this.light.lightY,this.light.lightZ,this.light.sigmaCandidate],35);
      v[45]=app.grainFeaturePx;v[46]=app.grainSigma;v[47]=app.strataStrengthScale;v[48]=app.strataLambdaMaterial;v[49]=app.configRevision;v[50]=pipeline===0?20261020:20261026;
      if(pipeline!==0)v.set([145,168,111,216,196,150],51);
      let changed=!this.configured;for(let i=0;i<57;i++)if(i!==5&&v[i]!==this.previous[i])changed=true;
      const restore=this.configuredContext!==target.contextEpoch||this.configuredSurface!==target.surfaceEpoch;
      if(changed||restore){
        if(changed)this.revision++;v[5]=this.revision;
        const digest=p013Sha256(new Uint8Array(v.buffer));
        const binding=native.bindMaterialConfiguration({sessionId:this.session,appearanceConfigRevision:this.revision,configurationDigest:digest});
        if(binding.code!==0){this.failure='CONFIG_BIND_'+binding.code;return null;}
        const registered=native.registerSceneConfiguration(binding,v);if(registered.code!==0){this.failure='SCENE_CONFIG_'+registered.code;return null;}
        this.configBindingId=binding.bindingId;this.configDigest=digest;
        this.configurationFailure='';this.requestedGeneration=0;
        if(pipeline!==0){
          const assets=this.preparation.numeric.build(app,px,context.geometryEpoch,context.renderSeed);const d=assets.desc;
          d.apiVersion=2;d.sessionId=this.session;d.surfaceEpoch=target.surfaceEpoch;d.contextEpoch=target.contextEpoch;
          d.appearanceConfigRevision=this.revision;d.assetGeneration=++this.generation;d.materialBundleId='scene-'+this.session+'-'+this.generation;this.requestedGeneration=d.assetGeneration;
          const stage=native.stageMaterialBundle(d,assets.grain,assets.strata);if(stage.code!==0)this.configurationFailure='STAGE_'+stage.code+'_'+stage.errorStage;
        }
        this.previous.set(v);this.configured=true;this.configuredContext=target.contextEpoch;this.configuredSurface=target.surfaceEpoch;
      }
      const f=this.frame;f.sessionId=this.session;f.frameId=++this.serial;f.snapshotId=state.physicsCommitId;f.surfaceEpoch=target.surfaceEpoch;f.contextEpoch=target.contextEpoch;
      f.fieldEpoch=context.fieldEpoch;f.geometryEpoch=context.geometryEpoch;f.continuityEpoch=context.continuityEpoch;f.appearanceConfigRevision=this.revision;f.lightConfigRevision=this.revision;f.frameTimeNs=timeNs;
      f.surfaceWidthPx=prepared.widthPx;f.surfaceHeightPx=prepared.heightPx;f.boxXPx=context.boxX*scale;f.boxYPx=context.boxY*scale;f.boxWidthPx=context.boxW*scale;f.boxHeightPx=context.boxH*scale;
      if(context.boxX===0&&context.boxY===0){f.boxWidthPx=prepared.widthPx;f.boxHeightPx=prepared.heightPx;}
      f.capacity=state.field.capacity;f.visualInverted=context.visualInverted;
      f.stateEpoch=state.stateEpoch;f.physicsCommitId=state.physicsCommitId;f.runId=state.runId;f.ledgerCommitKey=state.ledgerCommitKey===null?'':JSON.stringify(state.ledgerCommitKey);f.routeId=state.routeId ?? 'column-field';f.mediumMode=state.mediumMode;
      f.frameToken=state.runId+':'+state.stateEpoch+':'+state.physicsCommitId+':'+f.frameId;f.requestedGeneration=this.requestedGeneration;f.particleLayerEnabled=context.particleLayerEnabled!==false;
      f.sourceFailure=this.preparation.pack(state,context);if(f.sourceFailure==='OK')f.sourceFailure='';
      f.particleCount=this.preparation.count;f.particleCapacity=this.preparation.capacity;
      f.scenePacketRevision=pipeline===0?'P013_SCENE_PACKET_V1':'P013_SCENE_PACKET_V2_BASE_STRIP';
      if(pipeline!==0&&f.sourceFailure===''){
        const evaluator=this.baseEvaluator,profile=this.preparation.data;
        if(!evaluator.evaluate(prepared.widthPx,0,f.boxXPx,f.boxWidthPx,f.visualInverted,v[18]!==0,profile)){
          f.sourceFailure='BASE_REPRESENTATION_UNSUPPORTED';
        }else{
          const strip=new P013BaseStripDescriptor();strip.sessionId=this.session;strip.stripSerial=f.frameId;
          strip.frameToken=f.frameToken;strip.runId=f.runId;strip.stateEpoch=f.stateEpoch;strip.physicsCommitId=f.physicsCommitId;
          strip.configBindingId=this.configBindingId;strip.configDigest=this.configDigest;
          strip.geometryEpoch=f.geometryEpoch;strip.contextEpoch=f.contextEpoch;strip.surfaceEpoch=f.surfaceEpoch;
          strip.viewportOriginX=0;strip.viewportOriginY=0;strip.width=prepared.widthPx;strip.height=prepared.heightPx;
          strip.boxXPx=f.boxXPx;strip.boxWidthPx=f.boxWidthPx;strip.mirror=f.visualInverted;strip.sparkEnabled=v[18]!==0;
          evaluator.populateDescriptor(strip,profile);f.baseStripSerial=strip.stripSerial;
          f.baseStripProfileHash=strip.profileSha256;f.baseStripPayloadHash=strip.payloadSha256;
          const staged=native.stageBaseByteStrip(strip,evaluator.bytes);
          if(staged.code!==0)f.sourceFailure='BASE_STAGE_'+staged.errorStage;
          else {const pumped=native.pumpBaseByteStripUploads(65536);
            if(pumped.code!==0)f.sourceFailure='BASE_PUMP_'+pumped.errorStage;
            else if(this.configurationFailure===''){const spent=pumped.baseStripUploadedBytes-staged.baseStripUploadedBytes;
              const staticBudget=65536-spent;if(staticBudget>0){const pump=native.pumpMaterialUploads(staticBudget);
                if(pump.code!==0&&pump.code!==401)this.configurationFailure='PUMP_'+pump.code+'_'+pump.errorStage;}}
          }
        }
      }
      const receipt=pipeline===0?native.drawNewPipelineFrame(f,this.preparation.data,this.preparation.xy,this.preparation.meta,this.preparation.motion):
        native.drawNewPipelineFrameV2(f,this.preparation.data,this.preparation.xy,this.preparation.meta,this.preparation.motion);
      this.lastReceipt=receipt;this.failure=receipt.currentFrameSucceeded?'':receipt.firstFailureStage;
      if(timeNs-this.lastLogNs>=1e9||receipt.presentationState!=='CURRENT'){this.lastLogNs=timeNs;console.info('WS_EVENT p013_scene '+JSON.stringify({run:state.runId,commit:state.physicsCommitId,frame:f.frameId,request:receipt.requestedFrameToken,actual:receipt.lastGoodFrameToken,state:receipt.presentationState,first:receipt.firstFailureStage,swap:receipt.requestedSwapSucceeded,staleSwap:receipt.staleSwapSucceeded,generation:receipt.actualPresentationGeneration,config:receipt.renderedConfigRevision,desiredConfig:this.revision,configurationFailure:this.configurationFailure}));}
      return receipt;
    }catch(e){this.failure='SCENE_EXCEPTION';console.error('P013 scene preparation exception '+e);return null;}
  }
}
