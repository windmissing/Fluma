import { rasterNodeReason } from './RasterGeometry';
import { T413Capture, CaptureGeometry, CaptureReply, HoldStatus, ClockSample } from './T413Capture';
import { WindowProbeEvidence, WindowProbeRequest, cancelledWindowProbe, parseWindowRoi } from './SingleSandWindowCold';
import { windowFixtureKey } from './RoiFixture';
import { p013NowMs } from './P013RenderController';
import { P051DryAdmissionReceipt } from '../physics/P051DryParticleAdmission';
import { T394Archive, T394Hold, T394Input, T394Phase, t394Geometry, t394DynamicRoi } from './T394Phase';
import nativeRenderer from 'libp013renderer.so';
import { P011PublishedRenderState } from '../physics/P011PublishedRenderState';
import { P012FrameContext } from './P012RenderTypes';
import { P013SingleSandPreparation } from './P013SingleSandPreparation';
import { SingleSandConfigV1,SingleSandGrainV1,SingleSandNative,SingleSandReceipt,SingleSandPacketV1 } from './P013SingleSandTypes';
import { SINGLE_GENERATOR_HASH,buildSingleSandGrain } from './preparation/P013SingleSandAssets';
import { p013Sha256 } from './preparation/P013CanvasAssetExport';

export interface PrepareNative {
 packetWidth:number;packetHeight:number;controller:number;session:number;surface:number;context:number;geometry:number;width:number;height:number;run:string;revision:number;ready:boolean;
}
export function singleSandConfigDigest(c:SingleSandConfigV1,values:Float64Array):string {
  const parts:string[]=[c.versions,String(c.revision),c.generatorHash,c.grainHash];
  let canonical='';
  for(const part of parts)canonical+=String(part.length)+':'+part;
  const data=new DataView(new ArrayBuffer(8));
  const hex='0123456789abcdef';
  for(let i=0;i<values.length;i++){
    data.setFloat64(0,values[i],true);
    for(let j=0;j<8;j++){const byte=data.getUint8(j);canonical+=hex.charAt(byte>>4)+hex.charAt(byte&15);}
  }
  const bytes=new Uint8Array(canonical.length);
  for(let i=0;i<bytes.length;i++)bytes[i]=canonical.charCodeAt(i);
  return p013Sha256(bytes);
}

