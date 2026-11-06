interface HotWorkCurrent {run:number;step:number;attempt:number;phase:number;B:number;R:number;N:number;live:number;retries:number;attemptCumulativeMs:number;counts:Float64Array;totalMs:Float64Array;maxMs:Float64Array;}
// T311 CPU V2 stage receipts, independent of simulation dt and physical state.
// 32 first-preserved attempts * 80 F64 + 3*20 current F64 = 20,960 bytes.
export class P011HotWorkReceipt {
 readonly counts:Float64Array=new Float64Array(20);readonly totalMs:Float64Array=new Float64Array(20);readonly maxMs:Float64Array=new Float64Array(20);
 readonly rows:Float64Array=new Float64Array(32*80);count:number=0;dropped:number=0;clockInvalid:number=0;
 enabled:boolean=true;private open:boolean=false;exportSkipped:number=0;
 emit:((line:string)=>void)|null=null;private emitted:number=0;private emittedBytes:number=0;private lastEmit:number=0;
 run:number=0;step:number=0;attempt:number=0;phase:number=0;B:number=0;R:number=0;N:number=0;live:number=0;retries:number=0;copiedBytes:number=0;aliasComparisons:number=0;
 // T331 (T323 §2): attempt-level cumulative wall so watchdog risk reads out
 // directly; next() stamps the start, seal()/progress freeze the value. No new
 // measurement points — Date.now per T325 ruling.
 attemptWallStartMs:number=0;attemptCumulativeMs:number=0;
 // Sampled exclusive apply ledger. Legacy T311 stage times remain inclusive.
 fineEnabled:boolean=false;
 private readonly fineRows:Float64Array=new Float64Array(64*40);
 private readonly fineRow:Float64Array=new Float64Array(40);
 private readonly fineStack:Int32Array=new Int32Array(12);
 private fineHead=0;
 private fineCount=0;
 private fineDropped=0;
 private fineDepth=0;
 private fineClock=0;
 private fineStart=0;
 fineBegin(frame:number,substep:number,role:number,fast:number,checks:number,derivedBytes:number,events:number,lineage:number):void {
  if(!this.fineEnabled||(frame>0?frame:this.step)%16!==1)return;
  this.fineRow.fill(0);
  this.fineRow[0]=frame;this.fineRow[1]=substep;this.fineRow[2]=role;
  this.fineRow[3]=this.run;this.fineRow[4]=this.step;this.fineRow[5]=this.attempt;
  this.fineRow[24]=fast;this.fineRow[25]=checks;this.fineRow[26]=derivedBytes;
  this.fineRow[27]=this.copiedBytes;this.fineRow[28]=events;this.fineRow[29]=lineage;this.fineRow[30]=this.retries;
  this.fineDepth=1;this.fineStack[0]=0;
  this.fineStart=this.fineClock=Date.now();
 }
 private fineSettle():void {
  const now=Date.now(),delta=now-this.fineClock;
  this.fineRow[31]++;
  if(delta<0||!Number.isFinite(delta))this.clockInvalid++;
  else this.fineRow[8+this.fineStack[this.fineDepth-1]]+=delta;
  this.fineClock=now;
 }
 fineEnter(stage:number):number {
  if(this.fineDepth===0)return 0;
  if(this.fineDepth===12){this.clockInvalid++;return 0;}
  this.fineSettle();this.fineStack[this.fineDepth++]=stage;
  return this.fineDepth;
 }
 fineLeave(scope:number):void {
  if(scope===0)return;
  if(scope!==this.fineDepth){this.clockInvalid++;return;}
  this.fineSettle();this.fineDepth--;
 }
 fineAdd(index:number):void {if(this.fineDepth!==0)this.fineRow[index]++;}
 fineEnd(code:number,fast:number,checks:number,derivedBytes:number,events:number,lineage:number):void {
  if(this.fineDepth===0)return;
  this.fineSettle();this.fineDepth=0;
  const row=this.fineRow;
  row[6]=code;row[7]=this.fineClock-this.fineStart;
  row[24]=fast-row[24];row[25]=checks-row[25];row[26]=derivedBytes-row[26];
  row[27]=this.copiedBytes-row[27];row[28]=events-row[28];row[29]=lineage-row[29];row[30]=this.retries-row[30];
  this.fineRows.set(row,this.fineHead*40);this.fineHead=(this.fineHead+1)%64;
  if(this.fineCount<64)this.fineCount++;else this.fineDropped++;
 }
 fineOutcome(granted:number,budget:number,guard:number,capacity:number,pool:number):void {
  if(this.fineDepth===0)return;
  this.fineRow[33]+=granted;this.fineRow[34]+=budget;this.fineRow[35]+=guard;
  this.fineRow[36]+=capacity;this.fineRow[37]+=pool;
  if(guard>0)this.fineRow[38]++;
 }
 exportFineCold():string {
  const rows:number[][]=[];
  for(let i=0;i<this.fineCount;i++){
   const at=(this.fineHead-this.fineCount+64+i)%64;
   rows.push(Array.from(this.fineRows.subarray(at*40,at*40+40)));
  }
  return JSON.stringify({revision:'T366_HOT_APPLY_EXCLUSIVE_V1',clock:'DATE_MS',capacity:64,dropped:this.fineDropped,
   role:['ADMISSION_OR_DIRECT','AUTHORITATIVE','PREVIEW'],stages:['applyTail','planIdentity','prepareLease','transferAndDerived','rowAudit','budgetGuardEvents'],
   stagesAt:8,requestKindsAt:16,fullAt:22,incrementalAt:23,fallbackAt:32,
   fields:['frame','substep','role','runToken','stepEpoch','attempt','code','wallMs'],
   countersAt:24,counters:['fastBindings','fullAliasComparisons','derivedCopyBytes','poolPayloadBytes','eventDelta','lineageDelta','retryDelta','clockBoundaries'],
   outcomesAt:33,outcomes:['grantedMass','budgetClippedMass','guardRejectedMass','capacityClippedMass','poolClippedMass','guardRejectedRows'],
   clockInvalid:this.clockInvalid,rows:rows});
 }
 begin(stage:number):number {if(!this.enabled)return -1;this.counts[stage]++;return Date.now();}
 end(stage:number,start:number):void {if(start<0||!this.enabled)return;const now=Date.now(),cost=now-start;if(!Number.isFinite(cost)||cost<0){this.clockInvalid++;return;}this.totalMs[stage]+=cost;this.maxMs[stage]=Math.max(this.maxMs[stage],cost);}
 // Called only by the explicit diagnostic timer, never by patch/lease/audit.
 emitProgressCold():void {const now=Date.now();this.attemptCumulativeMs=this.open?Math.max(0,now-this.attemptWallStartMs):this.attemptCumulativeMs;if(this.emit===null||now-this.lastEmit<1000||this.emitted===32)return;this.lastEmit=now;const line='T311_STAGE '+JSON.stringify({run:this.run,step:this.step,attempt:this.attempt,phase:this.phase,B:this.B,R:this.R,N:this.N,live:this.live,retry:this.retries,bytes:this.copiedBytes,attemptCumulativeMs:this.attemptCumulativeMs,counts:Array.from(this.counts),totalMs:Array.from(this.totalMs),maxMs:Array.from(this.maxMs)});if(line.length<=1800&&this.emittedBytes+line.length<=57600){this.emitted++;this.emittedBytes+=line.length;try{this.emit(line);}catch(e){this.clockInvalid++;}}else this.exportSkipped++;}
 next(run:number,step:number,attempt:number,B:number,N:number):void {if(!this.enabled)return;this.seal(0);this.counts.fill(0);this.totalMs.fill(0);this.maxMs.fill(0);this.run=run;this.step=step;this.attempt=attempt;this.B=B;this.N=N;this.phase=0;this.R=this.live=this.retries=this.copiedBytes=this.aliasComparisons=0;this.attemptWallStartMs=Date.now();this.attemptCumulativeMs=0;this.open=true;}
 seal(code:number):void {if(!this.enabled||!this.open)return;this.open=false;this.attemptCumulativeMs=Math.max(0,Date.now()-this.attemptWallStartMs);if(this.count===32){this.dropped++;return;}const at=this.count++*80;this.rows[at]=this.run;this.rows[at+1]=this.step;this.rows[at+2]=this.attempt;this.rows[at+3]=this.phase;this.rows[at+4]=this.B;this.rows[at+5]=this.R;this.rows[at+6]=this.N;this.rows[at+7]=this.live;this.rows[at+8]=this.retries;this.rows[at+9]=this.copiedBytes;this.rows[at+10]=this.aliasComparisons;this.rows[at+11]=code;this.rows[at+12]=this.clockInvalid;this.rows[at+13]=this.attemptCumulativeMs;this.rows.set(this.counts,at+16);this.rows.set(this.totalMs,at+36);this.rows.set(this.maxMs,at+56);}
 private currentCold():HotWorkCurrent {return {run:this.run,step:this.step,attempt:this.attempt,phase:this.phase,B:this.B,R:this.R,N:this.N,live:this.live,retries:this.retries,attemptCumulativeMs:this.attemptCumulativeMs,counts:this.counts,totalMs:this.totalMs,maxMs:this.maxMs};}
 // Explicit cold export only. Hot path never serializes or emits rows.
 exportCold():string{return JSON.stringify({revision:'T311_HOT_WORK_V2',clock:'DATE_MS_NOT_SIMULATION_CLOCK',gcCounters:'NOT_AVAILABLE',rowWidth:80,capacity:32,count:this.count,dropped:this.dropped,exportSkipped:this.exportSkipped,clockInvalid:this.clockInvalid,incomplete:this.dropped>0||this.clockInvalid>0||this.exportSkipped>0,rowFields:['run','step','attempt','phase','B','R','N','live','retries','copiedBytes','aliasComparisons','code','clockInvalid','attemptCumulativeMs'],current:this.currentCold(),stages:['storage','fieldBinding','prepare','lease','readTransfer','restoreTransfer','snapshotCopy','audit','sort','budgetAudit','derivedFlush','collect','apply','sink','retry','traceSync','capacityRead','preview','rollback','finish'],rows:this.rows.slice(0,this.count*80)});}
}
