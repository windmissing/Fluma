// P-012 表现层渲染器——E 档：光照与色调映射（algorithm-P012-render-v1.md §3）
// 问题 ID：P-012；方案版本：p012-render-material-v1（E-batch）
// 职责：帧绘制编排（E 色调/伪法向光照/液面高光/亮粒 tile），拥有 adapter。
// 红线：只读物理场；一次帧只选一个完整渲染分支；不进入物理 checker。
// 预算：E 档 draw ≤16（P012_E_BATCH_DRAW_BUDGET）；CPU 目标 ≤0.25ms/帧（待真机验证）。
import { WaterSandSnapshot, WaterSandField } from '../physics/WaterSandField';
import { P012WaterSandAdapter, P012MaterialFrameView } from './P012WaterSandAdapter';
import {
  P012FrameContext, P012AppearanceTier, P012_APPEARANCE_ROUTE_ID,
  P012_E_BATCH_DRAW_BUDGET
} from './P012RenderTypes';

/** 预计算材质色阶（设计稿 §3.1：重沙浅黄绿 / 轻沙暖浅 / 水体近薄荷白） */
interface P012Palette {
  /** 重沙基色（低亮度端） */
  heavyR: number; heavyG: number; heavyB: number;
  /** 重沙高光端（伪法向光照最强方向） */
  heavyHiR: number; heavyHiG: number; heavyHiB: number;
  /** 轻沙基色 → 高光端 */
  lightR: number; lightG: number; lightB: number;
  lightHiR: number; lightHiG: number; lightHiB: number;
  /** 水体基色（近薄荷白）→ 深水端 */
  waterR: number; waterG: number; waterB: number;
  waterDeepR: number; waterDeepG: number; waterDeepB: number;
  /** 混合层端点（fH=0 → 轻沙色，fH=1 → 重沙色，运行时插值） */
}

/** E 档调色板（数值来自设计稿 §3.1 effector 目标 ROI 候选基色） */
function p012DefaultPalette(): P012Palette {
  return {
    // 重沙：浅黄绿 (145,168,111) → 高光 (188,205,158)
    heavyR: 145, heavyG: 168, heavyB: 111,
    heavyHiR: 188, heavyHiG: 205, heavyHiB: 158,
    // 轻沙：暖浅 (216,196,150) → 高光 (238,226,192)
    lightR: 216, lightG: 196, lightB: 150,
    lightHiR: 238, lightHiG: 226, lightHiB: 192,
    // 水体：近薄荷白 (192,202,196) → 深水端 (168,186,182)
    waterR: 192, waterG: 202, waterB: 196,
    waterDeepR: 168, waterDeepG: 186, waterDeepB: 182
  };
}

/** 伪法向光照参数（§3.1：L = La + Ld·max(dot(n,l),0)，La=0.85 Ld=0.15） */
const P012_AMBIENT = 0.85;
const P012_DIFFUSE = 0.15;
/** 固定柔和左上主光（屏幕空间，n=(-s,0,1) 归一化后与 l 点积） */
const P012_LIGHT_X = -0.55;
const P012_LIGHT_Y = -0.55;
const P012_LIGHT_Z = 0.63;

/** tone mapping 压缩函数 T(x)=x/(1+x) 的反函数预查找：直接用曝光参数近似——
 * 实际热路径用预计算 LUT：shade ∈ [0,255] → 显示亮度（含高光肩部，无 127 钳位） */
const P012_LUT_STEPS = 64;

/** 亮粒 tile：确定性 LCG 种子生成 64×64 的 1~2px 柔亮粒（§3.1 稀疏亮粒） */
const P012_SPARK_TILE = 64;
const P012_SPARK_COVERAGE = 0.012;   // 0.5%~2% 取中值
const P012_SPARK_GAIN = 0.5;         // 亮粒提亮幅度（<=T 肩部内）

