import { P011PublishedRenderState } from '../physics/P011PublishedRenderState';
import { derivedPublicationGateV2 } from '../physics/derived/P011DerivedCommitV2';
import { DERIVED_POLICY_V2 } from '../physics/derived/P011DerivedTypesV2';
import { P013DualFieldAdapter,P013DualFieldUploadSlot } from './P013DualFieldAdapter';
import { P013_COLUMN_PROJECTION } from './P013ParticleProjection';
import { P012FrameContext } from './P012RenderTypes';
import { SingleSandPacketV1 } from './P013SingleSandTypes';
import { p013Sha256 } from './preparation/P013CanvasAssetExport';

export class P013SingleSandPreparation {
  readonly profile:Float32Array=new Float32Array(256);
  readonly packet:SingleSandPacketV1=new SingleSandPacketV1();
  slot:P013DualFieldUploadSlot=new P013DualFieldUploadSlot(2);
  private readonly adapter:P013DualFieldAdapter=new P013DualFieldAdapter();
  private readonly displayed:Float64Array=new Float64Array(128);
  private readonly scratch:Float64Array=new Float64Array(128);
  private readonly gaussian:Float64Array=new Float64Array(128);
  private readonly weights:Float64Array=new Float64Array(37);
  private readonly projectionBytes:Uint8Array=new Uint8Array(8192);
  private readonly sourceBytes:Uint8Array=new Uint8Array(10240);
  constructor(){
    let sum=0;
    for(let j=-18;j<=18;j++){this.weights[j+18]=Math.exp(-j*j/72);sum+=this.weights[j+18];}
    for(let j=0;j<37;j++)this.weights[j]/=sum;
  }
  pack(state:P011PublishedRenderState,context:P012FrameContext):string {
    const seal=state.derivedSeal;
    if(state.numericPolicy!==DERIVED_POLICY_V2||seal===null||!derivedPublicationGateV2(state)||
      context.runIdentity!==state.runId||context.fieldEpoch!==state.stateEpoch)return 'SINGLE_PUBLICATION_MISMATCH';
    const f=state.field,pool=f.particleState,p=this.packet;
    if(state.mediumMode!=='DRY_AIR_V1'||f.mediumMode!=='DRY_AIR_V1'||f.cols!==128||f.capacity!==256||
      !state.particleRouteActive||state.routeId!=='dual-field-particle-v1'||pool===undefined||pool===null)
      return 'UNSUPPORTED_MATERIAL_SET';
    if(pool.nmax<2||pool.nmax>512)return 'SINGLE_TYPED_RANGE';
    const arrays:Float64Array[]=[f.sandTop,f.waterTop,f.bh,f.bl,f.mh,f.ml,f.suspH,f.suspL,pool.colParticleMassH,pool.colParticleMassL];
    for(let i=0;i<arrays.length;i++){
      const a=arrays[i];
      if(a.length!==128||a.byteLength!==1024)return 'SINGLE_TYPED_RANGE';
      this.sourceBytes.set(new Uint8Array(a.buffer,a.byteOffset,a.byteLength),i*1024);
    }
    for(let c=0;c<128;c++){
      if(f.bl[c]!==0||f.ml[c]!==0||f.suspH[c]!==0||f.suspL[c]!==0||pool.colParticleMassL[c]!==0)
        return 'UNSUPPORTED_MATERIAL_SET';
      const bed=f.bh[c]+f.mh[c],top=f.sandTop[c],water=f.waterTop[c],flight=pool.colParticleMassH[c];
      if(!Number.isFinite(top)||!Number.isFinite(water)||!Number.isFinite(bed)||!Number.isFinite(flight)||
         f.bh[c]<0||f.mh[c]<0||flight<0||bed!==top||water!==bed+flight||water>f.capacity)
        return 'SINGLE_TYPED_RANGE';
      this.displayed[c]=top;
    }
    if(this.slot.capacity!==pool.nmax)this.slot=new P013DualFieldUploadSlot(pool.nmax);
    const result=this.adapter.packInto(state,context,this.slot,P013_COLUMN_PROJECTION,false,true);
    if(result!=='OK')return result==='UNSUPPORTED_MATERIAL_SET'?result:'SINGLE_TYPED_RANGE';
    for(let pass=0;pass<4;pass++){
      for(let c=0;c<128;c++)this.scratch[c]=(this.displayed[Math.max(0,c-1)]+this.displayed[c]+this.displayed[Math.min(127,c+1)])/3;
      this.displayed.set(this.scratch);
    }
    for(let c=0;c<128;c++){
      let sum=0;
      for(let j=-18;j<=18;j++)sum+=this.weights[j+18]*this.displayed[Math.max(0,Math.min(127,c+j))];
      this.gaussian[c]=sum;
    }
    const slopeScale=(p.h/f.capacity)/(2*p.w/128);
    for(let c=0;c<128;c++){
      this.profile[c]=this.displayed[c];
      this.profile[128+c]=(this.gaussian[Math.min(127,c+1)]-this.gaussian[Math.max(0,c-1)])*slopeScale;
    }
    p.run=state.runId;p.epoch=state.stateEpoch;p.commit=state.physicsCommitId;p.kind=state.publicationKind;
    p.ledger=state.ledgerCommitKey===null?'':JSON.stringify(state.ledgerCommitKey);
    p.certificate=state.derivedCertificateId;p.seal=seal.serial;p.capacity=f.capacity;
    p.geometry=context.geometryEpoch;p.continuity=context.continuityEpoch;p.frame=context.frameSerial;
    p.count=this.slot.frame.particleCount;p.slots=this.slot.capacity;
    p.inverted=context.visualInverted;p.enabled=context.particleLayerEnabled!==false;
    p.rawHash=p013Sha256(this.sourceBytes);
    p.displayHash=p013Sha256(new Uint8Array(this.profile.buffer,0,512));
    p.pbHash=p013Sha256(new Uint8Array(this.profile.buffer,512,512));
    const bytes=p.count*8;
    this.projectionBytes.set(new Uint8Array(this.slot.particleXY.buffer,0,bytes),0);
    this.projectionBytes.set(new Uint8Array(this.slot.particleMeta.buffer,0,bytes),bytes);
    p.particleHash=p013Sha256(this.projectionBytes.subarray(0,bytes*2));
    return 'OK';
  }
}
