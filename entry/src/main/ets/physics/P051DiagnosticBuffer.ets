// T72 passive, bounded diagnostic stream. No clocks/setters/physics getters with side effects.
export interface P051DiagnosticEvent {
  inputRevision?: string; geometryEpoch?: number; inputGeneration?: number; gestureId?: number;
  eventOrdinal?: number; pointerId?: number; changedCount?: number; touchCount?: number;
  widthVp?: number; heightVp?: number; foreground?: boolean; geometryReady?: boolean;
  rawTimestamp?: number; timestampUnit?: string; pageFrame?: number; sourceApiRevision?: string;
  kind: string; runId: string; commit: number; frame: number; time: number;
  x?: number; y?: number; source?: string; reason?: string; sequence?: number;
  staleCount?: number; sensorCount?: number; value?: number; requested?: number; admitted?: number; code?: number; swap?: boolean;
}
export interface P051DiagnosticRow { sequence: number; event: P051DiagnosticEvent; }
export interface P051DiagnosticChunk {
  revision: string; dropped: number; firstSequence: number; lastSequence: number; rows: P051DiagnosticRow[];
}
export interface P051PhysicsFailureV2 {
  revision: string; firstFailureId: string; runId: string; modelGeneration: number;
  beforeCommit: number; afterCommit: number; rawCode: number | null; stage: string | null;
  hotCode: number | null; m2Code: number | null; attemptId: string | null; substepId: number | null;
  candidateRole: string | null; candidateOrdinal: number | null;
  dt: number; dtUnit: string; requestedDt: number; frameTime: number; frameTimeUnit: string;
  sourceSequenceBefore: number; sourceSequenceAfter: number; sourceEpochBefore: number; sourceEpochAfter: number;
  externalAdmissionCountBefore: number | null; externalAdmissionCountAfter: number | null;
  acceptedTiltDeg: number | null; inputSource: string | null; inputSampleTimeMs: number | null;
  targetTiltDeg: number | null; shakeInput: number | null; staleCount: number;
  sensorCount: number | null; sensorX: number | null; sensorY: number | null; sensorZ: number | null; sensorTimeMs: number | null;
  lastGoodRunId: string | null; lastGoodCommit: number | null; lastGoodFrame: number | null;
  traceStatus: string; missingStatus: string; repeatCount: number;
}
export interface P051LifecycleV2 {
  runId: string; modelGeneration: number; kind: string; reason: string; commit: number;
  frame: number; timeMs: number; firstFailureId: string | null;
}
export interface P051FailureExportV2 {
  revision: string; firstFailure: P051PhysicsFailureV2 | null; lifecycle: P051LifecycleV2[]; dropped: number;
}
// Dedicated bounded sidecar, independent of the lossy passive event stream.
export class P051FailureStoreV2 {
  private first: P051PhysicsFailureV2 | null = null;
  private rows: Array<P051LifecycleV2 | null> = [null,null,null,null,null,null,null,null];
  private head: number = 0; private count: number = 0; private dropped: number = 0;
  latch(row: P051PhysicsFailureV2): void { if(this.first===null)this.first=row; }
  hasFailure(): boolean { return this.first!==null; }
  repeat(): void { if(this.first!==null)this.first.repeatCount++; }
  lifecycle(runId: string, generation: number, kind: string, reason: string, commit: number, frame: number, timeMs: number): void {
    if(this.count===8){this.head=(this.head+1)%8;this.count--;this.dropped++;}
    this.rows[(this.head+this.count)%8]={runId:runId,modelGeneration:generation,kind:kind,reason:reason,commit:commit,
      frame:frame,timeMs:timeMs,firstFailureId:this.first?.firstFailureId ?? null};this.count++;
  }
  exportFrozen(): P051FailureExportV2 {
    const rows: P051LifecycleV2[]=[];
    for(let i=0;i<this.count;i++){const row=this.rows[(this.head+i)%8];if(row!==null)rows.push(JSON.parse(JSON.stringify(row)) as P051LifecycleV2);}
    return {revision:'P051_FIRST_PHYSICS_FAILURE_V2',firstFailure:this.first===null?null:JSON.parse(JSON.stringify(this.first)) as P051PhysicsFailureV2,lifecycle:rows,dropped:this.dropped};
  }
}
export class P051DiagnosticBuffer {
  private rows: Array<P051DiagnosticRow | null> = [];
  private head: number = 0; private count: number = 0; private serial: number = 0;
  private dropped: number = 0;
  constructor() { for(let i=0;i<256;i++)this.rows.push(null); }
  record(event: P051DiagnosticEvent): void {
    if(this.count===256) { this.rows[this.head]=null;this.head=(this.head+1)%256;this.count--;this.dropped++; }
    this.rows[(this.head+this.count)%256]={sequence:++this.serial,event:event};this.count++;
  }
  drain(): P051DiagnosticChunk {
    const out: P051DiagnosticRow[]=[];
    // Two small records per line; correlation sequence detects any dropped/log-truncated interval.
    for(let i=0;i<2 && this.count>0;i++) {
      const row=this.rows[this.head];if(row!==null)out.push(row);
      this.rows[this.head]=null;this.head=(this.head+1)%256;this.count--;
    }
    return {revision:'P051_PASSIVE_DIAGNOSTICS_V1',dropped:this.dropped,
      firstSequence:out.length?out[0].sequence:0,lastSequence:out.length?out[out.length-1].sequence:0,rows:out};
  }
}