export class P013SingleSandController {
  readonly preparation:P013SingleSandPreparation=new P013SingleSandPreparation();
  private readonly config:SingleSandConfigV1=new SingleSandConfigV1();
  private readonly desc:SingleSandGrainV1=new SingleSandGrainV1();
  private readonly values:Float64Array=new Float64Array([145,168,111,16,16,36,2,2,.08,.10,8,-.35,0,.82,.10,.16,.60,.95,20261022]);
  private readonly native:SingleSandNative;
  private pendingRoi:WindowProbeRequest|null=null;
  private probeEvidence:WindowProbeEvidence|null=null;
  private probeSerial:number=0;
  private probeOwnerKey:string='';
  probeBusy():boolean {return this.pendingRoi!==null||this.probeEvidence!==null;}
  private ownerKey():string {
    const p=this.preparation.packet;
    return p.run.length===0?'':p.run+':'+p.epoch+':'+p.session+':'+p.surface+':'+p.context+':'+p.geometry+':'+p.width+':'+p.height+':'+p.x+':'+p.y+':'+p.w+':'+p.h;
  }
  private t406Calibration=false;
  private t406PlanRoi:Int32Array|null=null;
  private t406PlanId='';
  private t406CalibrationId='';
  setT406Plan(text:string):string {
    if(!this.capture.configurable())return 'HOLD_ACTIVE';
    const fields=text.split('|');
    if(fields.length!==3||!/^[a-f0-9]{64}$/.test(fields[0])||!/^[a-f0-9]{64}$/.test(fields[1]))return 'INVALID_FROZEN_PLAN';
    const roi=parseWindowRoi(fields[2]);
    if(roi===null||roi.length!==12||this.probeBusy())return 'INVALID_FROZEN_ROIS';
    this.t406PlanRoi=roi;this.t406CalibrationId=fields[0];this.t406PlanId=fields[1];return 'FROZEN_PLAN_READY';
  }
  setT406Calibration(enabled:boolean):void {if(this.capture.configurable())this.t406Calibration=enabled;}
  private prepareGate:(()=>string)|null=null;
  bindPrepareGateCold(check:()=>string):void {this.prepareGate=check;}
  prepareReadinessCold():PrepareNative {
    const p=this.preparation.packet,n=this.native.getState();
    return {controller:this.session,session:p.session,surface:n.surfaceEpoch,context:n.contextEpoch,
     geometry:p.geometry,width:n.widthPx,height:n.heightPx,packetWidth:p.width,packetHeight:p.height,run:p.run,revision:p.revision,
     ready:this.session>0&&p.enabled&&p.surface===n.surfaceEpoch&&p.context===n.contextEpoch&&
      p.width===n.widthPx&&p.height===n.heightPx&&this.lastReceipt?.presentationState==='CURRENT'&&this.lastReceipt.actualToken===p.token&&this.lastReceipt.requestedToken===p.token&&this.lastReceipt.swapSucceeded};
  }
  readonly capture=new T413Capture();
  private rasterForDraw:CaptureGeometry=new CaptureGeometry();
  captureGeometry(g:CaptureGeometry):void {this.rasterForDraw=g;this.capture.setGeometry(g);}
  private captureValidate():void {
    const s=this.capture.status();
    if(s.state!=='HOLD_OPEN'&&s.state!=='SEALED')return;
    const p=this.preparation.packet,n=this.native.getState();
    if(s.owner!==this.ownerKey()||s.token!==p.token||s.frame!==p.frame||
       n.surfaceEpoch!==p.surface||n.contextEpoch!==p.context||n.widthPx!==p.width||n.heightPx!==p.height)
      this.capture.invalidate('DISPLAY_HOLD_OWNER_CHANGED');
  }
  holdStatus():HoldStatus {this.captureValidate();return this.capture.status();}
  reserveCapture(roi:Int32Array,fixtureKey:string,maxHoldMs:number):CaptureReply {
    const prepare=this.prepareGate===null?'PREPARE_NOT_READY':this.prepareGate();
    if(prepare)return this.capture.reply(prepare);
    const p=this.preparation.packet;
    const invalid=this.capture.canReserve(roi,maxHoldMs,p.width,p.height);
    if(invalid)return this.capture.reply(invalid);
    if(this.probeBusy()||this.probeSerial>=Number.MAX_SAFE_INTEGER)return this.capture.reply('PROBE_BUSY');
    if(this.t394.sequence>=Number.MAX_SAFE_INTEGER)return this.capture.reply('PHASE_ID_EXHAUSTED');
    if(this.t406PlanRoi!==null&&this.t394.name!=='CALIBRATION'&&Array.from(roi).join(',')!==Array.from(this.t406PlanRoi).join(','))return this.capture.reply('FROZEN_PLAN_ROI_MISMATCH');
    if(fixtureKey&&fixtureKey!==windowFixtureKey(p))return this.capture.reply('FIXTURE_IDENTITY');
    const native=this.native.getState();
    if(!p.enabled||!p.token||p.geometry!==this.capture.geometry().geometry||native.surfaceEpoch!==p.surface||native.contextEpoch!==p.context||
       native.widthPx!==p.width||native.heightPx!==p.height)return this.capture.reply('OWNER_NOT_READY');
    // All rejectable validation precedes the phase mutation and request commit.
    this.beginT394(this.t394.name,this.t394.locked,false,true);
    this.pendingRoi=new WindowProbeRequest(++this.probeSerial,new Int32Array(roi),fixtureKey,this.probeOwnerKey);
    return this.capture.reserve(roi,fixtureKey,maxHoldMs,this.probeSerial,this.t394.sequence);
  }
  sampleHoldClock(expectedHoldId:string):ClockSample|null {this.captureValidate();return this.capture.sample(expectedHoldId);}
  sealCapture(expectedHoldId:string):CaptureReply {this.captureValidate();return this.capture.seal(expectedHoldId);}
  ackArchive(attemptId:string,requestId:number,manifestSha256:string):CaptureReply {
    const reply=this.capture.ack(attemptId,requestId,manifestSha256);
    if(reply.state!=='REJECTED')this.probeEvidence=null;return reply;
  }
  cancelCapture(expectedAttemptId:string,reason:string):CaptureReply {
    const reply=this.capture.cancel(expectedAttemptId,reason);
    if(reply.state!=='REJECTED')this.cancelWindowProbe(reason);return reply;
  }
  releaseCapture(expectedAttemptId:string,expectedHoldId:string):CaptureReply {
    const reply=this.capture.release(expectedAttemptId,expectedHoldId);
    if(reply.state!=='REJECTED'){this.t394HoldState=null;this.probeEvidence=null;this.pendingRoi=null;}
    return reply;
  }
  private captureAfterDraw():void {
    const s=this.capture.status();if(s.state!=='RESERVED')return;
    const h=this.t394HoldState,e=this.probeEvidence,p=this.preparation.packet;
    if(h!==null&&e!==null&&e.manifest.captureStatus==='CAPTURED'&&e.manifest.actualToken===h.token&&
       e.manifest.requestedToken===h.token&&e.manifest.frameSerial===h.frame&&h.token===p.token&&h.frame===p.frame&&
       e.manifest.captureStage==='COMPLETE'&&e.manifest.presentationState==='CURRENT'&&
       e.manifest.swapSucceeded===true&&e.manifest.unbindSucceeded===true&&
       e.manifest.session===p.session&&e.manifest.surfaceEpoch===p.surface&&e.manifest.contextEpoch===p.context&&
       e.manifest.geometryEpoch===p.geometry&&e.manifest.requestedRoiWords?.join(',')===s.roi.join(',')&&
       this.lastReceipt?.presentationState==='CURRENT'){
      this.capture.open(h.requestId,h.sequence,h.token,h.frame,p.run,this.ownerKey(),h.startMonoMs);
    }else if(e!==null)this.capture.invalidate('NATIVE_CAPTURE_NOT_CURRENT');
  }

