// T133 stage 1. Bounded shadow schemas and material-only local edge verifier.
// The production strict route is intentionally not wired to these classes.
import { P011ExactDyadic } from './P011ExactDyadic';
import { P011ActualMaterialPatch, P011StrictInputIdentity, p011CanonicalSandTop } from './P011StateCertificate';

export type P011EnvelopeCode='OK'|'NODE_CAPACITY'|'BRANCH_CAPACITY'|'INVALID_NODE'|'INVALID_BRANCH';
export class P011WriterEnvelope {
  readonly manifestRevision: string='P011_WRITER_ENVELOPE_STAGE1_V1';
  readonly op: Uint8Array;readonly inputA: Int16Array;readonly inputB: Int16Array;
  readonly actual: Float64Array;readonly idealLo: Float64Array;readonly idealHi: Float64Array;
  readonly errorLo: Float64Array;readonly errorHi: Float64Array;readonly eta: Float64Array;
  readonly branch: Int16Array;readonly provenance: Int16Array;
  readonly branchKind: Uint8Array;readonly branchLo: Float64Array;readonly branchHi: Float64Array;
  readonly branchOpenLo: Uint8Array;readonly branchOpenHi: Uint8Array;
  nodeCount: number=0;branchCount: number=0;reason: P011EnvelopeCode='OK';
  constructor(nodeCapacity: number,branchCapacity: number){
    if(!Number.isSafeInteger(nodeCapacity)||nodeCapacity<1||nodeCapacity>128||
      !Number.isSafeInteger(branchCapacity)||branchCapacity<1||branchCapacity>16)
      throw new Error('P011_WRITER_ENVELOPE_CAPACITY');
    this.op=new Uint8Array(nodeCapacity);this.inputA=new Int16Array(nodeCapacity);this.inputB=new Int16Array(nodeCapacity);
    this.actual=new Float64Array(nodeCapacity);this.idealLo=new Float64Array(nodeCapacity);
    this.idealHi=new Float64Array(nodeCapacity);this.errorLo=new Float64Array(nodeCapacity);
    this.errorHi=new Float64Array(nodeCapacity);this.eta=new Float64Array(nodeCapacity);
    this.branch=new Int16Array(nodeCapacity);this.provenance=new Int16Array(nodeCapacity);
    this.branchKind=new Uint8Array(branchCapacity);this.branchLo=new Float64Array(branchCapacity);
    this.branchHi=new Float64Array(branchCapacity);this.branchOpenLo=new Uint8Array(branchCapacity);
    this.branchOpenHi=new Uint8Array(branchCapacity);
  }
  reset(): void {this.nodeCount=0;this.branchCount=0;this.reason='OK';}
  addBranch(kind: number,lo: number,hi: number,openLo: boolean,openHi: boolean): number {
    if(this.branchCount>=this.branchKind.length){this.reason='BRANCH_CAPACITY';return -1;}
    if(!Number.isSafeInteger(kind)||kind<1||kind>255||!Number.isFinite(lo)||!Number.isFinite(hi)||
      lo>hi||(lo===hi&&(openLo||openHi))){this.reason='INVALID_BRANCH';return -1;}
    const i=this.branchCount++;this.branchKind[i]=kind;this.branchLo[i]=lo;this.branchHi[i]=hi;
    this.branchOpenLo[i]=openLo?1:0;this.branchOpenHi[i]=openHi?1:0;return i;
  }
  addNode(op: number,inputA: number,inputB: number,actual: number,idealLo: number,idealHi: number,
    errorLo: number,errorHi: number,eta: number,branch: number,provenance: number): number {
    if(this.nodeCount>=this.op.length){this.reason='NODE_CAPACITY';return -1;}
    if(!Number.isSafeInteger(op)||op<1||op>7||!Number.isSafeInteger(inputA)||!Number.isSafeInteger(inputB)||
      inputA>=this.nodeCount||inputB>=this.nodeCount||inputA<-1||inputB<-1||
      !Number.isFinite(actual)||!Number.isFinite(idealLo)||!Number.isFinite(idealHi)||
      !Number.isFinite(errorLo)||!Number.isFinite(errorHi)||!Number.isFinite(eta)||
      idealLo>idealHi||errorLo>errorHi||eta<0||branch<-1||branch>=this.branchCount||
      !Number.isSafeInteger(provenance)||provenance<0||provenance>32767){this.reason='INVALID_NODE';return -1;}
    const i=this.nodeCount++;this.op[i]=op;this.inputA[i]=inputA;this.inputB[i]=inputB;
    this.actual[i]=actual;this.idealLo[i]=idealLo;this.idealHi[i]=idealHi;
    this.errorLo[i]=errorLo;this.errorHi[i]=errorHi;this.eta[i]=eta;
    this.branch[i]=branch;this.provenance[i]=provenance;return i;
  }
  coldBuffers(): ArrayBuffer[]{return [this.op.buffer,this.inputA.buffer,this.inputB.buffer,this.actual.buffer,
    this.idealLo.buffer,this.idealHi.buffer,this.errorLo.buffer,this.errorHi.buffer,this.eta.buffer,
    this.branch.buffer,this.provenance.buffer,this.branchKind.buffer,this.branchLo.buffer,this.branchHi.buffer,
    this.branchOpenLo.buffer,this.branchOpenHi.buffer];}
}

