// T135 isolated writer owner. No production route or renderer imports.
import { P011ActualMaterialPatch, P011StrictInputIdentity, p011CanonicalSandTop } from './P011StateCertificate';
import { P011LocalPatchVerifier, P011NoWritePatch } from './P011WriterEnvelope';
import { P011OutwardInterval } from './P011OutwardArithmetic';
import { P011WriterRNGraph } from './P011WriterRNGraph';
import { p011KernelStatus } from './P011WriterRegistry';

export type P011KernelDecision='RAW'|'INWARD'|'ZERO'|'UNSUPPORTED';
export type P011KernelReason='OK'|'INVALID_INPUT'|'STRICT_WRITER_UNSUPPORTED'|
  'CONTACT_HOLD_UNSUPPORTED'|'INWARD_UNSUPPORTED'|'ENVELOPE_CERTIFICATE_MISMATCH'|
  'STRICT_EDGE'|'STOCK_INVALID'|'IDENTITY_MISMATCH'|'AUDIT_FAILED'|'STALE_TOKEN';
export class P011KernelFrozen {
  readonly identity: P011StrictInputIdentity=new P011StrictInputIdentity();
  readonly bh: Float64Array;readonly bl: Float64Array;readonly mh: Float64Array;readonly ml: Float64Array;
  readonly suspH: Float64Array;readonly suspL: Float64Array;readonly surface: Float64Array;
  readonly waterTop: Float64Array;readonly cacheH: Float64Array;readonly cacheL: Float64Array;
  readonly sourceH: Float64Array;readonly sourceL: Float64Array;
  readonly landedH: Float64Array;readonly landedL: Float64Array;readonly localRemaining: Float64Array;
  readonly poolMass: Float64Array;readonly poolId: Uint32Array;readonly poolState: Uint8Array;
  readonly poolColumn: Int32Array;readonly poolPhase: Uint8Array;readonly poolSource: Uint8Array;
  readonly poolSourceId: Uint32Array;readonly poolX: Float64Array;readonly poolY: Float64Array;
  readonly poolVx: Float64Array;readonly poolVy: Float64Array;readonly freeSlots: Int32Array;
  readonly eventKind: Uint8Array;readonly eventId: Uint32Array;readonly eventMass: Float64Array;
  readonly eventSource: Uint32Array;readonly lineageId: Uint32Array;
  readonly eventRequested: Float64Array;readonly eventRefund: Float64Array;
  readonly eventTicket: Float64Array;readonly eventColumn: Int32Array;readonly eventPhase: Uint8Array;
  readonly columnArrays: Float64Array[]=[];
  count: number;ownerId: number=0;snapshotSerial: number=0;dry: boolean=false;
  u: number=0;hardMax: number=0;capacity: number=0;eps: number=1e-9;
  freeCount: number=0;liveCount: number=0;nextId: number=1;eventCount: number=0;lineageCount: number=0;
  totalRemaining: number=0;ticketToken: number=0;ticketState: number=0;
  ticketDebit: number=0;ticketRefund: number=0;
  constructor(count: number,poolCapacity: number){
    if(!Number.isSafeInteger(count)||count<2||count>128||!Number.isSafeInteger(poolCapacity)||
      poolCapacity<1||poolCapacity>512)throw new Error('KERNEL_CAPACITY');
    this.count=count;this.bh=new Float64Array(count);this.bl=new Float64Array(count);
    this.mh=new Float64Array(count);this.ml=new Float64Array(count);this.suspH=new Float64Array(count);
    this.suspL=new Float64Array(count);this.surface=new Float64Array(count);this.waterTop=new Float64Array(count);
    this.cacheH=new Float64Array(count);this.cacheL=new Float64Array(count);
    this.sourceH=new Float64Array(count);this.sourceL=new Float64Array(count);
    this.landedH=new Float64Array(count);this.landedL=new Float64Array(count);
    this.localRemaining=new Float64Array(count);
    this.poolMass=new Float64Array(poolCapacity);this.poolId=new Uint32Array(poolCapacity);
    this.poolState=new Uint8Array(poolCapacity);this.poolColumn=new Int32Array(poolCapacity);
    this.poolPhase=new Uint8Array(poolCapacity);this.poolSource=new Uint8Array(poolCapacity);
    this.poolSourceId=new Uint32Array(poolCapacity);this.poolX=new Float64Array(poolCapacity);
    this.poolY=new Float64Array(poolCapacity);this.poolVx=new Float64Array(poolCapacity);
    this.poolVy=new Float64Array(poolCapacity);this.freeSlots=new Int32Array(poolCapacity);
    this.eventKind=new Uint8Array(poolCapacity+1);this.eventId=new Uint32Array(poolCapacity+1);
    this.eventMass=new Float64Array(poolCapacity+1);this.eventSource=new Uint32Array(poolCapacity+1);
    this.eventRequested=new Float64Array(poolCapacity+1);this.eventRefund=new Float64Array(poolCapacity+1);
    this.eventTicket=new Float64Array(poolCapacity+1);this.eventColumn=new Int32Array(poolCapacity+1);
    this.eventPhase=new Uint8Array(poolCapacity+1);
    this.lineageId=new Uint32Array(poolCapacity+1);
    this.freeCount=poolCapacity;for(let i=0;i<poolCapacity;i++)this.freeSlots[i]=i;
    this.columnArrays.push(this.bh,this.bl,this.mh,this.ml,this.suspH,this.suspL,this.surface,
      this.waterTop,this.cacheH,this.cacheL,this.sourceH,this.sourceL,this.landedH,this.landedL,this.localRemaining);
  }
}
export class P011KernelRequest {
  kernel: string='';branch: string='';column: number=0;receiver: number=-1;
  qGranted: number=0;requested: number=0;originalRequested: number=0;
  dt: number=0;heavySpeed: number=0;lightSpeed: number=0;
  slot: number=-1;particleId: number=0;phase: number=0;particleState: string='CONTACT_WAIT';
  ticketToken: number=0;actualDebit: number=0;sourceH: number=0;sourceL: number=0;
  sourceActive: boolean=false;predictedY: number=0;contactEps: number=1e-6;
}
export class P011KernelShadow {
  readonly patch: P011ActualMaterialPatch=new P011ActualMaterialPatch(2);
  readonly graph: P011WriterRNGraph=new P011WriterRNGraph();
  readonly envelope=this.graph.envelope;readonly noWrite: P011NoWritePatch=new P011NoWritePatch();
  readonly coefficientLo: Float64Array=new Float64Array(2);
  readonly coefficientHi: Float64Array=new Float64Array(2);
  readonly interceptLo: Float64Array=new Float64Array(2);
  readonly interceptHi: Float64Array=new Float64Array(2);
  readonly delta0Lo: Float64Array=new Float64Array(2);readonly delta0Hi: Float64Array=new Float64Array(2);
  decision: P011KernelDecision='UNSUPPORTED';reason: P011KernelReason='INVALID_INPUT';
  qControl: number=0;actualDebit: number=0;refund: number=0;
  phaseHeavy: number=0;phaseLight: number=0;poolRemainder: number=0;
  poolRetired: boolean=false;slot: number=-1;particleId: number=0;
  previewCount: number=0;zeroCount: number=0;nodeCount: number=0;
  token: number=0;domainLo: number=0;domainHi: number=0;inwardReason: string='NOT_NEEDED';
  ALo: number=0;AHi: number=0;bLo: number=0;bHi: number=0;
  selectedDomainLo: number=0;selectedDomainHi: number=0;
  readonly columnBefore: Float64Array=new Float64Array(30);
  readonly columnAfter: Float64Array=new Float64Array(30);
  readonly poolBefore: Float64Array=new Float64Array(12);
  readonly poolAfter: Float64Array=new Float64Array(12);
  readonly ledgerBefore: Float64Array=new Float64Array(16);
  ownerId: number=0;snapshotSerial: number=0;transactionAudited: boolean=false;
}
export class P011StrictWriterKernels {
  readonly verifier: P011LocalPatchVerifier;
  readonly shadow: P011KernelShadow=new P011KernelShadow();
  private readonly columns: Int32Array=new Int32Array(4);
  private readonly before: Float64Array=new Float64Array(60);
  private readonly after: Float64Array=new Float64Array(30);
  private readonly sealedAfter: Float64Array=new Float64Array(30);
  private readonly slotBefore: Float64Array=new Float64Array(12);
  private readonly slotAfter: Float64Array=new Float64Array(12);
  private readonly scalarBefore: Float64Array=new Float64Array(16);
  private readonly terms: Float64Array=new Float64Array(7);
  private readonly cacheAuditH: Float64Array=new Float64Array(128);
  private readonly cacheAuditL: Float64Array=new Float64Array(128);
  private readonly freeSeen: Uint8Array=new Uint8Array(512);
  private readonly interval: P011OutwardInterval[]=[];
  private readonly materialNodes: Int32Array=new Int32Array(8);
  private readonly sealedPatch: P011ActualMaterialPatch=new P011ActualMaterialPatch(2);
  private count: number=0;private serial: number=0;private pending: number=0;
  private owner: P011KernelFrozen|null=null;private identity: string='';
  private kernel: string='';private donor: number=0;private receiver: number=-1;private slot: number=-1;
  private sealedQ: number=0;private sealedRefund: number=0;private sealedDecision: P011KernelDecision='UNSUPPORTED';
  private sealedMass: number=0;private retired: boolean=false;private requestId: number=0;
  private eventSource: number=0;private eventBefore: number=0;private lineageBefore: number=0;
  private sealedRequested: number=0;private sealedPhase: number=0;
  constructor(columnCapacity: number){this.verifier=new P011LocalPatchVerifier(columnCapacity);
    for(let i=0;i<8;i++)this.interval.push(new P011OutwardInterval());}
  private identityOf(f: P011KernelFrozen): string {
    const x=f.identity;
    return `${f.ownerId}/${f.snapshotSerial}/${x.runId}/${x.stateEpoch}/${x.attemptId}/${x.substepId}/`+
      `${x.configRevision}/${x.snapshotSerial}/${x.thetaBits}/${x.uThetaBits}/${x.hBits}/`+
      `${x.writerKernelRevision}/${x.legalStateRevision}/${x.decisionArithmeticRevision}/${f.u}/${f.hardMax}/${f.capacity}/${f.eps}/${f.dry}`;
  }
  private captureColumn(f: P011KernelFrozen,c: number,row: number): void {
    for(let i=0;i<15;i++)this.before[row*15+i]=f.columnArrays[i][c];
  }
  private addColumn(f: P011KernelFrozen,c: number): void {
    if(c<0||c>=f.count)return;
    for(let i=0;i<this.count;i++)if(this.columns[i]===c)return;
    if(this.count>=4)throw new Error('KERNEL_LOCAL_CAPACITY');
    this.columns[this.count]=c;this.captureColumn(f,c,this.count++);
  }
  private slotRead(f: P011KernelFrozen,out: Float64Array): void {
    const i=this.slot;if(i<0)return;
    out[0]=f.poolMass[i];out[1]=f.poolId[i];out[2]=f.poolState[i];out[3]=f.poolColumn[i];
    out[4]=f.poolPhase[i];out[5]=f.poolSource[i];out[6]=f.poolSourceId[i];
    out[7]=f.poolX[i];out[8]=f.poolY[i];out[9]=f.poolVx[i];out[10]=f.poolVy[i];out[11]=f.nextId;
  }
  private reset(f: P011KernelFrozen,r: P011KernelRequest): void {
    const s=this.shadow;this.pending=0;this.owner=f;this.identity=this.identityOf(f);
    this.kernel=r.kernel;this.donor=r.column;this.receiver=r.kernel==='TOP_SPLIT_RN_GRAPH_V1'?r.receiver:-1;
    this.slot=r.kernel==='LAND_RN_GRAPH_V1'?r.slot:-1;this.count=0;
    s.decision='UNSUPPORTED';s.reason='INVALID_INPUT';s.qControl=0;s.actualDebit=r.actualDebit;s.refund=0;
    s.patch.changedCount=0;s.previewCount=0;s.zeroCount=0;s.nodeCount=0;s.slot=-1;s.poolRetired=false;
    s.phaseHeavy=0;s.phaseLight=0;s.poolRemainder=0;s.token=++this.serial;
    s.inwardReason='NOT_NEEDED';s.envelope.reset();
    s.transactionAudited=false;s.ownerId=f.ownerId;s.snapshotSerial=f.snapshotSerial;
    s.selectedDomainLo=0;s.selectedDomainHi=0;s.ALo=0;s.AHi=0;s.bLo=0;s.bHi=0;
    this.slotBefore.fill(0);this.slotAfter.fill(0);this.before.fill(0);this.after.fill(0);
  }
  private finite(f: P011KernelFrozen,c: number): boolean {
    if(!Number.isSafeInteger(c)||c<0||c>=f.count)return false;
    for(let i=0;i<15;i++)if(!Number.isFinite(f.columnArrays[i][c])||
      (i!==6&&f.columnArrays[i][c]<0))return false;
    return Object.is(f.surface[c],p011CanonicalSandTop(f.bh[c],f.bl[c],f.mh[c],f.ml[c]))&&
      f.surface[c]<=f.capacity&&f.sourceH[c]<=f.suspH[c]&&f.sourceL[c]<=f.suspL[c];
  }
  private freeze(f: P011KernelFrozen,r: P011KernelRequest): boolean {
    if(!Number.isSafeInteger(f.ownerId)||f.ownerId<1||!Number.isSafeInteger(f.snapshotSerial)||
      f.snapshotSerial<1||f.snapshotSerial>=Number.MAX_SAFE_INTEGER||f.identity.snapshotSerial!==f.snapshotSerial||
      !f.identity.runId||!f.identity.attemptId||f.identity.writerKernelRevision!==r.kernel||
      !Number.isFinite(f.u)||!Number.isFinite(f.hardMax)||f.hardMax<0||
      !Number.isFinite(f.capacity)||f.capacity<=0||!(f.eps>0)||
      !Number.isFinite(r.qGranted)||r.qGranted<0||!Number.isFinite(r.actualDebit)||
      r.qGranted>r.actualDebit||r.actualDebit<0||!Number.isFinite(r.requested)||r.requested<r.qGranted||
      !this.finite(f,r.column)||
      r.ticketToken<=0||r.ticketToken!==f.ticketToken||f.ticketState!==1||
      !Object.is(f.ticketDebit,r.actualDebit)||f.ticketRefund!==0||
      !Number.isFinite(f.totalRemaining)||f.totalRemaining<0)return false;
    if(this.receiver>=0&&(this.receiver===r.column||Math.abs(this.receiver-r.column)!==1||
      !this.finite(f,this.receiver)))return false;
    if(this.kernel==='LAND_RN_GRAPH_V1'){
      if(!Number.isSafeInteger(this.slot)||this.slot<0||this.slot>=f.poolMass.length||
        f.poolId[this.slot]!==r.particleId||f.poolColumn[this.slot]!==r.column||
        f.poolPhase[this.slot]!==r.phase||f.poolState[this.slot]!==2||
        r.phase<0||r.phase>1||r.qGranted>f.poolMass[this.slot]||
        f.eventCount>=f.eventKind.length||f.lineageCount>=f.lineageId.length)return false;
      this.slotRead(f,this.slotBefore);this.slotAfter.set(this.slotBefore);
      for(let i=0;i<12;i++)if(!Number.isFinite(this.slotBefore[i]))return false;
      if(f.freeCount<0||f.freeCount>=f.freeSlots.length||f.liveCount<1)return false;
      if(!this.poolAudit(f))return false;
    }
    this.addColumn(f,r.column);if(this.receiver>=0)this.addColumn(f,this.receiver);
    this.addColumn(f,Math.min(r.column,this.receiver<0?r.column:this.receiver)-1);
    this.addColumn(f,Math.max(r.column,this.receiver<0?r.column:this.receiver)+1);
    const b=this.scalarBefore;b[0]=f.totalRemaining;b[1]=f.ticketToken;b[2]=f.ticketState;
    b[3]=f.ticketDebit;b[4]=f.ticketRefund;b[5]=f.freeCount;b[6]=f.liveCount;b[7]=f.nextId;
    b[8]=f.eventCount;b[9]=f.lineageCount;b[10]=f.freeSlots[f.freeCount]??0;
    this.requestId=r.particleId;this.eventSource=this.slot<0?0:f.poolSourceId[this.slot];
    this.eventBefore=f.eventCount;this.lineageBefore=f.lineageCount;return true;
  }
  private poolAudit(f: P011KernelFrozen): boolean {
    this.cacheAuditH.fill(0);this.cacheAuditL.fill(0);this.freeSeen.fill(0);let live=0;
    for(let i=0;i<f.poolMass.length;i++){
      if(f.poolState[i]===0)continue;
      const c=f.poolColumn[i];if(c<0||c>=f.count||f.poolPhase[i]>1||
        !Number.isFinite(f.poolMass[i])||f.poolMass[i]<=0||f.poolId[i]===0)return false;
      for(let j=0;j<i;j++)if(f.poolState[j]!==0&&f.poolId[i]===f.poolId[j])return false;
      if(f.poolPhase[i]===0)this.cacheAuditH[c]+=f.poolMass[i];else this.cacheAuditL[c]+=f.poolMass[i];live++;
    }
    if(live!==f.liveCount||live+f.freeCount!==f.poolMass.length)return false;
    for(let i=0;i<f.freeCount;i++){
      const slot=f.freeSlots[i];if(slot<0||slot>=f.poolMass.length||f.poolState[slot]!==0||this.freeSeen[slot]!==0)return false;
      this.freeSeen[slot]=1;
    }
    for(let c=0;c<f.count;c++)if(!Number.isFinite(f.cacheH[c])||!Number.isFinite(f.cacheL[c])||
      Math.abs(this.cacheAuditH[c]-f.cacheH[c])>1e-9||Math.abs(this.cacheAuditL[c]-f.cacheL[c])>1e-9)return false;
    return true;
  }
  private build(f: P011KernelFrozen,r: P011KernelRequest,q: number,lo: number,hi: number): boolean {
    const s=this.shadow,g=s.graph;s.patch.changedCount=0;s.previewCount++;
    g.reset(lo,hi);s.domainLo=lo;s.domainHi=hi;
    for(let row=0;row<(this.receiver<0?1:2);row++){
      for(let i=0;i<15;i++)this.after[row*15+i]=this.before[row*15+i];
      for(let i=0;i<4;i++)this.materialNodes[row*4+i]=g.constant(this.before[row*15+i]);
    }
    const c=r.column,qNode=g.control(q);let h=0,l=0;
    if(this.kernel==='SETTLE_RN_GRAPH_V1'){
      if(!(r.originalRequested>f.eps)||r.qGranted>r.originalRequested||r.dt<0||
        r.heavySpeed<0||r.lightSpeed<0)return false;
      const depth0=g.op(2,g.constant(f.waterTop[c]),g.constant(f.surface[c]));
      const depth=g.constant(Math.max(g.actual[depth0],f.eps));
      const ah0=g.op(4,g.op(3,g.constant(r.heavySpeed),g.constant(r.dt)),depth);
      const al0=g.op(4,g.op(3,g.constant(r.lightSpeed),g.constant(r.dt)),depth);
      const ah=g.constant(Math.min(1,g.actual[ah0])),al=g.constant(Math.min(1,g.actual[al0]));
      const dh0=g.op(3,g.constant(f.suspH[c]),ah),dl0=g.op(3,g.constant(f.suspL[c]),al);
      const dh=g.constant(g.actual[dh0]),dl=g.constant(g.actual[dl0]);
      const denom=g.constant(Math.max(r.originalRequested,f.eps));const scale=g.op(4,qNode,denom);
      h=g.op(3,scale,dh);l=g.op(3,scale,dl);
      this.after[4]=g.actual[g.op(2,g.constant(f.suspH[c]),h)];
      this.after[5]=g.actual[g.op(2,g.constant(f.suspL[c]),l)];
      this.materialNodes[2]=g.op(1,this.materialNodes[2],h);
      this.materialNodes[3]=g.op(1,this.materialNodes[3],l);
      const oa=g.oa,t=this.interval;
      oa.addInto(g.actual[dh],g.actual[dl],t[0]);g.set(t[1],r.originalRequested,r.originalRequested);
      oa.intervalDivInto(t[0],t[1],t[2]);s.ALo=t[2].lo;s.AHi=t[2].hi;s.bLo=0;s.bHi=0;
      if(r.sourceActive){
        const sh=g.op(4,g.op(3,h,g.constant(f.sourceH[c])),g.constant(Math.max(f.suspH[c],f.eps)));
        const sl=g.op(4,g.op(3,l,g.constant(f.sourceL[c])),g.constant(Math.max(f.suspL[c],f.eps)));
        this.after[10]=Math.max(0,g.actual[g.op(2,g.constant(f.sourceH[c]),sh)]);
        this.after[11]=Math.max(0,g.actual[g.op(2,g.constant(f.sourceL[c]),sl)]);
        this.after[12]=g.actual[g.op(1,g.constant(f.landedH[c]),sh)];
        this.after[13]=g.actual[g.op(1,g.constant(f.landedL[c]),sl)];
      }
      s.patch.branch='SETTLE';
    }else if(this.kernel==='TOP_SPLIT_RN_GRAPH_V1'){
      const mixNode=g.op(1,g.constant(f.mh[c]),g.constant(f.ml[c]));const mix=g.actual[mixNode];
      const fromMix=q<=mix?qNode:g.constant(mix),denom=g.constant(Math.max(mix,f.eps));
      const scale=g.op(4,fromMix,denom),h0=g.op(3,scale,g.constant(f.mh[c]));
      const l0=g.op(3,scale,g.constant(f.ml[c]));const r1=g.op(2,qNode,fromMix);
      const takeL=g.actual[r1]<=f.bl[c]?r1:g.constant(f.bl[c]);
      l=g.op(1,l0,takeL);const r2=g.op(2,r1,takeL);
      const takeH=g.actual[r2]<=f.bh[c]?r2:g.constant(f.bh[c]);h=g.op(1,h0,takeH);
      this.materialNodes[2]=g.op(2,this.materialNodes[2],h0);
      this.materialNodes[3]=g.op(2,this.materialNodes[3],l0);
      this.materialNodes[1]=g.op(2,this.materialNodes[1],takeL);
      this.materialNodes[0]=g.op(2,this.materialNodes[0],takeH);
      this.materialNodes[6]=g.op(1,this.materialNodes[6],h);
      this.materialNodes[7]=g.op(1,this.materialNodes[7],l);
      s.patch.branch=q<=mix?'MIX':g.actual[r1]<=f.bl[c]?'MIX_LIGHT':'MIX_LIGHT_HEAVY';
      if(g.actual[r2]>f.bh[c])return false;
      const oa=g.oa,t=this.interval;
      oa.addInto(f.mh[c],f.ml[c],t[0]);g.set(t[1],Math.max(mix,f.eps),Math.max(mix,f.eps));
      oa.intervalDivInto(t[0],t[1],t[2]);s.ALo=t[2].lo;s.AHi=t[2].hi;
      g.set(t[3],mix,mix);oa.intervalMulInto(t[2],t[3],t[4]);oa.intervalSubInto(t[4],t[3],t[5]);
      s.bLo=t[5].lo;s.bHi=t[5].hi;
    }else{
      h=r.phase===0?qNode:g.constant(0);l=r.phase===1?qNode:g.constant(0);
      if(r.phase===0)this.materialNodes[2]=g.op(1,this.materialNodes[2],qNode);
      else this.materialNodes[3]=g.op(1,this.materialNodes[3],qNode);
      const mass=f.poolMass[this.slot];const rem=q===mass?g.constant(0):g.op(2,g.constant(mass),qNode);
      s.poolRemainder=g.actual[rem];if(q<mass&&!(s.poolRemainder<mass))return false;
      const cacheIndex=r.phase===0?8:9;
      this.after[cacheIndex]=g.actual[g.op(2,g.constant(this.before[cacheIndex]),qNode)];
      this.slotAfter.set(this.slotBefore);this.slotAfter[0]=s.poolRemainder;
      this.retired=q===mass;s.poolRetired=this.retired;s.slot=this.slot;s.particleId=r.particleId;
      if(this.retired)this.slotAfter[2]=0;
      // T148 supports already waiting particles: motion/source fields stay frozen.
      s.ALo=1;s.AHi=1;s.bLo=0;s.bHi=0;s.patch.branch=q===mass?'FULL':'PARTIAL';
    }
    s.phaseHeavy=g.actual[h];s.phaseLight=g.actual[l];s.qControl=q;
    const rows=this.receiver<0?1:2;
    for(let row=0;row<rows;row++){
      const base=row*15,col=row===0?c:r.receiver;
      for(let i=0;i<4;i++)this.after[base+i]=g.actual[this.materialNodes[row*4+i]];
      const top=g.op(1,g.op(1,g.op(1,this.materialNodes[row*4],this.materialNodes[row*4+1]),
        this.materialNodes[row*4+2]),this.materialNodes[row*4+3]);
      this.after[base+6]=g.actual[top];
      const oa=g.oa,t=this.interval;
      oa.addInto(this.before[base],this.before[base+1],t[0]);g.set(t[1],this.before[base+2],this.before[base+2]);
      oa.intervalAddInto(t[0],t[1],t[2]);g.set(t[1],this.before[base+3],this.before[base+3]);
      oa.intervalAddInto(t[2],t[1],t[0]);g.set(t[1],this.before[base+6],this.before[base+6]);
      oa.intervalSubInto(t[1],t[0],t[2]);s.delta0Lo[row]=t[2].lo;s.delta0Hi[row]=t[2].hi;
      s.coefficientLo[row]=g.coefficient[top].lo;s.coefficientHi[row]=g.coefficient[top].hi;
      oa.intervalAddInto(g.intercept[top],g.error[top],t[0]);oa.intervalSubInto(t[0],t[1],t[2]);
      s.interceptLo[row]=t[2].lo;s.interceptHi[row]=t[2].hi;
      s.patch.changedColumns[row]=col;
      for(let i=0;i<7;i++){s.patch.beforeBits[row*7+i]=this.before[base+i];s.patch.afterBits[row*7+i]=this.after[base+i];}
    }
    s.patch.changedCount=rows;s.patch.operationGraphRevision=this.kernel;s.nodeCount=g.count;
    g.envelope.addBranch(1,lo,hi,false,false);return g.valid;
  }
  private exactSign(a: number,q: number,count: number): number {
    const r=this.shadow.graph.oa.exact.compareAffineBound(a,q,this.terms,count);
    const code=r.code,sign=r.sign;return code==='OK'?sign:2;
  }
  private envelopeContains(): boolean {
    const s=this.shadow;
    for(let i=0;i<s.patch.changedCount;i++){
      this.terms[0]=s.patch.afterBits[i*7+6];this.terms[1]=-s.patch.beforeBits[i*7+6];
      this.terms[2]=-s.interceptLo[i];if(this.exactSign(-s.coefficientLo[i],s.qControl,3)<0)return false;
      this.terms[2]=-s.interceptHi[i];const sign=this.exactSign(-s.coefficientHi[i],s.qControl,3);
      if(sign===2||sign>0)return false;
    }return true;
  }
  private audit(f: P011KernelFrozen,r: P011KernelRequest): boolean {
    const s=this.shadow;
    for(let row=0;row<s.patch.changedCount;row++){
      const base=row*15;for(let i=0;i<15;i++)if(!Number.isFinite(this.after[base+i])||
        (i!==6&&this.after[base+i]<0))return false;
      if(this.after[base+6]>f.capacity)return false;
      if(this.after[base+10]>this.after[base+4]||this.after[base+11]>this.after[base+5])return false;
      if(r.sourceActive&&this.kernel==='SETTLE_RN_GRAPH_V1'){
        if(Math.abs((this.after[base+10]+this.after[base+12])-
          (this.before[base+10]+this.before[base+12]))>1e-9||
          Math.abs((this.after[base+11]+this.after[base+13])-
          (this.before[base+11]+this.before[base+13]))>1e-9)return false;
      }
      // Existing material mass tolerance; it does not enlarge the exact edge bound.
      const dh=(this.after[base]+this.after[base+2]+this.after[base+4])-
        (this.before[base]+this.before[base+2]+this.before[base+4]);
      const dl=(this.after[base+1]+this.after[base+3]+this.after[base+5])-
        (this.before[base+1]+this.before[base+3]+this.before[base+5]);
      if(this.kernel==='SETTLE_RN_GRAPH_V1'&&
        (Math.abs(dh)>1e-9||Math.abs(dl)>1e-9))return false;
      if(f.dry)this.after[base+7]=((this.after[base+6]+this.after[base+4])+this.after[base+5])+
        (this.after[base+8]+this.after[base+9]);
      if(!Number.isFinite(this.after[base+7])||this.after[base+7]>f.capacity)return false;
    }
    if(this.kernel==='LAND_RN_GRAPH_V1'){
      const q=s.qControl,m=this.slotBefore[0];
      if(Math.abs((s.poolRemainder+q)-m)>1e-9||this.after[r.phase===0?8:9]<0)return false;
      const p=r.phase;
      const bedChange=(this.after[p]+this.after[p+2])-(this.before[p]+this.before[p+2]);
      if(Math.abs(bedChange+(s.poolRemainder-m))>1e-9||
        Math.abs((this.after[p===0?8:9]-this.before[p===0?8:9])-(s.poolRemainder-m))>1e-9)return false;
      if(this.slotBefore[11]!==f.nextId)return false;
    }
    if(this.kernel==='TOP_SPLIT_RN_GRAPH_V1'){
      for(let phase=0;phase<2;phase++){
        let change=0;for(let row=0;row<2;row++){
          const b=row*15;change+=(this.after[b+phase]+this.after[b+2+phase])-
            (this.before[b+phase]+this.before[b+2+phase]);
        }if(Math.abs(change)>1e-9)return false;
      }
    }
    return true;
  }
  private local(f: P011KernelFrozen): boolean {
    const c=this.verifier.verifyLocalPatch(f.identity,f.ownerId,f.snapshotSerial,this.shadow.patch,
      f.bh,f.bl,f.mh,f.ml,f.surface,f.u,f.hardMax,f.capacity,'LOCAL');return c.legal;
  }
  private constrain(a: number,n: number): boolean {
    const s=this.shadow,oa=s.graph.oa,t=this.interval;
    if(a===0)return this.exactSign(0,0,n)<=0;
    s.graph.set(t[0],0,0);
    for(let i=0;i<n;i++){s.graph.set(t[1],this.terms[i],this.terms[i]);
      oa.intervalAddInto(t[0],t[1],t[2]);s.graph.set(t[0],t[2].lo,t[2].hi);}
    if(oa.divInto(-t[0].hi,a,t[3])!=='OK')return false;
    if(a>0)s.domainHi=Math.min(s.domainHi,t[3].lo);else s.domainLo=Math.max(s.domainLo,t[3].hi);
    return s.domainLo<=s.domainHi;
  }
  private edgeTerms(left: number,right: number,u: number,h: number,b: number,upper: boolean): number {
    this.terms[0]=right;this.terms[1]=-left;this.terms[2]=-u;this.terms[3]=-h;
    const hi=this.exactSign(0,0,4);
    this.terms[0]=-right;this.terms[1]=left;this.terms[2]=u;
    const low=this.exactSign(0,0,4);
    if(hi===2||low===2)return 0;
    if(hi<=0&&low<=0){
      this.terms[0]=upper?right:-right;this.terms[1]=upper?-left:left;
      this.terms[2]=upper?-u:u;this.terms[3]=-h;this.terms[4]=b;return 5;
    }
    this.terms[0]=right;this.terms[1]=-left;this.terms[2]=-u;
    const sign=this.exactSign(0,0,3);
    if((upper&&sign>=0)||(!upper&&sign<0)){this.terms[0]=b;return 1;}
    const p=upper?1:-1;
    this.terms[0]=p*right;this.terms[1]=-p*left;this.terms[2]=-p*u;
    this.terms[3]=p*right;this.terms[4]=-p*left;this.terms[5]=-p*u;this.terms[6]=b;return 7;
  }
  private solve(f: P011KernelFrozen): number {
    const s=this.shadow,oa=s.graph.oa,t=this.interval;
    const start=Math.max(0,Math.min(this.donor,this.receiver<0?this.donor:this.receiver)-1);
    const end=Math.min(f.count-2,Math.max(this.donor,this.receiver<0?this.donor:this.receiver));
    for(let e=start;e<=end;e++){
      let la=0,lb=0,ha=0,hb=0;
      for(let row=0;row<s.patch.changedCount;row++){
        const c=s.patch.changedColumns[row];if(c!==e&&c!==e+1)continue;
        const p=c===e?-1:1;
        oa.addInto(la,p>0?s.coefficientLo[row]:-s.coefficientHi[row],t[0]);la=t[0].lo;
        oa.addInto(ha,p>0?s.coefficientHi[row]:-s.coefficientLo[row],t[0]);ha=t[0].hi;
        oa.addInto(lb,p>0?s.interceptLo[row]:-s.interceptHi[row],t[0]);lb=t[0].lo;
        oa.addInto(hb,p>0?s.interceptHi[row]:-s.interceptLo[row],t[0]);hb=t[0].hi;
      }
      let n=this.edgeTerms(f.surface[e],f.surface[e+1],f.u,f.hardMax,hb,true);
      if(n===0||!this.constrain(ha,n))return 0;
      n=this.edgeTerms(f.surface[e],f.surface[e+1],f.u,f.hardMax,-lb,false);
      if(n===0||!this.constrain(-la,n))return 0;
    }
    if(!(s.domainHi>=s.domainLo&&s.domainHi>0))return 0;
    // Verify both endpoints against every exact short affine inequality.
    const q=s.domainHi;
    for(let e=start;e<=end;e++){
      let loA=0,hiA=0,loB=0,hiB=0;
      for(let row=0;row<s.patch.changedCount;row++){
        const c=s.patch.changedColumns[row];if(c!==e&&c!==e+1)continue;const p=c===e?-1:1;
        oa.addInto(loA,p>0?s.coefficientLo[row]:-s.coefficientHi[row],t[0]);loA=t[0].lo;
        oa.addInto(hiA,p>0?s.coefficientHi[row]:-s.coefficientLo[row],t[0]);hiA=t[0].hi;
        oa.addInto(loB,p>0?s.interceptLo[row]:-s.interceptHi[row],t[0]);loB=t[0].lo;
        oa.addInto(hiB,p>0?s.interceptHi[row]:-s.interceptLo[row],t[0]);hiB=t[0].hi;
      }
      let n=this.edgeTerms(f.surface[e],f.surface[e+1],f.u,f.hardMax,hiB,true);
      if(this.exactSign(hiA,q,n)>0||this.exactSign(hiA,s.domainLo,n)>0)return 0;
      n=this.edgeTerms(f.surface[e],f.surface[e+1],f.u,f.hardMax,-loB,false);
      if(this.exactSign(-loA,q,n)>0||this.exactSign(-loA,s.domainLo,n)>0)return 0;
    }return q;
  }
  private seal(f: P011KernelFrozen,r: P011KernelRequest,decision: P011KernelDecision,q: number): P011KernelShadow {
    const s=this.shadow;s.decision=decision;s.reason='OK';s.qControl=q;s.refund=r.actualDebit-q;
    if(!Number.isFinite(f.totalRemaining+s.refund)||!Number.isFinite(f.localRemaining[this.donor]+s.refund)||
      (this.receiver>=0&&!Number.isFinite(f.localRemaining[this.receiver]+s.refund))){
      s.decision='UNSUPPORTED';s.reason='AUDIT_FAILED';return s;
    }
    s.patch.qGranted=r.qGranted;s.patch.qActual=q;s.patch.budgetDebit=r.actualDebit;
    s.patch.budgetRefund=s.refund;this.sealedQ=q;this.sealedRefund=s.refund;this.sealedDecision=decision;
    this.sealedMass=s.poolRemainder;this.pending=s.token;
    this.sealedAfter.set(this.after);this.sealedPatch.changedCount=s.patch.changedCount;
    this.sealedPatch.changedColumns.set(s.patch.changedColumns);
    this.sealedPatch.beforeBits.set(s.patch.beforeBits);this.sealedPatch.afterBits.set(s.patch.afterBits);
    for(let i=0;i<30;i++)s.columnBefore[i]=this.before[i];s.columnAfter.set(this.after);
    s.poolBefore.set(this.slotBefore);s.poolAfter.set(this.slotAfter);s.ledgerBefore.set(this.scalarBefore);
    s.transactionAudited=true;this.sealedRequested=r.requested;this.sealedPhase=r.phase;
    return s;
  }
  preview(f: P011KernelFrozen,r: P011KernelRequest): P011KernelShadow {
    this.reset(f,r);const s=this.shadow,status=p011KernelStatus(r.kernel,r.particleState);
    if(status!=='SUPPORTED'){s.reason=status;return s;}
    if(!this.freeze(f,r))return s;
    if(r.qGranted===0||(this.kernel!=='LAND_RN_GRAPH_V1'&&r.qGranted<=f.eps))return this.zero(f,r);
    const oa=s.graph.oa;let lo=oa.nextUp(0),hi=r.qGranted;
    let inward=true;
    if(this.kernel==='TOP_SPLIT_RN_GRAPH_V1'){
      const mix=f.mh[r.column]+f.ml[r.column];lo=oa.nextUp(f.eps);
      inward=r.qGranted<=mix;if(!inward)lo=hi;
      else hi=Math.min(hi,mix);
    }else if(this.kernel==='LAND_RN_GRAPH_V1'){
      const mass=f.poolMass[r.slot];inward=r.qGranted<mass;
      if(inward){lo=mass-oa.nextDown(mass);hi=Math.min(hi,oa.nextDown(mass));}
      else lo=hi;
    }
    if(lo>hi)return this.zero(f,r);
    if(!this.build(f,r,r.qGranted,lo,hi)){s.reason='STOCK_INVALID';return s;}
    if(!this.envelopeContains()){s.reason='ENVELOPE_CERTIFICATE_MISMATCH';return s;}
    if(!this.audit(f,r)){s.reason='AUDIT_FAILED';return s;}
    if(this.local(f))return this.seal(f,r,'RAW',r.qGranted);
    if(this.verifier.certificate.reason!=='STRICT_EDGE'){s.reason='INVALID_INPUT';return s;}
    if(inward){
      const q=this.solve(f);
      s.selectedDomainLo=s.domainLo;s.selectedDomainHi=s.domainHi;
      if(q>0&&q<r.qGranted){
        // Keep the original certified domain and entry bound, never rebase on RAW scratch.
        if(!this.build(f,r,q,lo,hi)||!this.envelopeContains()){
          s.reason='ENVELOPE_CERTIFICATE_MISMATCH';return s;}
        if(!this.audit(f,r)){s.reason='AUDIT_FAILED';return s;}
        if(!this.local(f)){
          s.reason='ENVELOPE_CERTIFICATE_MISMATCH';return s;}
        return this.seal(f,r,'INWARD',q);
      }s.inwardReason='INWARD_NONE';
    }else s.inwardReason='INWARD_UNSUPPORTED';
    return this.zero(f,r);
  }
  private zero(f: P011KernelFrozen,r: P011KernelRequest): P011KernelShadow {
    const s=this.shadow;s.zeroCount=1;s.noWrite.prepare(false);s.patch.changedCount=0;
    s.phaseHeavy=0;s.phaseLight=0;s.poolRetired=false;s.slot=-1;s.poolRemainder=0;
    return this.seal(f,r,'ZERO',0);
  }
  commit(f: P011KernelFrozen,s: P011KernelShadow,ownerId: number,snapshotSerial: number,token: number): P011KernelReason {
    if(this.pending===0||token!==this.pending||s!==this.shadow)return 'STALE_TOKEN';
    if(this.owner!==f||f.ownerId!==ownerId||f.snapshotSerial!==snapshotSerial||
      this.identityOf(f)!==this.identity)return 'IDENTITY_MISMATCH';
    for(let row=0;row<this.count;row++)for(let i=0;i<15;i++)
      if(!Object.is(this.before[row*15+i],f.columnArrays[i][this.columns[row]]))return 'IDENTITY_MISMATCH';
    const b=this.scalarBefore;
    if(!Object.is(b[0],f.totalRemaining)||b[1]!==f.ticketToken||b[2]!==f.ticketState||
      !Object.is(b[3],f.ticketDebit)||!Object.is(b[4],f.ticketRefund)||b[5]!==f.freeCount||
      b[6]!==f.liveCount||b[7]!==f.nextId||b[8]!==f.eventCount||b[9]!==f.lineageCount||
      b[10]!== (f.freeSlots[f.freeCount]??0))return 'IDENTITY_MISMATCH';
    if(this.slot>=0){
      const saved=this.slotAfter;this.slotRead(f,saved);
      for(let i=0;i<12;i++)if(!Object.is(saved[i],this.slotBefore[i]))return 'IDENTITY_MISMATCH';
    }
    // All validation, capacity and audits complete before the first authority write.
    if(this.sealedDecision!=='ZERO'){
      for(let row=0;row<this.sealedPatch.changedCount;row++){
        const c=this.sealedPatch.changedColumns[row];
        for(let i=0;i<15;i++)if(i!==14&&!Object.is(f.columnArrays[i][c],this.sealedAfter[row*15+i]))
          f.columnArrays[i][c]=this.sealedAfter[row*15+i];
      }
      if(this.slot>=0){
        const i=this.slot;f.poolMass[i]=this.sealedMass;
        if(this.retired){f.poolState[i]=0;f.freeSlots[f.freeCount++]=i;f.liveCount--;}
        // CONTACT_WAIT partial retains position, velocity and all source/ID fields.
        f.eventKind[f.eventCount]=2;f.eventId[f.eventCount]=this.requestId;
        f.eventMass[f.eventCount]=this.sealedQ;f.eventSource[f.eventCount]=this.eventSource;
        f.eventRequested[f.eventCount]=this.sealedRequested;f.eventRefund[f.eventCount]=this.sealedRefund;
        f.eventTicket[f.eventCount]=f.ticketToken;f.eventColumn[f.eventCount]=this.donor;
        f.eventPhase[f.eventCount++]=this.sealedPhase;
        f.lineageId[f.lineageCount++]=this.requestId;
      }
    }
    f.localRemaining[this.donor]+=this.sealedRefund;
    if(this.receiver>=0)f.localRemaining[this.receiver]+=this.sealedRefund;
    f.totalRemaining+=this.sealedRefund;f.ticketRefund=this.sealedRefund;
    f.ticketState=this.sealedDecision==='ZERO'?3:2;
    f.snapshotSerial++;f.identity.snapshotSerial=f.snapshotSerial;
    this.pending=0;return 'OK';
  }
}
