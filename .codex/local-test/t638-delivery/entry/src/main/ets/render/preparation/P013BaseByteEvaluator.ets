import { p013Sha256 } from './P013CanvasAssetExport';

export const P013_BASE_EVALUATOR='P1_FROZEN_F64_EVAL_V1';
export const P013_BASE_REPRESENTATION='P013_NATIVE_X_F64_BASEBYTE_V2';
export const P013_BASE_PALETTE='P1_ENCODED_PALETTE_F64_V1';
export class P013BaseStripDescriptor {
  apiRevision: number=1; representation: string=P013_BASE_REPRESENTATION;
  evaluator: string=P013_BASE_EVALUATOR;paletteInputRevision: string=P013_BASE_PALETTE;
  sessionId: number=0;stripSerial: number=0;frameToken: string='';runId: string='';
  stateEpoch: number=0;physicsCommitId: number=0;configBindingId: number=0;configDigest: string='';
  geometryEpoch: number=0;contextEpoch: number=0;surfaceEpoch: number=0;
  viewportOriginX: number=0;viewportOriginY: number=0;width: number=0;height: number=0;
  boxXPx: number=0;boxWidthPx: number=0;mirror: boolean=false;sparkEnabled: boolean=false;
  profileSha256: string='';paletteHash: string='';payloadSha256: string='';byteLength: number=0;
}
export class P013BaseByteEvaluator {
  private paletteEncodedF64: Float64Array=new Float64Array([145,168,111,216,196,150]);
  private scratch: Uint8Array=new Uint8Array(0);
  get bytes(): Uint8Array {return this.scratch;}
  ensureWidth(width: number): boolean {
    if(!Number.isSafeInteger(width)||width<1||width>32768)return false;
    if(this.scratch.length!==width*12)this.scratch=new Uint8Array(width*12);
    return true;
  }
  evaluate(width: number,originX: number,boxX: number,boxWidth: number,mirror: boolean,spark: boolean,
    profile: Float32Array): boolean {
    if(!this.ensureWidth(width)||profile.length<1536||!Number.isSafeInteger(originX)||
      !Number.isFinite(boxX)||!Number.isFinite(boxWidth)||boxWidth<=0)return false;
    const out=this.scratch,p=this.paletteEncodedF64;
    for(let j=0;j<width;j++){
      const pixelX=originX+j+0.5,q0=pixelX-boxX,valid=q0>=0&&q0<boxWidth;
      const q=mirror?boxWidth-q0:q0;
      const r=Math.min(Math.max(q/(boxWidth/128),0),127);
      const i=Math.min(Math.floor(r),126),t=r-i;
      const heavy=profile[(128+i)*4+1]*(1-t)+profile[(128+i+1)*4+1]*t;
      const gain=spark?profile[(256+i)*4+1]*(1-t)+profile[(256+i+1)*4+1]*t:1;
      for(let k=0;k<3;k++)for(let c=0;c<3;c++){
        let base=k===0?p[c]:k===1?p[3+c]:p[3+c]*(1-heavy)+p[c]*heavy;
        base=base*gain;
        out[(k*width+j)*4+c]=valid?Math.floor(Math.min(Math.max(base,0),248)+0.5):0;
      }
      for(let k=0;k<3;k++)out[(k*width+j)*4+3]=valid?1:0;
    }
    return true;
  }
  populateDescriptor(d: P013BaseStripDescriptor,profile: Float32Array): void {
    d.byteLength=this.scratch.length;d.profileSha256=p013Sha256(new Uint8Array(profile.buffer,profile.byteOffset,1536*4));
    d.paletteHash=p013Sha256(new Uint8Array(this.paletteEncodedF64.buffer));
    d.payloadSha256=p013Sha256(this.scratch);
  }
}
