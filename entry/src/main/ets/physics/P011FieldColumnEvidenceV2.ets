// Opt-in read-only evidence. The stored snapshot bytes come from the original
// Float64Array buffers, so NaN payloads and signed zero survive serialization.
import { P011ParticleHotFieldBuffer } from './P011ParticleHot';

export const P011_FIELD_COLUMN_EVIDENCE_V2='P011_FIELD_COLUMN_EVIDENCE_V2';
export interface P011FieldStageIdentity {
  runId: string;stateEpoch: number;configRevision: number;substepId: number;
  attemptId: string;candidateRole: string;callOrdinal: number;theta: number;
  commitBefore: number;commitCurrent: number;sourceEpoch: number;
  writerStage: string;preview: boolean;poolEpoch: number;
}
export class P011FieldStageMeta {
  stage: string='';writerStage: string='';preview: boolean=false;
  runId: string='';stateEpoch: number=0;configRevision: number=0;substepId: number=0;
  attemptId: string='';candidateRole: string='';callOrdinal: number=0;theta: number=0;
  commitBefore: number=0;commitCurrent: number=0;sourceEpoch: number=0;poolEpoch: number=0;
  firstInvalidColumn: number=-1;legal: boolean=true;
  set(stage: string,id: P011FieldStageIdentity,column: number): void {
    this.stage=stage;this.writerStage=id.writerStage;this.preview=id.preview;
    this.runId=id.runId;this.stateEpoch=id.stateEpoch;this.configRevision=id.configRevision;
    this.substepId=id.substepId;this.attemptId=id.attemptId;this.candidateRole=id.candidateRole;
    this.callOrdinal=id.callOrdinal;this.theta=id.theta;this.commitBefore=id.commitBefore;
    this.commitCurrent=id.commitCurrent;this.sourceEpoch=id.sourceEpoch;this.poolEpoch=id.poolEpoch;
    this.firstInvalidColumn=column;this.legal=column<0;
  }
}
export interface P011FieldFailureV2 {
  revision: string;column: number;badMask: number;valueText: string[];
  rawBits: string[];sumBits: string[];finite: boolean[];negative: boolean[];
  sumFinite: boolean;fieldBinding: string[];surfaceIsBound: boolean;
  surfaceIsSandTopScratch: boolean;bindingStable: boolean;
  hotOwnerId: number;hotStepEpochBefore: number;hotStepEpochAfter: number;
  runToken: number;attemptToken: number;substepId: number;sourceEpoch: number;
  poolMassHBits: string;poolMassLBits: string;poolEpoch: number;
  poolCanonicalBytesHex: string[];poolCanonicalByteLengths: number[];
  poolStorageValid: boolean;poolLiveCount: number;poolFreeCount: number;poolNextId: number;
}
export interface P011FieldEvidenceExportV2 {
  revision: string;status: string;windowStatus: string;dropped: number;
  lastLegalBytesHex: string|null;firstIllegalBytesHex: string|null;
  stages: P011FieldStageMeta[];failure: P011FieldFailureV2|null;
}
export class P011FieldColumnEvidenceV2 {
  readonly revision: string=P011_FIELD_COLUMN_EVIDENCE_V2;
  readonly cols: number;
  // Exactly two fixed 8 * 128 * 8 byte snapshots when cols=128.
  readonly lastLegalBytes: Uint8Array;readonly firstIllegalBytes: Uint8Array;
  readonly stages: P011FieldStageMeta[]=[];
  private readonly views: DataView[]=[];
  private readonly byteViews: Uint8Array[]=[];
  private readonly sources: Float64Array[]=[];
  private readonly bufferIds: Int32Array=new Int32Array(8);
  private readonly sumBits: DataView=new DataView(new ArrayBuffer(8));
  lastLegalValid: boolean=false;firstIllegalValid: boolean=false;
  windowStatus: string='NO_OBSERVATION';dropped: number=0;
  stageWrite: number=0;stageCount: number=0;
  failure: P011FieldFailureV2|null=null;
  constructor(cols: number){
    this.cols=cols;this.lastLegalBytes=new Uint8Array(8*cols*8);
    this.firstIllegalBytes=new Uint8Array(8*cols*8);
    for(let i=0;i<32;i++)this.stages.push(new P011FieldStageMeta());
  }
  private arrays(f: P011ParticleHotFieldBuffer): Float64Array[]{
    // Called only when enabled. Subsequent stages reuse all eight array refs.
    if(this.sources.length===0){
      this.sources.push(f.waterTop,f.bh,f.bl,f.mh,f.ml,f.suspH,f.suspL,f.surface);
      for(let i=0;i<8;i++){
        this.bufferIds[i]=i+1;
        for(let j=0;j<i;j++)if(this.sources[i].buffer===this.sources[j].buffer){
          this.bufferIds[i]=this.bufferIds[j];break;}
        this.views.push(new DataView(this.sources[i].buffer,
          this.sources[i].byteOffset,this.sources[i].byteLength));
        this.byteViews.push(new Uint8Array(this.sources[i].buffer,
          this.sources[i].byteOffset,this.cols*8));
      }
    }
    return this.sources;
  }
  bind(f: P011ParticleHotFieldBuffer): void {
    if(this.sources.length===0)this.arrays(f);
    else if(!this.sameBinding(f))this.windowStatus='BINDING_CHANGED';
  }
  private sameBinding(f: P011ParticleHotFieldBuffer): boolean {
    const a=this.sources;return a.length===8&&a[0]===f.waterTop&&a[1]===f.bh&&
      a[2]===f.bl&&a[3]===f.mh&&a[4]===f.ml&&a[5]===f.suspH&&
      a[6]===f.suspL&&a[7]===f.surface;
  }
  private scan(f: P011ParticleHotFieldBuffer): number {
    for(let c=0;c<this.cols;c++)if(!Number.isFinite(f.waterTop[c])||
      !Number.isFinite(f.bh[c])||!Number.isFinite(f.bl[c])||!Number.isFinite(f.mh[c])||
      !Number.isFinite(f.ml[c])||!Number.isFinite(f.suspH[c])||
      !Number.isFinite(f.suspL[c])||!Number.isFinite(f.surface[c])||
      f.waterTop[c]<0||f.bh[c]<0||f.bl[c]<0||f.mh[c]<0||f.ml[c]<0||
      f.suspH[c]<0||f.suspL[c]<0||
      !Number.isFinite(((f.bh[c]+f.bl[c])+f.mh[c])+f.ml[c]))return c;
    return -1;
  }
  private copyBytes(target: Uint8Array): void {
    for(let i=0;i<8;i++)target.set(this.byteViews[i],i*this.cols*8);
  }
  observe(stage: string,f: P011ParticleHotFieldBuffer,id: P011FieldStageIdentity): void {
    if(!this.sameBinding(f)){
      if(this.sources.length!==0){this.windowStatus='BINDING_CHANGED';return;}
      this.arrays(f);
    }
    const bad=this.scan(f);
    const row=this.stages[this.stageWrite];row.set(stage,id,bad);
    this.stageWrite=(this.stageWrite+1)&31;
    if(this.stageCount<32)this.stageCount++;else this.dropped++;
    if(bad<0){
      if(!this.firstIllegalValid){this.copyBytes(this.lastLegalBytes);this.lastLegalValid=true;}
      if(this.windowStatus==='NO_OBSERVATION')this.windowStatus='LEGAL_IN_WINDOW';
    }else if(!this.firstIllegalValid){
      this.copyBytes(this.firstIllegalBytes);this.firstIllegalValid=true;
      this.windowStatus=this.lastLegalValid?'LEGAL_TO_INVALID':'PREEXISTING_IN_WINDOW';
    }
  }
  private hex(view: DataView,offset: number): string {
    return view.getUint32(offset,false).toString(16).padStart(8,'0')+
      view.getUint32(offset+4,false).toString(16).padStart(8,'0');
  }
  private rawHex(view: DataView,offset: number): string {
    // ARM/Node Float64Array storage is little endian. Read its two original
    // words directly, then format the conventional high-word-first bit value.
    return view.getUint32(offset+4,true).toString(16).padStart(8,'0')+
      view.getUint32(offset,true).toString(16).padStart(8,'0');
  }
  private numberBits(value: number): string {
    this.sumBits.setFloat64(0,value,false);return this.hex(this.sumBits,0);
  }
  captureFailure(f: P011ParticleHotFieldBuffer,col: number,ownerId: number,
    stepEpoch: number,runToken: number,attemptToken: number,substepId: number,
    poolMassH: number,poolMassL: number,poolEpoch: number,
    poolBuffers: ArrayBuffer[],poolStorageValid: boolean,poolLiveCount: number,
    poolFreeCount: number,poolNextId: number): void {
    if(this.failure!==null)return;
    if(!this.sameBinding(f)){
      if(this.sources.length!==0){this.windowStatus='BINDING_CHANGED';return;}
      this.arrays(f);
    }
    if(!this.firstIllegalValid){
      this.copyBytes(this.firstIllegalBytes);this.firstIllegalValid=true;
      this.windowStatus=this.lastLegalValid?'LEGAL_TO_INVALID':'PREEXISTING_IN_WINDOW';
    }
    const rawBits: string[]=[],valueText: string[]=[],finite: boolean[]=[],negative: boolean[]=[];
    const bindings: string[]=[];let mask=0;
    for(let i=0;i<8;i++){
      const v=this.sources[i][col];
      rawBits.push(this.rawHex(this.views[i],col*8));valueText.push(String(v));
      finite.push(Number.isFinite(v));negative.push(i<7&&v<0);
      if(!finite[i])mask|=1<<i;if(negative[i])mask|=1<<(8+i);
      const a=this.sources[i];bindings.push(`${this.bufferIds[i]}:${a.byteOffset}:${a.length}:${a.byteLength}`);
    }
    const s1=f.bh[col]+f.bl[col],s2=s1+f.mh[col],s3=s2+f.ml[col];
    if(!Number.isFinite(s3))mask|=1<<15;
    const poolHex: string[]=[],poolLengths: number[]=[];
    for(let i=0;i<poolBuffers.length;i++){
      const bytes=new Uint8Array(poolBuffers[i]);poolLengths.push(bytes.length);
      poolHex.push(this.bytesHex(bytes));
    }
    this.failure={revision:this.revision,column:col,badMask:mask,
      valueText:valueText,rawBits:rawBits,sumBits:[this.numberBits(s1),this.numberBits(s2),
        this.numberBits(s3)],finite:finite,negative:negative,sumFinite:Number.isFinite(s3),
      fieldBinding:bindings,surfaceIsBound:f.surface===this.sources[7],
      surfaceIsSandTopScratch:false,bindingStable:this.sameBinding(f),
      hotOwnerId:ownerId,hotStepEpochBefore:stepEpoch,hotStepEpochAfter:stepEpoch,
      runToken:runToken,attemptToken:attemptToken,substepId:substepId,sourceEpoch:f.sourceEpoch,
      poolMassHBits:this.numberBits(poolMassH),poolMassLBits:this.numberBits(poolMassL),poolEpoch:poolEpoch,
      poolCanonicalBytesHex:poolHex,poolCanonicalByteLengths:poolLengths,
      poolStorageValid:poolStorageValid,poolLiveCount:poolLiveCount,
      poolFreeCount:poolFreeCount,poolNextId:poolNextId};
  }
  private bytesHex(data: Uint8Array): string {
    let out='';for(let i=0;i<data.length;i++)out+=data[i].toString(16).padStart(2,'0');return out;
  }
  exportEvidence(): P011FieldEvidenceExportV2 {
    const rows: P011FieldStageMeta[]=[];
    for(let i=0;i<this.stageCount;i++){
      const x=this.stages[(this.stageWrite-this.stageCount+i+32)&31],copy=new P011FieldStageMeta();
      copy.stage=x.stage;copy.writerStage=x.writerStage;copy.preview=x.preview;
      copy.runId=x.runId;copy.stateEpoch=x.stateEpoch;copy.configRevision=x.configRevision;
      copy.substepId=x.substepId;copy.attemptId=x.attemptId;copy.candidateRole=x.candidateRole;
      copy.callOrdinal=x.callOrdinal;copy.theta=x.theta;copy.commitBefore=x.commitBefore;
      copy.commitCurrent=x.commitCurrent;copy.sourceEpoch=x.sourceEpoch;copy.poolEpoch=x.poolEpoch;
      copy.firstInvalidColumn=x.firstInvalidColumn;copy.legal=x.legal;rows.push(copy);
    }
    return {revision:this.revision,status:this.failure===null?'NO_FAILURE':'CAPTURED',
      windowStatus:this.windowStatus,dropped:this.dropped,
      lastLegalBytesHex:this.lastLegalValid?this.bytesHex(this.lastLegalBytes):null,
      firstIllegalBytesHex:this.firstIllegalValid?this.bytesHex(this.firstIllegalBytes):null,
      stages:rows,failure:this.failure};
  }
}
