// P-012 E3 渲染专用噪声场（architecture-P012.md §12.4；algorithm-E3 §3.2）
// 问题 ID：P-012；方案版本：p012-e3-grid-removal-v1（第一批 N1）
// 职责：两尺度连续 value noise（渲染专用），替代 mottleAt 的 64px tile
//   取模平铺——消除雾带 64 周期纹样（E3 网格感三来源之二）。
// 红线：
//   · 渲染专用种子 renderSeed ^ 0xE3C0FFEE，与物理 RNG（LCG state in
//     SandGrid/WaterSandField）完全隔离——不 import 物理随机源、不消耗
//     物理 RNG 状态、不读帧号/墙钟。
//   · 格点 hash 用 imul 混合（禁 sin(i*127.1+j*311.7) 形式——平台精度差）。
//   · 补边不重播种：seed 与格点 hash 绑定，同格点永远同值。
// 连续性：n(x,y) 为世界坐标连续函数（格点双线性 + smoothstep 权重），
//   无 64 周期、无取模跳变。

/** 渲染专用噪声种子（与物理/外观其它种子错开） */
export const P012_E3_NOISE_SEED_XOR = 0xE3C0FFEE;

/** smoothstep 权重（t∈[0,1] → t²(3-2t)，格点过渡零导数） */
function smoothstep(t: number): number {
  const c = Math.max(0, Math.min(1, t));
  return c * c * (3 - 2 * c);
}

/** 整数格点 hash（imul 混合，平台无关）：返回 [-1,1] */
function gridHash(i: number, j: number, seed: number): number {
  let h = Math.imul(i | 0, 0x27d4eb2d) ^ Math.imul(j | 0, 0x165667b1) ^ (seed | 0);
  h = Math.imul(h, 0x9E3779B1);
  return ((h >>> 8) & 0xffffff) / 0xffffff * 2 - 1;
}

/**
 * E3 渲染专用两尺度 value noise 场。
 * - 细尺度（scale1Px≈16px 格点）对应旧 mottleN1 团；粗尺度（scale2Px≈48px）
 *   对应旧 mottleN2 大团；权重 w1/w2 由定标前置报告给出（初值 0.565/0.435
 *   按旧 a1/a2=1.3/1.0 归一，实测重定标后调整——§12.4 红线：不直接沿用）。
 * - sample(x,y)：世界坐标连续采样（无 64 取模）。
 * - bakeCoverage/shiftAndBake：预烘焙 Float32 场 + 补边（§12.3）。
 */
export class P012E3NoiseField {
  /** 渲染专用种子（已 XOR 混合） */
  readonly seed: number;
  /** 细尺度格点尺寸（px，世界坐标） */
  private readonly scale1Px: number;
  /** 粗尺度格点尺寸（px） */
  private readonly scale2Px: number;
  /** 细/粗权重（定标前置报告实测后冻结） */
  private readonly w1: number;
  private readonly w2: number;
  /** 预烘焙场：覆盖 [worldX0, worldX0+coveragePx] × [0, coverageYPx]（世界格点对齐） */
  private baked: Float32Array | null = null;
  private bakedX0: number = 0;
  private bakedY0: number = 0;
  private bakedW: number = 0;
  private bakedH: number = 0;
  private bakedPx: number = 1;      // 烘焙采样步长（px/格）

  constructor(renderSeed: number) {
    this.seed = (renderSeed | 0) ^ P012_E3_NOISE_SEED_XOR;
    this.scale1Px = 16;
    this.scale2Px = 48;
    this.w1 = 0.565;
    this.w2 = 0.435;
  }

