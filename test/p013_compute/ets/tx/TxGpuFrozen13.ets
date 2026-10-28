import { P011ParticleSnapshot } from '../P011ParticleField';
export type TxArray=Float64Array|Uint32Array|Int32Array|Uint8Array;
export function txArrayAt(p:P011ParticleSnapshot,i:number):TxArray {
  switch(i){case 0:return p.pMass;case 1:return p.pX;case 2:return p.pY;case 3:return p.pVx;case 4:return p.pVy;case 5:return p.pPhase;case 6:return p.pSource;case 7:return p.pState;case 8:return p.pId;case 9:return p.pSourceId;case 10:return p.freeSlots;case 11:return p.colParticleMassH;default:return p.colParticleMassL;}
}
export class TxGpuFrozen13 {
  readonly copies:Uint8Array[]=[];readonly sourceBuffers:ArrayBuffer[]=new Array<ArrayBuffer>(13);
  readonly offsets:Uint32Array=new Uint32Array(13);readonly lengths:Uint32Array=new Uint32Array(13);
  nmax:number=0;cols:number=0;live:number=0;free:number=0;nextId:number=0;
  constructor(k:number){for(let i=0;i<13;i++)this.copies.push(new Uint8Array(i>=11?1024:i<5?k*8:i<8?k:k*4));}
  freeze(p:P011ParticleSnapshot):void {
    this.nmax=p.nmax;this.cols=p.cols;this.live=p.liveCount;this.free=p.freeCount;this.nextId=p.nextId;
    for(let i=0;i<13;i++){const a=txArrayAt(p,i);if(a.byteLength!==this.copies[i].length)throw new Error('TX_GPU_SNAPSHOT_SHAPE');this.sourceBuffers[i]=a.buffer as ArrayBuffer;this.offsets[i]=a.byteOffset;this.lengths[i]=a.byteLength;
      // These bounded view wrappers are accounted separately from backing.
      this.copies[i].set(new Uint8Array(a.buffer as ArrayBuffer,a.byteOffset,a.byteLength));}
  }
  same(p:P011ParticleSnapshot):boolean {
    if(p.nmax!==this.nmax||p.cols!==this.cols||p.liveCount!==this.live||p.freeCount!==this.free||p.nextId!==this.nextId)return false;
    for(let i=0;i<13;i++){const a=txArrayAt(p,i);if(a.buffer!==this.sourceBuffers[i]||a.byteOffset!==this.offsets[i]||a.byteLength!==this.lengths[i])return false;
      const bytes=new Uint8Array(a.buffer as ArrayBuffer,a.byteOffset,a.byteLength);for(let j=0;j<bytes.length;j++)if(bytes[j]!==this.copies[i][j])return false;}return true;
  }
}
