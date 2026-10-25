// T24 layout2 staging only; never passed to the P0/native product ABI.
import { WaterSandSnapshot } from '../../physics/WaterSandField';
import { P013FrameAdapter } from '../P013FrameAdapter';
export class P013MaterialDataPreparation {
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
    if (!Number.isFinite(boxWidthCanvasPx) || boxWidthCanvasPx <= 0 ||
      !Number.isFinite(boxHeightCanvasPx) || boxHeightCanvasPx <= 0 ||
      snapshot.mh.length !== 128 || snapshot.ml.length !== 128 ||
      warpH.length !== 128 || warpL.length !== 128 || warpM.length !== 128 || spark.length !== 128)
      throw new Error('MATERIAL_DATA_LAYOUT');
    for (let c = 0; c < 128; c++) if (![warpH[c],warpL[c],warpM[c],spark[c],snapshot.mh[c],snapshot.ml[c]].every(
      (v: number): boolean => Number.isFinite(v)) || spark[c] < 0 || snapshot.mh[c] < 0 || snapshot.ml[c] < 0)
      throw new Error('MATERIAL_DATA_VALUE');
    if (!this.display.pack(snapshot)) throw new Error('MATERIAL_DATA_PROFILE');
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
    for (let i = 0; i < this.staging.length; i++) if (!Number.isFinite(this.staging[i]))
      throw new Error('MATERIAL_FLOAT32_RANGE');
    this.data.set(this.staging);
  }
}
