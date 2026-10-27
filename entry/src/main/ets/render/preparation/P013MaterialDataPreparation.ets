// T24 layout2 staging only; never passed to the P0/native product ABI.
import { WaterSandSnapshot } from '../../physics/WaterSandField';
import { P013FrameAdapter } from '../P013FrameAdapter';
import { P013MaterialFailurePoint, P013MaterialRejectionStore } from '../P013MaterialRejection';
export class P013MaterialDataPreparation {
  readonly failurePoint: P013MaterialFailurePoint = new P013MaterialFailurePoint();
  private displayExecuted: boolean = false;
  captureDiagnostic(store: P013MaterialRejectionStore): void {
    if(this.displayExecuted)this.display.captureDiagnostic(store);else{store.copy(12,null);store.copy(19,null);}
    store.copy(13,this.staging,this.failurePoint.stagingValid*4);
  }
  readonly data: Float32Array = new Float32Array(1536);
  private readonly staging: Float32Array = new Float32Array(1536);
  private readonly display: P013FrameAdapter = new P013FrameAdapter();
  private readonly displayed: Float64Array = new Float64Array(128);
  private readonly scratch: Float64Array = new Float64Array(128);
  private readonly smooth: Float64Array = new Float64Array(128);
  private readonly weights: Float64Array = new Float64Array(37);
  constructor() {
    let sum = 0;
    for (let j = -18; j <= 18; j++) { this.weights[j + 18] = Math.exp(-j*j/72); sum += this.weights[j+18]; }
    for (let j = 0; j < 37; j++) this.weights[j] /= sum;
  }
  pack(snapshot: WaterSandSnapshot, boxWidthCanvasPx: number, boxHeightCanvasPx: number,
    warpH: Float64Array, warpL: Float64Array, warpM: Float64Array, spark: Float64Array): void {
    this.failurePoint.reset();this.displayExecuted=false;
    if (!Number.isFinite(boxWidthCanvasPx) || boxWidthCanvasPx <= 0 ||
      !Number.isFinite(boxHeightCanvasPx) || boxHeightCanvasPx <= 0 ||
      snapshot.mh.length !== 128 || snapshot.ml.length !== 128 ||
      warpH.length !== 128 || warpL.length !== 128 || warpM.length !== 128 || spark.length !== 128)
      {this.failurePoint.fail('MATERIAL_DATA_LAYOUT','MATERIAL_LAYOUT',-1,-1,-1,
        (!Number.isFinite(boxWidthCanvasPx)||boxWidthCanvasPx<=0?1:0)|(!Number.isFinite(boxHeightCanvasPx)||boxHeightCanvasPx<=0?2:0)|
        (snapshot.mh.length!==128?4:0)|(snapshot.ml.length!==128?8:0)|(warpH.length!==128?16:0)|(warpL.length!==128?32:0)|(warpM.length!==128?64:0)|(spark.length!==128?128:0));throw new Error('MATERIAL_DATA_LAYOUT');}
    for (let c = 0; c < 128; c++) if (![warpH[c],warpL[c],warpM[c],spark[c],snapshot.mh[c],snapshot.ml[c]].every(
      (v: number): boolean => Number.isFinite(v)) || spark[c] < 0 || snapshot.mh[c] < 0 || snapshot.ml[c] < 0)
      {const mask=(!Number.isFinite(warpH[c])?1:0)|(!Number.isFinite(warpL[c])?2:0)|(!Number.isFinite(warpM[c])?4:0)|(!Number.isFinite(spark[c])?8:0)|(!Number.isFinite(snapshot.mh[c])?16:0)|(!Number.isFinite(snapshot.ml[c])?32:0)|(spark[c]<0?64:0)|(snapshot.mh[c]<0?128:0)|(snapshot.ml[c]<0?256:0);
      this.failurePoint.fail('MATERIAL_DATA_VALUE','MATERIAL_VALUE',c,-1,mask&1?0:mask&2?1:mask&4?2:mask&72?3:mask&144?4:5,mask);throw new Error('MATERIAL_DATA_VALUE');}
    this.displayExecuted=true;
    if (!this.display.pack(snapshot)) {
      const p=this.display.failurePoint;this.failurePoint.fail(p.reason,p.stage,p.column,p.row,p.channel,p.predicateMask);this.failurePoint.profileValid=p.profileValid;
      throw new Error('MATERIAL_DATA_PROFILE');
    }
    this.failurePoint.profileValid=512;
    const row0 = this.display.data;
    this.displayed.set(snapshot.sandTop);
    for (let pass = 0; pass < 4; pass++) {
      for (let c = 0; c < 128; c++) this.scratch[c] =
        (this.displayed[Math.max(0,c-1)] + this.displayed[c] + this.displayed[Math.min(127,c+1)]) / 3;
      this.displayed.set(this.scratch);
    }
    for (let c = 0; c < 128; c++) {
      let sg = 0;
      for (let j = -18; j <= 18; j++) sg += this.weights[j+18] * this.displayed[Math.max(0,Math.min(127,c+j))];
      this.smooth[c] = sg;
    }
    this.staging.set(row0,0);
    this.failurePoint.stagingValid=512;
    for (let c = 0; c < 128; c++) {
      const row1 = (128+c)*4, row2 = (256+c)*4;
      this.staging[row1] = (this.smooth[Math.min(127,c+1)]-this.smooth[Math.max(0,c-1)]) *
        (boxHeightCanvasPx/snapshot.capacity)/(2*boxWidthCanvasPx/128);
      const mass = snapshot.mh[c] + snapshot.ml[c];
      this.staging[row1+1] = mass === 0 ? 0.5 : snapshot.mh[c]/mass;
      this.staging[row1+2] = warpH[c]; this.staging[row1+3] = warpL[c];
      this.staging[row2] = warpM[c]; this.staging[row2+1] = spark[c];
      this.staging[row2+2] = 0; this.staging[row2+3] = 0;
    }
    this.failurePoint.stagingValid=1536;
    for (let i = 0; i < this.staging.length; i++) if (!Number.isFinite(this.staging[i]))
      {this.failurePoint.fail('MATERIAL_FLOAT32_RANGE','MATERIAL_F32_VALIDATION',Math.floor(i/4)%128,Math.floor(i/512),i%4,1);throw new Error('MATERIAL_FLOAT32_RANGE');}
    this.data.set(this.staging);
  }
}
