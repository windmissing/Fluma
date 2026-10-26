// T135 isolated RN operation graphs. This module does not arm the production strict route.
import { P011ActualMaterialPatch, P011StrictInputIdentity, p011CanonicalSandTop } from './P011StateCertificate';
import { P011LocalPatchVerifier, P011WriterEnvelope, P011NoWritePatch } from './P011WriterEnvelope';
import { p011KernelStatus } from './P011WriterRegistry';

export type P011KernelDecision='RAW'|'INWARD'|'ZERO'|'UNSUPPORTED';
export type P011KernelReason='OK'|'INVALID_INPUT'|'STRICT_WRITER_UNSUPPORTED'|
  'CONTACT_HOLD_UNSUPPORTED'|'INWARD_UNSUPPORTED'|'ENVELOPE_CERTIFICATE_MISMATCH'|
  'STRICT_EDGE'|'STOCK_INVALID'|'IDENTITY_MISMATCH';
export class P011KernelFrozen {
  readonly identity: P011StrictInputIdentity=new P011StrictInputIdentity();
  readonly bh: Float64Array;readonly bl: Float64Array;readonly mh: Float64Array;readonly ml: Float64Array;
  readonly suspH: Float64Array;readonly suspL: Float64Array;readonly surface: Float64Array;
  readonly waterTop: Float64Array;readonly cacheH: Float64Array;readonly cacheL: Float64Array;
  readonly poolMass: Float64Array;readonly poolId: Uint32Array;readonly poolState: Uint8Array;
  readonly poolColumn: Int32Array;readonly poolPhase: Uint8Array;
  count: number;ownerId: number=0;snapshotSerial: number=0;
  u: number=0;hardMax: number=0;capacity: number=0;eps: number=1e-9;
  constructor(count: number,poolCapacity: number){
    this.count=count;this.bh=new Float64Array(count);this.bl=new Float64Array(count);
    this.mh=new Float64Array(count);this.ml=new Float64Array(count);this.suspH=new Float64Array(count);
    this.suspL=new Float64Array(count);this.surface=new Float64Array(count);
    this.waterTop=new Float64Array(count);this.cacheH=new Float64Array(count);this.cacheL=new Float64Array(count);
    this.poolMass=new Float64Array(poolCapacity);this.poolId=new Uint32Array(poolCapacity);
    this.poolState=new Uint8Array(poolCapacity);this.poolColumn=new Int32Array(poolCapacity);
    this.poolPhase=new Uint8Array(poolCapacity);
  }
}
export class P011KernelRequest {
  kernel: string='';branch: string='';column: number=0;receiver: number=-1;
  qGranted: number=0;requested: number=0;originalRequested: number=0;
  dt: number=0;heavySpeed: number=0;lightSpeed: number=0;
  slot: number=-1;particleId: number=0;phase: number=0;particleState: string='CONTACT_WAIT';
  ticketToken: number=0;actualDebit: number=0;sourceH: number=0;sourceL: number=0;
}
export class P011KernelShadow {
  readonly patch: P011ActualMaterialPatch=new P011ActualMaterialPatch(2);
  readonly envelope: P011WriterEnvelope=new P011WriterEnvelope(32,4);
  readonly noWrite: P011NoWritePatch=new P011NoWritePatch();
  decision: P011KernelDecision='UNSUPPORTED';reason: P011KernelReason='INVALID_INPUT';
  qControl: number=0;actualDebit: number=0;refund: number=0;
  phaseHeavy: number=0;phaseLight: number=0;poolRemainder: number=0;
  poolRetired: boolean=false;slot: number=-1;particleId: number=0;
  previewCount: number=0;zeroCount: number=0;nodeCount: number=0;
  constructor(){this.reset();}
  reset(): void {
    this.patch.changedCount=0;this.patch.poolPatchDigest='';this.patch.budgetDebit=0;
    this.patch.budgetRefund=0;this.patch.qGranted=0;this.patch.qActual=0;
    this.patch.branch='';this.patch.operationGraphRevision='';this.envelope.reset();
    this.decision='UNSUPPORTED';this.reason='INVALID_INPUT';this.qControl=0;
    this.actualDebit=0;this.refund=0;this.phaseHeavy=0;this.phaseLight=0;
    this.poolRemainder=0;this.poolRetired=false;this.slot=-1;this.particleId=0;
    this.previewCount=0;this.zeroCount=0;this.nodeCount=0;
  }
}
export class P011StrictWriterKernels {
  readonly verifier: P011LocalPatchVerifier;
  readonly shadow: P011KernelShadow=new P011KernelShadow();
  private readonly before: Float64Array=new Float64Array(7);
  private readonly after: Float64Array=new Float64Array(7);
  private readonly receiverBefore: Float64Array=new Float64Array(7);
  private readonly receiverAfter: Float64Array=new Float64Array(7);
  constructor(columnCapacity: number){this.verifier=new P011LocalPatchVerifier(columnCapacity);}
  private capture(f: P011KernelFrozen,c: number,out: Float64Array): void {
    out[0]=f.bh[c];out[1]=f.bl[c];out[2]=f.mh[c];out[3]=f.ml[c];
    out[4]=f.suspH[c];out[5]=f.suspL[c];out[6]=f.surface[c];
  }
  private finite(f: P011KernelFrozen,c: number): boolean {
    return c>=0&&c<f.count&&Number.isFinite(f.bh[c])&&Number.isFinite(f.bl[c])&&
      Number.isFinite(f.mh[c])&&Number.isFinite(f.ml[c])&&Number.isFinite(f.suspH[c])&&
      Number.isFinite(f.suspL[c])&&Number.isFinite(f.waterTop[c])&&
      f.bh[c]>=0&&f.bl[c]>=0&&f.mh[c]>=0&&f.ml[c]>=0&&
      f.suspH[c]>=0&&f.suspL[c]>=0&&
      Object.is(f.surface[c],p011CanonicalSandTop(f.bh[c],f.bl[c],f.mh[c],f.ml[c]));
  }
  private node(s: P011KernelShadow,op: number,a: number,b: number,value: number): boolean {
    // Count the RN trace only. A real interval/eta certificate has not been
    // derived for this node, so it must not be written into the proof record.
    if(!Number.isFinite(value)||s.nodeCount>=32)return false;
    s.nodeCount++;return true;
  }
  private zero(s: P011KernelShadow,contactHold: boolean): P011KernelShadow {
    s.zeroCount++;s.noWrite.prepare(contactHold);
    if(contactHold){s.decision='UNSUPPORTED';s.reason='CONTACT_HOLD_UNSUPPORTED';}
    else{s.decision='ZERO';s.reason='OK';}
    s.patch.changedCount=0;s.refund=s.actualDebit;s.patch.budgetRefund=s.refund;
    s.qControl=0;s.phaseHeavy=0;s.phaseLight=0;s.poolRemainder=0;
    s.poolRetired=false;s.slot=-1;s.particleId=0;
    return s;
  }
  preview(f: P011KernelFrozen,r: P011KernelRequest): P011KernelShadow {
    const s=this.shadow;s.reset();
    const registryStatus=p011KernelStatus(r.kernel,r.particleState);
    if(registryStatus!=='SUPPORTED'){
      s.reason=registryStatus;return s;
    }
    if(f.ownerId<1||f.snapshotSerial<1||f.identity.snapshotSerial!==f.snapshotSerial||
      !f.identity.runId||!f.identity.attemptId||f.identity.writerKernelRevision!==r.kernel||
      !Number.isFinite(f.u)||!Number.isFinite(f.hardMax)||f.hardMax<0||
      !Number.isFinite(f.capacity)||f.capacity<=0||!Number.isFinite(r.qGranted)||
      r.qGranted<0||!Number.isFinite(r.actualDebit)||r.actualDebit<0||
      r.qGranted>r.actualDebit||!this.finite(f,r.column))return s;
    const c=r.column;s.actualDebit=r.actualDebit;s.patch.qGranted=r.qGranted;
    s.patch.operationGraphRevision=r.kernel;s.patch.branch=r.branch;
    if(r.kernel==='TOP_SPLIT_RN_GRAPH_V1'&&
      (r.receiver===c||!this.finite(f,r.receiver)))return s;
    if(r.kernel==='LAND_RN_GRAPH_V1'&&
      (r.slot<0||r.slot>=f.poolMass.length||f.poolId[r.slot]!==r.particleId||
      f.poolColumn[r.slot]!==c||f.poolPhase[r.slot]!==r.phase||
      f.poolState[r.slot]===0||r.phase>1||!Number.isFinite(f.poolMass[r.slot])||
      r.qGranted>f.poolMass[r.slot]))return s;
    if(r.qGranted===0||
      (r.kernel!=='LAND_RN_GRAPH_V1'&&r.qGranted<=f.eps))return this.zero(s,false);
    this.capture(f,c,this.before);this.after.set(this.before);
    if(r.kernel==='SETTLE_RN_GRAPH_V1'){
      if(!(r.originalRequested>f.eps)||!Number.isFinite(r.dt)||
        !Number.isFinite(r.heavySpeed)||!Number.isFinite(r.lightSpeed))return s;
      const depth=Math.max(f.waterTop[c]-f.surface[c],f.eps);
      const aH=Math.min(1,r.heavySpeed*r.dt/depth),aL=Math.min(1,r.lightSpeed*r.dt/depth);
      const dH=f.suspH[c]*aH,dL=f.suspL[c]*aL;
      const scale=r.qGranted/Math.max(r.originalRequested,f.eps);
      const h=dH*scale,l=dL*scale;
      if(!this.node(s,1,-1,-1,depth)||!this.node(s,2,0,-1,aH)||
        !this.node(s,2,0,-1,aL)||!this.node(s,3,1,-1,dH)||
        !this.node(s,3,2,-1,dL)||!this.node(s,2,-1,-1,scale)||
        !this.node(s,3,3,5,h)||!this.node(s,3,4,5,l))return s;
      if(h>f.suspH[c]||l>f.suspL[c]||h<0||l<0)return s;
      this.after[4]=f.suspH[c]-h;this.after[2]=f.mh[c]+h;
      this.after[5]=f.suspL[c]-l;this.after[3]=f.ml[c]+l;
      s.phaseHeavy=h;s.phaseLight=l;
    }else if(r.kernel==='TOP_SPLIT_RN_GRAPH_V1'){
      const mix=f.mh[c]+f.ml[c],denom=Math.max(mix,f.eps);
      const fromMix=Math.min(r.qGranted,mix),scale=fromMix/denom;
      const h0=f.mh[c]*scale,l0=f.ml[c]*scale;
      let remainder=r.qGranted-fromMix;const fromLight=Math.min(remainder,f.bl[c]);
      const light=l0+fromLight;remainder-=fromLight;
      const fromHeavy=Math.min(remainder,f.bh[c]),heavy=h0+fromHeavy;
      if(!this.node(s,1,-1,-1,mix)||!this.node(s,1,0,-1,denom)||
        !this.node(s,2,1,-1,scale)||!this.node(s,3,2,-1,h0)||
        !this.node(s,3,2,-1,l0)||!this.node(s,4,-1,-1,remainder)||
        !this.node(s,1,4,-1,light)||!this.node(s,4,5,-1,heavy))return s;
      this.after[2]=f.mh[c]-h0;this.after[3]=f.ml[c]-l0;
      this.after[1]=f.bl[c]-fromLight;this.after[0]=f.bh[c]-fromHeavy;
      this.capture(f,r.receiver,this.receiverBefore);this.receiverAfter.set(this.receiverBefore);
      this.receiverAfter[2]=f.mh[r.receiver]+heavy;
      this.receiverAfter[3]=f.ml[r.receiver]+light;
      s.phaseHeavy=heavy;s.phaseLight=light;
      s.patch.branch=r.qGranted<=mix?'MIX':remainder<=f.bh[c]?'MIX_LIGHT':'MIX_LIGHT_HEAVY';
    }else{
      const mass=f.poolMass[r.slot];
      const remainder=r.qGranted===mass?0:mass-r.qGranted;
      if(r.qGranted<mass&&!(remainder<mass))return this.zero(s,false);
      if(r.phase===0)this.after[2]=f.mh[c]+r.qGranted;
      else this.after[3]=f.ml[c]+r.qGranted;
      s.poolRemainder=remainder;s.poolRetired=remainder===0;
      s.slot=r.slot;s.particleId=r.particleId;
      s.phaseHeavy=r.phase===0?r.qGranted:0;s.phaseLight=r.phase===1?r.qGranted:0;
      if(!this.node(s,4,-1,-1,remainder))return s;
    }
    this.after[6]=p011CanonicalSandTop(this.after[0],this.after[1],this.after[2],this.after[3]);
    if(!this.node(s,1,-1,-1,this.after[6])||this.after[6]>f.capacity)return s;
    s.previewCount=1;s.qControl=r.qGranted;
    s.patch.addColumn(c,this.before,this.after);
    if(r.kernel==='TOP_SPLIT_RN_GRAPH_V1'){
      this.receiverAfter[6]=p011CanonicalSandTop(this.receiverAfter[0],this.receiverAfter[1],
        this.receiverAfter[2],this.receiverAfter[3]);
      if(this.receiverAfter[6]>f.capacity)return s;
      s.patch.addColumn(r.receiver,this.receiverBefore,this.receiverAfter);
    }
    const cert=this.verifier.verifyLocalPatch(f.identity,f.ownerId,f.snapshotSerial,s.patch,
      f.bh,f.bl,f.mh,f.ml,f.surface,f.u,f.hardMax,f.capacity,'LOCAL');
    const legal=cert.legal,reason=cert.reason;
    if(!legal){
      if(reason==='STRICT_EDGE')return this.zero(s,false);
      s.reason=reason==='STOCK_INVALID'?'STOCK_INVALID':'ENVELOPE_CERTIFICATE_MISMATCH';return s;
    }
    s.decision='RAW';s.reason='OK';s.refund=r.actualDebit-r.qGranted;
    s.patch.qActual=r.qGranted;s.patch.budgetDebit=r.actualDebit;s.patch.budgetRefund=s.refund;
    return s;
  }
  // Commit only the previously verified immutable snapshot, never recompute against a changed owner.
  commit(f: P011KernelFrozen,s: P011KernelShadow,ownerId: number,snapshotSerial: number): P011KernelReason {
    if(s.decision!=='RAW'&&s.decision!=='INWARD'&&s.decision!=='ZERO')return s.reason;
    if(f.ownerId!==ownerId||f.snapshotSerial!==snapshotSerial)return 'IDENTITY_MISMATCH';
    if(s.decision==='ZERO')return 'OK';
    for(let row=0;row<s.patch.changedCount;row++){
      const c=s.patch.changedColumns[row],base=row*7;
      if(!Object.is(f.bh[c],s.patch.beforeBits[base])||
        !Object.is(f.bl[c],s.patch.beforeBits[base+1])||
        !Object.is(f.mh[c],s.patch.beforeBits[base+2])||
        !Object.is(f.ml[c],s.patch.beforeBits[base+3])||
        !Object.is(f.suspH[c],s.patch.beforeBits[base+4])||
        !Object.is(f.suspL[c],s.patch.beforeBits[base+5])||
        !Object.is(f.surface[c],s.patch.beforeBits[base+6]))return 'IDENTITY_MISMATCH';
    }
    for(let row=0;row<s.patch.changedCount;row++){
      const c=s.patch.changedColumns[row],base=row*7;
      f.bh[c]=s.patch.afterBits[base];f.bl[c]=s.patch.afterBits[base+1];
      f.mh[c]=s.patch.afterBits[base+2];f.ml[c]=s.patch.afterBits[base+3];
      f.suspH[c]=s.patch.afterBits[base+4];f.suspL[c]=s.patch.afterBits[base+5];
      f.surface[c]=s.patch.afterBits[base+6];
    }
    if(s.slot>=0){f.poolMass[s.slot]=s.poolRemainder;if(s.poolRetired)f.poolState[s.slot]=0;}
    return 'OK';
  }
}
