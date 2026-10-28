// T209/T203: exact nonnegative binary64 mass in units of 2^-1074.
import { ClearWaterState } from './P011ClearWaterState';
import { BReason } from './P011ClearWaterBTypes';
export class ClearWaterExactMass {
  readonly pExact70:Uint16Array=new Uint16Array(70*128);
  readonly capacityBound70:Uint16Array=new Uint16Array(70*128);
  readonly scratch0:Uint16Array=new Uint16Array(70);readonly scratch1:Uint16Array=new Uint16Array(70);
  readonly bits:Uint8Array=new Uint8Array(8);private readonly view:DataView=new DataView(this.bits.buffer as ArrayBuffer);
  reason:BReason=BReason.NONE;
  constructor(privateCounters:Float64Array){this.counters=privateCounters;}
  private readonly counters:Float64Array;
  encodeInto(value:number,target:Uint16Array,offset:number):boolean {
    this.counters[26]++;if(!Number.isFinite(value)||value<0||offset<0||offset+70>target.length){this.reason=BReason.MASS_ENCODING_DOMAIN;return false;}
    this.view.setFloat64(0,value,true);const lo=this.view.getUint32(0,true),hi=this.view.getUint32(4,true),exponent=Math.floor(hi/1048576)%2048;
    const shift=exponent===0?0:exponent-1,word=Math.floor(shift/16),rem=shift%16;
    // Three full U16 words plus the top five significand bits, all exact.
    const top=(hi%1048576)+(exponent===0?0:1048576);
    if(value>=Math.pow(2,46)){this.reason=BReason.MASS_ENCODING_DOMAIN;return false;}
    for(let j=0;j<70;j++)target[offset+j]=0;
    let carry=0;
    for(let j=0;j<4;j++){
      const digit=j===0?lo%65536:j===1?Math.floor(lo/65536):j===2?top%65536:Math.floor(top/65536);
      const n=digit*Math.pow(2,rem)+carry,k=word+j;
      if(k<70)target[offset+k]=n%65536;else if(n!==0){this.reason=BReason.MASS_ENCODING_DOMAIN;return false;}
      carry=Math.floor(n/65536);
    }
    if(carry!==0){if(word+4>=70){this.reason=BReason.MASS_ENCODING_DOMAIN;return false;}target[offset+word+4]=carry;}
    return true;
  }
  compare(a:Uint16Array,o:number,b:Uint16Array,p:number):number {this.counters[29]++;for(let j=69;j>=0;j--)if(a[o+j]!==b[p+j])return a[o+j]<b[p+j]?-1:1;return 0;}
  add(a:Uint16Array,o:number,b:Uint16Array,p:number):boolean {
    this.counters[27]++;let carry=0;for(let j=0;j<70;j++)carry=Math.floor((a[o+j]+b[p+j]+carry)/65536);
    if(carry){this.reason=BReason.MASS_ACCUM_OVERFLOW;return false;}
    carry=0;for(let j=0;j<70;j++){const n=a[o+j]+b[p+j]+carry;a[o+j]=n%65536;carry=Math.floor(n/65536);}return true;
  }
  subtract(a:Uint16Array,o:number,b:Uint16Array,p:number):boolean {
    this.counters[28]++;if(this.compare(a,o,b,p)<0){this.reason=BReason.MASS_ACCUM_UNDERFLOW;return false;}
    let borrow=0;for(let j=0;j<70;j++){const n=a[o+j]-b[p+j]-borrow;a[o+j]=n<0?n+65536:n;borrow=n<0?1:0;}return true;
  }
  copy(a:Uint16Array,o:number,b:Uint16Array,p:number):void {for(let j=0;j<70;j++)a[o+j]=b[p+j];}
  buildFromStateInto(s:ClearWaterState,freeze:boolean):boolean {
    this.reason=BReason.NONE;this.pExact70.fill(0);
    let i=s.pool.head,n=0;while(i!==-1&&n++<s.pool.nmax){const c=Math.floor(s.pool.pX[i]);if(!this.encodeInto(s.pool.pMass[i],this.scratch0,0)||!this.add(this.pExact70,c*70,this.scratch0,0))return false;i=s.pool.next[i];}
    if(!freeze)return true;
    for(let c=0;c<128;c++){
      this.copy(this.scratch0,0,this.pExact70,c*70);
      if(!this.encodeInto(s.S[c],this.scratch1,0)||!this.add(this.scratch0,0,this.scratch1,0)||!this.encodeInto(s.W[c],this.scratch1,0)||!this.add(this.scratch0,0,this.scratch1,0))return false;
      if(!this.encodeInto(256,this.scratch1,0))return false;
      this.copy(this.capacityBound70,c*70,this.compare(this.scratch0,0,this.scratch1,0)>0?this.scratch0:this.scratch1,0);
    }return true;
  }
  // Preview one column. P masses are encoded separately, never RN(old-new).
  fitsPatchInto(c:number,newS:number,newW:number,oldMass:number,newMass:number):boolean {
    this.copy(this.scratch0,0,this.pExact70,c*70);
    if(!this.encodeInto(oldMass,this.scratch1,0)||!this.subtract(this.scratch0,0,this.scratch1,0)||!this.encodeInto(newMass,this.scratch1,0)||!this.add(this.scratch0,0,this.scratch1,0)||
      !this.encodeInto(newS,this.scratch1,0)||!this.add(this.scratch0,0,this.scratch1,0)||!this.encodeInto(newW,this.scratch1,0)||!this.add(this.scratch0,0,this.scratch1,0))return false;
    return this.compare(this.scratch0,0,this.capacityBound70,c*70)<=0;
  }
  applyMassInto(c:number,oldMass:number,newMass:number):boolean {
    return this.encodeInto(oldMass,this.scratch1,0)&&this.subtract(this.pExact70,c*70,this.scratch1,0)&&this.encodeInto(newMass,this.scratch1,0)&&this.add(this.pExact70,c*70,this.scratch1,0);
  }
  nextDown(v:number):number {
    if(!Number.isFinite(v)||v<0){this.reason=BReason.MASS_ENCODING_DOMAIN;return NaN;}if(v===0)return -Number.MIN_VALUE;
    this.view.setFloat64(0,v,true);let lo=this.view.getUint32(0,true),hi=this.view.getUint32(4,true);if(lo===0){lo=0xffffffff;hi--;}else lo--;
    this.view.setUint32(0,lo,true);this.view.setUint32(4,hi,true);return this.view.getFloat64(0,true);
  }
}
