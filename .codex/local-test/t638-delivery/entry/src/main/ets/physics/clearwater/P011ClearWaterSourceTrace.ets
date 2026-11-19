// T321. C F64 reference source only. Fixed recent-window + first-boundary lanes;
// observation never calls engine, advances a clock, or authorizes a pointer.
export class ClearWaterSourceTrace {
 readonly capacity:number=512;readonly boundaryCapacity:number=64;readonly width:number=32;
 readonly rows:Float64Array=new Float64Array((512+64)*32);
 private readonly domains:string[]=new Array<string>(512+64).fill('');
 private readonly reasons:string[]=new Array<string>(512+64).fill('');
 private readonly runs:string[]=new Array<string>(512+64).fill('');
 enabled:boolean=true;sequence:number=0;count:number=0;overwritten:number=0;
 boundaries:number=0;boundaryDropped:number=0;metadataDropped:number=0;
 private head:number=0;
 // Fixed reusable scalar packet. Caller fills this before record; no history
 // refers to a mutable clock/request/terminal receipt after the call returns.
 readonly packet:Float64Array=new Float64Array(32);
 record(domain:string,reason:string,run:string,boundary:boolean):void {
  if(!this.enabled)return;
  this.sequence++;
  if(domain.length>128||reason.length>128||run.length>128){this.metadataDropped++;return;}
  const row=this.head,at=row*this.width;this.packet[0]=this.sequence;
  for(let j=0;j<this.width;j++)this.rows[at+j]=this.packet[j];
  this.domains[row]=domain;this.reasons[row]=reason;this.runs[row]=run;
  this.head=(this.head+1)%this.capacity;if(this.count<this.capacity)this.count++;else this.overwritten++;
  if(boundary){if(this.boundaries===this.boundaryCapacity){this.boundaryDropped++;return;}
   const dest=this.capacity+this.boundaries++;
   for(let j=0;j<this.width;j++)this.rows[dest*this.width+j]=this.packet[j];
   this.domains[dest]=domain;this.reasons[dest]=reason;this.runs[dest]=run;
  }
 }
 // Explicit cold export only. Bounded 576 rows / 18,432 scalar values; caller
 // saves one checksummed file. A recent window never claims a full run.
 exportCold():string {
  const indices:number[]=[];const data:number[][]=[];const domains:string[]=[];const reasons:string[]=[];const runs:string[]=[];
  for(let i=0;i<this.count;i++)indices.push((this.head-this.count+this.capacity+i)%this.capacity);
  for(let i=0;i<this.boundaries;i++)indices.push(this.capacity+i);
  for(const row of indices){data.push(Array.from(this.rows.subarray(row*this.width,(row+1)*this.width)));domains.push(this.domains[row]);reasons.push(this.reasons[row]);runs.push(this.runs[row]);}
  return JSON.stringify({revision:'CW_SOURCE_CLOCK_RECEIPTS_T321_V1',scope:'C_F64_REFERENCE_ONLY',coverage:'RECENT_WINDOW_PLUS_FIRST_BOUNDARIES_NOT_FULL_RUN',
   clock:'ORIGINAL_UNIT_DOMAIN_RAW_DIFFERENCE_BEFORE_CONVERSION',nonfinite:'JSON_NULL_WITH_FINITE_FLAG',events:['INVALID','DOWN','MOVE','STOP','CLOCK','SAMPLE','TERMINAL'],
   columns:['serial','event','previousRaw','currentRaw','deltaRaw','unit_NS1_MS2','validWindow','seconds','windowId','pointerBefore','pointerAfter','gesture','inputBefore','inputAfter','runGeneration','geometry','sampleCount','terminalCount','sourceCommit','feedbackCount','requested','admitted','rejected','status','kind','gestureValidSeconds','gestureSampledSeconds','configuredRate','state_IDLE0_ACTIVE1_STOPPED2','finiteRaw','safeIntegerRawNS','deltaNsDiagnosticOnly'],
   capacity:this.capacity,boundaryCapacity:this.boundaryCapacity,typedPayloadBytes:this.rows.byteLength+this.packet.byteLength,metadataUtf16MaxBytes:(this.capacity+this.boundaryCapacity)*3*128*2,
   sequence:this.sequence,recentCount:this.count,boundaries:this.boundaries,overwritten:this.overwritten,boundaryDropped:this.boundaryDropped,metadataDropped:this.metadataDropped,incomplete:this.overwritten>0||this.boundaryDropped>0||this.metadataDropped>0,
   data,domains,reasons,runs});
 }
}
