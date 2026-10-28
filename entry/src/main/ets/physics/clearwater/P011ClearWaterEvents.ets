// T209/T203 frozen event40, lineage3, summary16; no borrowed public views.
import { BKind,BReason,bFlags,bField,CW_B_USED_MASKS } from './P011ClearWaterBTypes';
import { ClearWaterState } from './P011ClearWaterState';
import { cwSameFiniteBits } from './P011ClearWaterTypes';
export class ClearWaterBOrigins {
  readonly bytes:Uint8Array=new Uint8Array(4096*16);readonly view:DataView=new DataView(this.bytes.buffer as ArrayBuffer);count:number=0;
  append(first:number,last:number,origin:number):boolean {
    if(!this.canAppend(first,last,origin))return false;
    const o=this.count*16;this.view.setUint32(o,first,true);this.view.setUint32(o+4,last,true);this.view.setFloat64(o+8,origin,true);this.count++;return true;
  }
  canAppend(first:number,last:number,origin:number):boolean {
    if(this.count>=4096||!Number.isInteger(first)||first<=0||last<first||last>0xffffffff||!Number.isSafeInteger(origin)||origin<=0)return false;
    if(this.count>0&&(first<=this.view.getUint32((this.count-1)*16+4,true)||origin<=this.view.getFloat64((this.count-1)*16+8,true)))return false;
    return true;
  }
  find(id:number):number {let l=0,r=this.count-1;while(l<=r){const mid=Math.floor((l+r)/2),o=mid*16,first=this.view.getUint32(o,true),last=this.view.getUint32(o+4,true);if(id<first)r=mid-1;else if(id>last)l=mid+1;else return this.view.getFloat64(o+8,true);}return 0;}
  reset():void {this.bytes.fill(0);this.count=0;}
}
export class ClearWaterBArena {
  readonly events:Float64Array;readonly lineage:Float64Array;readonly header:Float64Array=new Float64Array(64);readonly summary:Float64Array;
  count:number=0;lineageCount:number=0;reason:BReason=BReason.NONE;row:number=0;
  constructor(e:number,l:number,k:number){this.events=new Float64Array(e*40);this.lineage=new Float64Array(l*3);this.summary=new Float64Array(k*16);}
  reset():void {this.count=this.lineageCount=0;this.reason=BReason.NONE;this.header.fill(0);for(let i=40;i<=54;i++)this.header[i]=NaN;this.summary.fill(0);}
  reserveInto(kind:number,slot:number,id:number,origin:number,from:number,to:number,lines:number,eLimit:number):boolean {
    if(this.count>=this.events.length/40){this.reason=BReason.EVENT_ARENA_FULL;return false;}
    if(this.lineageCount+lines>this.lineage.length/3){this.reason=BReason.LINEAGE_ARENA_FULL;return false;}
    if(slot>=0&&this.summary[slot*16+14]>=eLimit){this.reason=BReason.EVENT_LIMIT;return false;}
    this.row=this.count*40;for(let j=0;j<40;j++)this.events[this.row+j]=0;
    this.set(0,kind);this.set(1,this.count+1);this.set(2,slot);this.set(3,id);this.set(4,origin);this.set(5,from);this.set(6,to);this.set(37,this.lineageCount);this.set(38,lines);return true;
  }
  set(j:number,v:number):void {this.events[this.row+j]=v;}
  get(j:number):number {return this.events[this.row+j];}
  particleBefore(s:ClearWaterState,slot:number):void {
    const p=s.pool;this.set(9,p.pMass[slot]);this.set(10,p.pMass[slot]);this.set(11,p.pX[slot]);this.set(12,p.pY[slot]);this.set(13,p.pVx[slot]);this.set(14,p.pVy[slot]);
  }
  particleAfter(s:ClearWaterState,slot:number,ns:number,nw:number,om:number,nm:number,reason:number):void {
    const p=s.pool;this.set(15,p.pX[slot]);this.set(16,p.pY[slot]);this.set(17,p.pVx[slot]);this.set(18,p.pVy[slot]);
    this.set(39,bFlags(p.pState[slot],ns,s.waitReason![slot],nw,om,nm,reason));
  }
  commitRowInto():boolean {
    const a=this.events,b=this.row,kind=a[b],slot=a[b+2],count=a[b+38],mask=CW_B_USED_MASKS[kind];
    for(let j=0;j<40;j++){const v=a[b+j];if(!Number.isFinite(v)||Math.floor(mask/Math.pow(2,j))%2===0&&!cwSameFiniteBits(v,0)){this.reason=BReason.LOCAL_NUMERIC_INVALID;return false;}}
    if(slot>=0){const start=slot*16;if(this.summary[start+14]===0)this.summary[start+13]=this.count+1;this.summary[start+14]++;
      this.summary[start]=a[b+3];this.summary[start+1]=slot;this.summary[start+2]=a[b+4];this.summary[start+3]=bField(a[b+39],2,2);this.summary[start+4]=bField(a[b+39],7,3);this.summary[start+5]=a[b+6];this.summary[start+6]=a[b+10];
      this.summary[start+7]=a[b+15];this.summary[start+8]=a[b+16];this.summary[start+9]=a[b+17];this.summary[start+10]=a[b+18];this.summary[start+15]=a[b+39];}
    for(let n=0;n<count;n++){const l=(this.lineageCount+n)*3;this.lineage[l]=a[b+3];this.lineage[l+1]=a[b+4];this.lineage[l+2]=kind===BKind.LAND?(a[b+10]===0?(n===0?2:3):(n===0?1:2)):1;}
    this.count++;this.lineageCount+=count;return true;
  }
  validateRow(b:number):boolean {
    const a=this.events,k=a[b];if(!Number.isInteger(k)||k<1||k>7||a[b+1]!==b/40+1||!Number.isInteger(a[b+39])||a[b+39]<0||a[b+39]>=4194304)return false;
    const mask=CW_B_USED_MASKS[k];for(let j=0;j<40;j++)if(!Number.isFinite(a[b+j])||Math.floor(mask/Math.pow(2,j))%2===0&&!cwSameFiniteBits(a[b+j],0))return false;
    const n=a[b+38],start=a[b+37];if(!Number.isInteger(start)||start<0||!Number.isInteger(n)||n!==(k===5?2:k<=4?1:0)||start+n>this.lineageCount)return false;
    for(let j=0;j<n;j++){const o=(start+j)*3,edge=k===5?(a[b+10]===0?(j===0?2:3):(j===0?1:2)):1;if(this.lineage[o]!==a[b+3]||this.lineage[o+1]!==a[b+4]||this.lineage[o+2]!==edge)return false;}
    if(k<=5&&(slotInvalid(a[b+2],this.summary.length/16)||a[b+3]<=0||a[b+3]>0xffffffff||!Number.isSafeInteger(a[b+4])||a[b+4]<=0))return false;
    if(k>5&&(a[b+2]!==-1||a[b+3]!==0||a[b+4]!==0))return false;
    if(k<=4&&(bField(a[b+39],10,2)!==a[b+25]||bField(a[b+39],12,2)!==a[b+26]||a[b+21]!==a[b+19]*a[b+22]))return false;
    return true;
  }
}
function slotInvalid(i:number,k:number):boolean {return !Number.isInteger(i)||i<0||i>=k;}
