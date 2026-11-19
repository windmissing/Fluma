import { P011ExactDyadic, P011ExactCode } from './P011ExactDyadic';

export type P011CertificateScope = 'LOCAL' | 'FINAL';
export type P011CertificateReason = 'OK' | 'CANONICAL_S_MISMATCH' | 'STRICT_EDGE' |
  'NON_FINITE' | 'CERTIFICATE_CAPACITY' | 'ARITHMETIC_ENV_UNSUPPORTED' |
  'ARITHMETIC_EXPRESSION_UNSUPPORTED';
export class P011StrictInputIdentity {
  legalStateRevision: string = 'P011_STRICT_REPRESENTABLE_STATE_V1';
  decisionArithmeticRevision: string = 'DYADIC_SIGN_V1';
  runId: string = ''; stateEpoch: number = 0; attemptId: string = ''; substepId: number = 0;
  configRevision: number = 0; snapshotSerial: number = 0; thetaBits: string = '';
  uThetaBits: string = ''; hBits: string = ''; writerKernelRevision: string = '';
}
export class P011ActualMaterialPatch {
  readonly changedColumns: Int32Array;
  readonly beforeBits: Float64Array; readonly afterBits: Float64Array;
  changedCount: number = 0; poolPatchDigest: string = ''; budgetDebit: number = 0;
  budgetRefund: number = 0; qGranted: number = 0; qActual: number = 0;
  branch: string = ''; operationGraphRevision: string = '';
  constructor(capacity: number) {
    this.changedColumns=new Int32Array(capacity);
    this.beforeBits=new Float64Array(capacity*7);this.afterBits=new Float64Array(capacity*7);
  }
  addColumn(column: number, before: Float64Array, after: Float64Array): P011CertificateReason {
    if(this.changedCount>=this.changedColumns.length||before.length!==7||after.length!==7)return 'CERTIFICATE_CAPACITY';
    const row=this.changedCount++;this.changedColumns[row]=column;
    for(let i=0;i<7;i++){this.beforeBits[row*7+i]=before[i];this.afterBits[row*7+i]=after[i];}
    return 'OK';
  }
}
export class P011StateCertificate {
  readonly identity: P011StrictInputIdentity = new P011StrictInputIdentity();
  scope: P011CertificateScope = 'LOCAL'; legal: boolean = false;
  firstEdge: number = -1; reason: P011CertificateReason = 'OK'; certificateSerial: number = 0;
  canonicalS1Digest: string = ''; patchDigest: string = ''; fullStateDigest: string = '';
  exactSign: number = 0; diagnosticResidual: number = 0;
}
export class P011WriterCandidate {
  caller: string = ''; branch: string = ''; inputDigest: string = '';
  readonly patch: P011ActualMaterialPatch; actualDebit: number = 0; actualRefund: number = 0;
  candidateOrdinal: number = 0; envelopeRevision: string = '';
  readonly localCertificate: P011StateCertificate = new P011StateCertificate();
  constructor(capacity: number) { this.patch=new P011ActualMaterialPatch(capacity); }
}
export function p011CanonicalSandTop(bh: number, bl: number, mh: number, ml: number): number {
  return (((bh+bl)+mh)+ml);
}
export class P011StrictStateCertifier {
  readonly exact: P011ExactDyadic = new P011ExactDyadic();
  readonly capacity: number;
  readonly certificate: P011StateCertificate = new P011StateCertificate();
  private serial: number = 0;
  constructor(capacity: number) { this.capacity=capacity; }
  verifySandTop(bh: Float64Array,bl: Float64Array,mh: Float64Array,ml: Float64Array,
    cached: Float64Array,thetaTangent: number,hardMax: number,scope: P011CertificateScope): P011StateCertificate {
    const c=this.certificate;c.scope=scope;c.legal=false;c.firstEdge=-1;c.reason='OK';c.exactSign=0;
    c.certificateSerial=++this.serial;
    if(bh.length>this.capacity||bh.length!==bl.length||bh.length!==mh.length||bh.length!==ml.length||bh.length!==cached.length){c.reason='CERTIFICATE_CAPACITY';return c;}
    if(!Number.isFinite(thetaTangent)||!Number.isFinite(hardMax)||hardMax<0){c.reason='NON_FINITE';return c;}
    let previous=0;
    for(let i=0;i<bh.length;i++){
      const s=p011CanonicalSandTop(bh[i],bl[i],mh[i],ml[i]);
      if(!Number.isFinite(s)||bh[i]<0||bl[i]<0||mh[i]<0||ml[i]<0){c.reason='NON_FINITE';c.firstEdge=i;return c;}
      if(!Object.is(s,cached[i])){c.reason='CANONICAL_S_MISMATCH';c.firstEdge=i;return c;}
      if(i>0){
        const hi=this.exact.compareSum(previous,s,thetaTangent,hardMax);
        if(hi.code!=='OK'){c.reason=hi.code as P011CertificateReason;c.firstEdge=i-1;return c;}
        const hiSign=hi.sign;
        const low=this.exact.compareSum(s,previous,-thetaTangent,hardMax);
        if(low.code!=='OK'){c.reason=low.code as P011CertificateReason;c.firstEdge=i-1;return c;}
        if(hiSign>0||low.sign>0){c.reason='STRICT_EDGE';c.firstEdge=i-1;c.exactSign=hiSign>0?hiSign:low.sign;return c;}
      }
      previous=s;
    }
    c.legal=true;return c;
  }
}