  readonly t394=new T394Phase();
  private t394Input:T394Input|null=null;
  private t394Auto=false;
  private t394Roi:Int32Array|null=null;
  private t394GeometryKey='';
  private t394HoldState:T394Hold|null=null;
  private t394PrepareCut='';
  private t394PrepareCode:number|null=null;
  private t394BaselineHash='';
  private t394BaselineDigest='';
  private t394LastEvidence:T394Archive|null=null;
  beginT394(name:string,locked:boolean,auto:boolean=false,reuse:boolean=false):void {
    if(!this.capture.configurable())return;
    if(this.pendingRoi!==null)this.rejectT394('PHASE_CHANGED');
    this.t394.begin(name,locked,p013NowMs());
    this.t394Input=null;this.t394HoldState=null;this.t394Auto=auto;
    this.t394LastEvidence=null;
    if(!reuse){this.t394Roi=null;this.t394GeometryKey='';this.t394BaselineHash='';this.t394BaselineDigest='';}
  }
  t394WantsEvidence():boolean {
    return this.t394Auto||this.pendingRoi!==null||this.t394.locked&&this.t394.sample===null;
  }
  t394Frame(input:T394Input):void {this.t394Input=input;}
  t394Hold():T394Hold|null {return this.t394HoldState;}
  t394VerifyHold():T394Hold|null {
    const hold=this.t394HoldState;
    if(hold===null)return null;
    const p=this.preparation.packet,current=this.native.getState();
    if(current.surfaceEpoch!==p.surface||current.contextEpoch!==p.context||hold.token!==p.token||hold.frame!==p.frame){
      this.rejectT394('DISPLAY_HOLD_OWNER_CHANGED');return null;
    }
    return hold;
  }
  t394PhaseStatus():string {return this.t394.status;}
  rejectT394(cut:string,receipt:SingleSandReceipt|null=null):void {
    this.capture.invalidate(cut);
    this.t394.reject(cut);this.t394Auto=false;this.t394HoldState=null;
    this.cancelWindowProbe(cut);
    this.t394LastEvidence=new T394Archive(this.t394.sequence,this.t394.name,this.t394.startedMs,
      this.t394.status,this.t394.firstFailure,null,this.t394Input,null,receipt,this.t394PrepareCode);
    if(this.probeEvidence!==null&&this.probeEvidence.phase===undefined)
      this.probeEvidence.phase=this.t394LastEvidence;
    this.t394Input=null;
  }
  exportT394Phase():string {return this.t394LastEvidence===null?'':JSON.stringify(this.t394LastEvidence);}
  windowFixtureKey():string {return this.t394.sample?.fixtureKey??'';}
  windowFixture():string {return this.t394.locked?this.t394.sample?.fixture.roi??'':'';}
  private t394ConfigureAuto(p:SingleSandPacketV1):void {
    if(!this.t394Auto)return;
    this.t394Auto=false;
    const geometry=t394Geometry(p);
    if(this.t394Roi===null){
      this.t394Roi=this.t406PlanRoi===null?null:new Int32Array(this.t406PlanRoi);this.t394GeometryKey=geometry;
    }
    const roi=this.t394Roi;
    if(roi===null||geometry!==this.t394GeometryKey||!this.armWindowProbe(roi,'')){
      this.rejectT394('GEOMETRY_UNSUPPORTED');return;
    }
  }
  private t394AfterDraw(state:P011PublishedRenderState,receipt:SingleSandReceipt,
    packMs:number,drawMs:number,request:WindowProbeRequest|null):void {
    const prep=this.preparation,p=prep.packet,input=this.t394Input;
    if(!receipt.currentFrameSucceeded||!receipt.swapSucceeded||receipt.actualToken!==p.token){
      this.rejectT394('DRAW:'+receipt.firstFailureStage+':'+receipt.reason,receipt);
      return;
    }
    this.t394Input=null;
    if(request===null&&(!this.t394.locked||this.t394.sample!==null))return;
    const lockedSnapshot=this.t394.sample;
    const sample=this.t394.accept(p,receipt,state,prep.profile,prep.slot.particleXY,prep.slot.particleMeta,
      input?.copyMs??0,packMs,drawMs,input?.frameNs??0);
    if(sample!==null&&this.t394.locked&&lockedSnapshot!==null)this.t394.sample=lockedSnapshot;
    const evidence=this.probeEvidence;
    if(evidence!==null){evidence.calibrationId=this.t406CalibrationId;evidence.roiPlanId=this.t406PlanId;}
    let phaseStatus=this.t394.status;
    if(request!==null&&evidence!==null){
      const isA=this.t394.name.endsWith('_A'),isB=this.t394.name.endsWith('_B');
      let holdable=sample!==null&&evidence.manifest.captureStatus==='CAPTURED'&&evidence.manifest.actualToken===sample.token;
      if(holdable&&sample!==null&&isB){
        const newVisible=sample.projectionMeta.some((v:number,i:number)=>i%2===1&&v===2);
        const admitted=input?.admissions.some((row:P051DryAdmissionReceipt)=>row.sequence===2&&row.admittedH>0)??false;
        holdable=this.t394BaselineHash!==sample.particleHash&&this.t394BaselineDigest===sample.digest&&
          newVisible&&admitted&&(input?.steps.length??0)>0;
        if(!holdable)phaseStatus='DYNAMIC_B_INPUT_NOT_PROVEN';
      }
      if(holdable&&sample!==null){
        if(isA){this.t394BaselineHash=sample.particleHash;this.t394BaselineDigest=sample.digest;}
        this.t394HoldState=new T394Hold(this.t394.sequence,this.t394.name,request.id,sample.token,sample.frame,
          sample.width,sample.height,sample.geometry,Date.now(),p013NowMs());
      }
    }
    this.t394LastEvidence=new T394Archive(this.t394.sequence,this.t394.name,this.t394.startedMs,phaseStatus,
      this.t394.firstFailure,sample,input,this.t394HoldState,receipt);
    if(request!==null&&evidence!==null)evidence.phase=this.t394LastEvidence;
  }
  armWindowProbe(roi:Int32Array,fixtureKey:string):boolean {
    if(this.probeBusy()||this.probeSerial>=Number.MAX_SAFE_INTEGER)return false;
    this.pendingRoi=new WindowProbeRequest(++this.probeSerial,new Int32Array(roi),fixtureKey,this.probeOwnerKey);
    return true;
  }
  cancelWindowProbe(stage:string):void {
    const request=this.pendingRoi;
    if(request===null)return;
    this.pendingRoi=null;this.probeEvidence=cancelledWindowProbe(request,stage);
  }
  takeWindowProbe():WindowProbeEvidence|null {return this.probeEvidence;}
  ackWindowProbe():void {if(this.capture.configurable())this.probeEvidence=null;}
  setProbeNeutral(neutral:boolean):void {
    if(!this.capture.configurable()||this.probeBusy())return;
    this.values[8]=neutral?0:.08;this.values[9]=neutral?0:.10;
    this.values[14]=neutral?0:.10;this.values[15]=neutral?0:.16;
    this.config.revision++;this.tile=null;this.bound=false;
  }
  private tile:Uint8Array|null=null;
  private session:number=0;
  private bound:boolean=false;
  private shaderHash:string='';
  failure:string='NOT_READY';
  lastReceipt:SingleSandReceipt|null=null;