/** renderer 对外生命周期接口（architecture §1 模块表） */
export class P012WaterSandRenderer {
  private readonly adapter: P012WaterSandAdapter;
  private readonly palette: P012Palette = p012DefaultPalette();
  /** 显示 mask 用的平滑 profile（承接既有平滑，绝不覆盖 mass/raw 边界） */
  private readonly smoothWaterTop: Float64Array;
  private readonly smoothSandTop: Float64Array;
  private readonly smoothBh: Float64Array;
  private readonly smoothMixStart: Float64Array;
  private readonly scratch: Float64Array;
  /** E 档 shade→显示亮度 LUT（64 档，[0,1]） */
  private readonly shadeLut: Float64Array = new Float64Array(P012_LUT_STEPS + 1);
  /** 亮粒 tile（沙面稀疏亮粒；64×64，值域 [0,1]，0=无颗粒） */
  private readonly sparkTile: Float64Array = new Float64Array(P012_SPARK_TILE * P012_SPARK_TILE);
  private readonly cols: number;
  private readonly capacity: number;
  /** 最近一次 draw 提交计数（性能预检只读钩子；不进遥测） */
  lastDrawCount: number = 0;
  /** 最近一次 adapter 配对结论（只读诊断） */
  lastPairValid: boolean = false;
  private disposed: boolean = false;

  constructor(cols: number, capacity: number) {
    this.cols = cols;
    this.capacity = capacity;
    this.adapter = new P012WaterSandAdapter(cols, capacity);
    this.smoothWaterTop = new Float64Array(cols);
    this.smoothSandTop = new Float64Array(cols);
    this.smoothBh = new Float64Array(cols);
    this.smoothMixStart = new Float64Array(cols);
    this.scratch = new Float64Array(cols);
    this.buildShadeLut();
    this.buildSparkTile(20261020);
  }

  /** 外观路线身份（独立视觉/性能证据 manifest 用） */
  get appearanceRouteId(): string {
    return P012_APPEARANCE_ROUTE_ID;
  }

  /**
   * 帧入口（§2）：同步读取页面刚完成 copyStateTo 的完整快照。
   * OFF/canonicalEvidence 由页面薄分发决定，不会进入本方法。
   */
  render(ctx: CanvasRenderingContext2D, snapshot: WaterSandSnapshot,
    frameContext: P012FrameContext): void {
    if (this.disposed) {
      return;
    }
    let drawCount = 0;
    const result = this.adapter.update(snapshot, frameContext);
    this.lastPairValid = result.pair.valid;
    const view = result.curr;
    if (view === null) {
      // curr 非有限：不替换合法 prev；保留最后合法显示 = 不绘制（页面背景已清）
      this.lastDrawCount = 0;
      return;
    }
    // ── 显示 mask：承接既有平滑（4 遍 3 点平均），只用于显示几何 ──
    this.buildDisplayProfiles(view);
    const n = this.cols;
    const boxX = frameContext.boxX;
    const boxY = frameContext.boxY;
    const boxW = frameContext.boxW;
    const boxH = frameContext.boxH;
    const colW = boxW / n;
    const heightToY = (h: number): number =>
      boxY + (this.capacity - Math.max(0, Math.min(this.capacity, h))) * boxH / this.capacity;
    ctx.save();
    if (frameContext.visualInverted) {
      const cx = boxX + boxW / 2;
      const cy = boxY + boxH / 2;
      ctx.translate(cx, cy);
      ctx.rotate(Math.PI);
      ctx.translate(-cx, -cy);
    }
    // 1) 水体带（waterTop→sandTop）：基色→深水端线性渐变 + 深度调制（1 draw）
    const gradWater = ctx.createLinearGradient(0, boxY, 0, boxY + boxH);
    gradWater.addColorStop(0, this.rgb(this.palette.waterR, this.palette.waterG, this.palette.waterB));
    gradWater.addColorStop(1, this.rgb(this.palette.waterDeepR, this.palette.waterDeepG, this.palette.waterDeepB));
    ctx.fillStyle = gradWater;
    ctx.beginPath();
    ctx.moveTo(boxX, heightToY(this.smoothWaterTop[0]));
    for (let c = 1; c < n; c++) {
      ctx.lineTo(boxX + c * colW, heightToY(this.smoothWaterTop[c]));
    }
    for (let c = n - 1; c >= 0; c--) {
      ctx.lineTo(boxX + (c + 1) * colW, heightToY(this.smoothSandTop[c]));
    }
    ctx.closePath();
    ctx.fill();
    drawCount++;
    // 2) 沙体三带：重沙带（bh 以下）/ 轻沙带（mixStart→bh）/ 混合带（sandTop→mixStart）
    //    逐列伪法向光照：s = d(sandTop)/dx（屏幕单位），L = La + Ld·max(dot(n,l),0)
    //    用 8 档游程近似逐列光照（run 合并相邻同档列，draw 计数受控）
    drawCount += this.drawSandColumns(ctx, view, heightToY, colW, boxX, boxY, boxH);
    // 3) 液面高光：沿 waterTop 的宽软带（§3.1 柔和高光，alpha≤0.12）（1 draw）
    ctx.beginPath();
    ctx.moveTo(boxX, heightToY(this.smoothWaterTop[0]));
    for (let c = 1; c < n; c++) {
      ctx.lineTo(boxX + c * colW, heightToY(this.smoothWaterTop[c]));
    }
    ctx.strokeStyle = 'rgba(255,255,255,0.12)';
    ctx.lineWidth = 5;
    ctx.stroke();
    drawCount++;
    // 4) 容器边框（与原渲染一致的白框）
    ctx.strokeStyle = 'rgba(255,255,255,0.35)';
    ctx.lineWidth = 3;
    ctx.strokeRect(boxX, boxY, boxW, boxH);
    drawCount++;
    ctx.restore();
    this.lastDrawCount = drawCount;
  }

