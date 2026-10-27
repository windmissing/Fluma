import { P011ParticleSnapshot, particleSnapshotBufferAtHot } from './P011ParticleField';

export const P011_PROGRESS_COUNTERS: string[]=['auditSinkRow','auditApplyTail','auditFinish','auditOther',
  'sortCollectCalls','sortAuditCalls','sortComparisons','sortSiftIterations','hotPoolTransfers',
  'indexSortCalls','indexComparisons','indexSiftIterations','indexTreeVisits','indexRangeQueries',
  'seedCalls','beginHotCalls','collectCalls','applyCalls','finishCalls','birthRequested','indexRebuilds',
  'poolCopies','fieldPoolCopyStateTo','fieldPoolRestoreSnapshot'];
export type P011ProgressClock=()=>number;
export class P011PoolProgressContext {
  runToken: number=0;ownerId: number=0;
  runId: string='';stateEpoch: number=0;configRevision: number=0;substepId: number=0;
  attemptId: string='';candidateRole: string='NOT_CAPTURED';callOrdinal: number=0;
  theta: number=0;preview: boolean=false;lambdaAttempt: number=-1;lambda: number=0;
  sourceEpoch: number=0;commit: number=0;sourceKind: number=-1;column: number=-1;phase: number=-1;
  seed: number=0;mass: number=0;x: number=0;y: number=0;vx: number=0;vy: number=0;
  energyBefore: number=0;energyAfter: number=0;parentSequence: number=0;
}
export class P011PoolProgressRow extends P011PoolProgressContext {
  readonly delta: Float64Array=new Float64Array(24);
  span: number=0;stage: string='';status: string='NOT_CAPTURED';
  startMs: number=NaN;endMs: number=NaN;error: number=0;
  nmax: number=0;live: number=0;free: number=0;epoch: number=0;attemptToken: number=0;
  root: number=-1;end: number=-1;parent: number=-1;child: number=-1;count: number=0;
  assign(c: P011PoolProgressContext): void {
    this.runToken=c.runToken;this.ownerId=c.ownerId;
    this.runId=c.runId;this.stateEpoch=c.stateEpoch;this.configRevision=c.configRevision;
    this.substepId=c.substepId;this.attemptId=c.attemptId;this.candidateRole=c.candidateRole;
    this.callOrdinal=c.callOrdinal;this.theta=c.theta;this.preview=c.preview;
    this.lambdaAttempt=c.lambdaAttempt;this.lambda=c.lambda;this.sourceEpoch=c.sourceEpoch;
    this.commit=c.commit;this.sourceKind=c.sourceKind;this.column=c.column;this.phase=c.phase;
    this.seed=c.seed;this.mass=c.mass;this.x=c.x;this.y=c.y;this.vx=c.vx;this.vy=c.vy;
    this.energyBefore=c.energyBefore;this.energyAfter=c.energyAfter;this.parentSequence=c.parentSequence;
  }
  copy(r: P011PoolProgressRow): void {
    this.assign(r);this.delta.set(r.delta);this.span=r.span;this.stage=r.stage;this.status=r.status;
    this.startMs=r.startMs;this.endMs=r.endMs;this.error=r.error;
    this.nmax=r.nmax;this.live=r.live;this.free=r.free;this.epoch=r.epoch;this.attemptToken=r.attemptToken;
    this.root=r.root;this.end=r.end;this.parent=r.parent;this.child=r.child;this.count=r.count;
  }
}
export class P011PoolProgressDiagnostic {
  readonly revision: string='P011_POOL_PROGRESS_DIAGNOSTIC_V3';
  readonly counters: Float64Array=new Float64Array(24);
  readonly context: P011PoolProgressContext=new P011PoolProgressContext();
  readonly ring: P011PoolProgressRow[]=[];readonly first32: P011PoolProgressRow[]=[];
  readonly active: P011PoolProgressRow[]=[];
  readonly firstLong: P011PoolProgressRow=new P011PoolProgressRow();
  readonly config: Float64Array=new Float64Array(16);
  private readonly before: Float64Array=new Float64Array(16*24);
  private readonly open: Uint8Array=new Uint8Array(16);
  private readonly raw: Uint8Array[]=[];
  private poolInput: P011ParticleSnapshot|null=null;
  private sequence: number=0;private write: number=0;private count: number=0;private firstCount: number=0;
  private ticks: number=0;private lastTime: number=-1;
  dropped: number=0;spanOverflow: number=0;longCaptured: boolean=false;
  clockStatus: string='NOT_CAPTURED';inputStatus: string='NOT_CAPTURED';thresholdMs: number=100;
  nmax: number=0;live: number=0;free: number=0;epoch: number=0;attemptToken: number=0;
  root: number=-1;end: number=-1;parent: number=-1;child: number=-1;loopCount: number=0;
  inputLive: number=0;inputFree: number=0;inputNextId: number=0;rawHash: string='NOT_CAPTURED';
  private readonly clock: P011ProgressClock|null;
  constructor(clock: P011ProgressClock|null,buffers: ArrayBuffer[]){
    this.clock=clock;
    for(let i=0;i<32;i++){this.ring.push(new P011PoolProgressRow());this.first32.push(new P011PoolProgressRow());}
    for(let i=0;i<16;i++)this.active.push(new P011PoolProgressRow());
    for(let i=0;i<buffers.length;i++)this.raw.push(new Uint8Array(buffers[i].byteLength));
  }
  private now(): number {
    if(this.clock===null)return NaN;
    if(this.clockStatus==='CLOCK_INVALID')return NaN;
    try{const n=this.clock();if(!Number.isFinite(n)||n<this.lastTime){this.clockStatus='CLOCK_INVALID';return NaN;}
      this.lastTime=n;this.clockStatus='MONOTONIC_INJECTED_MS';return n;
    }catch(_e){this.clockStatus='CLOCK_UNAVAILABLE';return NaN;}
  }
  clockNow(): number {return this.now();}
  input(p: P011ParticleSnapshot): void {this.poolInput=p;this.nmax=p.nmax;this.live=p.liveCount;this.free=p.freeCount;}
  increment(index: number,amount: number=1): void {this.counters[index]+=amount;}
  private cursors(row: P011PoolProgressRow): void {
    row.root=this.root;row.end=this.end;row.parent=this.parent;row.child=this.child;row.count=this.loopCount;
    row.nmax=this.nmax;row.live=this.live;row.free=this.free;row.epoch=this.epoch;row.attemptToken=this.attemptToken;
  }
  private record(row: P011PoolProgressRow): void {
    this.ring[this.write].copy(row);this.write=(this.write+1)&31;
    if(this.count<32)this.count++;else this.dropped++;
    if(this.firstCount<32)this.first32[this.firstCount++].copy(row);
  }
  begin(stage: string): number {
    let slot=-1;for(let i=0;i<16;i++)if(this.open[i]===0){slot=i;break;}
    if(slot<0){this.spanOverflow++;return -1;}
    this.open[slot]=1;const row=this.active[slot];row.assign(this.context);
    row.span=++this.sequence;row.stage=stage;row.status='RUNNING';row.startMs=this.now();
    row.endMs=NaN;row.error=0;row.delta.fill(0);this.cursors(row);
    for(let i=0;i<24;i++)this.before[slot*24+i]=this.counters[i];this.record(row);return slot;
  }
  endSpan(slot: number,error: number): void {
    if(slot<0||slot>=16||this.open[slot]===0)return;
    const row=this.active[slot];row.endMs=this.now();row.status='COMPLETE';row.error=error;
    for(let i=0;i<24;i++)row.delta[i]=this.counters[i]-this.before[slot*24+i];
    this.cursors(row);this.checkLong(row);this.record(row);this.open[slot]=0;
  }
  cursor(root: number,end: number,parent: number,child: number,count: number): void {
    this.root=root;this.end=end;this.parent=parent;this.child=child;this.loopCount=count;
    if((++this.ticks&127)!==0||this.longCaptured)return;
    const now=this.now();for(let i=15;i>=0;i--)if(this.open[i]!==0){
      const row=this.active[i];if(Number.isFinite(now)&&Number.isFinite(row.startMs)&&
        now-row.startMs>=this.thresholdMs){this.cursors(row);this.checkLong(row,now);break;}
    }
  }
  private checkLong(row: P011PoolProgressRow,time: number=row.endMs): void {
    if(this.longCaptured||!Number.isFinite(time)||!Number.isFinite(row.startMs)||time-row.startMs<this.thresholdMs)return;
    this.longCaptured=true;
    for(let slot=0;slot<16;slot++)if(this.active[slot]===row)
      for(let i=0;i<24;i++)row.delta[i]=this.counters[i]-this.before[slot*24+i];
    this.firstLong.copy(row);
    if(row.status==='RUNNING'){this.firstLong.endMs=NaN;this.firstLong.status='RUNNING_PARTIAL';}
    const p=this.poolInput;if(p===null)return;
    this.inputLive=p.liveCount;this.inputFree=p.freeCount;this.inputNextId=p.nextId;
    let hash=2166136261;
    try {
    for(let i=0;i<this.raw.length;i++){
      const buffer=particleSnapshotBufferAtHot(p,i);
      if(buffer.byteLength!==this.raw[i].length){this.inputStatus='CAPTURE_CAPACITY_MISMATCH';return;}
      const src=new Uint8Array(buffer);this.raw[i].set(src);
      for(let k=0;k<src.length;k++)hash=Math.imul(hash^src[k],16777619);
    }
    this.inputStatus='EXACT_STAGE_POOL_BYTES';this.rawHash=(hash>>>0).toString(16).padStart(8,'0');
    }catch(_e){this.inputStatus='CAPTURE_FAILED';}
  }
  private rowJson(row: P011PoolProgressRow): string {
    // Nonfinite timestamps serialize to null with explicit clock/status, never measured zero.
    return JSON.stringify({span:row.span,stage:row.stage,status:row.status,startMs:row.startMs,
      endMs:row.endMs,error:row.status==='RUNNING'?null:row.error,
      elapsedMs:Number.isFinite(row.startMs)&&Number.isFinite(row.endMs)?row.endMs-row.startMs:null,
      delta:row.status==='RUNNING'?null:Array.from(row.delta),
      requestedBirths:row.status==='RUNNING'?null:row.delta[19],nmax:row.nmax,live:row.live,free:row.free,
      epoch:row.epoch,attemptToken:row.attemptToken,root:row.root,end:row.end,parent:row.parent,
      child:row.child,count:row.count,runId:row.runId,stateEpoch:row.stateEpoch,configRevision:row.configRevision,
      runToken:row.runToken,ownerId:row.ownerId,
      substepId:row.substepId,attemptId:row.attemptId,candidateRole:row.candidateRole,
      callOrdinal:row.callOrdinal,theta:row.theta,preview:row.preview,lambdaAttempt:row.lambdaAttempt,
      lambda:row.lambda,sourceEpoch:row.sourceEpoch,commit:row.commit,sourceKind:row.sourceKind,
      column:row.column,phase:row.phase,seed:row.seed,mass:row.mass,x:row.x,y:row.y,vx:row.vx,vy:row.vy,
      energyBefore:row.energyBefore,energyAfter:row.energyAfter,parentSequence:row.parentSequence});
  }
  exportJson(): string {
    const ring: string[]=[],first: string[]=[],running: string[]=[],rawHex: string[]=[];
    for(let i=0;i<this.count;i++)ring.push(this.rowJson(this.ring[(this.write-this.count+i+32)&31]));
    for(let i=0;i<this.firstCount;i++)first.push(this.rowJson(this.first32[i]));
    for(let i=0;i<16;i++)if(this.open[i]!==0)running.push(this.rowJson(this.active[i]));
    if(this.inputStatus==='EXACT_STAGE_POOL_BYTES')for(const bytes of this.raw){
      let hex='';for(let i=0;i<bytes.length;i++)hex+=bytes[i].toString(16).padStart(2,'0');rawHex.push(hex);
    }
    const configBytes=new Uint8Array(this.config.buffer);let configHex='';
    for(let i=0;i<configBytes.length;i++)configHex+=configBytes[i].toString(16).padStart(2,'0');
    const seedFloats=new Float64Array([this.firstLong.seed,this.firstLong.mass,this.firstLong.x,this.firstLong.y,
      this.firstLong.vx,this.firstLong.vy,this.firstLong.lambda,this.firstLong.theta]);
    const seedBytes=new Uint8Array(seedFloats.buffer);let seedHex='';
    for(let i=0;i<seedBytes.length;i++)seedHex+=seedBytes[i].toString(16).padStart(2,'0');
    const summary=JSON.stringify({revision:this.revision,clockStatus:this.clockStatus,
      countersScope:'DIAGNOSTIC_SESSION_CUMULATIVE',
      poolCopyCounterScope:'HOT_TRANSFER_AND_FIELD_CHECKPOINT_READ_RESTORE; EXPORT_SNAPSHOTS_NOT_CAPTURED',counterNames:P011_PROGRESS_COUNTERS,
      counters:Array.from(this.counters),
      dropped:this.dropped,spanOverflow:this.spanOverflow,thresholdMs:this.thresholdMs,
      inputStatus:this.inputStatus,inputLive:this.inputLive,inputFree:this.inputFree,inputNextId:this.inputNextId,
      inputPoolHex:rawHex,inputHash:this.rawHash,inputHashSchema:'FNV32_RAW_BYTES_CORRELATION_ONLY',
      inputCapturePoint:'AT_THRESHOLD_DETECTION_NOT_REPLAY_START',inputByteOrder:'NATIVE_LITTLE_ENDIAN',
      configF64:Array.from(this.config),configF64Hex:configHex,
      firstLongSeedF64Hex:this.longCaptured?seedHex:null,exactFullTransactionReplay:false});
    return summary.substring(0,summary.length-1)+',"ring":['+ring.join(',')+'],"first32":['+
      first.join(',')+'],"running":['+running.join(',')+'],"firstLong":'+
      (this.longCaptured?this.rowJson(this.firstLong):'null')+'}';
  }
}
