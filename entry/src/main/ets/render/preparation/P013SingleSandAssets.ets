const P012_GRAIN_TILE=256;
const P012_B_ENV_AMP=.4;
const P012_B_GRAIN_CLIP=2.5;
export const SINGLE_GENERATOR_HASH:string='fb09fe156489f7373d3b2837fec2d48909b24b055ca8440bcb6271fa01889cb3';
export function singleSandRawGrain(seed: number, finePx: number): Float64Array {
    const size = P012_GRAIN_TILE;
    const fineA = new Float64Array(size * size);
    const fineB = new Float64Array(size * size);
    const env = new Float64Array(size * size);
    const envPx = 2 * finePx;           // envelope = 2×fine（候选稿 §2）
    buildGrainNoise(fineA, (seed | 0) + 11, finePx);
    buildGrainNoise(fineB, (seed | 0) + 23, finePx);
    buildGrainNoise(env, (seed | 0) + 37, envPx);
    const out = new Float64Array(size * size);
    let mean = 0;
    for (let i = 0; i < out.length; i++) {
      const g = 0.5 * fineA[i] + 0.5 * fineB[i];
      const e = 1 + P012_B_ENV_AMP * env[i];
      out[i] = g * e;
      mean += out[i];
    }
    mean /= out.length;
    let varSum = 0;
    for (let i = 0; i < out.length; i++) {
      varSum += (out[i] - mean) * (out[i] - mean);
    }
    const inv = 1 / Math.max(1e-9, Math.sqrt(varSum / out.length));
    let mean3 = 0;
    for (let i = 0; i < out.length; i++) {
      out[i] = (out[i] - mean) * inv;
      mean3 += out[i];
    }
    mean3 /= out.length;
    for (let i = 0; i < out.length; i++) {
      out[i] -= mean3;
    }
    for (let i = 0; i < out.length; i++) {
      out[i] = Math.max(-P012_B_GRAIN_CLIP, Math.min(P012_B_GRAIN_CLIP, out[i]));
    }
    return out;
  }
function buildGrainNoise(out: Float64Array, seed: number, featurePx: number): void {
    const size = Math.max(1, Math.round(Math.sqrt(out.length)));
    let state = seed | 0;
    const next = (): number => {
      state = (Math.imul(state, 1664525) + 1013904223) | 0;
      return ((state >>> 8) & 0xffffff) / 0xffffff;
    };
    const white: Float64Array = new Float64Array(size * size);
    for (let i = 0; i < white.length; i++) {
      white[i] = next() * 2 - 1;
    }
    // 真盒滤波（步长 1；环形索引保可平铺）
    const r = Math.max(1, Math.round(featurePx / 2));
    for (let y = 0; y < size; y++) {
      for (let x = 0; x < size; x++) {
        let sum = 0;
        let cnt = 0;
        for (let dy = -r; dy <= r; dy++) {
          for (let dx = -r; dx <= r; dx++) {
            const yy = (y + dy + size) % size;
            const xx = (x + dx + size) % size;
            sum += white[yy * size + xx];
            cnt++;
          }
        }
        out[y * size + x] = sum / cnt;
      }
    }
    // 单位方差 + 零均值
    let mean = 0;
    for (let i = 0; i < out.length; i++) {
      mean += out[i];
    }
    mean /= out.length;
    let varSum = 0;
    for (let i = 0; i < out.length; i++) {
      varSum += (out[i] - mean) * (out[i] - mean);
    }
    const inv = 1 / Math.max(1e-9, Math.sqrt(varSum / out.length));
    for (let i = 0; i < out.length; i++) {
      out[i] = (out[i] - mean) * inv;
    }
    // 截幅到有限范围（§5.1）。顺序：先去均值后截幅——反序会让边界值
    // 被减负均值推出截幅之外（2.5+|mean|）。截幅后不再方差缩放。
    let mean2 = 0;
    for (let i = 0; i < out.length; i++) {
      mean2 += out[i];
    }
    mean2 /= out.length;
    for (let i = 0; i < out.length; i++) {
      out[i] -= mean2;
    }
    for (let i = 0; i < out.length; i++) {
      out[i] = Math.max(-P012_B_GRAIN_CLIP, Math.min(P012_B_GRAIN_CLIP, out[i]));
    }
  }
export function buildSingleSandGrain(seed:number,feature:number):Uint8Array {
  if(seed!==20261022||(feature!==2&&feature!==3))throw new Error('SINGLE_CONFIG_INVALID');
  const raw=singleSandRawGrain(seed,feature);
  const tile=new Uint8Array(raw.length);
  for(let i=0;i<tile.length;i++){
    const n=raw[i]/2.5;
    tile[i]=Math.floor(128+(n<0?128:127)*n+.5);
  }
  return tile;
}
