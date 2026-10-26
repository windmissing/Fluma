// Opt-in T85 evidence. No callbacks or physics references are retained in a trace.
export interface P011TraceChannel { channelId: string; deltaSandTop: number[]; }
export interface P011TraceEdge {
  column: number; s0Difference: number; deltaDifference: number; gNext: number;
  lo: number; hi: number; cumulativeLo: number; cumulativeHi: number; limitingEdge: number | null;
  cumulativeLoEdge: number | null; cumulativeHiEdge: number | null;
  gNextHex: string; loHex: string; hiHex: string;
}
export interface P011TraceDecision {
  slopeLo: number; slopeHi: number; angleLo: number | null; angleHi: number | null;
  stepLo: number; stepHi: number; directedLo: number; directedHi: number;
  finalLo: number | null; finalHi: number | null; feasible: boolean;
  thetaClamped: number | null; candidateDifference: number | null; acceptedComparison: boolean;
}
export interface P011TraceCandidate {
  callOrdinal: number; role: string; theta: number; stage: string;
  collectCode: number | null; collectColumn: number | null; particleStatus: number | null;
  particleDeltaSandTop: number[]; channels: P011TraceChannel[]; deltaS: number[]; edges: P011TraceEdge[]; decision: P011TraceDecision | null;
}
export type P011TiltPreviewTraceSink = (trace: P011TiltPreviewTrace) => void;
export class P011TiltPreviewTrace {
  readonly revision: string = 'P011_TILT_PREVIEW_TRACE_V1';
  status: string = 'COMPLETE';
  runId: string = ''; stateEpoch: number = 0; attemptId: string = ''; substepId: number = 0; configRevision: number = 0;
  dt: number = 0; thetaBefore: number = 0; target: number = 0; requested: number | null = null; maxTiltOmega: number = 0; sampleSequence: number = 0; frameId: number = 0;
  hardMax: number = 0; angleEps: number = 0; fieldEps: number = 0; preexistingTolerance: number = 1e-12;
  s0: number[] = []; preexistingResiduals: number[] = []; preexistingViolation: boolean = false;
  candidates: P011TraceCandidate[] = []; notExecuted: string[] = [];
  resultStatus: string = ''; blockedReason: string | null = null;
  current: P011TraceCandidate | null = null;
  // All instrumentation is best effort; allocation/encoding errors cannot alter the decision.
  incomplete(): void { this.status='TRACE_INCOMPLETE'; }
  begin(role: string, theta: number): void {
    try {
      if(this.candidates.length>=3){this.incomplete();this.current=null;return;}
      const c: P011TraceCandidate={callOrdinal:this.candidates.length+1,role:role,theta:theta,stage:'ENTERED',
        collectCode:null,collectColumn:null,particleStatus:null,particleDeltaSandTop:[],channels:[],deltaS:[],edges:[],decision:null};
      this.candidates.push(c);this.current=c;
    } catch(e){this.incomplete();this.current=null;}
  }
  stage(stage: string, collectCode: number | null, collectColumn: number | null, particleStatus: number | null): void {
    try {if(this.current!==null){this.current.stage=stage;this.current.collectCode=collectCode;this.current.collectColumn=collectColumn;this.current.particleStatus=particleStatus;}}catch(e){this.incomplete();}
  }
  edge(column: number, s0Difference: number, deltaDifference: number, gNext: number, lo: number, hi: number,
    cumulativeLo: number, cumulativeHi: number, limitingEdge: number | null): void {
    try {
      const edges=this.current===null?[]:this.current.edges;
      const previous=edges.length?edges[edges.length-1]:null;
      const loEdge=lo>(previous===null?Number.NEGATIVE_INFINITY:previous.cumulativeLo)?column:previous===null?null:previous.cumulativeLoEdge;
      const hiEdge=hi<(previous===null?Number.POSITIVE_INFINITY:previous.cumulativeHi)?column:previous===null?null:previous.cumulativeHiEdge;
      this.current?.edges.push({column:column,s0Difference:s0Difference,deltaDifference:deltaDifference,gNext:gNext,
      lo:lo,hi:hi,cumulativeLo:cumulativeLo,cumulativeHi:cumulativeHi,limitingEdge:limitingEdge,
      cumulativeLoEdge:loEdge,cumulativeHiEdge:hiEdge,gNextHex:this.hex(gNext),loHex:this.hex(lo),hiHex:this.hex(hi)});}catch(e){this.incomplete();}
  }
  decision(slopeLo: number, slopeHi: number, stepLo: number, stepHi: number, directedLo: number, directedHi: number,
    finalLo: number | null, finalHi: number | null, feasible: boolean, thetaClamped: number | null, candidate: number, eps: number): void {
    try {if(this.current!==null)this.current.decision={slopeLo:slopeLo,slopeHi:slopeHi,
      angleLo:finalLo===null?null:Math.atan(slopeLo)*180/Math.PI,angleHi:finalHi===null?null:Math.atan(slopeHi)*180/Math.PI,
      stepLo:stepLo,stepHi:stepHi,directedLo:directedLo,directedHi:directedHi,finalLo:finalLo,finalHi:finalHi,
      feasible:feasible,thetaClamped:thetaClamped,candidateDifference:thetaClamped===null?null:thetaClamped-candidate,
      acceptedComparison:feasible&&thetaClamped!==null&&Math.abs(thetaClamped-candidate)<=eps};}catch(e){this.incomplete();}
  }
  hex(value: number): string {
    const bytes=new ArrayBuffer(8);const v=new DataView(bytes);v.setFloat64(0,value,false);
    return v.getUint32(0,false).toString(16).padStart(8,'0')+v.getUint32(4,false).toString(16).padStart(8,'0');
  }
}
