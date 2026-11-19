import { P011OutwardArithmetic, P011OutwardInterval } from './P011OutwardArithmetic';
import { P011WriterEnvelope } from './P011WriterEnvelope';

// Bounded affine graph: every ideal node is a*q+b. RN error is propagated
// separately, including correlated repeated nodes (the caller reuses indices).
export class P011WriterRNGraph {
  readonly oa: P011OutwardArithmetic=new P011OutwardArithmetic();
  readonly envelope: P011WriterEnvelope=new P011WriterEnvelope(128,8);
  readonly coefficient: P011OutwardInterval[]=[];
  readonly intercept: P011OutwardInterval[]=[];
  readonly ideal: P011OutwardInterval[]=[];
  readonly error: P011OutwardInterval[]=[];
  readonly actual: Float64Array=new Float64Array(128);
  private readonly t: P011OutwardInterval[]=[];
  private readonly domain: P011OutwardInterval=new P011OutwardInterval();
  count: number=0;valid: boolean=true;
  constructor(){for(let i=0;i<128;i++){
    this.coefficient.push(new P011OutwardInterval());this.intercept.push(new P011OutwardInterval());
    this.ideal.push(new P011OutwardInterval());this.error.push(new P011OutwardInterval());
  }for(let i=0;i<12;i++)this.t.push(new P011OutwardInterval());}
  set(x: P011OutwardInterval,lo: number,hi: number): void {
    x.lo=lo;x.hi=hi;x.valid=Number.isFinite(lo)&&Number.isFinite(hi)&&lo<=hi;
    x.reason=x.valid?'OK':'INVALID_INTERVAL';if(!x.valid)this.valid=false;
  }
  reset(lo: number,hi: number): void {this.count=0;this.valid=true;this.envelope.reset();this.set(this.domain,lo,hi);}
  constant(value: number): number {
    const i=this.count++;if(i>=128){this.valid=false;return 0;}
    this.actual[i]=value;this.set(this.coefficient[i],0,0);this.set(this.intercept[i],value,value);
    this.set(this.ideal[i],value,value);this.set(this.error[i],0,0);
    this.record(i,7,-1,-1,0);return i;
  }
  control(value: number): number {
    const i=this.constant(value);this.set(this.coefficient[i],1,1);this.set(this.intercept[i],0,0);
    this.set(this.ideal[i],this.domain.lo,this.domain.hi);
    this.envelope.idealLo[i]=this.domain.lo;this.envelope.idealHi[i]=this.domain.hi;return i;
  }
  private record(i: number,op: number,a: number,b: number,eta: number): void {
    if(this.envelope.addNode(op,a,b,this.actual[i],this.ideal[i].lo,this.ideal[i].hi,
      this.error[i].lo,this.error[i].hi,eta,-1,i)<0)this.valid=false;
  }
  // op 1 add, 2 subtract, 3 multiply by a frozen constant, 4 divide by positive frozen constant.
  op(op: number,a: number,b: number): number {
    const i=this.count++;if(i>=128){this.valid=false;return 0;}
    const oa=this.oa,t=this.t;
    const aa=this.coefficient[a],ab=this.intercept[a],ba=this.coefficient[b],bb=this.intercept[b];
    const ea=this.error[a],eb=this.error[b];
    if(op===1||op===2){
      this.actual[i]=op===1?this.actual[a]+this.actual[b]:this.actual[a]-this.actual[b];
      if(op===1){oa.intervalAddInto(aa,ba,this.coefficient[i]);oa.intervalAddInto(ab,bb,this.intercept[i]);
        oa.intervalAddInto(ea,eb,t[0]);}
      else{oa.intervalSubInto(aa,ba,this.coefficient[i]);oa.intervalSubInto(ab,bb,this.intercept[i]);
        oa.intervalSubInto(ea,eb,t[0]);}
    }else if(op===3){
      this.actual[i]=this.actual[a]*this.actual[b];
      if(ba.lo!==0||ba.hi!==0||bb.lo!==bb.hi){this.valid=false;return i;}
      oa.intervalMulInto(aa,bb,this.coefficient[i]);oa.intervalMulInto(ab,bb,this.intercept[i]);
      // a*Eb + b*Ea + Ea*Eb, with every interval operation directed outward.
      oa.intervalMulInto(this.ideal[a],eb,t[1]);oa.intervalMulInto(this.ideal[b],ea,t[2]);
      oa.intervalMulInto(ea,eb,t[3]);oa.intervalAddInto(t[1],t[2],t[4]);
      oa.intervalAddInto(t[4],t[3],t[0]);
    }else{
      this.actual[i]=this.actual[a]/this.actual[b];
      if(ba.lo!==0||ba.hi!==0||bb.lo!==bb.hi||bb.lo<=0||eb.lo!==0||eb.hi!==0){this.valid=false;return i;}
      oa.intervalDivInto(aa,bb,this.coefficient[i]);oa.intervalDivInto(ab,bb,this.intercept[i]);
      oa.intervalDivInto(ea,bb,t[0]);
    }
    oa.intervalMulInto(this.coefficient[i],this.domain,t[1]);
    oa.intervalAddInto(t[1],this.intercept[i],this.ideal[i]);
    // Local eta uses the range of the exact operation on the actual inputs,
    // rather than a uniform per-kernel epsilon.
    oa.intervalAddInto(this.ideal[a],ea,t[5]);oa.intervalAddInto(this.ideal[b],eb,t[6]);
    if(op===1)oa.intervalAddInto(t[5],t[6],t[7]);
    else if(op===2)oa.intervalSubInto(t[5],t[6],t[7]);
    else if(op===3)oa.intervalMulInto(t[5],t[6],t[7]);
    else oa.intervalDivInto(t[5],t[6],t[7]);
    oa.etaForRangeInto(t[7].lo,t[7].hi,t[8]);const eta=t[8].hi;
    this.set(t[9],-eta,eta);oa.intervalAddInto(t[0],t[9],this.error[i]);
    if(!this.coefficient[i].valid||!this.intercept[i].valid||!this.ideal[i].valid||
      !this.error[i].valid||!t[7].valid||!Number.isFinite(this.actual[i]))this.valid=false;
    this.record(i,op,a,b,eta);return i;
  }
}