  constructor(native:SingleSandNative|null=null){
    this.native=native===null?nativeRenderer as SingleSandNative:native;
  }
  prepareSingleSandOptics():boolean {
    if(this.bound)return true;
    if(this.tile===null){
      this.tile=buildSingleSandGrain(this.values[18],this.values[7]);
      this.config.grainHash=p013Sha256(this.tile);
      this.config.generatorHash=SINGLE_GENERATOR_HASH;
      this.config.digest=singleSandConfigDigest(this.config,this.values);
    }
    this.config.session=this.session;
    const configured=this.native.registerSingleSandConfiguration(this.config,this.values);
    if(configured.reason!=='OK'){this.failure=configured.reason;return false;}
    this.shaderHash=configured.shaderHash;
    this.desc.session=this.session;
    this.desc.digest=this.config.digest;
    this.desc.generation++;
    const staged=this.native.stageSingleSandGrain(this.desc,this.tile);
    this.failure=staged.reason;
    this.bound=staged.reason==='OK';
    return this.bound;
  }
  prepareSingleSandFrame(state:P011PublishedRenderState,context:P012FrameContext,scale:number):boolean {
    this.t394PrepareCode=null;
    this.t394PrepareCut='GEOMETRY';
    if(!Number.isFinite(scale)||scale<=0||!Number.isFinite(context.boxW)||context.boxW<=0||
       !Number.isFinite(context.boxH)||context.boxH<=0){
      this.failure='SINGLE_GEOMETRY_UNSUPPORTED';return false;
    }
    const p=this.preparation.packet;
    const raster=this.rasterForDraw;
    if(raster.rasterRevision!=='T532_WINDOW_HOST_RASTER_V3'||raster.raster.revision!=='T532_WINDOW_HOST_RASTER_V3'||raster.rasterWidth<1||raster.rasterHeight<1||raster.vp2px!==scale||raster.geometry!==context.geometryEpoch||
       rasterNodeReason(raster.raster).length>0){
      this.failure='RASTER_PLAN_NOT_READY';return false;
    }
    p.x=raster.raster.actual.x;p.y=raster.raster.actual.y;p.w=raster.rasterWidth;p.h=raster.rasterHeight;
    p.logicalW=context.boxW;p.logicalH=context.boxH;
    this.t394PrepareCut='PACK';
    this.failure=this.preparation.pack(state,context);
    if(this.failure!=='OK')return false;
    this.t394PrepareCut='TARGET';
    if(this.session===0){
      const init=this.native.init('p013_lmat_surface');
      if(init.code!==0){this.t394PrepareCode=init.code;this.failure='SINGLE_TARGET_UNSUPPORTED';return false;}
      this.session=init.sessionId;this.bound=false;
    }
    const target=this.native.getState();
    const ready=this.native.prepareDualFieldSurface(this.session,target.surfaceEpoch);
    if(ready.code!==0||ready.sizeSource!=='EGL_QUERY_SURFACE'){
      this.t394PrepareCode=ready.code;
      this.failure='SINGLE_TARGET_UNSUPPORTED';return false;
    }
    if(ready.widthPx!==raster.rasterWidth||ready.heightPx!==raster.rasterHeight){this.failure='RASTER_NATIVE_SIZE_MISMATCH';return false;}
    p.session=this.session;p.surface=ready.surfaceEpoch;p.context=ready.contextEpoch;
    p.width=ready.widthPx;p.height=ready.heightPx;
    this.t394PrepareCut='OPTICS';
    if(!this.prepareSingleSandOptics())return false;
    p.revision=this.config.revision;p.digest=this.config.digest;
    p.generation=this.desc.generation;p.grainHash=this.config.grainHash;
    p.shaderHash=this.shaderHash;
    p.token=p.run+':'+p.epoch+':'+p.commit+':'+p.frame+':'+p.generation+':'+p.digest+':'+p.displayHash+':'+p.pbHash+':'+p.particleHash;
    return true;
  }
  drawSingleSandScene(state:P011PublishedRenderState,context:P012FrameContext,scale:number):SingleSandReceipt|null {
    const stamp=this.t394WantsEvidence()?p013NowMs():0;
    if(this.capture.blocksFrames())return null;
    if(!this.prepareSingleSandFrame(state,context,scale)){
      this.rejectT394(this.t394PrepareCut+':'+this.failure);return null;
    }
    const packMs=stamp===0?0:p013NowMs()-stamp;
    const prep=this.preparation;
    const prior=this.t394.sample;
    if(prior!==null&&(prior.geometry!==t394Geometry(prep.packet)||prior.fixtureKey!==windowFixtureKey(prep.packet)))
      this.rejectT394('STALE_INPUT');
    this.probeOwnerKey=this.ownerKey();
    this.t394ConfigureAuto(prep.packet);
    const drawStamp=stamp===0?0:p013NowMs();
    let request=this.pendingRoi;
    if(request!==null&&((request.ownerKey.length!==0&&request.ownerKey!==this.probeOwnerKey)||
       (request.fixtureKey.length!==0&&request.fixtureKey!==windowFixtureKey(prep.packet)))){
      this.cancelWindowProbe('RESERVATION_IDENTITY_CHANGED');request=null;
    }
    let receipt:SingleSandReceipt;
    if(request===null){
      receipt=this.native.drawSingleSandScene(prep.packet,prep.profile,prep.slot.particleXY,prep.slot.particleMeta);
    }else{
      prep.packet.calibration=this.t406Calibration;
      prep.packet.probeRequestId=request.id;prep.packet.probeElements=request.roi.length;
      prep.packet.probeBytes=request.roi.byteLength;this.pendingRoi=null;
      try{
        receipt=this.native.drawSingleSandScene(prep.packet,prep.profile,prep.slot.particleXY,prep.slot.particleMeta,request.roi);
        const evidence=this.native.takeSingleSandWindowProbe();
        this.probeEvidence=evidence!==null&&evidence.manifest.requestId===request.id?
          evidence:cancelledWindowProbe(request,'NATIVE_RECEIPT_MISSING_OR_MISMATCH');
      }catch(e){
        this.probeEvidence=cancelledWindowProbe(request,'NATIVE_PROBE_EXCEPTION');
        this.failure='NATIVE_PROBE_EXCEPTION';this.rejectT394(this.failure);return null;
      }finally{
        prep.packet.calibration=false;
        prep.packet.probeRequestId=0;prep.packet.probeElements=0;prep.packet.probeBytes=0;
      }
    }
    this.t394AfterDraw(state,receipt,packMs,drawStamp===0?0:p013NowMs()-drawStamp,request);
    this.lastReceipt=receipt;
    this.captureAfterDraw();
    this.failure=receipt.reason;
    return receipt;
  }
}
