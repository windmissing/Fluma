import { WaterSandSnapshot } from '../../physics/WaterSandField';
interface T248BedColumns {bh:number[];mh:number[];}
// Cold diagnostic copies only; caller retains the original dt/command semantics.
export class T248Observation {
  readonly revision: string = 'T248_READONLY_V1';
  acceptedSeconds: number = 0; stepCount: number = 0;
  recordStep(dt: number,ok: boolean,stepped: boolean): void {
    if(ok&&stepped&&Number.isFinite(dt)&&dt>0){this.acceptedSeconds+=dt;this.stepCount++;}
  }
  capture(s: WaterSandSnapshot): string {
    let bh=0,bl=0,mh=0,ml=0;
    for(let c=0;c<s.cols;c++){bh+=s.bh[c];bl+=s.bl[c];mh+=s.mh[c];ml+=s.ml[c];}
    const counts:number[]=[0,0,0,0];let minY=Infinity,maxY=-Infinity,minVy=Infinity,maxVy=-Infinity;
    const p=s.particleState;
    if(p!==null&&p!==undefined)for(let i=0;i<p.nmax;i++)if(p.pState[i]!==0){
      if(p.pState[i]<counts.length)counts[p.pState[i]]++;
      minY=Math.min(minY,p.pY[i]);maxY=Math.max(maxY,p.pY[i]);minVy=Math.min(minVy,p.pVy[i]);maxVy=Math.max(maxVy,p.pVy[i]);
    }
    const bedColumns:T248BedColumns={bh:Array.from(s.bh),mh:Array.from(s.mh)};
    return JSON.stringify({revision:this.revision,acceptedSeconds:this.acceptedSeconds,stepCount:this.stepCount,
      bh:bh,bl:bl,mh:mh,ml:ml,states:counts,minY:Number.isFinite(minY)?minY:null,maxY:Number.isFinite(maxY)?maxY:null,
      minVy:Number.isFinite(minVy)?minVy:null,maxVy:Number.isFinite(maxVy)?maxVy:null,
      bedColumns:bedColumns,landEvents:'NOT_CAPTURED_USE_WRITER_HOOK',pixel:'NOT_CAPTURED_USE_NATIVE_SAME_TOKEN_HOOK'});
  }
}
