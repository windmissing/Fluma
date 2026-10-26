// P011_STRICT_REPRESENTABLE_STATE_V1, stage A. Fixed 272 x 16-bit signed
// magnitude integers in units of 2^-2148. No BigInt or heap growth in compare.
export type P011ExactCode = 'OK' | 'ARITHMETIC_ENV_UNSUPPORTED' |
  'ARITHMETIC_EXPRESSION_UNSUPPORTED' | 'NON_FINITE' | 'CERTIFICATE_CAPACITY';
export class P011ExactResult { code: P011ExactCode = 'OK'; sign: number = 0; }
export class P011ExactDyadic {
  readonly revision: string = 'DYADIC_SIGN_V1';
  private readonly acc: Uint16Array = new Uint16Array(272);
  private readonly term: Uint16Array = new Uint16Array(272);
  private readonly scratch: Uint16Array = new Uint16Array(272);
  private readonly words: Uint16Array = new Uint16Array(4);
  private readonly left: Uint16Array = new Uint16Array(4);
  private readonly right: Uint16Array = new Uint16Array(4);
  private readonly product: Uint16Array = new Uint16Array(8);
  private readonly bits: ArrayBuffer = new ArrayBuffer(8);
  private readonly view: DataView = new DataView(this.bits);
  readonly result: P011ExactResult = new P011ExactResult();
  readonly environmentSupported: boolean;
  constructor() { this.environmentSupported=P011ExactDyadic.environmentCheck(); }
  static environmentCheck(): boolean {
    const b=new ArrayBuffer(8),v=new DataView(b);
    v.setFloat64(0,-0,false);
    if(v.getUint32(0,false)!==0x80000000||v.getUint32(4,false)!==0)return false;
    v.setFloat64(0,5e-324,false);
    if(v.getUint32(0,false)!==0||v.getUint32(4,false)!==1)return false;
    if(5e-324*2!==1e-323)return false;
    if(1+Number.EPSILON===1||1+Number.EPSILON/2!==1)return false;
    return (1+Number.EPSILON)*0.5===0.5000000000000001;
  }
  private decode(value: number, out: Uint16Array): number {
    if(!Number.isFinite(value))return 0;
    this.view.setFloat64(0,value,false);
    const hi=this.view.getUint32(0,false),lo=this.view.getUint32(4,false);
    const exponent=(hi>>>20)&2047;
    const significand=(hi&0xfffff)*4294967296+lo+(exponent===0?0:4503599627370496);
    let n=significand;
    for(let i=0;i<4;i++){out[i]=n%65536;n=Math.floor(n/65536);}
    return exponent===0?-1074:exponent-1075;
  }
  private load(value: number, negative: boolean): P011ExactCode {
    const exponent=this.decode(value,this.words);
    if(!Number.isFinite(value))return 'NON_FINITE';
    return this.shifted(this.words,4,exponent+2148,(value<0)!==negative?-1:1);
  }
  private shifted(digits: Uint16Array, count: number, shift: number, sign: number): P011ExactCode {
    if(shift<0||shift>=4352)return 'ARITHMETIC_EXPRESSION_UNSUPPORTED';
    this.term.fill(0);
    const whole=Math.floor(shift/16),fraction=shift%16;
    let carry=0;
    for(let i=0;i<count;i++){
      const n=digits[i]*Math.pow(2,fraction)+carry;
      if(whole+i>=272)return 'CERTIFICATE_CAPACITY';
      this.term[whole+i]=n%65536;carry=Math.floor(n/65536);
    }
    if(carry!==0){if(whole+count>=272)return 'CERTIFICATE_CAPACITY';this.term[whole+count]=carry;}
    return this.accumulate(sign);
  }
  private magnitudeCompare(a: Uint16Array,b: Uint16Array): number {
    for(let i=271;i>=0;i--)if(a[i]!==b[i])return a[i]>b[i]?1:-1;
    return 0;
  }
  private accSign: number = 0;
  private accumulate(sign: number): P011ExactCode {
    if(this.accSign===0){this.acc.set(this.term);this.accSign=this.isZero(this.acc)?0:sign;return 'OK';}
    if(this.accSign===sign){let carry=0;for(let i=0;i<272;i++){
      const n=this.acc[i]+this.term[i]+carry;this.acc[i]=n%65536;carry=Math.floor(n/65536);
    }return carry===0?'OK':'CERTIFICATE_CAPACITY';}
    const cmp=this.magnitudeCompare(this.acc,this.term);
    if(cmp===0){this.acc.fill(0);this.accSign=0;return 'OK';}
    if(cmp<0){this.scratch.set(this.acc);this.acc.set(this.term);this.term.set(this.scratch);this.accSign=sign;}
    let borrow=0;for(let i=0;i<272;i++){
      let n=this.acc[i]-this.term[i]-borrow;borrow=n<0?1:0;if(borrow)n+=65536;this.acc[i]=n;
    }
    return 'OK';
  }
  private isZero(x: Uint16Array): boolean {for(let i=0;i<272;i++)if(x[i]!==0)return false;return true;}
  private reset(): void {this.acc.fill(0);this.accSign=0;this.result.code='OK';this.result.sign=0;}
  private finish(code: P011ExactCode): P011ExactResult {
    this.result.code=code;this.result.sign=code==='OK'?this.accSign:0;return this.result;
  }
  compareSum(left: number, right: number, u: number, bound: number): P011ExactResult {
    if(!this.environmentSupported)return this.finish('ARITHMETIC_ENV_UNSUPPORTED');
    this.reset();
    for(let i=0;i<4;i++){
      const code=this.load(i===0?right:i===1?left:i===2?u:bound,i!==0);
      if(code!=='OK')return this.finish(code);
    }
    return this.finish('OK'); // sign of right-left-u-bound
  }
  compareProduct(a: number,b: number,c: number): P011ExactResult {
    if(!this.environmentSupported)return this.finish('ARITHMETIC_ENV_UNSUPPORTED');
    this.reset();
    const ea=this.decode(a,this.left),eb=this.decode(b,this.right);
    if(!Number.isFinite(a)||!Number.isFinite(b))return this.finish('NON_FINITE');
    const product=this.product;product.fill(0);let carry=0;
    for(let k=0;k<8;k++){
      let n=carry;
      for(let i=0;i<4;i++){const j=k-i;if(j>=0&&j<4)n+=this.left[i]*this.right[j];}
      product[k]=n%65536;carry=Math.floor(n/65536);
    }
    if(carry!==0)return this.finish('CERTIFICATE_CAPACITY');
    let code=this.shifted(product,8,ea+eb+2148,(a<0)!==(b<0)?-1:1);
    if(code!=='OK')return this.finish(code);
    code=this.load(c,true);return this.finish(code);
  }
  compareRate(theta: number,before: number,omega: number,dt: number): P011ExactResult {
    if(!this.environmentSupported)return this.finish('ARITHMETIC_ENV_UNSUPPORTED');
    if(!Number.isFinite(theta)||!Number.isFinite(before)||!Number.isFinite(omega)||!Number.isFinite(dt))return this.finish('NON_FINITE');
    // First determine the exact sign of theta-before using the same fixed sum.
    const direction=this.compareSum(before,theta,0,0);
    if(direction.code!=='OK')return direction;
    const positive=direction.sign>=0;
    this.reset();
    let code=this.load(positive?theta:before,false);if(code!=='OK')return this.finish(code);
    code=this.load(positive?before:theta,true);if(code!=='OK')return this.finish(code);
    const ea=this.decode(omega,this.left),eb=this.decode(dt,this.right);
    const product=this.product;product.fill(0);let carry=0;
    for(let k=0;k<8;k++){
      let n=carry;for(let i=0;i<4;i++){const j=k-i;if(j>=0&&j<4)n+=this.left[i]*this.right[j];}
      product[k]=n%65536;carry=Math.floor(n/65536);
    }
    if(carry!==0)return this.finish('CERTIFICATE_CAPACITY');
    code=this.shifted(product,8,ea+eb+2148,-1);return this.finish(code);
  }
  unsupportedExpression(): P011ExactResult {return this.finish('ARITHMETIC_EXPRESSION_UNSUPPORTED');}
}