  /** 生命周期：清连续性配对（reset/inject/invert/后台恢复时页面通知） */
  resetContinuity(): void {
    this.adapter.dispose();
  }

  /** dispose：断开全部引用（aboutToDisappear 调用；幂等） */
  dispose(): void {
    if (this.disposed) {
      return;
    }
    this.disposed = true;
    this.adapter.dispose();
  }

  get isDisposed(): boolean {
    return this.disposed;
  }

  /** E-batch draw 预算断言（单测钩子：≤16 次提交） */
  withinDrawBudget(): boolean {
    return this.lastDrawCount <= P012_E_BATCH_DRAW_BUDGET;
  }

  // ───────────────────────── private ─────────────────────────

  /** shade LUT：T(x)=x/(1+x) 压缩 + sRGB 编码近似（去 127 钳位，高光到 ~240） */
  private buildShadeLut(): void {
    for (let i = 0; i <= P012_LUT_STEPS; i++) {
      const x = i / P012_LUT_STEPS;
      // 曝光斜率 2.2 → 压缩后线性值；再近似 sRGB gamma（1/2.2）
      const tone = x * 2.2 / (1 + x * 2.2);
      this.shadeLut[i] = Math.pow(tone, 1 / 2.2);
    }
  }

  /** 确定性亮粒 tile：LCG 种子 20261020（不调用/消耗物理随机源） */
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

  /** 显示 profile：4 遍 3 点移动平均（与原 renderWaterSand 完全一致的平滑） */
  private buildDisplayProfiles(view: P012MaterialFrameView): void {
    const n = this.cols;
    for (let c = 0; c < n; c++) {
      this.smoothBh[c] = view.bhAt(c);
      this.smoothMixStart[c] = view.bhAt(c) + view.blAt(c);
      this.smoothSandTop[c] = view.sandTopAt(c);
      this.smoothWaterTop[c] = view.waterTopAt(c);
    }
    const passes: Float64Array[] = [this.smoothBh, this.smoothMixStart,
      this.smoothSandTop, this.smoothWaterTop];
    for (let p = 0; p < 4; p++) {
      for (let k = 0; k < passes.length; k++) {
        const src = passes[k];
        for (let c = 0; c < n; c++) {
          const left = src[Math.max(0, c - 1)];
          const right = src[Math.min(n - 1, c + 1)];
          this.scratch[c] = (left + src[c] + right) / 3;
        }
        src.set(this.scratch);
      }
    }
  }