export type P011LocalReason='OK'|'INVALID_IDENTITY'|'CERTIFICATE_CAPACITY'|'BEFORE_MISMATCH'|
  'CANONICAL_S_MISMATCH'|'NON_FINITE'|'STOCK_INVALID'|'STRICT_EDGE'|
  'ARITHMETIC_ENV_UNSUPPORTED'|'ARITHMETIC_EXPRESSION_UNSUPPORTED';
export class P011LocalPatchCertificate {
  scope: string='LOCAL';boundMode: string='ENTRY_H_OR_OLD_RESIDUAL';
  legal: boolean=false;materialOnly: boolean=true;reason: P011LocalReason='INVALID_IDENTITY';
  firstEdge: number=-1;affectedCount: number=0;ownerId: number=0;snapshotSerial: number=0;
}
export class P011LocalPatchVerifier {
  readonly exact: P011ExactDyadic=new P011ExactDyadic();
  readonly certificate: P011LocalPatchCertificate=new P011LocalPatchCertificate();
  readonly affected: Int32Array=new Int32Array(4);
  private readonly terms: Float64Array=new Float64Array(7);
  constructor(readonly capacity: number){}
  private sign(count: number): number {
    const result=this.exact.compareAffineBound(0,0,this.terms,count);
    const code=result.code,sign=result.sign; // shared result snapshot
    if(code!=='OK')return 2;
    return sign;
  }
  private sandAt(c: number,patch: P011ActualMaterialPatch,bh: Float64Array,bl: Float64Array,
    mh: Float64Array,ml: Float64Array,before: boolean): number {
    for(let row=0;row<patch.changedCount;row++)if(patch.changedColumns[row]===c){
      const bits=before?patch.beforeBits:patch.afterBits,base=row*7;
      return p011CanonicalSandTop(bits[base],bits[base+1],bits[base+2],bits[base+3]);
    }
    return p011CanonicalSandTop(bh[c],bl[c],mh[c],ml[c]);
  }
  private addEdge(e: number,cols: number,count: number): number {
    if(e<0||e>=cols-1)return count;
    for(let i=0;i<count;i++)if(this.affected[i]===e)return count;
    if(count>=4)return -1;this.affected[count]=e;return count+1;
  }
  private legalAgainstH(left: number,right: number,u: number,h: number): boolean {
    this.terms[0]=right;this.terms[1]=-left;this.terms[2]=-u;this.terms[3]=-h;
    const hi=this.sign(4);if(hi===2)return false;
    this.terms[0]=-right;this.terms[1]=left;this.terms[2]=u;
    const low=this.sign(4);return low!==2&&hi<=0&&low<=0;
  }
  verifyLocalPatch(identity: P011StrictInputIdentity,ownerId: number,snapshotSerial: number,
    patch: P011ActualMaterialPatch,bh: Float64Array,bl: Float64Array,mh: Float64Array,ml: Float64Array,
    surface: Float64Array,u: number,h: number,capacity: number,scope: 'LOCAL'|'FINAL'): P011LocalPatchCertificate {
    const cert=this.certificate;cert.scope=scope;cert.boundMode=scope==='FINAL'?'H_ALL_EDGES':'ENTRY_H_OR_OLD_RESIDUAL';
    cert.legal=false;cert.reason='INVALID_IDENTITY';cert.firstEdge=-1;cert.affectedCount=0;
    cert.ownerId=ownerId;cert.snapshotSerial=snapshotSerial;
    if(ownerId<1||snapshotSerial<1||identity.snapshotSerial!==snapshotSerial||!identity.runId||
      !identity.attemptId||!identity.writerKernelRevision)return cert;
    const cols=bh.length;
    if(cols<2||cols>this.capacity||bl.length!==cols||mh.length!==cols||ml.length!==cols||
      surface.length!==cols||patch.changedCount<0||patch.changedCount>2||
      patch.changedCount>patch.changedColumns.length||!Number.isFinite(u)||!Number.isFinite(h)||
      h<0||!Number.isFinite(capacity)||capacity<=0){cert.reason='CERTIFICATE_CAPACITY';return cert;}
    let count=0;
    for(let row=0;row<patch.changedCount;row++){
      const c=patch.changedColumns[row],base=row*7;if(c<0||c>=cols){cert.reason='CERTIFICATE_CAPACITY';return cert;}
      for(let prior=0;prior<row;prior++)if(patch.changedColumns[prior]===c){cert.reason='CERTIFICATE_CAPACITY';return cert;}
      if(base+7>patch.beforeBits.length||base+7>patch.afterBits.length){cert.reason='CERTIFICATE_CAPACITY';return cert;}
      if(!Object.is(patch.beforeBits[base],bh[c])||!Object.is(patch.beforeBits[base+1],bl[c])||
        !Object.is(patch.beforeBits[base+2],mh[c])||!Object.is(patch.beforeBits[base+3],ml[c])){
        cert.reason='BEFORE_MISMATCH';return cert;
      }
      for(let k=0;k<7;k++)if(!Number.isFinite(patch.afterBits[base+k])){cert.reason='NON_FINITE';return cert;}
      for(let k=0;k<4;k++)if(patch.afterBits[base+k]<0){cert.reason='STOCK_INVALID';return cert;}
      const after=this.sandAt(c,patch,bh,bl,mh,ml,false);
      if(!Number.isFinite(after)||after>capacity){cert.reason='STOCK_INVALID';return cert;}
      count=this.addEdge(c-1,cols,count);if(count<0){cert.reason='CERTIFICATE_CAPACITY';return cert;}
      count=this.addEdge(c,cols,count);if(count<0){cert.reason='CERTIFICATE_CAPACITY';return cert;}
    }
    cert.affectedCount=count;
    if(scope==='FINAL')for(let c=0;c<cols;c++){
      let changedRow=-1;for(let row=0;row<patch.changedCount;row++)if(patch.changedColumns[row]===c)changedRow=row;
      const base=changedRow*7;
      const xbh=changedRow<0?bh[c]:patch.afterBits[base];
      const xbl=changedRow<0?bl[c]:patch.afterBits[base+1];
      const xmh=changedRow<0?mh[c]:patch.afterBits[base+2];
      const xml=changedRow<0?ml[c]:patch.afterBits[base+3];
      const top=p011CanonicalSandTop(xbh,xbl,xmh,xml);
      if(!Number.isFinite(xbh)||!Number.isFinite(xbl)||!Number.isFinite(xmh)||!Number.isFinite(xml)||
        xbh<0||xbl<0||xmh<0||xml<0||!Number.isFinite(top)||top>capacity){
        cert.reason='STOCK_INVALID';cert.firstEdge=c;return cert;
      }
      if(changedRow<0&&!Object.is(surface[c],top)){
        cert.reason='CANONICAL_S_MISMATCH';cert.firstEdge=c;return cert;
      }
    }
    const edgeCount=scope==='FINAL'?cols-1:count;
    for(let i=0;i<edgeCount;i++){
      const edge=scope==='FINAL'?i:this.affected[i];
      const blS=this.sandAt(edge,patch,bh,bl,mh,ml,true),brS=this.sandAt(edge+1,patch,bh,bl,mh,ml,true);
      const alS=this.sandAt(edge,patch,bh,bl,mh,ml,false),arS=this.sandAt(edge+1,patch,bh,bl,mh,ml,false);
      if(!Number.isFinite(blS)||!Number.isFinite(brS)||!Number.isFinite(alS)||!Number.isFinite(arS)){
        cert.reason='NON_FINITE';cert.firstEdge=edge;return cert;}
      // Read untouched neighbor authority, but do not scan distant edges in LOCAL.
      if(scope==='LOCAL')for(let c=edge;c<=edge+1;c++){
        let changed=false;for(let row=0;row<patch.changedCount;row++)if(patch.changedColumns[row]===c)changed=true;
        if(!changed&&!Object.is(surface[c],p011CanonicalSandTop(bh[c],bl[c],mh[c],ml[c]))){
          cert.reason='CANONICAL_S_MISMATCH';cert.firstEdge=edge;return cert;}
      }
      if(scope==='FINAL'||this.legalAgainstH(blS,brS,u,h)){
        if(!this.legalAgainstH(alS,arS,u,h)){cert.reason='STRICT_EDGE';cert.firstEdge=edge;return cert;}
      }else{
        this.terms[0]=brS;this.terms[1]=-blS;this.terms[2]=-u;
        const oldSign=this.sign(3);if(oldSign===2){cert.reason='ARITHMETIC_EXPRESSION_UNSUPPORTED';cert.firstEdge=edge;return cert;}
        // |after residual| <= |entry residual| with the same exact entry terms.
        const polarity=oldSign>=0?-1:1;
        this.terms[0]=arS;this.terms[1]=-alS;this.terms[2]=-u;
        this.terms[3]=polarity*brS;this.terms[4]=-polarity*blS;this.terms[5]=-polarity*u;
        const upper=this.sign(6);
        this.terms[0]=-arS;this.terms[1]=alS;this.terms[2]=u;
        this.terms[3]=polarity*brS;this.terms[4]=-polarity*blS;this.terms[5]=-polarity*u;
        const lower=this.sign(6);
        if(upper===2||lower===2||upper>0||lower>0){cert.reason='STRICT_EDGE';cert.firstEdge=edge;return cert;}
      }
    }
    cert.reason='OK';cert.legal=true;return cert;
  }
}

