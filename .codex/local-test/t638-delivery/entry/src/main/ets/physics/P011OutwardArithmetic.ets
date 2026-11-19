// T133 stage 1: binary64 outward endpoints using bounded exact comparisons.
// Caller owns result slots. No production BigInt, ULP loop or heap growth.
import { P011ExactDyadic, P011ExactCode } from './P011ExactDyadic';

export type P011OutwardCode=P011ExactCode|'RANGE_UNSUPPORTED'|'DIVIDE_BY_ZERO'|'INVALID_INTERVAL';
export class P011OutwardInterval {
  lo: number=NaN;hi: number=NaN;valid: boolean=false;reason: P011OutwardCode='INVALID_INTERVAL';
}
export class P011OutwardArithmetic {
  readonly revision: string='P011_OUTWARD_RN64_V1';
  readonly exact: P011ExactDyadic=new P011ExactDyadic();
  private readonly bits: ArrayBuffer=new ArrayBuffer(8);
  private readonly view: DataView=new DataView(this.bits);
  private readonly scalars: Float64Array=new Float64Array(7);
  private readonly corner: P011OutwardInterval=new P011OutwardInterval();
  get environmentSupported(): boolean{return this.exact.environmentSupported;}
  nextUp(value: number): number {
    if(Number.isNaN(value)||value===Infinity)return value;
    if(value===0)return Number.MIN_VALUE;
    this.view.setFloat64(0,value,false);
    let hi=this.view.getUint32(0,false),lo=this.view.getUint32(4,false);
    if(value>0){if(lo===0xffffffff){lo=0;hi++;}else lo++;}
    else if(lo===0){lo=0xffffffff;hi--;}else lo--;
    this.view.setUint32(0,hi,false);this.view.setUint32(4,lo,false);
    return this.view.getFloat64(0,false);
  }
  nextDown(value: number): number{return -this.nextUp(-value);}
  private fail(out: P011OutwardInterval,reason: P011OutwardCode): P011OutwardCode {
    out.lo=NaN;out.hi=NaN;out.valid=false;out.reason=reason;return reason;
  }
  private set(out: P011OutwardInterval,lo: number,hi: number): P011OutwardCode {
    if(!Number.isFinite(lo)||!Number.isFinite(hi))return this.fail(out,'RANGE_UNSUPPORTED');
    out.lo=lo;out.hi=hi;out.valid=true;out.reason='OK';return 'OK';
  }
  private checkPair(out: P011OutwardInterval,a: number,b: number): boolean {
    if(!this.environmentSupported){this.fail(out,'ARITHMETIC_ENV_UNSUPPORTED');return false;}
    if(!Number.isFinite(a)||!Number.isFinite(b)){this.fail(out,'NON_FINITE');return false;}
    return true;
  }
  private directed(out: P011OutwardInterval,v: number,exactMinusRounded: number): P011OutwardCode {
    const lo=exactMinusRounded<0?this.nextDown(v):v;
    const hi=exactMinusRounded>0?this.nextUp(v):v;
    return this.set(out,lo,hi);
  }
  addInto(a: number,b: number,out: P011OutwardInterval): P011OutwardCode {
    if(!this.checkPair(out,a,b))return out.reason;
    const v=a+b;if(!Number.isFinite(v))return this.fail(out,'RANGE_UNSUPPORTED');
    this.scalars[0]=a;this.scalars[1]=b;this.scalars[2]=-v;
    const r=this.exact.compareAffineBound(0,0,this.scalars,3);
    const code=r.code,sign=r.sign; // shared exact result: snapshot before reuse
    return code==='OK'?this.directed(out,v,sign):this.fail(out,code);
  }
  subInto(a: number,b: number,out: P011OutwardInterval): P011OutwardCode {
    if(!this.checkPair(out,a,b))return out.reason;
    const v=a-b;if(!Number.isFinite(v))return this.fail(out,'RANGE_UNSUPPORTED');
    this.scalars[0]=a;this.scalars[1]=-b;this.scalars[2]=-v;
    const r=this.exact.compareAffineBound(0,0,this.scalars,3);
    const code=r.code,sign=r.sign;
    return code==='OK'?this.directed(out,v,sign):this.fail(out,code);
  }
  mulInto(a: number,b: number,out: P011OutwardInterval): P011OutwardCode {
    if(!this.checkPair(out,a,b))return out.reason;
    const v=a*b;if(!Number.isFinite(v))return this.fail(out,'RANGE_UNSUPPORTED');
    const r=this.exact.compareProduct(a,b,v);const code=r.code,sign=r.sign;
    return code==='OK'?this.directed(out,v,sign):this.fail(out,code);
  }
  divInto(a: number,b: number,out: P011OutwardInterval): P011OutwardCode {
    if(!this.checkPair(out,a,b))return out.reason;
    if(b===0)return this.fail(out,'DIVIDE_BY_ZERO');
    const v=a/b;if(!Number.isFinite(v))return this.fail(out,'RANGE_UNSUPPORTED');
    this.scalars[0]=-a;
    const r=this.exact.compareAffineBound(v,b,this.scalars,1);
    const code=r.code,sign=r.sign; // sign(v*b-a), then undo b's polarity
    return code==='OK'?this.directed(out,v,-sign*(b>0?1:-1)):this.fail(out,code);
  }
  private input(out: P011OutwardInterval,a: P011OutwardInterval,b: P011OutwardInterval): boolean {
    if(!a.valid||!b.valid||!Number.isFinite(a.lo)||!Number.isFinite(a.hi)||
      !Number.isFinite(b.lo)||!Number.isFinite(b.hi)||a.lo>a.hi||b.lo>b.hi){
      this.fail(out,'INVALID_INTERVAL');return false;}
    return true;
  }
  intervalAddInto(a: P011OutwardInterval,b: P011OutwardInterval,out: P011OutwardInterval): P011OutwardCode {
    if(!this.input(out,a,b))return out.reason;
    const al=a.lo,ah=a.hi,bl=b.lo,bh=b.hi;
    let code=this.addInto(al,bl,this.corner);if(code!=='OK')return this.fail(out,code);
    const lo=this.corner.lo;code=this.addInto(ah,bh,this.corner);if(code!=='OK')return this.fail(out,code);
    return this.set(out,lo,this.corner.hi);
  }
  intervalSubInto(a: P011OutwardInterval,b: P011OutwardInterval,out: P011OutwardInterval): P011OutwardCode {
    if(!this.input(out,a,b))return out.reason;
    const al=a.lo,ah=a.hi,bl=b.lo,bh=b.hi;
    let code=this.subInto(al,bh,this.corner);if(code!=='OK')return this.fail(out,code);
    const lo=this.corner.lo;code=this.subInto(ah,bl,this.corner);if(code!=='OK')return this.fail(out,code);
    return this.set(out,lo,this.corner.hi);
  }
  private corners(a: P011OutwardInterval,b: P011OutwardInterval,out: P011OutwardInterval,divide: boolean): P011OutwardCode {
    if(!this.input(out,a,b))return out.reason;
    const al=a.lo,ah=a.hi,bl=b.lo,bh=b.hi;
    if(divide&&bl<=0&&bh>=0)return this.fail(out,'DIVIDE_BY_ZERO');
    let lo=Infinity,hi=-Infinity;
    for(let i=0;i<4;i++){
      const x=i<2?al:ah,y=i%2===0?bl:bh;
      const code=divide?this.divInto(x,y,this.corner):this.mulInto(x,y,this.corner);
      if(code!=='OK')return this.fail(out,code);
      lo=Math.min(lo,this.corner.lo);hi=Math.max(hi,this.corner.hi);
    }
    return this.set(out,lo,hi);
  }
  intervalMulInto(a: P011OutwardInterval,b: P011OutwardInterval,out: P011OutwardInterval): P011OutwardCode {
    return this.corners(a,b,out,false);
  }
  intervalDivInto(a: P011OutwardInterval,b: P011OutwardInterval,out: P011OutwardInterval): P011OutwardCode {
    return this.corners(a,b,out,true);
  }
  etaForRangeInto(lo: number,hi: number,out: P011OutwardInterval): P011OutwardCode {
    if(!this.checkPair(out,lo,hi))return out.reason;
    if(lo>hi)return this.fail(out,'INVALID_INTERVAL');
    const largest=Math.max(Math.abs(lo),Math.abs(hi));
    if(largest===0)return this.set(out,0,Number.MIN_VALUE);
    this.view.setFloat64(0,largest,false);
    const exponent=(this.view.getUint32(0,false)>>>20)&2047;
    const half=exponent===0?Number.MIN_VALUE:Math.max(Number.MIN_VALUE,Math.pow(2,exponent-1076));
    return this.set(out,0,half);
  }
}
