import { ClearWaterStatus as S, ClearWaterRunConfig, ClearWaterSourceRequest, ClearWaterSourceReceipt, ClearWaterAllocation,
  CW_SOURCE, CW_ROW_REVISION, CW_QUEUE_OFFSETS, CW_DEDUP_OFFSETS, CW_EVENT_OFFSETS, cwSafePositive, cwSameFiniteBits } from './P011ClearWaterTypes';
import { ClearWaterState } from './P011ClearWaterState';
export class ClearWaterSource {
  readonly rowRevision: string = CW_ROW_REVISION;
  readonly queue: Float64Array;readonly dedup: Float64Array;readonly events: Float64Array;
  private readonly request: Float64Array=new Float64Array(20);private readonly receiptRow: Float64Array=new Float64Array(56);
  private readonly requestBits: DataView;private readonly queueBits: DataView;private readonly dedupBits: DataView;
  private head: number=0;private count: number=0;private ordinal: number=0;
  dedupCount: number=0;eventCount: number=0;
  private readonly config: ClearWaterRunConfig;
  constructor(config: ClearWaterRunConfig){
    this.config=config;
    // Fixed row revision declares native F64 storage little endian. Fail closed
    // on an unsupported host rather than silently mislabel exported bytes.
    const endianProbe=new Uint32Array(1);endianProbe[0]=0x01020304;
    if(new Uint8Array(endianProbe.buffer)[0]!==4)throw new Error('CW_UNSUPPORTED_ENDIANNESS');
    this.queue=new Float64Array(config.queueCapacity*20);this.dedup=new Float64Array(config.dedupCapacity*56);this.events=new Float64Array(config.eventCapacity*24);
    this.requestBits=new DataView(this.request.buffer);this.queueBits=new DataView(this.queue.buffer);this.dedupBits=new DataView(this.dedup.buffer);
  }
  get pendingCount(): number {return this.count;}
  get queueHead(): number {return this.head;}
  get enqueueOrdinal(): number {return this.ordinal;}
  clearPending(): void {this.queue.fill(0);this.head=this.count=0;}
  reset(): void {this.clearPending();this.dedup.fill(0);this.events.fill(0);this.ordinal=this.dedupCount=this.eventCount=0;}
  private encode(r: ClearWaterSourceRequest): void {
    const a=this.request;a.fill(0);a[0]=r.runGeneration;a[1]=r.configRev;a[2]=r.geometryEpoch;a[3]=r.inputGeneration;a[4]=r.gestureId;
    a[5]=r.sourceId;a[6]=r.requestSeq;a[7]=r.kind;a[8]=r.x;a[9]=r.y;a[10]=r.requestedVolume;a[11]=r.timestamp;
    a[12]=r.timestampUnit==='NS'?0:1;a[13]=r.seed;a[16]=-1;a[17]=r.timestamp/(a[12]===0?1000000:1);a[18]=r.clockWindowId;
  }
  private key(a: Float64Array, base: number, stride: number): boolean {
    const r=this.request;return a[base]===r[0]&&a[base+3]===r[3]&&a[base+4]===r[4]&&a[base+5]===r[5]&&a[base+6]===r[6]&&a[base+7]===r[7];
  }
  private payload(bits: DataView, base: number): boolean {
    for(let i=0;i<14;i++)for(let half=0;half<2;half++)if(bits.getUint32((base+i)*8+half*4,true)!==this.requestBits.getUint32(i*8+half*4,true))return false;
    for(let half=0;half<2;half++)if(bits.getUint32((base+18)*8+half*4,true)!==this.requestBits.getUint32(18*8+half*4,true))return false;
    return true;
  }
  private materialize(row: Float64Array, b: number, run: string, replay: boolean, out: ClearWaterSourceReceipt): void {
    out.revision=CW_SOURCE;out.runId=run;out.runGeneration=row[b];out.configRev=row[b+1];out.geometryEpoch=row[b+2];out.inputGeneration=row[b+3];
    out.gestureId=row[b+4];out.sourceId=row[b+5];out.requestSeq=row[b+6];out.kind=row[b+7];out.x=row[b+8];out.y=row[b+9];out.requested=row[b+10];
    out.timestamp=row[b+11];out.timestampUnit=row[b+12]===0?'NS':'MS';out.seed=row[b+13];out.clockWindowId=row[b+18];
    out.status=row[b+20];out.disposition=row[b+21]===1?'ACCEPTED':row[b+21]===2?'PARTIAL_CAPACITY':row[b+21]===3?'REJECTED':'PENDING';out.replay=replay;
    out.admitted=row[b+22];out.rejected=row[b+23];out.transactionId=row[b+24]===0?null:row[b+24];out.kindSuccessSeq=row[b+25]===0?null:row[b+25];
    out.sourceCommit=row[b+26];out.physicsCommitId=row[b+27];out.globalWaterBefore=row[b+28];out.globalWaterAfter=row[b+29];
    out.globalSandBefore=row[b+30];out.globalSandAfter=row[b+31];out.columnWaterBefore=row[b+32];out.columnWaterAfter=row[b+33];
    out.columnSandBefore=row[b+34];out.columnSandAfter=row[b+35];out.controlAdmitted=row[b+36];out.actualWaterDelta=row[b+37];out.actualSandDelta=row[b+38];
    out.waterSuccessCount=row[b+39];out.sandSuccessCount=row[b+40];out.I_water=row[b+41];out.I_sand=row[b+42];out.sourceLedgerRev=row[b+43];out.errorStage=row[b+44];
  }
  private reject(status: S, run: string, state: ClearWaterState, out: ClearWaterSourceReceipt): S {
    this.prepareReceipt(state);this.receiptRow[20]=status;this.receiptRow[21]=3;this.receiptRow[44]=status;
    this.materialize(this.receiptRow,0,run,false,out);return status;
  }
  private prepareReceipt(state: ClearWaterState): void {
    const row=this.receiptRow;row.fill(0);row.set(this.request);row[23]=this.request[10];row[26]=state.sourceCommit;row[27]=state.physicsCommit;
    row[28]=row[29]=state.waterTotal();row[30]=row[31]=state.sandTotal();const c=Math.floor(this.request[8]);
    row[32]=row[33]=c>=0&&c<128?state.W[c]:0;row[34]=row[35]=c>=0&&c<128?state.S[c]+state.P[c]:0;
    row[39]=state.waterSuccessCount;row[40]=state.sandSuccessCount;row[41]=state.I_water;row[42]=state.I_sand;row[43]=1;
  }
  enqueue(r: ClearWaterSourceRequest, run: string, generation: number, inputGeneration: number, geometry: number,
    clockId: number, clockRaw: number, clockUnit: string, state: ClearWaterState, out: ClearWaterSourceReceipt): S {
    this.encode(r);
    if(r.runId!==run||r.runGeneration!==generation)return this.reject(S.STALE_RUN,run,state,out);
    if(r.revision!==CW_SOURCE||!cwSafePositive(r.runGeneration)||!cwSafePositive(r.configRev)||!cwSafePositive(r.geometryEpoch)||
      !cwSafePositive(r.inputGeneration)||!cwSafePositive(r.gestureId)||!cwSafePositive(r.sourceId)||r.sourceId>0xffffffff||!cwSafePositive(r.requestSeq)||
      (r.kind!==0&&r.kind!==1)||!cwSafePositive(r.seed)||r.seed>0xffffffff||!cwSafePositive(r.clockWindowId))return this.reject(S.INVALID_IDENTITY,run,state,out);
    for(let i=0;i<this.dedupCount;i++)if(this.key(this.dedup,i*56,56)){
      if(!this.payload(this.dedupBits,i*56)||r.timestampUnit!=='NS'&&r.timestampUnit!=='MS')return this.reject(S.IDENTITY_CONFLICT,run,state,out);
      this.materialize(this.dedup,i*56,run,true,out);return out.status;
    }
    for(let i=0;i<this.count;i++){const b=((this.head+i)%this.config.queueCapacity)*20;if(this.key(this.queue,b,20)){
      if(!this.payload(this.queueBits,b)||r.timestampUnit!=='NS'&&r.timestampUnit!=='MS')return this.reject(S.IDENTITY_CONFLICT,run,state,out);
      this.prepareReceipt(state);this.materialize(this.receiptRow,0,run,true,out);return S.OK;
    }}
    if(r.configRev!==this.config.configRev)return this.reject(S.INVALID_IDENTITY,run,state,out);
    if(r.inputGeneration!==inputGeneration)return this.reject(S.STALE_INPUT_GENERATION,run,state,out);
    if(r.geometryEpoch!==geometry)return this.reject(S.STALE_GEOMETRY,run,state,out);
    if(!Number.isFinite(r.x)||r.x<0||r.x>=128||!Number.isFinite(r.y)||r.y<0||r.y>this.config.capacity||!Number.isFinite(r.requestedVolume)||r.requestedVolume<=0||!Number.isFinite(r.timestamp)||r.timestamp<0)return this.reject(S.INVALID_VALUE,run,state,out);
    if((r.timestampUnit!=='NS'&&r.timestampUnit!=='MS')||r.clockWindowId!==clockId||!cwSameFiniteBits(r.timestamp,clockRaw)||r.timestampUnit!==clockUnit)return this.reject(S.INVALID_TIME_WINDOW,run,state,out);
    if(this.dedupCount===this.config.dedupCapacity)return this.reject(S.DEDUP_FULL,run,state,out);
    if(this.count===this.config.queueCapacity)return this.reject(S.QUEUE_FULL,run,state,out);
    if(!cwSafePositive(this.ordinal+1))return this.reject(S.TOKEN_EXHAUSTED,run,state,out);
    this.request[14]=++this.ordinal;this.request[15]=1;
    this.queue.set(this.request,((this.head+this.count)%this.config.queueCapacity)*20);this.count++;
    this.prepareReceipt(state);this.materialize(this.receiptRow,0,run,false,out);return S.OK;
  }
  commitNext(run: string, inputGeneration: number, geometry: number, state: ClearWaterState, before: ClearWaterState,
    work: ClearWaterState, marks: Uint8Array, out: ClearWaterSourceReceipt): S {
    if(this.count===0)return S.OK;
    const base=this.head*20;for(let i=0;i<20;i++)this.request[i]=this.queue[base+i];
    this.head=(this.head+1)%this.config.queueCapacity;this.count--;
    this.prepareReceipt(state);
    let status=S.OK,q=0,births=0,firstId=state.pool.nextId;
    const c=Math.floor(this.request[8]),water=this.request[7]===1,requested=this.request[10];
    if(this.dedupCount===this.config.dedupCapacity)return this.reject(S.DEDUP_FULL,run,state,out);
    if(this.request[3]!==inputGeneration)status=S.STALE_INPUT_GENERATION;
    else if(this.request[2]!==geometry)status=S.STALE_GEOMETRY;
    else if(state.audit(marks,this.config.capacity)!==S.OK)status=S.AUDIT_FAILED;
    else if(this.request[9]<state.S[c])status=S.POSITION_IN_BED;
    else if(this.eventCount===this.config.eventCapacity)status=S.EVENT_FULL;
    else if(!cwSafePositive(state.sourceCommit+1)||!cwSafePositive(state.physicsCommit+1)||!cwSafePositive((water?state.waterSuccessCount:state.sandSuccessCount)+1))status=S.TOKEN_EXHAUSTED;
    else {
      const free=this.config.capacity-state.S[c]-state.P[c]-state.W[c];
      q=Math.min(requested,this.config.maxSourceVolume,free);
      if(q<=0)status=state.P[c]>0&&this.config.capacity-state.S[c]-state.W[c]>0?S.PARTICLE_RESERVED_CAPACITY:S.CAPACITY_FULL;
      else if(!water&&state.pool.freeCount===0)status=S.POOL_FULL;
      else {
        if(!water)q=Math.min(q,this.config.massPackage*state.pool.freeCount);
        const k=water?0:Math.floor(q/this.config.massPackage),tail=water?0:q-k*this.config.massPackage;
        births=k+(tail>0?1:0);
        if(!Number.isFinite(q)||q<=0||!Number.isSafeInteger(births)||tail<0||births>state.pool.freeCount)status=S.NUMERIC_UNREPRESENTABLE;
        else if(!water&&firstId+births-1>0xffffffff)status=S.TOKEN_EXHAUSTED;
        else {
          before.copyFrom(state);work.copyFrom(state);
          if(water)work.W[c]+=q;
          else for(let i=0;i<births;i++)if(!work.pool.birth(i<k?this.config.massPackage:tail,this.request[8],this.request[9],this.request[5]))status=S.POOL_FULL;
          work.rebuild();
          if(water){work.I_water+=q;work.waterSuccessCount++;}else{work.I_sand+=q;work.sandSuccessCount++;}
          work.sourceCommit++;work.physicsCommit++;
          const dw=work.waterTotal()-before.waterTotal(),ds=work.sandTotal()-before.sandTotal();
          // A sand birth changes authoritative pool mass/state/ID even if its
          // contribution rounds away in the derived P reduction. Water has W
          // as its sole target; a positive q that leaves W unchanged is no-op.
          if(water&&cwSameFiniteBits(work.W[c],before.W[c]))status=S.NUMERIC_UNREPRESENTABLE;
          else if(!Number.isFinite(dw)||!Number.isFinite(ds)||Math.abs(dw-(water?q:0))>1e-6||Math.abs(ds-(water?0:q))>1e-6||work.audit(marks,this.config.capacity)!==S.OK)status=S.AUDIT_FAILED;
          if(status===S.OK){
            const row=this.receiptRow;row[20]=q<requested?S.PARTIAL_CAPACITY:S.OK;row[21]=q<requested?2:1;row[22]=q;row[23]=requested-q;
            row[24]=work.sourceCommit;row[25]=water?work.waterSuccessCount:work.sandSuccessCount;row[26]=work.sourceCommit;row[27]=work.physicsCommit;
            row[29]=work.waterTotal();row[31]=work.sandTotal();row[33]=work.W[c];row[35]=work.S[c]+work.P[c];row[36]=q;row[37]=dw;row[38]=ds;
            row[39]=work.waterSuccessCount;row[40]=work.sandSuccessCount;row[41]=work.I_water;row[42]=work.I_sand;
            // Reserve/event bounds were checked before any authoritative write.
            const e=this.eventCount*24;for(let i=0;i<24;i++)this.events[e+i]=0;
            this.events[e]=work.sourceCommit;this.events[e+1]=this.request[7];this.events[e+2]=c;this.events[e+3]=q;
            this.events[e+4]=dw;this.events[e+5]=ds;this.events[e+6]=water?0:firstId;this.events[e+7]=births;this.events[e+8]=this.request[5];
            this.events[e+9]=work.physicsCommit;this.events[e+10]=this.request[14];this.events[e+11]=row[25];this.events[e+12]=before.W[c];this.events[e+13]=work.W[c];
            this.events[e+14]=before.P[c];this.events[e+15]=work.P[c];this.events[e+16]=this.request[8];this.events[e+17]=this.request[9];this.events[e+18]=this.request[13];
            this.events[e+19]=this.config.massPackage;this.events[e+20]=tail;this.events[e+21]=k;this.events[e+22]=1;
            state.copyFrom(work);this.eventCount++;
          }
        }
      }
    }
    if(status!==S.OK){this.prepareReceipt(state);this.receiptRow[20]=status;this.receiptRow[21]=3;this.receiptRow[44]=status;}
    this.receiptRow[16]=this.dedupCount;this.dedup.set(this.receiptRow,this.dedupCount*56);this.dedupCount++;
    this.materialize(this.receiptRow,0,run,false,out);return out.status;
  }
  allocations(): ClearWaterAllocation[] {
    return [{name:'source.queue20',type:'Float64Array',length:this.queue.length,byteLength:this.queue.byteLength,lifetime:'COLD_RUN_FIXED'},
      {name:'source.dedup56',type:'Float64Array',length:this.dedup.length,byteLength:this.dedup.byteLength,lifetime:'COLD_RUN_FIXED'},
      {name:'source.events24',type:'Float64Array',length:this.events.length,byteLength:this.events.byteLength,lifetime:'COLD_RUN_FIXED'},
      {name:'source.request20',type:'Float64Array',length:20,byteLength:160,lifetime:'COLD_RUN_FIXED'},
      {name:'source.receipt56',type:'Float64Array',length:56,byteLength:448,lifetime:'COLD_RUN_FIXED'}];
  }
  layoutManifest(): string {return JSON.stringify({revision:CW_ROW_REVISION,endianness:'LITTLE_ENDIAN_RAW_F64_BITS',queue:CW_QUEUE_OFFSETS,dedup:CW_DEDUP_OFFSETS,events:CW_EVENT_OFFSETS,eventSlots:24});}
}