export type P011NoWriteCode='NO_WRITE'|'CONTACT_HOLD_UNSUPPORTED';
export class P011NoWritePatch {
  readonly revision: string='P011_NO_WRITE_PATCH_STAGE1_V1';
  readonly identity: P011StrictInputIdentity=new P011StrictInputIdentity();
  status: P011NoWriteCode='NO_WRITE';changedCount: number=0;qControl: number=0;
  ticketDelta: number=0;eventCount: number=0;poolMutationCount: number=0;
  prepare(contactHoldRequired: boolean): P011NoWriteCode {
    this.changedCount=0;this.qControl=0;this.ticketDelta=0;this.eventCount=0;this.poolMutationCount=0;
    this.status=contactHoldRequired?'CONTACT_HOLD_UNSUPPORTED':'NO_WRITE';return this.status;
  }
  verifyFloatBits(before: Float64Array,after: Float64Array): boolean {
    if(this.status!=='NO_WRITE'||before.length!==after.length)return false;
    for(let i=0;i<before.length;i++)if(!Object.is(before[i],after[i]))return false;
    return true;
  }
  verifyIds(before: Uint32Array,after: Uint32Array): boolean {
    if(this.status!=='NO_WRITE'||before.length!==after.length)return false;
    for(let i=0;i<before.length;i++)if(before[i]!==after[i])return false;
    return true;
  }
}
