// T178 read-only sidecar. Raw copies freeze before a publication can be reused.
import { P011PublishedRenderState } from '../physics/P011PublishedRenderState';
import { P012FrameContext } from './P012RenderTypes';
import { P013SceneFrame, P013SceneReceipt } from './P013SceneTypes';
import { p013Sha256 } from './preparation/P013CanvasAssetExport';
export class P013MaterialFailurePoint {
  reason: string = ''; stage: string = ''; column: number = -1; row: number = -1; channel: number = -1;
  predicateMask: number = 0; profileValid: number = 0; stagingValid: number = 0;
  reset(): void {this.reason='';this.stage='';this.column=this.row=this.channel=-1;this.predicateMask=0;this.profileValid=this.stagingValid=0;}
  fail(reason: string, stage: string, column: number, row: number, channel: number, mask: number): void {
    if(this.reason!=='')return;this.reason=reason;this.stage=stage;this.column=column;this.row=row;this.channel=channel;this.predicateMask=mask;
  }
}
export interface P013MaterialArrayEvidence {
  name: string; type: string; length: number; byteOffset: number; byteLength: number; bindingId: number;
  capturedBytes: number; status: string; sha256: string | null; rawHex: string;
}
export interface P013MaterialFrameSummary {
  runId: string; stateEpoch: number; physicsCommitId: number; publicationKind: string;
  fieldCommitId: number | null; ledgerCommitKey: string | null; routeId: string | null; medium: string;
  pageFrame: number; contextFrame: number; sceneFrameId: number; frameToken: string;
  session: number; context: number; surface: number; geometry: number; continuity: number;
  viewportWidth: number; viewportHeight: number; sourceBoxX: number; sourceBoxY: number; sourceBoxW: number; sourceBoxH: number;
  targetBoxX: number; targetBoxY: number; targetBoxW: number; targetBoxH: number; scale: number;
  pipeline: number; configBindingId: number; configDigest: string; configRevision: number;
  sourceBuildIdentity: string; sourceFailure: string; sequence: number;
}
export interface P013MaterialPointExport {
  reason: string; stage: string; column: number | null; row: number | null; channel: number | null; predicateMask: number;
  profileValid: number; stagingValid: number; exceptionName: string | null; exceptionMessage: string | null;
}
export interface P013MaterialRejectionV1 {
  revision: string; firstSuccess: P013MaterialFrameSummary | null; firstFailure: P013MaterialFrameSummary | null;
  point: P013MaterialPointExport | null; recent: P013MaterialFrameSummary[]; repeatCount: number; dropped: number;
  arrays: P013MaterialArrayEvidence[]; receipt: P013SceneReceipt | null; poolReplayComplete: boolean;
  missingReason: string[]; captureBytes: number; captureByteCapacity: number; captureTimeMs: number | null;
  derivedSumNames: string[]; derivedSumHex: string[];
}
export class P013MaterialRejectionStore {
  private readonly bytes: Uint8Array[] = [];
  private readonly names: string[] = ['waterTop','bh','bl','mh','ml','suspH','suspL','sandTop','hs','hl','colParticleMassH','colParticleMassL',
    'displayF32','materialStagingF32','warpH','warpL','warpM','spark','motion','profileF64'];
  private readonly valid: Int32Array = new Int32Array(20);
  private metadata: P013MaterialArrayEvidence[] = [];
  private readonly bindings: Array<ArrayBuffer | null> = [];
  private bindingCount: number = 0;
  private run: string = ''; private sequence: number = 0;
  private firstSuccess: P013MaterialFrameSummary | null = null; private firstFailure: P013MaterialFrameSummary | null = null;
  private point: P013MaterialPointExport | null = null; private receipt: P013SceneReceipt | null = null;
  private readonly recent: Array<P013MaterialFrameSummary | null> = [null,null,null,null,null,null,null,null];
  private head: number = 0; private count: number = 0; private dropped: number = 0; private repeats: number = 0;
  constructor(){for(let i=0;i<20;i++)this.bytes.push(new Uint8Array(i===12?2048:i===13?6144:i===18?80:i===19?4096:1024));for(let i=0;i<64;i++)this.bindings.push(null);}
  begin(run: string): void {
    if(this.run===run)return;this.run=run;this.sequence=0;this.firstSuccess=this.firstFailure=null;this.point=null;this.receipt=null;
    this.head=this.count=this.dropped=this.repeats=this.bindingCount=0;this.valid.fill(0);this.metadata.length=0;
    for(let i=0;i<64;i++)this.bindings[i]=null;
  }
  summary(state: P011PublishedRenderState, c: P012FrameContext, f: P013SceneFrame, pipeline: number, scale: number, binding: number, digest: string): P013MaterialFrameSummary {
    return {runId:state.runId,stateEpoch:state.stateEpoch,physicsCommitId:state.physicsCommitId,publicationKind:state.publicationKind,
      fieldCommitId:state.field.physicsCommitId ?? null,ledgerCommitKey:state.ledgerCommitKey===null?null:JSON.stringify(state.ledgerCommitKey),routeId:state.routeId,medium:state.mediumMode,
      pageFrame:c.frameSerial,contextFrame:c.frameSerial,sceneFrameId:f.frameId,frameToken:f.frameToken,session:f.sessionId,context:f.contextEpoch,surface:f.surfaceEpoch,
      geometry:f.geometryEpoch,continuity:f.continuityEpoch,viewportWidth:f.surfaceWidthPx,viewportHeight:f.surfaceHeightPx,
      sourceBoxX:c.boxX,sourceBoxY:c.boxY,sourceBoxW:c.boxW,sourceBoxH:c.boxH,targetBoxX:f.boxXPx,targetBoxY:f.boxYPx,targetBoxW:f.boxWidthPx,targetBoxH:f.boxHeightPx,
      scale:scale,pipeline:pipeline,configBindingId:binding,configDigest:digest,configRevision:f.appearanceConfigRevision,
      sourceBuildIdentity:'NOT_CAPTURED_USE_PACKAGE_MANIFEST',sourceFailure:f.sourceFailure,sequence:++this.sequence};
  }
  observe(summary: P013MaterialFrameSummary): void {
    if(summary.sourceFailure===''&&this.firstSuccess===null)this.firstSuccess=summary;
    if(this.count===8){this.head=(this.head+1)%8;this.count--;this.dropped++;}
    this.recent[(this.head+this.count)%8]=summary;this.count++;
  }
  needsCapture(): boolean {return this.firstFailure===null;}
  capture(summary: P013MaterialFrameSummary, p: P013MaterialFailurePoint, name: string | null, message: string | null): void {
    if(this.firstFailure!==null){if(this.point?.reason===p.reason&&this.point?.column===(p.column<0?null:p.column)&&this.point?.predicateMask===p.predicateMask)this.repeats++;return;}
    this.firstFailure=summary;this.point={reason:p.reason,stage:p.stage,column:p.column<0?null:p.column,row:p.row<0?null:p.row,
      channel:p.channel<0?null:p.channel,predicateMask:p.predicateMask,profileValid:p.profileValid,stagingValid:p.stagingValid,exceptionName:name,exceptionMessage:message};
  }
  copy(index: number, a: Float64Array | Float32Array | null, validBytes: number = -1): void {
    if(a===null){this.metadata.push({name:this.names[index],type:'NOT_CAPTURED',length:0,byteOffset:0,byteLength:0,bindingId:0,capturedBytes:0,status:'NOT_CAPTURED',sha256:null,rawHex:''});return;}
    let id=0;for(let i=0;i<this.bindingCount;i++)if(this.bindings[i]===a.buffer)id=i+1;
    if(id===0&&this.bindingCount<64){id=++this.bindingCount;this.bindings[id-1]=a.buffer as ArrayBuffer;}
    const n=Math.min(this.bytes[index].length,a.byteLength,validBytes<0?a.byteLength:validBytes);
    this.bytes[index].set(new Uint8Array(a.buffer,a.byteOffset,n));this.valid[index]=n;
    this.metadata.push({name:this.names[index],type:a instanceof Float64Array?'Float64Array':'Float32Array',length:a.length,
      byteOffset:a.byteOffset,byteLength:a.byteLength,bindingId:id,capturedBytes:n,status:id===0?'BINDING_CAPACITY':n===a.byteLength?'CAPTURED':n===0?'NOT_VALID_CURRENT':'VALID_PREFIX_OR_TRUNCATED',sha256:null,rawHex:''});
  }
  capturePublication(state: P011PublishedRenderState): void {
    const f=state.field;this.copy(0,f.waterTop);this.copy(1,f.bh);this.copy(2,f.bl);this.copy(3,f.mh);this.copy(4,f.ml);
    this.copy(5,f.suspH);this.copy(6,f.suspL);this.copy(7,f.sandTop);this.copy(8,f.hs);this.copy(9,f.hl);
    this.copy(10,f.particleState?.colParticleMassH ?? null);this.copy(11,f.particleState?.colParticleMassL ?? null);
  }
  attachReceipt(receipt: P013SceneReceipt): void {if(this.firstFailure?.frameToken===receipt.requestedFrameToken)this.receipt=JSON.parse(JSON.stringify(receipt)) as P013SceneReceipt;}
  exportFrozen(): P013MaterialRejectionV1 {
    const arrays: P013MaterialArrayEvidence[]=[];
    for(let j=0;j<this.metadata.length;j++){
      const m=this.metadata[j],i=this.names.indexOf(m.name),n=this.valid[i],v=this.bytes[i].subarray(0,n);let hex='';
      for(let k=0;k<n;k++)hex+=v[k].toString(16).padStart(2,'0');
      arrays.push({name:m.name,type:m.type,length:m.length,byteOffset:m.byteOffset,byteLength:m.byteLength,bindingId:m.bindingId,capturedBytes:n,status:m.status,sha256:n?p013Sha256(v):null,rawHex:hex});
    }
    const rows: P013MaterialFrameSummary[]=[];for(let i=0;i<this.count;i++){const row=this.recent[(this.head+i)%8];if(row!==null)rows.push(row);}
    const derived: string[]=[];const column=this.point?.column ?? -1;
    if(column>=0&&this.valid[1]>=8*(column+1)&&this.valid[2]>=8*(column+1)&&this.valid[3]>=8*(column+1)&&this.valid[4]>=8*(column+1)){
      const bh=new DataView(this.bytes[1].buffer).getFloat64(column*8,true),bl=new DataView(this.bytes[2].buffer).getFloat64(column*8,true);
      const mh=new DataView(this.bytes[3].buffer).getFloat64(column*8,true),ml=new DataView(this.bytes[4].buffer).getFloat64(column*8,true);
      const values: number[]=[bh+bl,(bh+bl)+mh,((bh+bl)+mh)+ml,bh+mh,bl+ml,(bh+mh)+(bl+ml)];
      const out=new DataView(new ArrayBuffer(8));for(const value of values){out.setFloat64(0,value,true);let hex='';for(let i=0;i<8;i++)hex+=out.getUint8(i).toString(16).padStart(2,'0');derived.push(hex);}
    }
    const result: P013MaterialRejectionV1={revision:'P013_SCENE_MATERIAL_REJECTION_V1',firstSuccess:this.firstSuccess,firstFailure:this.firstFailure,point:this.point,
      recent:rows,repeatCount:this.repeats,dropped:this.dropped,arrays:arrays,receipt:this.receipt,poolReplayComplete:false,
      missingReason:[this.metadata.length===20?'ARRAY_CAPTURE_SET_COMPLETE':'ARRAY_CAPTURE_INCOMPLETE','FULL_POOL_NOT_CAPTURED','PACKAGE_SOURCE_IDENTITY_EXTERNAL_MANIFEST','CAPTURE_TIMING_NOT_CAPTURED','DERIVED_SUMS_COMPUTE_FROM_FROZEN_BYTES_NOT_ORIGINAL_STORAGE'],
      captureBytes:this.valid.reduce((a: number,b: number):number=>a+b,0),captureByteCapacity:28752,captureTimeMs:null,
      derivedSumNames:['DERIVED_bh_plus_bl','DERIVED_left_plus_mh','DERIVED_left_plus_ml','DERIVED_bh_plus_mh','DERIVED_bl_plus_ml','DERIVED_pairwise_sum'],derivedSumHex:derived};
    return JSON.parse(JSON.stringify(result)) as P013MaterialRejectionV1;
  }
}
