// T171 test-only common evidence; all routes pay the same fixed numeric recording cost.
import { P011ParticleSnapshot } from './P011ParticleField';
import { P011ParticleHotWorkspace } from './P011ParticleHot';
import { P011ParticleHotRefundReceipt } from './P011ParticleHotBudget';
import { T159Pair } from './T159Selector';
import { t159CandidateRole,t159CandidateOrdinal,t159PoolVersion,t171RelativeOrdinal } from './T159Metadata';
export class T171Evidence {
  readonly failedPair:T159Pair={status:'ERROR',slotA:-1,slotB:-1,errorCode:'SELECTOR_THROW'};
  selectorCount:number=0;pairCount:number=0;noPairCount:number=0;errorCount:number=0;
  mergeCommitted:number=0;pages:number=0;rollbackCalls:number=0;auditCalls:number=0;overflow:boolean=false;
  private pairRows:Float64Array=new Float64Array(16384*32);
  private eventRows:Float64Array=new Float64Array(65536*48);
  private lineageRows:Uint32Array=new Uint32Array(131072);
  private pageRows:Float64Array=new Float64Array(8192*12);
  private refundRows:Float64Array=new Float64Array(16384*20);private refundCount:number=0;
  lambdaCalls:number=0;
  private eventCount:number=0;private lineageCount:number=0;
  readonly roleCalls:Uint32Array=new Uint32Array(6);
  clock:()=>number=():number=>NaN;
  reset():void{this.selectorCount=this.pairCount=this.noPairCount=this.errorCount=this.mergeCommitted=this.pages=this.rollbackCalls=this.auditCalls=this.eventCount=this.lineageCount=0;this.overflow=false;this.refundCount=0;this.lambdaCalls=0;this.roleCalls.fill(0);}
  selected(p:P011ParticleSnapshot,radius:number,w:P011ParticleHotWorkspace,r:T159Pair,ms:number):void{
    const row=this.selectorCount++;if(row>=16384){this.overflow=true;return;}
    if(r.status==='PAIR')this.pairCount++;else if(r.status==='NO_PAIR')this.noPairCount++;else this.errorCount++;
    const offset=row*32;for(let i=0;i<32;i++)this.pairRows[offset+i]=0;const mutation=t159PoolVersion(p),role=t159CandidateRole();this.roleCalls[role]++;
    this.pairRows[offset]=role;this.pairRows[offset+1]=t171RelativeOrdinal();this.pairRows[offset+2]=p.nmax;this.pairRows[offset+3]=p.liveCount;
    this.pairRows[offset+4]=mutation.lo;this.pairRows[offset+5]=mutation.hi;this.pairRows[offset+6]=w.attemptToken;this.pairRows[offset+7]=w.substepId;
    this.pairRows[offset+8]=r.status==='PAIR'?1:r.status==='NO_PAIR'?0:-1;this.pairRows[offset+9]=r.slotA;this.pairRows[offset+10]=r.slotB;this.pairRows[offset+11]=radius;
    this.pairRows[offset+12]=ms;
    const a=r.slotA,b=r.slotB;
    if(r.status==='PAIR'&&a>=0&&b>=0){
      this.pairRows[offset+13]=p.pId[a];this.pairRows[offset+14]=p.pId[b];this.pairRows[offset+15]=p.pPhase[a];this.pairRows[offset+16]=p.pSource[a];
      this.pairRows[offset+17]=p.pSourceId[a];this.pairRows[offset+18]=p.pState[a];this.pairRows[offset+19]=Math.floor(p.pX[a]);
      this.pairRows[offset+20]=p.pMass[a];this.pairRows[offset+21]=p.pX[a];this.pairRows[offset+22]=p.pY[a];this.pairRows[offset+23]=p.pVx[a];this.pairRows[offset+24]=p.pVy[a];
      this.pairRows[offset+25]=p.pMass[b];this.pairRows[offset+26]=p.pX[b];this.pairRows[offset+27]=p.pY[b];this.pairRows[offset+28]=p.pVx[b];this.pairRows[offset+29]=p.pVy[b];
    }
    this.pairRows[offset+30]=w.ownerId;this.pairRows[offset+31]=w.runToken;
  }
  page(w:P011ParticleHotWorkspace):void{
    const e=w.events,page=this.pages++;if(page>=8192){this.overflow=true;return;}
    const m=page*12;this.pageRows[m]=w.ownerId;this.pageRows[m+1]=w.runToken;this.pageRows[m+2]=w.attemptToken;this.pageRows[m+3]=w.substepId;
    this.pageRows[m+4]=this.eventCount;this.pageRows[m+5]=e.count;this.pageRows[m+6]=this.lineageCount;this.pageRows[m+7]=e.lineageCount;
    this.pageRows[m+8]=w.sourceInjectedH;this.pageRows[m+9]=w.sourceInjectedL;this.pageRows[m+10]=w.stepExternalH;this.pageRows[m+11]=w.stepExternalL;
    for(let i=0;i<e.count;i++){
      if(e.kind[i]===6)this.mergeCommitted++;
      if(this.eventCount>=65536){this.overflow=true;continue;}const o=this.eventCount++*48;
      this.eventRows[o+0]=e.kind[i];
      this.eventRows[o+1]=e.phase[i];
      this.eventRows[o+2]=e.sourceKind[i];
      this.eventRows[o+3]=e.flag[i];
      this.eventRows[o+4]=e.remainderDisposition[i];
      this.eventRows[o+5]=e.particleId[i];
      this.eventRows[o+6]=e.sourceId[i];
      this.eventRows[o+7]=e.requestOrdinal[i];
      this.eventRows[o+8]=e.column[i];
      this.eventRows[o+9]=e.targetColumn[i];
      this.eventRows[o+10]=e.requested[i];
      this.eventRows[o+11]=e.granted[i];
      this.eventRows[o+12]=e.remainder[i];
      this.eventRows[o+13]=e.massBefore[i];
      this.eventRows[o+14]=e.massAfter[i];
      this.eventRows[o+15]=e.x[i];
      this.eventRows[o+16]=e.y[i];
      this.eventRows[o+17]=e.vx[i];
      this.eventRows[o+18]=e.vy[i];
      this.eventRows[o+19]=e.deltaS[i];
      this.eventRows[o+20]=e.budgetLocal[i];
      this.eventRows[o+21]=e.budgetTotal[i];
      this.eventRows[o+22]=e.budgetBottom[i];
      this.eventRows[o+23]=e.budgetClipped[i];
      this.eventRows[o+24]=e.guardRejected[i];
      this.eventRows[o+25]=e.capacityClipped[i];
      this.eventRows[o+26]=e.poolClipped[i];
      this.eventRows[o+27]=e.toSuspH[i];
      this.eventRows[o+28]=e.toSuspL[i];
      this.eventRows[o+29]=e.guardCount[i];
      this.eventRows[o+30]=e.guardLimitingEdge[i];
      this.eventRows[o+31]=e.guardAllLegal[i];
      this.eventRows[o+32]=e.guardAccepted[i];
      this.eventRows[o+33]=e.guardRejectedAmount[i];
      this.eventRows[o+34]=e.lineageLength[i];
      for(let j=0;j<3;j++){this.eventRows[o+35+j*3]=e.guardEdgeId[i*3+j];this.eventRows[o+36+j*3]=e.guardResidual[i*3+j];this.eventRows[o+37+j*3]=e.guardInfluence[i*3+j];}
      this.eventRows[o+44]=this.lineageCount;
      for(let j=0;j<e.lineageLength[i];j++){if(this.lineageCount>=131072){this.overflow=true;break;}this.lineageRows[this.lineageCount++]=e.lineage[e.lineageStart[i]+j];}
    }
  }
  refund(out:P011ParticleHotRefundReceipt):void{
    const row=this.refundCount++;if(row>=16384){this.overflow=true;return;}const o=row*20;
    this.refundRows[o]=out.requestOrdinal;this.refundRows[o+1]=out.amount;this.refundRows[o+2]=out.totalReserved;this.refundRows[o+3]=out.totalRefunded;this.refundRows[o+4]=out.localCount;
    this.refundRows[o+5]=out.netTotalUsed;this.refundRows[o+6]=out.netTotalDelta;this.refundRows[o+7]=out.bottomReserved;this.refundRows[o+8]=out.bottomRefunded;
    for(let i=0;i<2;i++){this.refundRows[o+9+i*5]=out.localColumns[i];this.refundRows[o+10+i*5]=out.localReserved[i];this.refundRows[o+11+i*5]=out.localRefunded[i];this.refundRows[o+12+i*5]=out.netLocalUsed[i];this.refundRows[o+13+i*5]=out.netLocalDelta[i];}
    this.refundRows[o+19]=out.netDeltaValid;
  }
  private hexValues(values:Float64Array,count:number):string{
    const bytes=new Uint8Array(values.buffer,values.byteOffset,count*8);let out='';for(let i=0;i<bytes.length;i++)out+=bytes[i].toString(16).padStart(2,'0');return out;
  }
  private digest(values:Float64Array,count:number,stride:number,kind:number):number{
    let hash=2166136261;const scratch=new DataView(new ArrayBuffer(8));
    for(let i=0;i<count;i++){
      const k=i%stride;if(kind===1&&(k===4||k===5||k===12||k===30||k===31)||kind===2&&(k===0||k===1))continue;
      scratch.setFloat64(0,values[i],true);for(let j=0;j<8;j++)hash=Math.imul(hash^scratch.getUint8(j),16777619);
    }return hash>>>0;
  }
  exportJson(full:boolean):string{
    return JSON.stringify({revision:'T171_COMMON_NUMERIC_V3',selectorCalls:this.selectorCount,pair:this.pairCount,noPair:this.noPairCount,error:this.errorCount,
      mergeCommitted:this.mergeCommitted,auditCalls:this.auditCalls,rollbackCalls:this.rollbackCalls,pages:this.pages,overflow:this.overflow,
      pairStride:32,eventStride:48,pageStride:12,
      pairDigest:this.digest(this.pairRows,Math.min(this.selectorCount,16384)*32,32,1),eventDigest:this.digest(this.eventRows,this.eventCount*48,48,0),pageDigest:this.digest(this.pageRows,Math.min(this.pages,8192)*12,12,2),
      digestScope:'FNV32_CORRELATION_ONLY_NOT_IDENTITY_SHA',pairRows:full?this.pairRows.slice(0,Math.min(this.selectorCount,16384)*32):null,
      eventRows:full?this.eventRows.slice(0,this.eventCount*48):null,lineageRows:full?this.lineageRows.slice(0,this.lineageCount):null,pageRows:full?this.pageRows.slice(0,Math.min(this.pages,8192)*12):null,
      refundCount:this.refundCount,refundDigest:this.digest(this.refundRows,Math.min(this.refundCount,16384)*20,20,0),refundRows:full?this.refundRows.slice(0,Math.min(this.refundCount,16384)*20):null,lambdaCalls:this.lambdaCalls,pageRawF64Hex:full?this.hexValues(this.pageRows,Math.min(this.pages,8192)*12):null,pairRawF64Hex:full?this.hexValues(this.pairRows,Math.min(this.selectorCount,16384)*32):null,eventRawF64Hex:full?this.hexValues(this.eventRows,this.eventCount*48):null,refundRawF64Hex:full?this.hexValues(this.refundRows,Math.min(this.refundCount,16384)*20):null,fullDump:full,overflowPolicy:'INVALID_MEASUREMENT_NO_PHYSICS_CHANGE'});
  }
}
export const t171Evidence:T171Evidence=new T171Evidence();