  /**
   * 沙体逐列绘制：重沙带 / 轻沙带 / 混合带，逐列伪法向光照 + 游程合并。
   * 返回 draw 提交次数（run 数，受列数与光照档数约束；典型 ≤ 3×8≈10）。
   */
  private drawSandColumns(ctx: CanvasRenderingContext2D, view: P012MaterialFrameView,
    heightToY: (h: number) => number, colW: number,
    boxX: number, boxY: number, boxH: number): number {
    const n = this.cols;
    const colPx = colW;                       // 每列屏幕宽（px）
    const bottomY = boxY + boxH;
    let draws = 0;
    // 逐列光照档（0..7）：s = (S[c+1]-S[c-1])/(2·colPx) 屏幕单位坡度
    const shadeOf = (c: number): number => {
      const left = this.smoothSandTop[Math.max(0, c - 1)];
      const right = this.smoothSandTop[Math.min(n - 1, c + 1)];
      const s = (right - left) / (2 * colPx * (boxH / this.capacity));
      // 伪法向 n=normalize(-s,0,1) 与左上主光点积
      const inv = 1 / Math.sqrt(s * s + 1);
      const dot = (-s * inv) * P012_LIGHT_X + (inv) * P012_LIGHT_Z;
      const lit = P012_AMBIENT + P012_DIFFUSE * Math.max(0, dot);
      const idx = Math.max(0, Math.min(P012_LUT_STEPS,
        Math.round(lit * P012_LUT_STEPS)));
      return this.shadeLut[idx];
    };
    // 三带逐列填色（带内游程合并同 shade 档）：
    // 重沙带（bhY→bottomY）、轻沙带（mixStartY→bhY）、混合带（sandTopY→mixStartY）
    const bands: P012SandBand[] = [];
    const heavyBand = new P012SandBand();
    heavyBand.top = (c: number): number => heightToY(this.smoothBh[c]);
    heavyBand.bottom = (_c: number): number => bottomY;
    heavyBand.kind = 0;
    bands.push(heavyBand);
    const lightBand = new P012SandBand();
    lightBand.top = (c: number): number => heightToY(this.smoothMixStart[c]);
    lightBand.bottom = (c: number): number => heightToY(this.smoothBh[c]);
    lightBand.kind = 1;
    bands.push(lightBand);
    const mixBand = new P012SandBand();
    mixBand.top = (c: number): number => heightToY(this.smoothSandTop[c]);
    mixBand.bottom = (c: number): number => heightToY(this.smoothMixStart[c]);
    mixBand.kind = 2;
    bands.push(mixBand);
    for (let b = 0; b < bands.length; b++) {
      const band = bands[b];
      let runStart = -1;
      let runShade = -1;
      for (let c = 0; c <= n; c++) {
        const shade = c < n ? shadeOf(c) : -1;
        const sameRun = runStart >= 0 && c < n &&
          Math.abs(shade - runShade) < 1 / P012_LUT_STEPS;
        if (sameRun) {
          continue;
        }
        if (runStart >= 0) {
          const c0 = runStart;
          const c1 = c;
          const mixH = view.mhAt(c0) + view.mlAt(c0);
          const fH = mixH > 0 ? view.mhAt(c0) / mixH : 0.5;
          ctx.fillStyle = this.bandColor(band.kind, fH, runShade, c0);
          const topY = band.top(c0);
          const botY = band.bottom(c1 - 1);
          ctx.fillRect(boxX + c0 * colW, topY, (c1 - c0) * colW, Math.max(0, botY - topY));
          draws++;
        }
        runStart = c < n ? c : -1;
        runShade = shade;
      }
    }
    // 亮粒 tile 叠加（稀疏柔亮粒；作用于已提亮材质，clip 在沙区 Path 内）。
    // draw 预算控制：不逐 tile 块 fillRect（最坏可到数十次），改为 3 个固定
    // alpha 档 → 每档把命中 tile 合并为一个 Path 一次 fill（合计 ≤3 draw + clip 1）。
    ctx.save();
    ctx.beginPath();
    ctx.moveTo(boxX, heightToY(this.smoothSandTop[0]));
    for (let c = 1; c < n; c++) {
      ctx.lineTo(boxX + c * colW, heightToY(this.smoothSandTop[c]));
    }
    ctx.lineTo(boxX + n * colW, bottomY);
    ctx.lineTo(boxX, bottomY);
    ctx.closePath();
    ctx.clip();
    const tilePx = 8;   // 1~2px 细粒在 64 tile 内 → tile 屏幕 8px
    const sandLeft = boxY + boxH * 0.25;   // 沙区大致起始 y（粗裁剪减少 Path 节点）
    const rows = Math.ceil((bottomY - sandLeft) / tilePx);
    const colsT = Math.ceil(n * colW / tilePx);
    // 3 个 alpha 档的矩形收集
    const tiers: number[][] = [[], [], []];
    for (let ty = 0; ty < rows; ty++) {
      for (let tx = 0; tx < colsT; tx++) {
        const v = this.sparkTile[
          ((ty % P012_SPARK_TILE) * P012_SPARK_TILE + (tx % P012_SPARK_TILE))];
        if (v <= 0) {
          continue;
        }
        const tierIdx = v < 0.6 ? 0 : (v < 0.8 ? 1 : 2);
        tiers[tierIdx].push(boxX + tx * tilePx, sandLeft + ty * tilePx);
      }
    }
    const tierAlpha: number[] = [0.10, 0.18, 0.28];
    for (let t = 0; t < 3; t++) {
      if (tiers[t].length === 0) {
        continue;
      }
      ctx.beginPath();
      for (let i = 0; i < tiers[t].length; i += 2) {
        ctx.rect(tiers[t][i], tiers[t][i + 1], tilePx, tilePx);
      }
      ctx.fillStyle = `rgba(255,252,240,${tierAlpha[t]})`;
      ctx.fill();
      draws++;
    }
    ctx.restore();
    return draws;
  }

