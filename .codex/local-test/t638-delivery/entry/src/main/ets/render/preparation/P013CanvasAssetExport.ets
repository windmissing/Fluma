// T48: copies the renderer's actual pre-putImageData bytes. No generator here.
import { P013PreparedMetadata, P013PreparedBundle, p013PrepareBundle } from './P013MaterialPreparation';
export function p013Sha256(bytes: Uint8Array): string {
  const k: number[] = [0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,
    0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,
    0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,
    0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,
    0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2];
  const h: number[] = [0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19];
  const n = Math.ceil((bytes.length+9)/64)*64, data = new Uint8Array(n), words = new Uint32Array(64);
  data.set(bytes); data[bytes.length]=128;
  const bits = bytes.length*8;
  for(let i=0;i<8;i++) data[n-1-i]=Math.floor(bits/Math.pow(256,i))%256;
  const rotate = (v: number, s: number): number => (v>>>s)|(v<<(32-s));
  for(let offset=0;offset<n;offset+=64) {
    for(let i=0;i<16;i++) { const j=offset+4*i; words[i]=(data[j]<<24)|(data[j+1]<<16)|(data[j+2]<<8)|data[j+3]; }
    for(let i=16;i<64;i++) { const a=words[i-15], b=words[i-2]; words[i]=(words[i-16]+(rotate(a,7)^rotate(a,18)^(a>>>3))+words[i-7]+(rotate(b,17)^rotate(b,19)^(b>>>10)))>>>0; }
    let a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],v=h[7];
    for(let i=0;i<64;i++) { const t1=(v+(rotate(e,6)^rotate(e,11)^rotate(e,25))+((e&f)^(~e&g))+k[i]+words[i])>>>0;
      const t2=((rotate(a,2)^rotate(a,13)^rotate(a,22))+((a&b)^(a&c)^(b&c)))>>>0;
      v=g;g=f;f=e;e=(d+t1)>>>0;d=c;c=b;b=a;a=(t1+t2)>>>0; }
    const row: number[]=[a,b,c,d,e,f,g,v];for(let i=0;i<8;i++)h[i]=(h[i]+row[i])>>>0;
  }
  return h.map((v: number): string => v.toString(16).padStart(8,'0')).join('');
}
export class P013CanvasAssetExport {
  private grain: Array<Uint8Array | null> = [null,null,null];
  private strata: Array<Uint8Array | null> = [null,null,null];
  capture(kind: 'grain' | 'strata', layer: number, data: Uint8ClampedArray): void {
    if (layer<0 || layer>2 || !Number.isInteger(layer)) throw new Error('MATERIAL_LAYER');
    const expected = kind === 'grain' ? 256*256*4 : 64*256*4;
    if(data.length!==expected)throw new Error('MATERIAL_LENGTH');
    const owned=new Uint8Array(data.length);owned.set(data);
    if(kind==='grain')this.grain[layer]=owned;else this.strata[layer]=owned;
  }
  clear(kind: 'grain' | 'strata'): void {
    const layers=kind==='grain'?this.grain:this.strata;
    for(let i=0;i<3;i++)layers[i]=null;
  }
  bundle(metadata: P013PreparedMetadata): P013PreparedBundle {
    const grain=new Uint8Array(786432),strata=new Uint8Array(196608);
    for(let i=0;i<3;i++) {
      const g=this.grain[i],s=this.strata[i];if(g===null||s===null)throw new Error('MATERIAL_MISSING');
      grain.set(g,i*262144);strata.set(s,i*65536);
    }
    metadata.grainHash=p013Sha256(grain); metadata.strataHash=p013Sha256(strata);
    return p013PrepareBundle(metadata,grain,strata);
  }
}
