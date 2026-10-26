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
