// P-012 表现层渲染器——E 档：光照与色调映射 + A 档：云雾/界面光学过渡
// 问题 ID：P-012；方案版本：p012-render-material-v1（E-batch → EA-batch）
// 职责：帧绘制编排（E 色调/伪法向光照/液面高光/亮粒 tile；A 界面散射带/
//   两尺度斑驳/水体雾），拥有 adapter。
// 红线：只读物理场；一次帧只选一个完整渲染分支；不进入物理 checker。
// 预算：E 档 draw ≤16；A 档新增 ≤48（EA 合计 ≤64）；CPU 0.25+0.35ms（待真机验证）。
import { WaterSandSnapshot, WaterSandField } from '../physics/WaterSandField';
import { P012WaterSandAdapter, P012MaterialFrameView } from './P012WaterSandAdapter';
import { P012MaterialMotionCache } from './P012MaterialMotionCache';
import hilog from '@ohos.hilog';
import {
  P012FrameContext, P012AppearanceTier, P012_APPEARANCE_ROUTE_ID,
  P012_E_BATCH_DRAW_BUDGET, P012_A_BATCH_DRAW_BUDGET,
  P012_B_BATCH_DRAW_BUDGET, P012_FIRST_TIER_DRAW_BUDGET
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

/** ── A 档云雾参数（算法稿 §4）── */
/** 界面过渡全宽 wFull：10-90% 宽 = 0.6084·wFull，目标 24~48px → wFull≈40~79 */
const P012_A_WFULL_PX = 56;
/** smoothstep 邻近系数：0.6084（10-90% / wFull） */
const P012_A_SMOOTHSTEP_1090 = 0.6084;
/** 两尺度 tile 尺寸（px）：细团 12~24 / 大团 40~80 → tile 用其公约尺度采样 */
const P012_A_TILE = 64;              // tile 逻辑尺寸（一个周期）
/** 斑驳调制幅度：mottle = 1 + a1·n1 + a2·n2（n1 细团/n2 大团，零均值归一化）。
 *  标定依据（test/calib_p012_tau_static.cjs）：a1/a2 = 1.3/1.0 使 mottle
 *  std≈0.35；经 stretch 线性拉伸 + 加性调制后水体 std 达 15+。 */
const P012_A_MOTTLE_A1 = 1.3;
const P012_A_MOTTLE_A2 = 1.0;
/** tauOptical 峰值（界面附近有限支撑的材质散射，非永久悬沙） */
const P012_A_TAU_INTERFACE = 1.35;
/** 悬浮浓度→水体雾 tau 系数（qH/qL clamp [0,1]） */
const P012_A_KH = 0.9;
const P012_A_KL = 0.7;
/** 雾最大 alpha：水体雾（静止斑驳主体）0.78 / 界面带 0.55（设计稿 §4.2 候选值）。
 *  标定依据（test/calib_p012_tau_static.cjs）：alpha 上限 0.55 时水体 std 天花板
 *  ≈10.8（range 30.8 × 钟形形状因子 0.35），无法达到 P4 锚点 std>15；
 *  0.78 时实测仿真 std=15.3（参考帧 20~42 的下界）。 */
const P012_A_FOG_ALPHA_MAX = 0.78;
/** 界面带雾 alpha 上限（设计候选值 0.55：界面是散射过渡不是浓雾遮盖） */
const P012_A_BAND_ALPHA_MAX = 0.55;
/** 静止水介质静态斑驳（q=0 时的低对比光照斑驳，§4.2"清水静态明暗"）：
 *  tauStatic = TAU_STATIC_BASE + TAU_STATIC_AMP·(stretch(mottle)-1)
 *  stretch=2.2 线性拉伸（clamp [0.2,2.0]）拉开 value-noise 钟形分布。 */
const P012_A_TAU_STATIC_BASE = 0.55;
const P012_A_TAU_STATIC_AMP = 1.8;
const P012_A_MOTTLE_STRETCH = 2.2;
/** A 档 alpha 档数（固定档合并 Path 控 draw；量化步 0.78/12≈0.065） */
const P012_A_ALPHA_TIERS = 12;
/** 云雾带绘制分组：16 组（每组 8 列，§4.3 共用缓存噪声裁片） */
const P012_A_GROUPS = 16;

/** ── B 档颗粒参数（算法稿 §5.1）── */
/** 颗粒 tile 尺寸（px，可平铺）；64×64 spark 之外另立 256² 主颗粒 tile */
const P012_GRAIN_TILE = 256;
/** 颗粒亮度偏移幅度（sigmaG，0-255 量级；设计稿候选 18~22）。
 *  语义：nG 为单位方差零均值噪声（截幅 ±P012_B_GRAIN_CLIP），
 *  故 Ygrain = Ymaterial + sigmaG·nG 的实际标准差就是 sigmaG 量级。 */
const P012_B_SIGMA = 20;
/** 颗粒叠加强度（§5.1 加性模型 Y=Ymaterial+sigmaG·nG 的等效缩放：
 *  alpha 复合 = base + alpha·delta → 等效 sigmaG = alpha×20 = 15 级，
 *  沙体 lumStd 锚点 ≥15（冻结附录 §4）；位图纯不透明，强度走 globalAlpha）。
 *  注：s94 消融证明把 alpha 烘焙进位图像素会被 pattern 采样按 1/alpha
 *  放大并钳位（0.55 → ×1.82 饱和白），故强度一律走 globalAlpha。 */
const P012_B_GRAIN_ALPHA = 0.75;
/** 细粒特征尺度（px，1~2 为主；盒滤波半径 = ceil(px/2)） */
const P012_B_FINE_PX = 2;
/** 团簇 envelope 特征尺度（px，3~5 弱团） */
const P012_B_ENV_PX = 4;
/** 团簇 envelope 调制幅度（弱团：out = g·(1 + amp·env)） */
const P012_B_ENV_AMP = 0.4;
/** 单位方差归一化后的截幅（有限范围，§5.1"被截到有限范围"） */
const P012_B_GRAIN_CLIP = 2.5;

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
  /** ── A 档资源（EA-batch）── */
  /** 两尺度零均值噪声 tile：n1 细团（12~24px）/ n2 大团（40~80px），值域约 [-1,1] */
  private readonly mottleN1: Float64Array = new Float64Array(P012_A_TILE * P012_A_TILE);
  private readonly mottleN2: Float64Array = new Float64Array(P012_A_TILE * P012_A_TILE);
  /** alphaFog LUT：tau∈[0,4] → 1-exp(-tau)（§4.2 预计算） */
  private readonly fogLut: Float64Array = new Float64Array(65);
  /** A 档帧间相位（静止不动；可信运动估计属 EAB-batch 运动代理） */
  private aPhaseX: number = 0;
  /** ── B 档资源（EAB-batch）：3 张 256² 颗粒 tile + 对应 ImageBitmap（on-device）── */
  /** 零均值归一化颗粒亮度偏移 tile（重/轻/混三套种子/对比度）；值域约 [-1,1] */
  private readonly grainTileH: Float64Array = new Float64Array(P012_GRAIN_TILE * P012_GRAIN_TILE);
  private readonly grainTileL: Float64Array = new Float64Array(P012_GRAIN_TILE * P012_GRAIN_TILE);
  private readonly grainTileM: Float64Array = new Float64Array(P012_GRAIN_TILE * P012_GRAIN_TILE);
  /** 预着色颗粒 ImageBitmap（on-device 经 OffscreenCanvas 生成；Node/canvas 缺失为 null） */
  private grainBitmapH: ImageBitmap | null = null;
  private grainBitmapL: ImageBitmap | null = null;
  private grainBitmapM: ImageBitmap | null = null;
  /** ── 运动代理（D-B2：相净搬运位移代理 + 连续 UV 平流）── */
  private readonly motionCache: P012MaterialMotionCache;
  private readonly cols: number;
  private readonly capacity: number;
  /** 最近一次 draw 提交计数（性能预检只读钩子；不进遥测） */
  lastDrawCount: number = 0;
  /** 最近一次 adapter 配对结论（只读诊断） */
  lastPairValid: boolean = false;
  private disposed: boolean = false;
  /** 一次性诊断日志闸门（避免逐帧刷日志） */
  private grainDiagLogged: boolean = false;

  constructor(cols: number, capacity: number) {
    this.cols = cols;
    this.capacity = capacity;
    this.adapter = new P012WaterSandAdapter(cols, capacity);
    this.smoothWaterTop = new Float64Array(cols);
    this.smoothSandTop = new Float64Array(cols);
    this.smoothBh = new Float64Array(cols);
    this.smoothMixStart = new Float64Array(cols);
    this.scratch = new Float64Array(cols);
    this.motionCache = new P012MaterialMotionCache(cols);
    this.buildShadeLut();
    this.buildSparkTile(20261020);
    this.buildMottleTiles(20261021);
    this.buildFogLut();
    this.grainTileH = this.buildGrainTile(20261022);
    this.grainTileL = this.buildGrainTile(20261023);
    this.grainTileM = this.buildGrainTile(20261024);
    this.buildGrainBitmaps();
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
    // ── 运动代理（§5.2）：配对有效且有 prev 视图才更新；否则冻结 ──
    const motionValid = result.pair.valid && result.prev !== null;
    this.motionCache.update(
      result.prev !== null ? result.prev : view,
      view, motionValid, colW, P012_GRAIN_TILE);
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
    // 2) A 档水体雾（EA-batch，算法稿 §4.2）：悬浮浓度 q + 静态水介质斑驳。
    //    只画水体区（sandTop 以上）；界面过渡带在沙体之后画（见步骤 5）。
    const tier = frameContext.appearanceTier;
    if (tier === P012AppearanceTier.EA || tier === P012AppearanceTier.EAB) {
      drawCount += this.drawWaterFog(ctx, view, heightToY, colW,
        boxX, boxY, boxW, boxH);
    }
    // 3) 沙体三带：重沙带（bh 以下）/ 轻沙带（mixStart→bh）/ 混合带（sandTop→mixStart）
    //    逐列伪法向光照：s = d(sandTop)/dx（屏幕单位），L = La + Ld·max(dot(n,l),0)
    //    用 8 档游程近似逐列光照（run 合并相邻同档列，draw 计数受控）
    //    A 档启用时沙顶 3px 硬描边语义由界面过渡带取代（P-012 分支无硬描边）
    drawCount += this.drawSandColumns(ctx, view, heightToY, colW, boxX, boxY, boxH);
    // 3.5) B 档粉质颗粒（EAB-batch，算法稿 §5.1）：三带各自 clip 到沙区曲线 Path +
    //      createPattern(repeat) 全局平流偏移填充；跟随材料净搬运、无缝、无贴纸。
    if (tier === P012AppearanceTier.EAB) {
      drawCount += this.drawSandGrain(ctx, view, heightToY, colW, boxX, boxY, boxH);
    }
    // 4) 液面高光：沿 waterTop 的宽软带（§3.1 柔和高光，alpha≤0.12）（1 draw）
    ctx.beginPath();
    ctx.moveTo(boxX, heightToY(this.smoothWaterTop[0]));
    for (let c = 1; c < n; c++) {
      ctx.lineTo(boxX + c * colW, heightToY(this.smoothWaterTop[c]));
    }
    ctx.strokeStyle = 'rgba(255,255,255,0.12)';
    ctx.lineWidth = 5;
    ctx.stroke();
    drawCount++;
    // 5) A 档界面光学过渡带（沙体之后绘制）：sandTop 两侧 ±halfW 内
    //    smoothstep 包络雾——沙侧上缘被雾柔和覆盖，消除沙体 fillRect 的硬边。
    if (tier === P012AppearanceTier.EA || tier === P012AppearanceTier.EAB) {
      drawCount += this.drawInterfaceBand(ctx, view, heightToY, colW,
        boxX, boxY, boxW, boxH);
    }
    // 6) 容器边框（与原渲染一致的白框）
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
    this.motionCache.reset();
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

  /** 第一梯队 E/A/B 合计 draw 预算断言（单测钩子：≤112 次提交） */
  withinFirstTierDrawBudget(): boolean {
    return this.lastDrawCount <= P012_FIRST_TIER_DRAW_BUDGET;
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

  /**
   * A 档两尺度零均值噪声 tile（算法稿 §4.2）：
   * n1 细团（12~24px 特征尺度）/ n2 大团（40~80px）。
   * 用多八度 value-noise：白噪声 → 盒滤波降采样到目标尺度 → 去均值归一化。
   * 错开固定相位（n2 偏移半个周期）；确定性（LCG 种子，不用物理随机源）。
   */
  private buildMottleTiles(seed: number): void {
    // n1：细团——特征尺度 16px（12~24 中值）→ tile 64px 含 4 个周期
    this.buildValueNoiseTile(this.mottleN1, (seed | 0) + 1, 16);
    // n2：大团——特征尺度 56px（40~80 中值）→ tile 64px 接近 1 个周期
    this.buildValueNoiseTile(this.mottleN2, (seed | 0) + 2, 56);
  }

  /** 单尺度 value-noise tile：白噪声 → 盒滤波 → 去均值 → 归一到 [-1,1]
   *  尺寸由 out.length 推导（mottle tile=64、grain tile=256 共用本函数）；
   *  早期版本硬编码 P012_A_TILE=64，导致 256² grain tile 仅左上 64×64 被填充、
   *  其余 93.75% 为常数 → 颗粒贴图近乎纯色（hfH≈0）。 */
  private buildValueNoiseTile(out: Float64Array, seed: number, featurePx: number): void {
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
    // 盒滤波半径 = featurePx/2（可平铺：环形索引）
    const r = Math.max(1, Math.round(featurePx / 2));
    for (let y = 0; y < size; y++) {
      for (let x = 0; x < size; x++) {
        let sum = 0;
        let cnt = 0;
        for (let dy = -r; dy <= r; dy += 2) {
          for (let dx = -r; dx <= r; dx += 2) {
            const yy = (y + dy + size) % size;
            const xx = (x + dx + size) % size;
            sum += white[yy * size + xx];
            cnt++;
          }
        }
        out[y * size + x] = sum / cnt;
      }
    }
    // 去均值 + 归一化到 [-1,1]
    let mean = 0;
    for (let i = 0; i < out.length; i++) {
      mean += out[i];
    }
    mean /= out.length;
    let maxAbs = 1e-9;
    for (let i = 0; i < out.length; i++) {
      out[i] -= mean;
      maxAbs = Math.max(maxAbs, Math.abs(out[i]));
    }
    for (let i = 0; i < out.length; i++) {
      out[i] /= maxAbs;
    }
  }

  /** alphaFog LUT：tau∈[0,4] → 1-exp(-tau)，64 档（§4.2 预计算，热路径零 exp） */
  private buildFogLut(): void {
    for (let i = 0; i <= 64; i++) {
      const tau = i / 16;
      this.fogLut[i] = 1 - Math.exp(-tau);
    }
  }

  /** fogLut 采样：tau clamp [0,4] */
  private fogAlpha(tau: number): number {
    const t = Math.max(0, Math.min(4, tau));
    const idx = Math.round(t * 16);
    return this.fogLut[idx];
  }

  // ───────────────────────── B 档颗粒（EAB-batch，§5.1） ─────────────────────────

  /** 颗粒专用 value-noise tile（B 档，§5.1）：
   *  与 A 档 mottle 共用 buildValueNoiseTile 的生成段，但归一化语义不同——
   *  A 档峰值归一化（mottle 幅度 a1/a2 已按峰值标定，不可动）；
   *  B 档按 §5.1 要求**单位方差、零均值**并截幅 ±P012_B_GRAIN_CLIP，
   *  使 sigmaG·nG 的实际亮度标准差就是 sigmaG 量级（18~22 级）。
   *  盒滤波为真盒（步长 1，非 mottle 的稀疏 X 采样）→ 相邻像素真实相关。 */
  private buildGrainNoise(out: Float64Array, seed: number, featurePx: number): void {
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

  /** 颗粒 tile：双细粒 tile 混合（减平铺）+ 团簇 envelope（3-5px 弱团），
   *  零均值单位方差归一化到 [-clip,clip]；确定性 LCG 种子（不调用/消耗物理随机源）。 */
  private buildGrainTile(seed: number): Float64Array {
    const size = P012_GRAIN_TILE;
    const fineA = new Float64Array(size * size);
    const fineB = new Float64Array(size * size);
    const env = new Float64Array(size * size);
    this.buildGrainNoise(fineA, (seed | 0) + 11, P012_B_FINE_PX);   // 细粒 A（~2px）
    this.buildGrainNoise(fineB, (seed | 0) + 23, P012_B_FINE_PX);   // 细粒 B（~2px，异种子）
    this.buildGrainNoise(env, (seed | 0) + 37, P012_B_ENV_PX);      // 团簇 envelope（3-5px）
    const out = new Float64Array(size * size);
    let mean = 0;
    for (let i = 0; i < out.length; i++) {
      const g = 0.5 * fineA[i] + 0.5 * fineB[i];   // 双 tile 混合
      const e = 1 + P012_B_ENV_AMP * env[i];        // 团簇 envelope 调制
      out[i] = g * e;
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
    // 包络乘积会超出细粒各自截幅 → 终 tile 再去均值 + 截幅（顺序同
    // buildGrainNoise：先均值后截幅；不再方差缩放）。
    let mean3 = 0;
    for (let i = 0; i < out.length; i++) {
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

  /** 生成 3 张预着色颗粒 ImageBitmap（on-device）；Node/canvas 缺失则 bitmap=null。 */
  private buildGrainBitmaps(): void {
    let ocKind = 'none';
    try {
      if (typeof OffscreenCanvas === 'undefined' || OffscreenCanvas === null) {
        hilog.info(0x0000, 'P012', 'grain: OffscreenCanvas undefined/null');
        return;
      }
      ocKind = 'class';
    } catch (e) {
      hilog.error(0x0000, 'P012', 'grain: typeof OffscreenCanvas threw: ' + JSON.stringify(e));
      return;
    }
    hilog.info(0x0000, 'P012', 'grain: OffscreenCanvas ' + ocKind + ' present, building bitmaps');
    const p = this.palette;
    this.grainBitmapH = this.makeGrainBitmap(this.grainTileH, p.heavyR, p.heavyG, p.heavyB);
    this.grainBitmapL = this.makeGrainBitmap(this.grainTileL, p.lightR, p.lightG, p.lightB);
    this.grainBitmapM = this.makeGrainBitmap(this.grainTileM,
      (p.lightR + p.heavyR) * 0.5, (p.lightG + p.heavyG) * 0.5, (p.lightB + p.heavyB) * 0.5);
    hilog.info(0x0000, 'P012', 'grain: bitmaps H=' + (this.grainBitmapH === null ? 'null' : 'ok') +
      ' L=' + (this.grainBitmapL === null ? 'null' : 'ok') +
      ' M=' + (this.grainBitmapM === null ? 'null' : 'ok'));
  }

  /** 单张颗粒 tile → ImageBitmap（基色 + sigmaG·tile 偏移；稀疏亮粒在 tile 之外不加，
   *  由 E 档 spark tile 承担）。on-device 经 OffscreenCanvas/putImageData/transfer。 */
  private makeGrainBitmap(tile: Float64Array, baseR: number, baseG: number, baseB: number): ImageBitmap | null {
    const size = P012_GRAIN_TILE;
    try {
      const off = new OffscreenCanvas(size, size);
      const octx = off.getContext('2d');
      if (octx === null || octx === undefined) {
        hilog.error(0x0000, 'P012', 'grain: getContext(2d) returned null/undefined');
        return null;
      }
      const img = octx.createImageData(size, size);
      const data = img.data;
      // alpha 不烘焙：CanvasPattern 采样会把烘焙了半透明 alpha 的像素颜色
      // 按 1/alpha 放大并钳位（s94 消融实测：alpha=140 → 颜色 ×1.82 饱和
      // 到 255，混合/轻沙带整片变白）。改为位图纯不透明（alpha=255），
      // 叠加强度（P012_B_GRAIN_ALPHA）由 drawSandGrain 的 ctx.globalAlpha 控制
      // （save/restore 作用域内自动恢复）。
      const pxAlpha = 255;
      for (let i = 0; i < size * size; i++) {
        const delta = tile[i] * P012_B_SIGMA;   // 颗粒亮度偏移（lumStd 主来源）
        data[i * 4] = this.clamp255(baseR + delta);
        data[i * 4 + 1] = this.clamp255(baseG + delta);
        data[i * 4 + 2] = this.clamp255(baseB + delta);
        data[i * 4 + 3] = pxAlpha;
      }
      octx.putImageData(img, 0, 0);
      const bmp = off.transferToImageBitmap();
      hilog.info(0x0000, 'P012', 'grain: makeGrainBitmap ok=' + (bmp === null ? 'null' : 'ok'));
      return bmp;
    } catch (e) {
      hilog.error(0x0000, 'P012', 'grain: makeGrainBitmap threw: ' + JSON.stringify(e));
      return null;
    }
  }

  private clamp255(v: number): number {
    return Math.max(0, Math.min(255, Math.round(v)));
  }

  /** 单带顶部屏幕 y（heightToY 映射）：kind 0=重沙顶(bh) 1=轻沙顶(bh+bl) 2=混合带顶(sandTop) */
  private bandTopY(kind: number, c: number, heightToY: (h: number) => number): number {
    if (kind === 0) {
      return heightToY(this.smoothBh[c]);
    }
    if (kind === 1) {
      return heightToY(this.smoothMixStart[c]);
    }
    return heightToY(this.smoothSandTop[c]);
  }

  /** 单带底部屏幕 y：kind 0=容器底(heightToY(0)) 1=重沙顶(bh) 2=轻沙顶(bh+bl) */
  private bandBottomY(kind: number, c: number, heightToY: (h: number) => number): number {
    if (kind === 0) {
      return heightToY(0);
    }
    if (kind === 1) {
      return heightToY(this.smoothBh[c]);
    }
    return heightToY(this.smoothMixStart[c]);
  }

  /**
   * B 档粉质颗粒（算法稿 §5.1）：三带各自 clip 到沙区曲线 Path，再以 createPattern
   * (repeat) + translate(全局平流偏移) 填充整片沙区。全局偏移（mean UV）→ 各列共享
   * 同一连续 pattern → 无缝、无跨列拼缝；结构预留 16 组供未来 D 各向异性。
   * 颗粒仅在 E 已提亮沙色之上叠加（globalAlpha=P012_B_GRAIN_ALPHA），保留部分 E 光照。
   * 返回 draw 提交次数（3 次 fillRect，远低于 B 档 ≤48 预算）。
   */
  private drawSandGrain(ctx: CanvasRenderingContext2D, view: P012MaterialFrameView,
    heightToY: (h: number) => number, colW: number,
    boxX: number, boxY: number, boxH: number): number {
    const n = this.cols;
    const tilePx = P012_GRAIN_TILE;
    const offH = this.motionCache.meanDeltaXPx(true);
    const offL = this.motionCache.meanDeltaXPx(false);
    const offM = (offH + offL) * 0.5;
    const bands: P012GrainBand[] = [
      { kind: 0, off: offH, bitmap: this.grainBitmapH },
      { kind: 1, off: offL, bitmap: this.grainBitmapL },
      { kind: 2, off: offM, bitmap: this.grainBitmapM }
    ];
    let draws = 0;
    for (let bi = 0; bi < bands.length; bi++) {
      const b = bands[bi];
      ctx.save();
      // clip 到该带沙区曲线 Path（保证颗粒不越过层边界/界面）
      ctx.beginPath();
      ctx.moveTo(boxX, this.bandTopY(b.kind, 0, heightToY));
      for (let c = 1; c < n; c++) {
        ctx.lineTo(boxX + c * colW, this.bandTopY(b.kind, c, heightToY));
      }
      for (let c = n - 1; c >= 0; c--) {
        ctx.lineTo(boxX + (c + 1) * colW, this.bandBottomY(b.kind, c, heightToY));
      }
      ctx.closePath();
      ctx.clip();
      // 全局平流偏移（纹理随材料净搬运滚动）
      ctx.translate(b.off, 0);
      // 颗粒叠加强度 0.55（§5.1 "作用于 E 已提亮的材质"）：位图纯不透明，
      // 强度走 globalAlpha；restore() 自动恢复，不影响后续描边。
      ctx.globalAlpha = P012_B_GRAIN_ALPHA;
      if (b.bitmap !== null) {
        const pat = ctx.createPattern(b.bitmap, 'repeat');
        if (pat !== null) {
          ctx.fillStyle = pat;
        } else {
          ctx.fillStyle = 'rgba(0,0,0,0)';
          if (!this.grainDiagLogged) {
            this.grainDiagLogged = true;
            hilog.error(0x0000, 'P012', 'grain: createPattern returned null (band ' + b.kind + ')');
          }
        }
      } else {
        // Node / 无 bitmap：fallback 仍计一次 draw（真实设备走 pattern 路径）
        ctx.fillStyle = 'rgba(0,0,0,0)';
        if (!this.grainDiagLogged) {
          this.grainDiagLogged = true;
          hilog.error(0x0000, 'P012', 'grain: bitmap null, fallback no-fill (band ' + b.kind + ')');
        }
      }
      const fullW = n * colW;
      ctx.fillRect(boxX - tilePx, boxY - tilePx, fullW + 2 * tilePx, boxH + 2 * tilePx);
      draws++;
      ctx.restore();
    }
    if (!this.grainDiagLogged) {
      this.grainDiagLogged = true;
      hilog.info(0x0000, 'P012', 'grain: pattern fill path used (all bands ok)');
      // 带几何一次性诊断（定位"白带/无颗粒"覆盖区）：中心列各边界屏幕 y
      const cm = n >> 1;
      hilog.info(0x0000, 'P012', 'grain: geom col=' + cm +
        ' sandTopY=' + heightToY(this.smoothSandTop[cm]).toFixed(1) +
        ' mixStartY=' + heightToY(this.smoothMixStart[cm]).toFixed(1) +
        ' bhY=' + heightToY(this.smoothBh[cm]).toFixed(1) +
        ' waterTopY=' + heightToY(this.smoothWaterTop[cm]).toFixed(1) +
        ' bottomY=' + (boxY + boxH).toFixed(1) +
        ' offH=' + offH.toFixed(2) + ' offL=' + offL.toFixed(2));
    }
    return draws;
  }

  /** 悬浮浓度 q（clamp [0,1]，水深≤eps 时 0；不改库存）——A 档共用 */
  private suspQ(susp: number, c: number): number {
    const depth = this.smoothSandTop[c] - this.smoothWaterTop[c];
    return depth > 1e-6 ? Math.max(0, Math.min(1, susp / depth)) : 0;
  }

  /** 斑驳采样：两尺度 tile 按屏幕坐标平铺（aPhaseX 静止=0；EAB 运动代理接手） */
  private mottleAt(xPx: number, yPx: number): number {
    const tx = ((Math.floor(xPx) % P012_A_TILE) + P012_A_TILE) % P012_A_TILE;
    const ty = ((Math.floor(yPx) % P012_A_TILE) + P012_A_TILE) % P012_A_TILE;
    const n1 = this.mottleN1[ty * P012_A_TILE + tx];
    const n2 = this.mottleN2[ty * P012_A_TILE + tx];
    return 1 + P012_A_MOTTLE_A1 * n1 + P012_A_MOTTLE_A2 * n2;
  }

  /** A 档 alpha 档矩形统一提交：每档一个合并 Path（fog 浅薄荷偏白） */
  private fillFogTiers(ctx: CanvasRenderingContext2D, tierRects: number[][],
    alphaStep: number, budget: number): number {
    let draws = 0;
    const fogR = 232;
    const fogG = 240;
    const fogB = 236;
    for (let t = 0; t < P012_A_ALPHA_TIERS; t++) {
      if (tierRects[t].length === 0) {
        continue;
      }
      const a = (t + 0.5) * alphaStep;
      ctx.beginPath();
      for (let i = 0; i < tierRects[t].length; i += 4) {
        ctx.rect(tierRects[t][i], tierRects[t][i + 1],
          tierRects[t][i + 2], tierRects[t][i + 3]);
      }
      ctx.fillStyle = `rgba(${fogR},${fogG},${fogB},${a.toFixed(3)})`;
      ctx.fill();
      draws++;
      if (draws >= budget) {
        break;   // 预算保护
      }
    }
    return draws;
  }

  /** 静止水介质静态斑驳（q=0 时的低对比光照斑驳，§4.2"清水静态明暗"）：
   *  tauStatic = TAU_STATIC_BASE + TAU_STATIC_AMP·(stretch(mottle)-1)
   *  stretch=2.2 线性拉伸（clamp [0.2,2.0]）拉开 value-noise 钟形分布。 */
  private staticTau(mottle: number): number {
    const stretched = Math.max(0.2, Math.min(2.0,
      1 + (mottle - 1) * P012_A_MOTTLE_STRETCH));
    return Math.max(0, P012_A_TAU_STATIC_BASE +
      P012_A_TAU_STATIC_AMP * (stretched - 1));
  }

  /**
   * A 档水体雾（算法稿 §4.2，沙体之前绘制）：16 组 × 每组 8 列竖条带，
   * 组内竖向 8px 段采样 tau（qH/qL 悬浮 + 静止水介质斑驳 staticTau）。
   * q 归零时保留静态斑驳（设计 §4.2），不制造持续漂浮粒子。
   */
  private drawWaterFog(ctx: CanvasRenderingContext2D, view: P012MaterialFrameView,
    heightToY: (h: number) => number, colW: number,
    boxX: number, boxY: number, boxW: number, boxH: number): number {
    const n = this.cols;
    const halfW = P012_A_WFULL_PX / 2;
    const alphaStep = P012_A_FOG_ALPHA_MAX / P012_A_ALPHA_TIERS;
    const tierRects: number[][] = [];
    for (let t = 0; t < P012_A_ALPHA_TIERS; t++) {
      tierRects.push([]);
    }
    const segH = 8;   // 竖向采样段高（px）
    // 逐列计算（不按组代表列：组边界会有水平硬跳变，maxGrad<8 锚点无法达成；
    // 128 列 × 每列 ~40 段 = O(5000) 次采样远低于预算，draw 数只取决于档数）
    for (let c = 0; c < n; c++) {
      const qH = this.suspQ(view.suspHAt(c), c);
      const qL = this.suspQ(view.suspLAt(c), c);
      const gTopY = heightToY(this.smoothWaterTop[c]);
      const gIfaceY = heightToY(this.smoothSandTop[c]);
      const xPx = boxX + (c + 0.5) * colW;
      for (let y = Math.ceil(gTopY); y < gIfaceY; y += segH) {
        const yMid = y + segH / 2;
        // tauOptical：界面附近有限支撑（半宽内峰值 → 线性衰减到 0）
        const dist = Math.abs(gIfaceY - yMid);
        const tauOpt = dist < halfW
          ? P012_A_TAU_INTERFACE * (1 - dist / halfW) : 0;
        // 静止水介质斑驳（加性调制 + 拉伸）
        const mot = this.mottleAt(xPx - this.aPhaseX, yMid);
        const tauStatic = this.staticTau(mot);
        const tau = Math.max(0, tauOpt + P012_A_KH * qH + P012_A_KL * qL + tauStatic);
        let alpha = this.fogAlpha(tau);
        if (alpha <= 0.004) {
          continue;
        }
        alpha = Math.min(P012_A_FOG_ALPHA_MAX, alpha);
        const tierIdx = Math.min(P012_A_ALPHA_TIERS - 1,
          Math.floor(alpha / alphaStep));
        const segBot = Math.min(gIfaceY, y + segH);
        tierRects[tierIdx].push(boxX + c * colW, y, colW, segBot - y);
      }
    }
    return this.fillFogTiers(ctx, tierRects, alphaStep, P012_A_BATCH_DRAW_BUDGET);
  }

  /**
   * A 档界面光学过渡带（算法稿 §4.1，沙体之后绘制）：
   * sandTop 两侧 ±halfW 内 smoothstep 包络（t=clamp((d+wFull/2)/wFull,0,1)，
   * waterWeight=t²(3-2t)），雾覆盖沙侧上缘 → 沙面到水体连续渐变无硬边。
   * 逐列 4 段采样，与水体雾同 alpha 档合并提交。
   */
  private drawInterfaceBand(ctx: CanvasRenderingContext2D, view: P012MaterialFrameView,
    heightToY: (h: number) => number, colW: number,
    boxX: number, boxY: number, boxW: number, boxH: number): number {
    const n = this.cols;
    const wFull = P012_A_WFULL_PX;
    const halfW = wFull / 2;
    // 界面带 alpha 上限守设计候选值 0.55（散射过渡，不是浓雾遮盖）
    const alphaStep = P012_A_BAND_ALPHA_MAX / P012_A_ALPHA_TIERS;
    // 预计算逐列 q（列内 8 段共享）
    const qHArr = new Float64Array(n);
    const qLArr = new Float64Array(n);
    for (let c = 0; c < n; c++) {
      qHArr[c] = this.suspQ(view.suspHAt(c), c);
      qLArr[c] = this.suspQ(view.suspLAt(c), c);
    }
    const tierRects: number[][] = [];
    for (let t = 0; t < P012_A_ALPHA_TIERS; t++) {
      tierRects.push([]);
    }
    for (let c = 0; c < n; c++) {
      const xPx = boxX + (c + 0.5) * colW;
      const ifaceY = heightToY(this.smoothSandTop[c]);
      // 带范围：sandTop 上方 halfW（水侧）→ 下方 quarterW（沙侧收尾）
      const bandTop = ifaceY - halfW;
      const bandBot = ifaceY + wFull * 0.25;
      // 8 段采样（maxGrad<8 锚点：段内 alpha 阶梯 ≤2.2 亮度/段）；
      // 顶部 ramp 渐入：第一段乘 (s+0.5)/8 防带顶硬起始
      const segH = (bandBot - bandTop) / 8;
      const qH = qHArr[c];
      const qL = qLArr[c];
      for (let s = 0; s < 8; s++) {
        const yTop = bandTop + segH * s;
        const yMid = yTop + segH / 2;
        // d：到 S 的法向距离（正=水侧，负=沙侧）
        const d = ifaceY - yMid;
        const t = Math.max(0, Math.min(1, (d + halfW) / wFull));
        const rampIn = Math.min(1, (s + 0.5) / 2.5);   // 前 2 段渐入
        const waterWeight = t * t * (3 - 2 * t) * rampIn;
        if (waterWeight <= 0.01) {
          continue;
        }
        const mot = this.mottleAt(xPx - this.aPhaseX, yMid);
        // 沙侧（d<0）：waterWeight 已随 smoothstep 衰减，雾色向沙色过渡
        const tau = Math.max(0, (P012_A_TAU_INTERFACE * waterWeight +
          P012_A_KH * qH * waterWeight + P012_A_KL * qL * waterWeight) * mot);
        let alpha = this.fogAlpha(tau);
        if (alpha <= 0.004) {
          continue;
        }
        alpha = Math.min(P012_A_BAND_ALPHA_MAX, alpha);
        const tierIdx = Math.min(P012_A_ALPHA_TIERS - 1,
          Math.floor(alpha / alphaStep));
        const segTop = Math.max(bandTop, yTop);
        const segBot = Math.min(bandBot, yTop + segH);
        if (segBot > segTop) {
          tierRects[tierIdx].push(boxX + c * colW, segTop, colW, segBot - segTop);
        }
      }
    }
    return this.fillFogTiers(ctx, tierRects, alphaStep, P012_A_BATCH_DRAW_BUDGET);
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

/** B 档颗粒单带描述（显式接口：kind/offset/bitmap；禁止匿名对象字面量） */
interface P012GrainBand {
  kind: number;
  off: number;
  bitmap: ImageBitmap | null;
}