  /** 带基色：kind 0=重沙 1=轻沙 2=混合（fH 连续混合，§3.1 不把两相涂成同色） */
  private bandColor(kind: number, fH: number, shade: number, col: number): string {
    const p = this.palette;
    let r: number = 0;
    let g: number = 0;
    let b: number = 0;
    if (kind === 0) {
      r = p.heavyR + (p.heavyHiR - p.heavyR) * shade;
      g = p.heavyG + (p.heavyHiG - p.heavyG) * shade;
      b = p.heavyB + (p.heavyHiB - p.heavyB) * shade;
    } else if (kind === 1) {
      r = p.lightR + (p.lightHiR - p.lightR) * shade;
      g = p.lightG + (p.lightHiG - p.lightG) * shade;
      b = p.lightB + (p.lightHiB - p.lightB) * shade;
    } else {
      // 混合带：轻沙色 → 重沙色按 fH 连续混合，再乘光照
      const lr = p.lightR + (p.heavyR - p.lightR) * fH;
      const lg = p.lightG + (p.heavyG - p.lightG) * fH;
      const lb = p.lightB + (p.heavyB - p.lightB) * fH;
      const hr = p.lightHiR + (p.heavyHiR - p.lightHiR) * fH;
      const hg = p.lightHiG + (p.heavyHiG - p.lightHiG) * fH;
      const hb = p.lightHiB + (p.heavyHiB - p.lightHiB) * fH;
      r = lr + (hr - lr) * shade;
      g = lg + (hg - lg) * shade;
      b = lb + (hb - lb) * shade;
    }
    // 亮粒采样（列级相位：列 × 每列 1 采样 → 低频，高频由 tile 平铺补）
    const spark = this.sparkTile[(col % P012_SPARK_TILE) * P012_SPARK_TILE +
      (col % P012_SPARK_TILE)];
    const gain = spark > 0 ? 1 + P012_SPARK_GAIN * spark * 0.3 : 1;
    r = Math.min(248, r * gain);
    g = Math.min(248, g * gain);
    b = Math.min(248, b * gain);
    return this.rgb(r, g, b);
  }

  private rgb(r: number, g: number, b: number): string {
    return `rgb(${Math.round(Math.max(0, Math.min(255, r)))},` +
      `${Math.round(Math.max(0, Math.min(255, g)))},` +
      `${Math.round(Math.max(0, Math.min(255, b)))})`;
  }
}

/** 纯函数：OFF 分支决策（页面薄分发用，可单测；不访问 renderer 状态） */
export function p012ShouldUseP012Renderer(renderer: P012WaterSandRenderer | null,
  appearanceTier: P012AppearanceTier, canonicalEvidence: boolean): boolean {
  if (renderer === null || renderer.isDisposed) {
    return false;
  }
  if (canonicalEvidence) {
    return false;   // 物理证据采集期间强制 OFF（§9.2 约束③）
  }
  return appearanceTier !== P012AppearanceTier.OFF;
}

/** 沙体单带描述（ArkTS 显式类：禁止匿名对象字面量；带边界高度→屏幕 y 映射） */
class P012SandBand {
  top: (c: number) => number = (c: number): number => c;
  bottom: (c: number) => number = (c: number): number => c;
  kind: number = 0;
}
