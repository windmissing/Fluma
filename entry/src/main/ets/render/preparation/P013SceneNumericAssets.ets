import { singleSandRawGrain } from './P013SingleSandAssets';
// T56 pure numeric extraction of the T48 signed generators. No Canvas/bitmap APIs.
import { P013MaterialBundleDesc } from '../P013MaterialTypes';
import { P012AppearanceParams } from '../P012RenderTypes';
import { p013Sha256 } from './P013CanvasAssetExport';
export class P013SceneAssets {
  desc: P013MaterialBundleDesc = new P013MaterialBundleDesc();
  grain: Uint8Array = new Uint8Array(786432);
  strata: Uint8Array = new Uint8Array(196608);
}
const P012_SPARK_TILE = 64;
const P012_SPARK_COVERAGE = 0.012;
const P012_SPARK_GAIN = 0.5;
const P012_GRAIN_TILE = 256;
const P012_B_SIGMA = 20;
const P012_B_GRAIN_ALPHA = 0.75;
const P012_B_FINE_PX = 2;
const P012_B_ENV_PX = 4;
const P012_C_BITMAP_W = 64;
const P012_C_BITMAP_H = 256;
const P012_C_LAMBDA_MATERIAL = 24;
const P012_C_WARP_RANGE = 0.25;
const P012_C_SEG_THRESHOLD = 1 / 16;
const P012_C_SEG_MAX = 16;
const P012_C_AMP_ORDERED = 6;
const P012_C_AMP_MIX = 3;
const P012_C_ALPHA = 0.5;
const P012_C_BAND_RPx = 6;
export class P013SceneNumericAssets {
  readonly warpBase: Float64Array = new Float64Array(128);
  readonly sparkTile: Float64Array = new Float64Array(4096);
  constructor(){this.buildWarpBase(20261028);this.buildSparkTile(20261020);}
  warp(x: number, boxX: number, colW: number, uv: number): number {return this.warpAt(x,boxX,colW,uv);}
  gain(col: number): number {const spark=this.sparkTile[(col%64)*64+(col%64)];return spark>0?1+.5*spark*.3:1;}
  build(app: P012AppearanceParams, pxPerMaterial: number, geometry: number, seed: number): P013SceneAssets {
    if(pxPerMaterial<=0)throw new Error('SCENE_ASSET_GEOMETRY');
    const out=new P013SceneAssets(),d=out.desc;
    d.geometryEpoch=geometry;d.renderSeed=seed;
    d.assetGeneratorVersion='P012_CANVAS_SEEDS_20261022_27_V1';
    d.assetGenerationParameters=JSON.stringify({grainSeeds:[20261022,20261023,20261024],strataSeeds:[20261025,20261026,20261027],grainSigma:app.grainSigma,grainFeaturePx:app.grainFeaturePx,strataStrengthScale:app.strataStrengthScale,nominalLambdaMaterial:app.strataLambdaMaterial,pxPerMaterial:pxPerMaterial});
    const lambda=256/Math.max(1,Math.ceil(256/(app.strataLambdaMaterial*pxPerMaterial)));
    d.effectiveLambdaCanvasPx=lambda;d.effectiveLambdaMaterial=lambda/pxPerMaterial;d.warpRangeCycles=.25;
    const bases:number[][]=[[145,168,111],[216,196,150],[180.5,182,130.5]];
    for(let k=0;k<3;k++){
      const tile=this.buildGrainTileFine(20261022+k,app.grainFeaturePx);
      for(let i=0;i<65536;i++){const delta=tile[i]*app.grainSigma;for(let c=0;c<3;c++)out.grain[k*262144+i*4+c]=this.clamp255(bases[k][c]+delta);out.grain[k*262144+i*4+3]=255;}
      const noise=new Float64Array(256);this.buildStratNoise(noise,20261025+k);
      for(let v=0;v<64;v++)for(let u=0;u<256;u++){
        const phase=u/lambda+(v/64)*.25,edge=Math.min(u,255-u)/(lambda*.5),env=Math.max(0,Math.min(1,edge));
        const gray=this.clamp255(128+env*(k===2?3:6)*app.strataStrengthScale*this.stratNoiseAt(noise,phase));
        const offset=k*65536+(u*64+v)*4;out.strata[offset]=out.strata[offset+1]=out.strata[offset+2]=gray;out.strata[offset+3]=255;
      }
    }
    d.grainHash=p013Sha256(out.grain);d.strataHash=p013Sha256(out.strata);d.paletteHash=p013Sha256(new Uint8Array([145,168,111,216,196,150]));
    return out;
  }
  private buildGrainTileFine(seed:number,finePx:number):Float64Array {
    return singleSandRawGrain(seed,finePx);
  }
  private buildStratNoise(out: Float64Array, seed: number): void {
    const n = out.length;                        // = P012_C_BITMAP_H = 256
    let state = seed | 0;
    const next = (): number => {
      state = (Math.imul(state, 1664525) + 1013904223) | 0;
      return ((state >>> 8) & 0xffffff) / 0xffffff;
    };
    const white = new Float64Array(n);
    for (let i = 0; i < n; i++) {
      white[i] = next() * 2 - 1;
    }
    // 盒滤波（环形索引保可平铺）：半径 P012_C_BAND_RPx → 相邻周期不规则
    const r = P012_C_BAND_RPx;
    for (let i = 0; i < n; i++) {
      let sum = 0;
      let cnt = 0;
      for (let d = -r; d <= r; d += 2) {
        sum += white[(i + d + n) % n];
        cnt++;
      }
      out[i] = sum / cnt;
    }
    // 去均值 + 峰值归一到 [-1,1]
    let mean = 0;
    for (let i = 0; i < n; i++) {
      mean += out[i];
    }
    mean /= n;
    let maxAbs = 1e-9;
    for (let i = 0; i < n; i++) {
      out[i] -= mean;
      maxAbs = Math.max(maxAbs, Math.abs(out[i]));
    }
    for (let i = 0; i < n; i++) {
      out[i] /= maxAbs;
    }
  }
  private stratNoiseAt(table: Float64Array, t: number): number {
    const n = table.length;
    const f = t * n;
    const i0 = ((Math.floor(f) % n) + n) % n;
    const i1 = (i0 + 1) % n;
    const frac = f - Math.floor(f);
    return table[i0] * (1 - frac) + table[i1] * frac;
  }
  private buildWarpBase(seed: number): void {
    const n = this.warpBase.length;              // 128
    let state = seed | 0;
    const next = (): number => {
      state = (Math.imul(state, 1664525) + 1013904223) | 0;
      return ((state >>> 8) & 0xffffff) / 0xffffff;
    };
    const white = new Float64Array(n);
    for (let i = 0; i < n; i++) {
      white[i] = next() * 2 - 1;
    }
    // 低频盒滤波（半径 8 列，环形保可平铺）
    const r = 8;
    for (let i = 0; i < n; i++) {
      let sum = 0;
      let cnt = 0;
      for (let d = -r; d <= r; d += 2) {
        sum += white[(i + d + n) % n];
        cnt++;
      }
      this.warpBase[i] = sum / cnt;
    }
    // 6 遍 3 点平滑（环形）：相邻列差 → 近线性慢变
    const tmp = new Float64Array(n);
    for (let pass = 0; pass < 6; pass++) {
      for (let i = 0; i < n; i++) {
        const left = this.warpBase[(i - 1 + n) % n];
        const right = this.warpBase[(i + 1) % n];
        tmp[i] = (left + this.warpBase[i] + right) / 3;
      }
      this.warpBase.set(tmp);
    }
    let maxAbs = 1e-9;
    for (let i = 0; i < n; i++) {
      maxAbs = Math.max(maxAbs, Math.abs(this.warpBase[i]));
    }
    for (let i = 0; i < n; i++) {
      this.warpBase[i] /= maxAbs;                // [-1,1]
    }
  }
  private warpAt(xPx: number, boxX: number, colW: number, acceptedUVPx: number): number {
    const n = this.warpBase.length;              // 128
    // 显示 x → 材料坐标（减去材料净位移）→ 列索引（环形）
    const mx = xPx - boxX - acceptedUVPx;
    const colF = mx / colW;
    const i = ((Math.floor(colF) % n) + n) % n;
    return this.warpBase[i] * P012_C_WARP_RANGE;
  }
  private buildSparkTile(seed: number): void {
    let state = seed | 0;
    const next = (): number => {
      state = (Math.imul(state, 1664525) + 1013904223) | 0;
      return ((state >>> 8) & 0xffffff) / 0xffffff;
    };
    for (let i = 0; i < this.sparkTile.length; i++) {
      this.sparkTile[i] = next() < P012_SPARK_COVERAGE
        ? 0.4 + P012_SPARK_GAIN * next()
        : 0;
    }
  }
  private clamp255(v: number): number {
    return Math.max(0, Math.min(255, Math.round(v)));
  }
}
