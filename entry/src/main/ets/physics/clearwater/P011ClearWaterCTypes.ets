import { ClearWaterSourceReceipt } from './P011ClearWaterTypes';
// T226: new C namespace. A/B serialized layouts and status values are unchanged.
export enum ClearWaterReceiptQuery { NOT_COMMITTED=0, TERMINAL=1, IDENTITY_CONFLICT=2, STALE_RUN=3, INVALID_REQUEST=4, BUSY=5 }
export class ClearWaterReceiptRead {
  query:ClearWaterReceiptQuery=ClearWaterReceiptQuery.NOT_COMMITTED;
  readonly receipt:ClearWaterSourceReceipt=new ClearWaterSourceReceipt();
}
export class ClearWaterCaptureBoundary {
  runId:string='';runGeneration:number=0;configRev:number=0;inputGeneration:number=0;geometryEpoch:number=0;publicationSerial:number=0;
}
export class ClearWaterSceneSlot {
  readonly profile64:Float64Array;readonly particleProof64:Float64Array;readonly originSourceCommit64:Float64Array;
  readonly waitReason8:Uint8Array;readonly xy32:Float32Array;readonly meta32:Uint32Array;
  readonly header64:Float64Array=new Float64Array(64);readonly ledger64:Float64Array=new Float64Array(64);readonly optics64:Float64Array=new Float64Array(16);
  payloadDigest:string='';private readonly bytes:Uint8Array[]=[];
  readonly buffers:ArrayBuffer[];readonly lengths:number[];readonly capacity:number;
  private readonly fold:Float64Array=new Float64Array(128);
  private readonly f:Float32Array=new Float32Array(1);private readonly bits:Uint32Array;
  constructor(k:number){
    if(!Number.isInteger(k)||k<1||k>512)throw new Error('CW_C_CAPACITY');this.capacity=k;
    this.profile64=new Float64Array(1024);this.particleProof64=new Float64Array(8*k);this.originSourceCommit64=new Float64Array(k);
    this.waitReason8=new Uint8Array(k);this.xy32=new Float32Array(2*k);this.meta32=new Uint32Array(2*k);this.bits=new Uint32Array(this.f.buffer);
    this.buffers=[this.profile64.buffer as ArrayBuffer,this.particleProof64.buffer as ArrayBuffer,this.originSourceCommit64.buffer as ArrayBuffer,this.waitReason8.buffer as ArrayBuffer,this.xy32.buffer as ArrayBuffer,this.meta32.buffer as ArrayBuffer,this.header64.buffer as ArrayBuffer,this.ledger64.buffer as ArrayBuffer,this.optics64.buffer as ArrayBuffer];
    this.lengths=[8192,64*k,8*k,k,8*k,8*k,512,512,128];
    for(let i=0;i<this.buffers.length;i++)this.bytes.push(new Uint8Array(this.buffers[i]));
  }
  valid(owned:ArrayBuffer[]):boolean {
    // Whole-buffer views only; disallow overlapping caller/owner storage before any write.
    if(this.profile64.byteOffset!==0||this.particleProof64.byteOffset!==0||this.originSourceCommit64.byteOffset!==0||this.waitReason8.byteOffset!==0||this.xy32.byteOffset!==0||this.meta32.byteOffset!==0||this.header64.byteOffset!==0||this.ledger64.byteOffset!==0||this.optics64.byteOffset!==0)return false;
    if(this.profile64.buffer!==this.buffers[0]||this.particleProof64.buffer!==this.buffers[1]||this.originSourceCommit64.buffer!==this.buffers[2]||this.waitReason8.buffer!==this.buffers[3]||this.xy32.buffer!==this.buffers[4]||this.meta32.buffer!==this.buffers[5]||this.header64.buffer!==this.buffers[6]||this.ledger64.buffer!==this.buffers[7]||this.optics64.buffer!==this.buffers[8])return false;
    if(this.profile64.byteLength!==this.lengths[0]||this.particleProof64.byteLength!==this.lengths[1]||this.originSourceCommit64.byteLength!==this.lengths[2]||this.waitReason8.byteLength!==this.lengths[3]||this.xy32.byteLength!==this.lengths[4]||this.meta32.byteLength!==this.lengths[5]||this.header64.byteLength!==512||this.ledger64.byteLength!==512||this.optics64.byteLength!==128)return false;
    for(let i=0;i<9;i++){if(this.buffers[i].byteLength!==this.lengths[i])return false;for(let j=0;j<i;j++)if(this.buffers[i]===this.buffers[j])return false;for(let j=0;j<owned.length;j++)if(this.buffers[i]===owned[j])return false;}return true;
  }
  verify():boolean {
    const h=this.header64,l=this.ledger64;this.fold.fill(0);let last=0,tw=0,ts=0,tp=0,tr=0;
    if(h[18]<0||h[18]>this.capacity||h[19]!==this.capacity||h[20]!==128||h[21]!==256||h[3]!==h[1])return false;
    for(let i=0;i<h[18];i++){const j=i*8,a=this.particleProof64,id=a[j],x=a[j+2],m=a[j+1];
      if(!Number.isSafeInteger(id)||id<=last||!Number.isFinite(m)||m<=0||!Number.isFinite(x)||x<0||x>=128||!Number.isFinite(a[j+3])||a[j+3]<0||a[j+3]>256||!Number.isFinite(a[j+4])||!Number.isFinite(a[j+5])||(a[j+7]!==256&&a[j+7]!==512)||this.meta32[i*2]!==a[j+7]||this.meta32[i*2+1]!==id||this.xy32[i*2]!==this.projectX(x)||this.xy32[i*2+1]!==this.projectY(a[j+3]))return false;
      this.fold[Math.floor(x)]+=m;last=id;
    }
    for(let c=0;c<128;c++){const j=c*8,a=this.profile64;for(let k=0;k<8;k++)if(!Number.isFinite(a[j+k])||a[j+k]<0)return false;
      if(a[j]!==((a[j+4]+0)+a[j+5])||a[j+2]!==a[j]+a[j+1]||a[j+3]!==this.fold[c]||a[j+6]!==a[j+2]+a[j+3]||a[j+6]>256||a[j+7]!== (a[j+1]>0?1:0))return false;
      tw+=a[j+1];ts+=a[j];tp+=a[j+3];tr+=a[j+6];
    }
    return tw===l[11]&&ts===l[12]&&tp===l[13]&&tr===l[14];
  }
  digest():void {let h=0x811c9dc5;for(let i=0;i<this.bytes.length;i++)for(let j=0;j<this.bytes[i].length;j++)h=Math.imul(h^this.bytes[i][j],0x01000193);this.payloadDigest='CW_RAW_FNV1A32:'+ (h>>>0).toString(16);}
  projectX(x:number):number {this.f[0]=x;if(this.f[0]>=Math.floor(x)+1)this.bits[0]--;return this.f[0];}
  projectY(y:number):number {this.f[0]=y;if(this.f[0]>256)this.bits[0]--;return this.f[0];}
}