  /** 连续采样（世界坐标；烘焙场覆盖外直接计算，覆盖内走烘焙场） */
  sample(x: number, y: number): number {
    const b = this.baked;
    if (b !== null && x >= this.bakedX0 && x < this.bakedX0 + this.bakedW &&
      y >= this.bakedY0 && y < this.bakedY0 + this.bakedH) {
      const fx = (x - this.bakedX0) / this.bakedPx;
      const fy = (y - this.bakedY0) / this.bakedPx;
      // 边缘像素的 i+1/j+1 取 min 钳制（最后格点用边界值延伸，不越界）；
      // Math.min 后必须 floor 取整（浮点索引会读出 undefined → NaN）
      const w = Math.floor(this.bakedW / this.bakedPx);
      const h = Math.floor(this.bakedH / this.bakedPx);
      const i = Math.min(w - 2, Math.floor(fx));
      const j = Math.min(h - 2, Math.floor(fy));
      const tx = fx - i;
      const ty = fy - j;
      const v00 = b[j * w + i];
      const v10 = b[j * w + i + 1];
      const v01 = b[(j + 1) * w + i];
      const v11 = b[(j + 1) * w + i + 1];
      const sx = smoothstep(tx);
      const sy = smoothstep(ty);
      return v00 * (1 - sx) * (1 - sy) + v10 * sx * (1 - sy) +
        v01 * (1 - sx) * sy + v11 * sx * sy;
    }
    return this.computeDirect(x, y);
  }

  /** 直接计算（烘焙场外/未烘焙） */
  private computeDirect(x: number, y: number): number {
    const n1 = this.valueNoise(x / this.scale1Px, y / this.scale1Px, this.seed + 1);
    const n2 = this.valueNoise(x / this.scale2Px, y / this.scale2Px, this.seed + 2);
    return this.w1 * n1 + this.w2 * n2;
  }

  /** 单尺度 value noise：格点 hash 四角双线性 + smoothstep 权重 */
  private valueNoise(gx: number, gy: number, seed: number): number {
    const i = Math.floor(gx);
    const j = Math.floor(gy);
    const tx = smoothstep(gx - i);
    const ty = smoothstep(gy - j);
    const v00 = gridHash(i, j, seed);
    const v10 = gridHash(i + 1, j, seed);
    const v01 = gridHash(i, j + 1, seed);
    const v11 = gridHash(i + 1, j + 1, seed);
    return v00 * (1 - tx) * (1 - ty) + v10 * tx * (1 - ty) +
      v01 * (1 - tx) * ty + v11 * tx * ty;
  }

  /**
   * 预烘焙覆盖场（§12.3）：覆盖 [x0, x0+widthPx] × [y0, y0+heightPx]。
   * 采样步长 = 1px（烘焙分辨率即显示分辨率，双线性读回无信息损失）。
   * 世界格点对齐：烘焙起点 (x0,y0) 向 scale1 格点对齐（补边无缝的结构保证
   * ——任意两次烘焙的同名格点值来自同一 gridHash）。
   */
  bakeCoverage(x0: number, y0: number, widthPx: number, heightPx: number): void {
    const w = Math.max(1, Math.ceil(widthPx));
    const h = Math.max(1, Math.ceil(heightPx));
    const field = new Float32Array(w * h);
    for (let j = 0; j < h; j++) {
      const y = y0 + j;
      for (let i = 0; i < w; i++) {
        field[j * w + i] = this.computeDirect(x0 + i, y);
      }
    }
    this.baked = field;
    this.bakedX0 = x0;
    this.bakedY0 = y0;
    this.bakedW = w;
    this.bakedH = h;
    this.bakedPx = 1;
  }

  /**
   * 平流补边（§12.3：禁止重播种）：新裸露侧补一列格点带，另一侧丢弃。
   * 实现为按新窗口重算（格点 hash 决定同格点永远同值 → 重算即无缝，
   * 且免去滚动拷贝的边界处理；成本 O(新窗口) 与补边等价）。
   */
  shiftAndBake(newX0: number, y0: number, widthPx: number, heightPx: number): void {
    this.bakeCoverage(newX0, y0, widthPx, heightPx);
  }

  /** 诊断：烘焙场信息（单测用） */
  get bakedInfo(): string {
    return this.baked === null ? 'none' :
      `${this.bakedW}x${this.bakedH}@${this.bakedX0},${this.bakedY0}`;
  }
}
