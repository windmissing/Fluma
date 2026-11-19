import { P013CanvasAssetExport, p013Sha256 } from './preparation/P013CanvasAssetExport';
import { P013PreparedMetadata, P013PreparedBundle } from './preparation/P013MaterialPreparation';
import { P012ParticleLayer, P012ParticleSampler, P012ParticleCanvas } from './P012ParticleLayer';
// P-012 表现层渲染器——E 档：光照与色调映射 + A 档：云雾/界面光学过渡
// 问题 ID：P-012；方案版本：p012-render-material-v1（E-batch → EA-batch）
// 职责：帧绘制编排（E 色调/伪法向光照/液面高光/亮粒 tile；A 界面散射带/
//   两尺度斑驳/水体雾），拥有 adapter。
// 红线：只读物理场；一次帧只选一个完整渲染分支；不进入物理 checker。
// 预算：E 档 draw ≤16；A 档新增 ≤48（EA 合计 ≤64）；CPU 0.25+0.35ms（待真机验证）。
import { WaterSandSnapshot, WaterSandField } from '../physics/WaterSandField';
import { P012WaterSandAdapter, P012MaterialFrameView, P012AdapterResult } from './P012WaterSandAdapter';
import { P012MaterialMotionCache } from './P012MaterialMotionCache';
import { P012MaterialColumnSampler } from './P012MaterialSampler';
import { P012SandTracer, P012TracerOcclusionView, P012TracerDrawContext } from './P012SandTracer';
import { P012E3NoiseField } from './P012E3Noise';
import { P012E3FogBuffer } from './P012E3FogBuffers';
import hilog from '@ohos.hilog';
import {
  P012FrameContext, P012AppearanceTier, P012_APPEARANCE_ROUTE_ID,
  P012_E_BATCH_DRAW_BUDGET, P012_A_BATCH_DRAW_BUDGET,
  P012_B_BATCH_DRAW_BUDGET, P012_FIRST_TIER_DRAW_BUDGET,
  P012Ablation, P012FillBox,
  P012AppearanceParams, p012FactoryAppearance, validateP012AppearanceParams
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

/** ── C 层理参数（architecture §11.1；design-P012-C-stratification §3.1）── */
/** 条纹位图：宽 64（warp 相位轴）× 高 256（深度相位轴）×3 张（H/L/M 各一 seed）。
 *  常驻 3×64×256×4 = 196KB ≤ 0.25MiB 预算（§11.1 选型 B）。 */
const P012_C_BITMAP_W = 64;
const P012_C_BITMAP_H = 256;
/** λ（层纹周期，材料深度单位）。设计稿：run 首个合格厚层场景按约 16–32
 *  分析行对应的材料深度冻结。实现取冻结候选中值 24 材料单位（≈分析行 24）。
 *  不随层高归一化（红线）：层高变化时层纹数量按实际厚度增减（设计行为）。 */
const P012_C_LAMBDA_MATERIAL = 24;
/** warp 横向慢变幅度（周期数，设计候选 ≤1/4 周期） */
const P012_C_WARP_RANGE = 0.25;
/** 分段阈值：相邻列 warp 相位差 >1/16 周期断段（§11.1；段内误差 ≤1/32） */
const P012_C_SEG_THRESHOLD = 1 / 16;
/** 段数上限（三带合计 K≤16 预算；极端满幅 warp 时按 1/16 阈值最多 4 段/带） */
const P012_C_SEG_MAX = 16;
/** 层纹幅度（灰度级，设计稿：有序层 ±4–8 / 混合区 ±2–4）。
 *  烘焙进位图（envelope·amp·bandNoise 已含），运行期不再缩放。 */
const P012_C_AMP_ORDERED = 6;
const P012_C_AMP_MIX = 3;
/** C1 叠加强度（globalAlpha：位图灰度 128±deltaY 与底色 source-over） */
const P012_C_ALPHA = 0.5;
/** C3 低频明暗带数/带（设计稿 2–4 宽块；取 2 控 draw 预算：2×3 带=6 ≤16
 *  扣除 C1 drawImage 后的余量；复用 E shadeLut，零新缓存） */
const P012_C3_BANDS = 2;
/** C3 明暗带叠加强度 */
const P012_C3_ALPHA = 0.10;
/** bandNoise 纵向特征尺度盒滤波半径（px；非等间距带纹的相邻周期不规则度） */
const P012_C_BAND_RPx = 6;

/** ── E3 网格感消除参数（architecture §12；algorithm-E3 §3）── */
/** ── E2 路线②原型参数（algorithm-P012-E2-top-light-opening-v1 §2；hardcode 候选值，不进面板）── */
/** 宽面坡度低通 σ（光学宽面尺度，材料深度单位；开题扫描域候选中值）。
 *  不得改物理表面——只在显示空间对 smoothSandTop 低通。 */
const P012_E2_SIGMA = 6.0;
/** 深度衰减 ℓ（ellLight，材料深度单位）：整片坡面向沙体内部衰减的可读深度 */
const P012_E2_ELL = 24.0;
/** Lface 界（clamp）：Lmin/Lmax（开题 §2 公式） */
const P012_E2_LMIN = 0.80;
const P012_E2_LMAX = 1.12;
/** 光照图深度分辨率（z 方向档数；128 列 × Z_STEPS 双语义图） */
const P012_E2_Z_STEPS = 256;
/** sandTop 形状变化重烘阈值（显示 px；§5「sandTop 形状变更→重算法线/光照图」） */
const P012_E2_SHAPE_EPS_PX = 2.0;
/** I2 缓冲宽。原 256 在真机 ArkTS 上烘焙循环 ~950ms/帧（s123 实测，
 *  方法调用密集的双层循环比 Node 慢两个数量级）——降到 64（雾为低频
 *  视觉带宽，64 宽采样后 12 档量化已无可见差异），循环量降 16 倍。
 *  384 备选构造定档、运行期不切换——设计稿红线不变。 */
const P012_E3_BUFFER_W = 64;
/** 平流相位脏阈值（px）：|aPhaseX - buffer.sampleU| > 此值 → 重烘 */
const P012_E3_PHASE_EPS = 0.5;
/** N1 等效 mottle 幅值（定标前置报告 calib-e3-noise-v1：N1 std 0.31 ×
 *  1.63 = 0.505 vs 旧 mottle 0.5494，比例 0.919 ∈ [0.90,1.10] 界内，
 *  等效幅值沿用旧链 1.63 = sqrt(1.3²+1.0²) 归一） */
const P012_E3_MOTTLE_EQUIV_AMP = 1.63;
/** N1 场烘焙余量（px，素材周期上界 64） */
const P012_E3_NOISE_MARGIN = 64;
/**
 * N1 场覆盖维护门控（保底）：窗口平移量 < 此值（px）跳过 shiftAndBake。
 * 与内部 0.5px 短路互补——phase 代理稳定化（治本）后残余亚视觉位移也不
 * 触发 110ms 全量重烘；genuine 搬运每移动 GATE px 才重烘一次（≤2px 滞后
 * 落在 64px 余量内，视觉不可辨）。settle 场景 → 零重烘。
 */
const P012_E3_NOISE_GATE_PX = 2;
/** 纯 alpha 诊断图开关（debug build 常量，不进 want/产品；§12.5-2） */
const P012_E3_DEBUG_ALPHA_ONLY = false;
/** 雾色（fillFogTiers 与 E3 I2 烘焙共用；rgb(232,240,236)） */
const P012_FOG_R = 232;
const P012_FOG_G = 240;
const P012_FOG_B = 236;
/** 旧 drawInterfaceBand 的 8 段渐入段高（E3 iface 烘焙 ramp 对齐用） */
function segHRamp8(bandH: number): number {
  return bandH / 8;
}

/** renderer 对外生命周期接口（architecture §1 模块表） */
/**
 * E3 悬沙 q 阈值脏检测（★s134 渲染回归修复，任务 6adb3508…0b97）。
 * 抽成模块级纯函数以便单测直接验证阈值语义。
 * 判据：|q - prev| > max(0.02, |prev|·0.2) 即脏。
 *   · 绝对 0.02 保底：q≤0.02 的静水不抖；
 *   · 相对 20%：小 q 微动不触发，真实悬浮变化（q>0.1）仍快响应；
 *   · 取代旧实现「绝对 0.01」，旧实现在 settle 缓变场景每帧触发（q 帧间差
 *     常 >0.01 → E3 双 buffer 每帧重烘 → 热态 126ms/帧）。
 * H/L 独立判定（重相/轻相各自阈值，互不连坐）。
 */
export function p012E3QDirty(prevH: number, prevL: number, qh: number, ql: number): boolean {
  const epsH = Math.max(0.02, Math.abs(prevH) * 0.2);
  const epsL = Math.max(0.02, Math.abs(prevL) * 0.2);
  return Math.abs(qh - prevH) > epsH || Math.abs(ql - prevL) > epsL;
}

export class P012WaterSandRenderer {
  private materialExportEnabled: boolean = false;
  private materialExportScale: number = 0;
  private readonly materialAssetExport: P013CanvasAssetExport = new P013CanvasAssetExport();
  prepareMaterialAssetExport(pxPerMaterial: number): void {
    if (!Number.isFinite(pxPerMaterial) || pxPerMaterial <= 0) throw new Error('MATERIAL_GEOMETRY');
    this.materialExportEnabled = true; this.materialExportScale = pxPerMaterial;
    this.buildGrainBitmapsFor(this.activeAppearance.grainSigma);
    this.buildStratBitmapsFor(this.activeAppearance.strataLambdaMaterial,this.activeAppearance.strataStrengthScale,pxPerMaterial);
  }
  exportMaterialAssets(metadata: P013PreparedMetadata): P013PreparedBundle {
    // Existing generators use fixed phase seeds 20261022..27, recorded in version.
    metadata.assetGeneratorVersion = 'P012_CANVAS_SEEDS_20261022_27_V1';
    metadata.assetGenerationParameters = JSON.stringify({grainSeeds:[20261022,20261023,20261024],
      strataSeeds:[20261025,20261026,20261027],grainSigma:this.activeAppearance.grainSigma,
      grainFeaturePx:this.activeAppearance.grainFeaturePx,strataStrengthScale:this.activeAppearance.strataStrengthScale,
      nominalLambdaMaterial:this.activeAppearance.strataLambdaMaterial,pxPerMaterial:this.materialExportScale});
    metadata.effectiveLambdaCanvasPx = this.stratLamPx;
    metadata.effectiveLambdaMaterial = this.stratLamPx / this.materialExportScale;
    metadata.warpRangeCycles = P012_C_WARP_RANGE;
    const p = this.palette;
    const palette = new Uint8Array([p.heavyR,p.heavyG,p.heavyB,p.lightR,p.lightG,p.lightB]);
    metadata.paletteHash = p013Sha256(palette);
    // Approved low-palette bundle only; never silently relabel a different palette.
    if (palette[0]!==145 || palette[1]!==168 || palette[2]!==111 || palette[3]!==216 || palette[4]!==196 || palette[5]!==150)
      throw new Error('MATERIAL_PALETTE_UNSUPPORTED');
    return this.materialAssetExport.bundle(metadata);
  }

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
  private grainTileH: Float64Array = new Float64Array(P012_GRAIN_TILE * P012_GRAIN_TILE);
  private grainTileL: Float64Array = new Float64Array(P012_GRAIN_TILE * P012_GRAIN_TILE);
  private grainTileM: Float64Array = new Float64Array(P012_GRAIN_TILE * P012_GRAIN_TILE);
  /** 预着色颗粒 ImageBitmap（on-device 经 OffscreenCanvas 生成；Node/canvas 缺失为 null） */
  private grainBitmapH: ImageBitmap | null = null;
  private grainBitmapL: ImageBitmap | null = null;
  private grainBitmapM: ImageBitmap | null = null;
  /** ── C 层理资源（§11.1 选型 B：窄高条纹位图，稳定深度函数一次性烘焙）── */
  /** bandNoise 纵向一维表（深度相位 → [-1,1] 连续带纹；烘焙源数据）。
   *  独立于位图保存：Node 通道无 OffscreenCanvas 时单测仍可验证零均值有界。 */
  private readonly stratNoiseH: Float64Array = new Float64Array(P012_C_BITMAP_H);
  private readonly stratNoiseL: Float64Array = new Float64Array(P012_C_BITMAP_H);
  private readonly stratNoiseM: Float64Array = new Float64Array(P012_C_BITMAP_H);
  /** C 层理灰度位图（64×256：横向=warp 相位轴，纵向=深度相位轴；
   *  纯不透明 alpha=255，RGB 同值灰度） */
  private stratBitmapH: ImageBitmap | null = null;
  private stratBitmapL: ImageBitmap | null = null;
  private stratBitmapM: ImageBitmap | null = null;
  /** C 位图烘焙时的 λpx（显示 px / 周期）；geometry epoch 变化重烘焙时更新 */
  private stratLamPx: number = 0;
  /** C 位图烘焙时的几何代次（检测 resize/DPI 重烘焙） */
  private stratBakedGeometryEpoch: number = -1;
  /** warp 基线横向慢变表（128 列，[-1,1]；×WARP_RANGE 得周期数）。
   *  确定性 LCG 生成、低频盒滤波，方向反转不换 seed（design §3.1）。 */
  private readonly warpBase: Float64Array = new Float64Array(128);
  /** ── E3 资源（§12：I2 双缓冲 + N1 噪声场 + 失效检测快照）── */
  /** N1 渲染专用噪声场（连续，无 64 周期；e3n1 消融时不用） */
  private e3Noise: P012E3NoiseField | null = null;
  /** N1 场覆盖维护门控：上次实际烘焙的窗口 x0（null=未烘焙/需强制） */
  private e3NoiseX0: number | null = null;
  /** I2 水体雾缓冲（waterTop..sandTop 域） */
  private e3WaterBuffer: P012E3FogBuffer | null = null;
  /** I2 界面带缓冲（sandTop±halfW 域） */
  private e3IfaceBuffer: P012E3FogBuffer | null = null;
  /** 失效检测：上一帧 smooth 数组快照（层高/waterTop 增量比较） */
  private readonly e3PrevSmooth: Float64Array[] = [];
  /** 失效检测：上一帧 q 列快照 */
  private e3PrevQH: Float64Array | null = null;
  private e3PrevQL: Float64Array | null = null;
  /** I2 烘焙/上传计数（单测钩子：静止态零重烘焙断言） */
  e3BakeCount: number = 0;
  e3UploadBytes: number = 0;
  /** ── E2 路线②原型：预烘焙光照图（128×Z_STEPS 单 float/格，Lface×w(z) 合成的 Lmat）── */
  /** 光照图（cols×Z_STEPS）：lightMap[c*Z+z] = Lmat(x_c, z)；长度 0=未分配（首帧构造） */
  private e2LightMap: Float64Array = new Float64Array(0);
  /** 上次烘焙的 sandTop 基线（形状变化检测） */
  private e2PrevSandTop: Float64Array = new Float64Array(0);
  /** 烘焙计数（单测钩子：静止零重烘/形状变化重烘断言） */
  e2BakeCount: number = 0;
  /** ── 运动代理（D-B2：相净搬运位移代理 + 连续 UV 平流）── */
  private readonly motionCache: P012MaterialMotionCache;
  private readonly cols: number;
  private readonly capacity: number;
  /** 最近一次 draw 提交计数（性能预检只读钩子；不进遥测） */
  lastDrawCount: number = 0;
  /** 最近一次 E/A 档（无颗粒）draw 提交计数，作为 B 档颗粒预算的基线 */
  lastDrawCountEa: number = 0;
  /** 最近一次 adapter 配对结论（只读诊断） */
  lastPairValid: boolean = false;
  private disposed: boolean = false;
  /** 一次性诊断日志闸门（避免逐帧刷日志） */
  private grainDiagLogged: boolean = false;
  /** §10.4 本帧消融开关（render() 入口从 frameContext.ablation 读入；
   *  外观诊断域：grain/fog/spark 分别跳过对应层，OFF=全部启用） */
  private frameAblation: P012Ablation = P012Ablation.OFF;
  // falling-sand B-route §3: the tracer spray (renderer-bystander; fed by
  // the page from injection receipts; occluded per frame).
  readonly sandTracer: P012SandTracer = new P012SandTracer();
  // Tracer occlusion sampler cache — rebuilt ONLY when boxW changes (the
  // underlying smooth* arrays are stable references; occlusion VALUES are
  // sampled per frame, this cache is geometry-only, not data caching).
  private cachedTracerSampler: P012MaterialColumnSampler | null = null;
  private cachedTracerBoxW = -1;
  private cachedTracerColW = 1;
  /** §10.4 本帧 fillRect 策略（TIGHT=精确平铺边界；FULL=旧整盒对照） */
  private frameFillBox: P012FillBox = P012FillBox.TIGHT;
  /** ── §13 参数可调化：active 外观配置 + staging（帧边界原子切换）── */
  /** 当前生效外观（六参数默认 = s133 冻结基线；f=c=1 恢复逐位一致） */
  private activeAppearance: P012AppearanceParams = p012FactoryAppearance();
  /** 待切换外观（§13.3：最多一个；render 入口消费后置 null） */
  private stagedAppearance: P012AppearanceParams | null = null;
  /** 外观烘焙计数（单测③重烘焙矩阵断言：fine/sigma→颗粒组+1，λ/强度→层纹组+1） */
  grainBakeCount: number = 0;
  stratBakeCount: number = 0;

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
    // C 层理（§11.1）：bandNoise 一维表 + warp 基线表一次性生成（确定性 seed，
    // 不调用/消耗物理随机源）；位图烘焙延迟到首帧（需 heightToY 逆变换系数）。
    this.buildStratNoise(this.stratNoiseH, 20261025);
    this.buildStratNoise(this.stratNoiseL, 20261026);
    this.buildStratNoise(this.stratNoiseM, 20261027);
    this.buildWarpBase(20261028);
    // E3（§12.4/12.1）：N1 场（渲染专用种子）+ I2 双缓冲（宽档 256，
    // boxW/boxH 变化时在 render 帧首重建）。快照数组初始化。
    this.e3Noise = new P012E3NoiseField(20261020);
    this.e3PrevSmooth.push(new Float64Array(cols), new Float64Array(cols),
      new Float64Array(cols), new Float64Array(cols));
  }

  /** 外观路线身份（独立视觉/性能证据 manifest 用） */
  get appearanceRouteId(): string {
    return P012_APPEARANCE_ROUTE_ID;
  }

  /**
   * 帧入口（§2）：同步读取页面刚完成 copyStateTo 的完整快照。
   * OFF/canonicalEvidence 由页面薄分发决定，不会进入本方法。
   */
  readonly particleLayer: P012ParticleLayer = new P012ParticleLayer();
  private particleWasActive: boolean = false;
  particlePresentedSource: string = 'NONE';
  private preparedResult: P012AdapterResult | null = null;
  private preparedContext: P012FrameContext | null = null;
  private preparedSnapshot: WaterSandSnapshot | null = null;
  prepareOnce(snapshot: WaterSandSnapshot, frameContext: P012FrameContext): void {
    if (this.preparedContext === frameContext && this.preparedSnapshot === snapshot) return;
    this.consumeStagedAppearance();
    this.preparedResult = this.adapter.update(snapshot, frameContext);
    this.preparedContext = frameContext; this.preparedSnapshot = snapshot;
  }
  renderBackground(ctx: CanvasRenderingContext2D): void {
    if (this.preparedContext !== null && this.preparedSnapshot !== null)
      this.renderPrepared(ctx, this.preparedSnapshot, this.preparedContext, 'background');
  }
  renderForeground(ctx: CanvasRenderingContext2D): void {
    if (this.preparedContext !== null && this.preparedSnapshot !== null)
      this.renderPrepared(ctx, this.preparedSnapshot, this.preparedContext, 'foreground');
  }
  render(ctx: CanvasRenderingContext2D, snapshot: WaterSandSnapshot, frameContext: P012FrameContext): void {
    this.prepareOnce(snapshot, frameContext);
    this.renderPrepared(ctx, snapshot, frameContext, 'legacy');
  }
  private renderPrepared(ctx: CanvasRenderingContext2D, snapshot: WaterSandSnapshot,
    frameContext: P012FrameContext, stage: 'legacy' | 'background' | 'foreground'): void {
    if (this.disposed) {
      return;
    }
    let drawCount = 0;
    // §13.3 帧边界原子切换：staged → active（applyAppearance 后的首帧生效）
    this.consumeStagedAppearance();
    const appF = this.activeAppearance.fogDensityScale;
    const appC = this.activeAppearance.clearMottleContrast;
    // §10.4 消融开关/fillRect 策略：单点读入，各绘制点只读该状态
    this.frameAblation = frameContext.ablation;
    this.frameFillBox = frameContext.fillBox;
    const result = this.preparedResult;
    if (result === null) return;
    this.lastPairValid = result.pair.valid;
    const view = result.curr;
    if (view === null) {
      // curr 非有限：不替换合法 prev；保留最后合法显示 = 不绘制（页面背景已清）
      this.lastDrawCount = 0;
      return;
    }
    // ── 显示 mask：承接既有平滑（4 遍 3 点平均），只用于显示几何 ──
    if (stage !== 'foreground') this.buildDisplayProfiles(view);
    const n = this.cols;
    const boxX = frameContext.boxX;
    const boxY = frameContext.boxY;
    const boxW = frameContext.boxW;
    const boxH = frameContext.boxH;
    const colW = boxW / n;
    // B-route: tracer sampler geometry cache (rebuild on boxW change only).
    if (this.cachedTracerBoxW !== boxW) {
      this.cachedTracerBoxW = boxW;
      this.cachedTracerColW = colW;
      this.cachedTracerSampler = new P012MaterialColumnSampler(
        this.smoothBh, this.smoothMixStart, this.smoothSandTop, boxW);
    }
    // E2 原型：pxPerMaterial 缓存（光照图形状阈值/深度换算用）
    this.pxPerMaterialCache = boxH > 0 ? boxH / this.capacity : 0;
    // ── 运动代理（§5.2）：配对有效且有 prev 视图才更新；否则冻结 ──
    const motionValid = result.pair.valid && result.prev !== null;
    if (stage !== 'foreground') this.motionCache.update(
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
    //    F1 消融：ablation=DYN 跳过静态层（底色渐变/雾/高光/层理/颗粒/边框）
    if (stage !== 'foreground' && this.frameAblation !== P012Ablation.DYN) {
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
    }
    // 2) A 档水体雾（EA-batch，算法稿 §4.2）：悬浮浓度 q + 静态水介质斑驳。
    //    只画水体区（sandTop 以上）；界面过渡带在沙体之后画（见步骤 5）。
    //    §10.4 消融：ablation=FOG 时跳过雾层（水体雾 + 界面带同属 A 档雾层）。
    //    §12 E3：默认走 I2 光学缓冲（连续雾，无 64 周期/12 档阶梯）；
    //    ablation=E3I2 回旧 fillFogTiers 12 档路径（消融对照）。
    const tier = frameContext.appearanceTier;
    if (stage !== 'foreground' && (tier === P012AppearanceTier.EA || tier === P012AppearanceTier.EAB) &&
      this.frameAblation !== P012Ablation.FOG &&
      this.frameAblation !== P012Ablation.DYN) {
      if (this.frameAblation !== P012Ablation.E3I2) {
        drawCount += this.e3DrawFogBuffers(ctx, heightToY, colW,
          boxX, boxY, boxW, boxH, view, false);
      } else {
        drawCount += this.drawWaterFog(ctx, view, heightToY, colW,
          boxX, boxY, boxW, boxH);
      }
    }
    // 3) 沙体三带：重沙带（bh 以下）/ 轻沙带（mixStart→bh）/ 混合带（sandTop→mixStart）
    //    逐列伪法向光照：s = d(sandTop)/dx（屏幕单位），L = La + Ld·max(dot(n,l),0)
    //    用 8 档游程近似逐列光照（run 合并相邻同档列，draw 计数受控）
    //    A 档启用时沙顶 3px 硬描边语义由界面过渡带取代（P-012 分支无硬描边）
    if (stage !== 'foreground' && !frameContext.dualFieldGpu) drawCount += this.drawSandColumns(ctx, view, heightToY, colW, boxX, boxY, boxH);
    // 3.25) C 层理（C-batch §11，C1 分段平铺 + C3 低频明暗基底）：
    //    时序按 design §3.1（E底色 → C层纹 → B颗粒 → A散射），C 只在 EAB 档
    //    叠加（不新增 tier 枚举）；§11.4 消融 ablation=STRAT 跳过。
    if (stage !== 'foreground' && !frameContext.dualFieldGpu && tier === P012AppearanceTier.EAB && this.frameAblation !== P012Ablation.STRAT &&
      this.frameAblation !== P012Ablation.DYN) {
      drawCount += this.drawStratification(ctx, view, heightToY, colW,
        boxX, boxY, boxW, boxH, frameContext.geometryEpoch);
    }
    // 3.5) B 档粉质颗粒（EAB-batch，算法稿 §5.1）：三带各自 clip 到沙区曲线 Path +
    //      drawImage(256² 颗粒 tile) 按全局平流偏移精确平铺；跟随材料净搬运、
    //      无缝、无贴纸。§10.4 消融：ablation=GRAIN 时跳过颗粒层。
    if (stage !== 'foreground' && !frameContext.dualFieldGpu && tier === P012AppearanceTier.EAB && this.frameAblation !== P012Ablation.GRAIN &&
      this.frameAblation !== P012Ablation.DYN) {
      drawCount += this.drawSandGrain(ctx, view, heightToY, colW, boxX, boxY, boxW, boxH);
    }
    // 3.75) falling-sand B-route §3: tracer 喷流（B 颗粒之后、液面高光之前）
    //       —— 全程水区（遮蔽保证），不遮挡沙；ablation=TRACER 跳过
    //       （锚点测量 OFF 臂）。≤2 draw（H/L 群）。
    if (stage !== 'background') {
    const particles = frameContext.particleSnapshot;
    const particleActive = particles !== undefined && particles.active;
    if (particleActive && !this.particleWasActive) this.sandTracer.clear();
    this.particleWasActive = particleActive;
    this.particlePresentedSource = 'NONE';
    if (particles !== undefined && particleActive && !frameContext.dualFieldGpu) {
      drawCount += this.particleLayer.draw(particles, frameContext, new RendererTracerOcclusion(this, heightToY),
        new RendererTracerDrawContext(ctx), this.capacity);
      if (this.particleLayer.drawnCount > 0) this.particlePresentedSource = 'C_PARTICLE';
    } else if (!particleActive && frameContext.sandTracerEnabled &&
      this.frameAblation !== P012Ablation.TRACER) {
      this.particlePresentedSource = 'B_TRACER';
      drawCount += this.drawSandTracers(ctx, view, heightToY,
        colW, boxX, boxY, boxW, boxH, frameContext);
    }
    // 4) 液面高光：沿 waterTop 的宽软带（§3.1 柔和高光，alpha≤0.12）（1 draw）
    //    F1 消融：DYN 跳过
    if (this.frameAblation !== P012Ablation.DYN) {
      ctx.beginPath();
      ctx.moveTo(boxX, heightToY(this.smoothWaterTop[0]));
      for (let c = 1; c < n; c++) {
        ctx.lineTo(boxX + c * colW, heightToY(this.smoothWaterTop[c]));
      }
      ctx.strokeStyle = 'rgba(255,255,255,0.12)';
      ctx.lineWidth = 5;
      ctx.stroke();
      drawCount++;
    }
    // 5) A 档界面光学过渡带（沙体之后绘制）：sandTop 两侧 ±halfW 内
    //    smoothstep 包络雾——沙侧上缘被雾柔和覆盖，消除沙体 fillRect 的硬边。
    //    §10.4 消融：与步骤 2 同属 A 档雾层，ablation=FOG 时一并跳过。
    //    §12 E3：默认走 I2 interface buffer（e3DrawFogBuffers 第二次调用，
    //    只画界面带）；E3I2 消融回旧 12 档。
    if ((tier === P012AppearanceTier.EA || tier === P012AppearanceTier.EAB) &&
      this.frameAblation !== P012Ablation.FOG) {
      if (this.frameAblation !== P012Ablation.E3I2) {
        drawCount += this.e3DrawFogBuffers(ctx, heightToY, colW,
          boxX, boxY, boxW, boxH, view, true);
      } else {
        drawCount += this.drawInterfaceBand(ctx, view, heightToY, colW,
          boxX, boxY, boxW, boxH);
      }
    }
    // 6) 容器边框（与原渲染一致的白框；F1 消融 DYN 跳过）
    if (this.frameAblation !== P012Ablation.DYN) {
      ctx.strokeStyle = 'rgba(255,255,255,0.35)';
      ctx.lineWidth = 3;
      ctx.strokeRect(boxX, boxY, boxW, boxH);
      drawCount++;
    }
    }
    ctx.restore();
    this.lastDrawCount = stage === 'foreground' ? this.lastDrawCount + drawCount : drawCount;
    // 记录无颗粒基线（E/A 档）：B 档颗粒预算钩子用它算净增量
    if (tier !== P012AppearanceTier.EAB) {
      this.lastDrawCountEa = drawCount;
    }
  }

  // ───────────── §13 参数可调化：applyAppearance（架构 §13.7.5 资源接口签名）─────────────

  /**
   * 应用外观配置（§13.3 草稿→校验→应用生命周期）。
   * 返回 null=成功（staged 待帧边界切换）；返回原因串=拒绝整包（维持旧包）。
   * 唯一消费点：f/c 进雾链、幅度 scale 进层理、fine/sigma 进颗粒烘焙（§13.7.2）。
   * pxPerMaterial ≤0 时跳过 λ 去重表校验（首帧几何未定场景，几何定值后
   * buildStratBitmaps 用 effectiveλ 兜底）。
   */
  applyAppearance(next: P012AppearanceParams, pxPerMaterial: number): string | null {
    const err = validateP012AppearanceParams(next, pxPerMaterial);
    if (err !== null) {
      return err;                       // 拒绝整包：不 clamp 不部分应用
    }
    // diff 检测（§13.7.5-2）：无变化直接成功
    const cur = this.activeAppearance;
    if (next.grainFeaturePx === cur.grainFeaturePx &&
      next.grainSigma === cur.grainSigma &&
      next.fogDensityScale === cur.fogDensityScale &&
      next.strataLambdaMaterial === cur.strataLambdaMaterial &&
      next.strataStrengthScale === cur.strataStrengthScale &&
      next.clearMottleContrast === cur.clearMottleContrast) {
      // 仅 source/configRevision 变化（恢复推荐等）——只更新元数据
      this.stagedAppearance = next;
      return null;
    }
    // 按重烘焙矩阵 stage（§13.4）：staging 阶段在应用回调内同步完成重活；
    // 任一步失败 → 维持旧包返回错误串（§13.3 失败分支）
    try {
      if (next.grainFeaturePx !== cur.grainFeaturePx ||
        next.grainSigma !== cur.grainSigma) {
        this.stageGrain(next);          // fine/sigma → 三张 256² 颗粒位图组
      }
      if (next.strataLambdaMaterial !== cur.strataLambdaMaterial ||
        next.strataStrengthScale !== cur.strataStrengthScale) {
        this.stageStrata(next, pxPerMaterial);   // λ/强度 → 三张 64×256 层纹组
      }
      // f/c → E3 雾缓冲失效（不触发 B/C 位图重烘——§13.4 矩阵保证）
      if (next.fogDensityScale !== cur.fogDensityScale ||
        next.clearMottleContrast !== cur.clearMottleContrast) {
        if (this.e3WaterBuffer !== null) {
          this.e3WaterBuffer.dirty = true;
        }
        if (this.e3IfaceBuffer !== null) {
          this.e3IfaceBuffer.dirty = true;
        }
      }
    } catch (e) {
      // staging 失败：staged 置空（staging 区资源交 GC），维持 active
      this.stagedAppearance = null;
      return `外观资源烘焙失败：${JSON.stringify(e)}`;
    }
    this.stagedAppearance = next;       // 最多一个待替换包（§13.3：新包覆盖旧 staging）
    return null;
  }

  /** 颗粒组 staging：同 seed 重生成 tile（保归一化/截幅/envelope=2×fine 关系） */
  private stageGrain(next: P012AppearanceParams): void {
    const prevFine = this.activeAppearance.grainFeaturePx;
    if (next.grainFeaturePx !== prevFine) {
      // fine 档变化：重生成 tile（盒滤波半径 R=fine/2；envelope=2×fine）
      this.grainTileH = this.buildGrainTileFine(20261022, next.grainFeaturePx);
      this.grainTileL = this.buildGrainTileFine(20261023, next.grainFeaturePx);
      this.grainTileM = this.buildGrainTileFine(20261024, next.grainFeaturePx);
    }
    // sigma 变化或 fine 变化都重烘位图（delta = tile×sigma 在位图内）
    this.buildGrainBitmapsFor(this.activeAppearance.grainSigma !== next.grainSigma
      ? next.grainSigma : this.activeAppearance.grainSigma);
    this.grainBakeCount++;
  }

  /** 层纹组 staging：λ 重算 effectiveλ（整周期对齐）+ 幅度 scale 重烘 */
  private stageStrata(next: P012AppearanceParams, pxPerMaterial: number): void {
    // λ 与强度都烘进位图（幅度一致性优先——§13.4 矩阵注记）
    this.buildStratBitmapsFor(next.strataLambdaMaterial,
      next.strataStrengthScale, pxPerMaterial);
    this.stratBakeCount++;
  }

  /** buildGrainTile 的参数化版（fine 档 → 盒滤波半径 R=fine/2；envelope=2×fine）。
   *  候选稿 §2.1「不靠降分辨率模拟粗颗粒」——同 seed 重生成，保留归一化链。 */
  private buildGrainTileFine(seed: number, finePx: number): Float64Array {
    const size = P012_GRAIN_TILE;
    const fineA = new Float64Array(size * size);
    const fineB = new Float64Array(size * size);
    const env = new Float64Array(size * size);
    const envPx = 2 * finePx;           // envelope = 2×fine（候选稿 §2）
    this.buildGrainNoise(fineA, (seed | 0) + 11, finePx);
    this.buildGrainNoise(fineB, (seed | 0) + 23, finePx);
    this.buildGrainNoise(env, (seed | 0) + 37, envPx);
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

  /** buildGrainBitmaps 的参数化版（sigma 直接替换常量位） */
  private buildGrainBitmapsFor(sigma: number): void {
    this.materialAssetExport.clear('grain');
    try {
      if (typeof OffscreenCanvas === 'undefined' || OffscreenCanvas === null) {
        return;                          // Node 通道：tile 已更新，bitmap 留 null
      }
    } catch (e) {
      return;
    }
    const p = this.palette;
    this.grainBitmapH = this.makeGrainBitmapFor(this.grainTileH, sigma, p.heavyR, p.heavyG, p.heavyB);
    this.grainBitmapL = this.makeGrainBitmapFor(this.grainTileL, sigma, p.lightR, p.lightG, p.lightB);
    this.grainBitmapM = this.makeGrainBitmapFor(this.grainTileM, sigma,
      (p.lightR + p.heavyR) * 0.5, (p.lightG + p.heavyG) * 0.5, (p.lightB + p.heavyB) * 0.5);
  }

  /** makeGrainBitmap 的参数化版 */
  private makeGrainBitmapFor(tile: Float64Array, sigma: number,
    baseR: number, baseG: number, baseB: number): ImageBitmap | null {
    const size = P012_GRAIN_TILE;
    try {
      const off = new OffscreenCanvas(size, size);
      const octx = off.getContext('2d');
      if (octx === null || octx === undefined) {
        return null;
      }
      const img = octx.createImageData(size, size);
      const data = img.data;
      for (let i = 0; i < size * size; i++) {
        const delta = tile[i] * sigma;
        data[i * 4] = this.clamp255(baseR + delta);
        data[i * 4 + 1] = this.clamp255(baseG + delta);
        data[i * 4 + 2] = this.clamp255(baseB + delta);
        data[i * 4 + 3] = 255;
      }
      if (this.materialExportEnabled) this.materialAssetExport.capture('grain', tile === this.grainTileH ? 0 : tile === this.grainTileL ? 1 : 2, data);
      octx.putImageData(img, 0, 0);
      return off.transferToImageBitmap();
    } catch (e) {
      return null;
    }
  }

  /** buildStratBitmaps 的参数化版（nominal λ + 强度 scale；整周期对齐不动） */
  private buildStratBitmapsFor(nominalLambda: number, strengthScale: number,
    pxPerMaterial: number): void {
    this.materialAssetExport.clear('strata');
    if (pxPerMaterial <= 0) {
      return;                            // 几何未定：延迟到首帧 buildStratBitmaps 兜底
    }
    const k = Math.max(1, Math.ceil(P012_C_BITMAP_H / (nominalLambda * pxPerMaterial)));
    this.stratLamPx = P012_C_BITMAP_H / k;
    try {
      if (typeof OffscreenCanvas === 'undefined' || OffscreenCanvas === null) {
        return;                          // Node 通道：stratLamPx 已更新即可
      }
    } catch (e) {
      return;
    }
    const ampOrdered = P012_C_AMP_ORDERED * strengthScale;
    const ampMixed = P012_C_AMP_MIX * strengthScale;
    this.stratBitmapH = this.makeStratBitmap(this.stratNoiseH, ampOrdered);
    this.stratBitmapL = this.makeStratBitmap(this.stratNoiseL, ampOrdered);
    this.stratBitmapM = this.makeStratBitmap(this.stratNoiseM, ampMixed);
  }

  /** 帧边界消费 staging（render 入口调用；§13.3 原子切换） */
  private consumeStagedAppearance(): void {
    if (this.stagedAppearance === null) {
      return;
    }
    this.activeAppearance = this.stagedAppearance;
    this.stagedAppearance = null;
    // 旧位图引用已在上一次 stage 中被覆盖（无引用交 GC）；
    // E3 雾缓冲 dirty 已在 applyAppearance 标记（若 f/c 变化）。
  }

  /** 外观配置只读钩子（UI 显示/持久化/manifest 用） */
  getAppearanceConfigRevision(): number {
    return this.activeAppearance.configRevision;
  }

  // ───────────── E2 路线②原型：预烘焙光照图（algorithm-P012-E2 §2/§4-②/§5）─────────────

  /**
   * E2 光照图烘焙（§2 公式）：
   * 1) 宽面坡度 p_b(x)=∂x G_σ*S（σ 光学宽面尺度——对 smoothSandTop 低通，
   *    不改物理表面）；
   * 2) 截面法线 n(x)=normalize(-p_b,0,1)（1D 截面风格代理，开题已登记）；
   * 3) Lambert：Lface(x)=clamp(La+Ld·max(0,n·ℓ), Lmin, Lmax)（复用既有
   *    P012_LIGHT_X/Y/Z 光向——含横向分量，反向坡面可区分）；
   * 4) 深度衰减合成：Lmat(x,z)=1+w(z)·(Lface(x)-Lref)，w(z)=exp(-z/ℓ)，
   *    z=显示沙面到该深度的材料深度；Lref=1（同画面参考漫反射，防无条件提亮）。
   * 失效：sandTop 形状变化 >P012_E2_SHAPE_EPS_PX（§5：形状变更→重烘；
   * 不为颗粒相位/雾浓度重烘坡面光照）。
   */
  private e2EnsureLightMap(): void {
    const n = this.cols;
    if (this.e2LightMap.length !== n * P012_E2_Z_STEPS) {
      this.e2LightMap = new Float64Array(n * P012_E2_Z_STEPS);
      this.e2PrevSandTop = new Float64Array(n);
      this.e2PrevSandTop.fill(NaN);       // 首帧必烘
    }
    // 形状变化检测（显示 px 域）
    let shapeChanged = false;
    for (let c = 0; c < n && !shapeChanged; c++) {
      const px = this.smoothSandTop[c] * this.pxPerMaterialCache;
      if (!Number.isFinite(this.e2PrevSandTop[c]) ||
        Math.abs(px - this.e2PrevSandTop[c]) > P012_E2_SHAPE_EPS_PX) {
        shapeChanged = true;
      }
    }
    if (!shapeChanged && this.e2BakeCount > 0) {
      return;                             // 静止/微变：零重烘（§5 静置零无故重烘）
    }
    // 1) G_σ 低通（σ 材料深度单位 → 离散核半径；简单盒滤波×2 近似高斯）
    const rad = Math.max(1, Math.round(P012_E2_SIGMA));
    const low = new Float64Array(n);
    for (let c = 0; c < n; c++) {
      let sum = 0;
      let cnt = 0;
      for (let k = -rad; k <= rad; k++) {
        const cc = Math.max(0, Math.min(n - 1, c + k));
        sum += this.smoothSandTop[cc];
        cnt++;
      }
      low[c] = sum / cnt;
    }
    // 2)+3)+4) 逐列 Lface → 逐深度 Lmat（z=材料深度单位，w(z)=exp(-z/ℓ)）
    for (let c = 0; c < n; c++) {
      const left = low[Math.max(0, c - 1)];
      const right = low[Math.min(n - 1, c + 1)];
      // 宽面坡度（材料深度/列）：∂x G_σ*S（相邻列差/2，材料单位）
      const pb = (right - left) / 2;
      const inv = 1 / Math.sqrt(pb * pb + 1);
      const dot = (-pb * inv) * P012_LIGHT_X + (inv) * P012_LIGHT_Z;
      const lit = P012_AMBIENT + P012_DIFFUSE * Math.max(0, dot);
      const lface = Math.max(P012_E2_LMIN, Math.min(P012_E2_LMAX, lit));
      for (let z = 0; z < P012_E2_Z_STEPS; z++) {
        const wz = Math.exp(-z / P012_E2_ELL);
        this.e2LightMap[c * P012_E2_Z_STEPS + z] = 1 + wz * (lface - 1);
      }
      this.e2PrevSandTop[c] = this.smoothSandTop[c] * this.pxPerMaterialCache;
    }
    this.e2BakeCount++;
  }

  /** E2 光照图采样：列 c、深度 z（材料单位）→ Lmat（TOPLIGHT ON 用） */
  private e2SampleLight(c: number, z: number): number {
    const zz = Math.max(0, Math.min(P012_E2_Z_STEPS - 1, Math.round(z)));
    const v = this.e2LightMap[c * P012_E2_Z_STEPS + zz];
    return Number.isFinite(v) ? v : 1;
  }

  /** pxPerMaterial 缓存（e2 烘焙的形状阈值与深度换算用；render 期更新） */
  private pxPerMaterialCache: number = 0;

  /** effectiveλ 只读钩子（§13.7.3 双登记的 effective 侧；几何未定时 = 名义值） */
  getEffectiveStrataLambda(): number {
    return this.stratLamPx > 0 ? this.stratLamPx : this.activeAppearance.strataLambdaMaterial;
  }

  /** 当前 active 外观（只读副本语义：调用方不得回写） */
  get activeAppearanceParams(): P012AppearanceParams {
    return this.activeAppearance;
  }

  /**
   * 3.75 slot: advance + draw the tracer spray. The tracer updates on
   * the renderer's frame clock (dt from frameContext), moves along the
   * display gravity vector, and is occluded per point by the three
   * segments (waterTop clip / fog band smoothstep / sandTop hide — all
   * sampled THIS frame, never cached).
   */
  private drawSandTracers(ctx: CanvasRenderingContext2D,
    view: P012MaterialFrameView,
    heightToY: (h: number) => number, colW: number, boxX: number,
    boxY: number, boxW: number, boxH: number,
    frameContext: P012FrameContext): number {
    const dtSeconds = frameContext.frameDeltaMs / 1000;
    const occlusion = new RendererTracerOcclusion(this, heightToY);
    this.sandTracer.update(dtSeconds, frameContext.gravityDirX,
      frameContext.gravityDirY, occlusion);
    if (this.sandTracer.activeCount() === 0) {
      return 0;
    }
    const drawCtx = new RendererTracerDrawContext(ctx);
    // Candidate colors: heavy = deep sand tone, light = pale sand tone
    // (tuning NOT frozen — pending effect calibration).
    return this.sandTracer.draw(drawCtx, occlusion,
      'rgba(196,168,120,1.0)', 'rgba(226,208,168,1.0)', 2.0);
  }

  /** B-route: sampler access for the file-scope occlusion adapter
   * (geometry-cached; data sampled per frame). */
  get tracerSamplerRef(): P012MaterialColumnSampler {
    return this.cachedTracerSampler as P012MaterialColumnSampler;
  }

  /** waterTop at display x (linear column interpolation, matching the
   * sampler's split semantics). */
  tracerWaterTopAtPx(x: number): number {
    const s = x / this.cachedTracerColW;
    const cx = Math.max(0, Math.min(this.cols - 1e-6, s));
    let i = Math.floor(cx);
    let frac = cx - i;
    if (i >= this.cols - 1) {
      i = this.cols - 2;
      frac = 1;
    }
    if (this.cols <= 1) {
      return this.smoothWaterTop[0];
    }
    return this.smoothWaterTop[i] * (1 - frac) +
      this.smoothWaterTop[i + 1] * frac;
  }

  /** 生命周期：清连续性配对（reset/inject/invert/后台恢复时页面通知）。
   *  §12.6：aPhaseX 清零 + I2 双 buffer dirty（防旧场景纹理相位泄漏）；
   *  N1 场不重建（seed 与格点 hash 绑定，泄漏的是相位不是纹理）。 */
  resetContinuity(): void {
    this.preparedResult = null; this.preparedContext = null; this.preparedSnapshot = null;
    this.adapter.dispose();
    this.motionCache.reset();
    // B-route §4: reset/invert clears all in-flight tracers (the ring
    // rides along — bounded, never replayed).
    this.sandTracer.clear();
    this.aPhaseX = 0;
    this.e3NoiseX0 = null;
    if (this.e3WaterBuffer !== null) {
      this.e3WaterBuffer.dirty = true;
    }
    if (this.e3IfaceBuffer !== null) {
      this.e3IfaceBuffer.dirty = true;
    }
    // 快照基线置 NaN：下帧必触发重烘（层高比较 |Δ|>1e-9 对 NaN 恒真）
    for (let p = 0; p < 4; p++) {
      this.e3PrevSmooth[p].fill(NaN);
    }
  }

  /** dispose：断开全部引用（aboutToDisappear 调用；幂等） */
  dispose(): void {
    if (this.disposed) {
      return;
    }
    this.disposed = true;
    this.sandTracer.clear();
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

  /** B 档颗粒自身新增 draw 预算断言（单测钩子：≤48 次提交）。
   *  取EAB 总额与 EA 总额之差（两者渲染路径除颗粒外完全一致），即颗粒净增量。
   *  lastDrawCountEa 为最近一帧 E/A 档（无颗粒）的提交数。 */
  withinBBatchDrawBudget(): boolean {
    return (this.lastDrawCount - this.lastDrawCountEa) <= P012_B_BATCH_DRAW_BUDGET;
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
    this.materialAssetExport.clear('grain');
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
      if (this.materialExportEnabled) this.materialAssetExport.capture('grain', tile === this.grainTileH ? 0 : tile === this.grainTileL ? 1 : 2, data);
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
   * B 档粉质颗粒（算法稿 §5.1；PERF 优化版 s99 —— clip + drawImage 平铺）。
   *
   *  纹理语义与 v1 完全一致：颗粒仅叠加在 E 已提亮的材质上（globalAlpha=
   *  P012_B_GRAIN_ALPHA），并按材料净搬运的全局位移做水平平流；每带用
   *  各自的 256² 加性颗粒 tile（重/轻/混三套种子），pattern 连续无拼缝。
   *
   *  与 v1 的像素等价性：v1 为「clip(带曲线 Path) + createPattern(repeat)
   *  + fillRect(整盒 ±tilePx)」；本版为「clip(同一带曲线 Path) +
   *  drawImage(同一 256² 位图) 逐格平铺」。
   *   · 图案相位：v1 在 translate(off) 之后以画布坐标铺 pattern，采样相位
   *     = 屏幕 x - off；本版把位图按 off + k·256 摆放，屏幕 x 命中的位图
   *     内坐标 = x - off，与 v1 相位逐点一致。
   *   · 纵向：v1 的 fillRect 覆盖 boxY ± tilePx，pattern 纵向相位为
   *     屏幕 y（mod 256）；本版按 j·256 逐行摆放，纵向相位同为 y（mod 256）。
   *   · 强度：v1 走 globalAlpha（P012_B_GRAIN_ALPHA）做 source-over 混合；
   *     本版同样走 globalAlpha，且位图保持纯不透明（alpha=255）——
   *     绝不可把 alpha 烘焙进位图：CanvasPattern 采样会把半透明像素
   *     的颜色按 1/alpha 放大并钳位（s94 实测 alpha=140 → ×1.82 饱和白）。
   *
   *  性能（s98/s99 实测）：瓶颈在 CanvasPattern 的逐像素采样/混合，
   *  而非 Path 光栅化或 draw 提交数（消融 D1：去掉 pattern 后 tier3 由
   *  20/17/16 回到 34/34/36；消融 D2：仅把 globalAlpha 提到 1.0 时
   *  tier3 仍为 16/15/18 → 证实 alpha 混合路径不是瓶颈）。
   *  改用 drawImage 平铺后 tier3 = 32/32/33，与 tier2（35/38/50）同档，
   *  消除 EAB 档 60→12fps 的退化。
   *
   *  返回 draw 提交次数（三带各 1 次 clip + 若干 drawImage）。
   */
  private drawSandGrain(ctx: CanvasRenderingContext2D, view: P012MaterialFrameView,
    heightToY: (h: number) => number, colW: number,
    boxX: number, boxY: number, boxW: number, boxH: number): number {
    const n = this.cols;
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
      const tile = b.bitmap;
      if (tile === null) {
        // Node / 无 bitmap：fallback 不绘制颗粒（与 v1 一致）
        if (!this.grainDiagLogged) {
          this.grainDiagLogged = true;
          hilog.error(0x0000, 'P012', 'grain: bitmap null, skip band ' + b.kind);
        }
        continue;
      }
      // 带的实际屏幕纵向范围（空带直接跳过：高度 ≤0.5px 无可填充面积）
      let minY = Number.MAX_VALUE;
      let maxY = -Number.MAX_VALUE;
      for (let c = 0; c < n; c++) {
        const t = this.bandTopY(b.kind, c, heightToY);
        const bo = this.bandBottomY(b.kind, c, heightToY);
        if (t < minY) {
          minY = t;
        }
        if (bo > maxY) {
          maxY = bo;
        }
      }
      if (maxY - minY <= 0.5) {
        continue;
      }
      ctx.save();
      // clip 到该带沙区曲线 Path（绝对屏幕坐标，与 v1 完全一致）
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
      // 颗粒叠加强度走 globalAlpha（位图纯不透明）
      ctx.globalAlpha = P012_B_GRAIN_ALPHA;
      // 平铺：横向按 off + k·256 摆放（图案相位 = 屏幕 x - off，与 v1 一致），
      // 纵向按 j·256 摆放（相位 = 屏幕 y mod 256，与 v1 一致）。
      // 只覆盖带的实际纵向范围，避免无效提交。
      // dx 取整到整数像素：drawImage 的小数目标坐标会触发双线性重采样
      // （会平滑掉颗粒噪声、压低 hfH 锚点），整数坐标为 1:1 直贴。
      const off = Math.round(b.off);
      // §10.4 fillRect 策略：
      //   TIGHT（默认）：精确平铺边界——只摆能覆盖 [boxX, boxX+boxW] ×
      //     [minY, maxY] 的 tile。drawImage 是 1:1 直贴，不像 pattern 采样
      //     需要四周外扩余量，外扩纯属无效提交（被 clip 裁掉）。
      //   FULL（对照）：v1 旧口径——四周各外扩一整圈 tile。输出像素集合与
      //     TIGHT 严格相同（外扩 tile 全部被带曲线 clip 裁掉），仅 draw 提交
      //     数不同，用于 §10.5 消融验证「视觉逐位一致 + 性能对比」。
      let k0 = Math.floor((boxX - off) / P012_GRAIN_TILE);
      let k1 = Math.floor((boxX + boxW - off) / P012_GRAIN_TILE);
      let j0 = Math.floor(minY / P012_GRAIN_TILE);
      let j1 = Math.ceil(maxY / P012_GRAIN_TILE) - 1;
      if (this.frameFillBox === P012FillBox.FULL) {
        k0 -= 1;
        k1 += 1;
        j0 -= 1;
        j1 += 1;
      }
      for (let k = k0; k <= k1; k++) {
        const dx = off + k * P012_GRAIN_TILE;
        for (let j = j0; j <= j1; j++) {
          ctx.drawImage(tile, dx, j * P012_GRAIN_TILE, P012_GRAIN_TILE, P012_GRAIN_TILE);
          draws++;
        }
      }
      ctx.restore();
    }
    if (!this.grainDiagLogged) {
      this.grainDiagLogged = true;
      hilog.info(0x0000, 'P012', 'grain: drawImage tile path used (all bands ok)');
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

  // ───────────────────── C 层理（C-batch §11；C1+C3） ─────────────────────

  /**
   * bandNoise 一维表（§11.1）：零均值、有界 [-1,1] 的连续带纹（纵向）。
   * 相邻周期轻微不规则（白噪声 → 盒滤波 → 去均值 → 峰值归一），可平铺
   * （表长 = 位图高 256 = λpx 的整数周期数 → 环形采样无缝）。
   * 禁止逐列独立随机相位（design §3.1）——本表只在烘焙时查一次。
   */
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

  /** warp 基线表：128 列 [-1,1] 低频慢变（横向特征尺度 ~32 列）。
   *  运行期 warp(x) = warpBase[col(x − acceptedUV)] × WARP_RANGE（周期数）。
   *  acceptedUV 仅在 pairValid/可信材料位移时推进（motionCache），否则冻结。
   *  慢变约束（§11.1 分段阈值 1/16 周期的前提）：相邻列差必须远小于
   *  1/16÷WARP_RANGE=1/4 表值 → 盒滤波后再做 6 遍 3 点平滑（与
   *  buildDisplayProfiles 同法），把白噪声残量压到逐列近线性。 */
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

  /** 一维连续噪声采样（环形表线性插值）：t 任意实数相位 → 表内连续值。
   *  表 = 一个「噪声周期」的 bandNoise 波形（含周期内不规则带距）；
   *  位图纵向 u 与表相位经 λpx=H/k 对齐 → 平铺严格无缝。 */
  private stratNoiseAt(table: Float64Array, t: number): number {
    const n = table.length;
    const f = t * n;
    const i0 = ((Math.floor(f) % n) + n) % n;
    const i1 = (i0 + 1) % n;
    const frac = f - Math.floor(f);
    return table[i0] * (1 - frac) + table[i1] * frac;
  }

  /**
   * C 层理灰度位图烘焙（§11.1 选型 B）：
   *   像素(v, u) = 128 + envelope(u) × amp × bandNoise(seed, u/λpx + (v/64)×WARP_RANGE)
   * - 纵向 u = 深度相位（px），λpx = λ_material × pxPerMaterial（render 期传入）
   * - 横向 v = warp 相位轴 [0, 1/4 周期]（64px 内完整表达 warp 慢变）
   * - envelope：位图纵向边缘平滑衰减（带边界；cos² 半周期边沿）
   * - 纯不透明 alpha=255，RGB 同值（灰度单通道语义）——增量在运行期经
   *   globalAlpha 混合，绝不烘焙 alpha（s94 教训）
   * - λ 冻结后重烘焙：geometry epoch 变化时按新 scale 重烘（epoch 钩子，
   *   复用 B 档 buildGrainBitmaps 模式）
   */
  private buildStratBitmaps(pxPerMaterial: number): void {
    if (pxPerMaterial <= 0) {
      return;
    }
    // λpx 对齐到「位图高的整数分割」：H = k×λpx（k=ceil(256/λpx)）。
    // 位图纵向按 256…H 平铺时相位严格连续（λpx 原始值 24×pxPerMaterial 与
    // H/k 的偏差 <1%，外观域可接受；§11.3 风险点「λ 换算」的工程化）。
    // §13：λ/强度从 activeAppearance 读（参数可调化；默认 = s133 常量基线）
    this.buildStratBitmapsFor(this.activeAppearance.strataLambdaMaterial,
      this.activeAppearance.strataStrengthScale, pxPerMaterial);
  }

  /** 单张 C 位图：64×256 灰度（横向 warp 相位 × 纵向深度相位）。
   *  λpx = H/k（k=ceil(256/λpx原始)）→ u∈[0,H) 相位∈[0,k) 整数圈
   *  → 环形噪声表线性插值在 u=H 与 u=0 处同相位 → 平铺严格无缝。 */
  private makeStratBitmap(noise: Float64Array, amp: number): ImageBitmap | null {
    const w = P012_C_BITMAP_W;
    const h = P012_C_BITMAP_H;
    try {
      const off = new OffscreenCanvas(w, h);
      const octx = off.getContext('2d');
      if (octx === null || octx === undefined) {
        hilog.error(0x0000, 'P012', 'strat: getContext(2d) returned null/undefined');
        return null;
      }
      const img = octx.createImageData(w, h);
      const data = img.data;
      for (let v = 0; v < w; v++) {
        // 该横向列的 warp 相位偏移（v/64 × 1/4 周期，周期数）
        const warpCycles = (v / w) * P012_C_WARP_RANGE;
        for (let u = 0; u < h; u++) {
          // 深度相位（周期数）：u/λpx + warp
          const phase = u / this.stratLamPx + warpCycles;
          // 带边界 envelope（位图纵向边缘线性衰减，半宽 = λpx/2）
          const edge = Math.min(u, h - 1 - u) / (this.stratLamPx * 0.5);
          const env = Math.max(0, Math.min(1, edge));
          // 连续噪声：相位（周期数）→ 表相位（表长=1 噪声周期 256 相位单位）
          const delta = env * amp * this.stratNoiseAt(noise, phase);
          const gray = this.clamp255(128 + delta);
          const idx = (u * w + v) * 4;
          data[idx] = gray;
          data[idx + 1] = gray;
          data[idx + 2] = gray;
          data[idx + 3] = 255;
        }
      }
      if (this.materialExportEnabled) this.materialAssetExport.capture('strata', noise === this.stratNoiseH ? 0 : noise === this.stratNoiseL ? 1 : 2, data);
      octx.putImageData(img, 0, 0);
      const bmp = off.transferToImageBitmap();
      return bmp;
    } catch (e) {
      hilog.error(0x0000, 'P012', 'strat: makeStratBitmap threw: ' + JSON.stringify(e));
      return null;
    }
  }

  /** warp 相位（周期数）@显示 x：warpBase 列采样 × WARP_RANGE；
   *  acceptedUV（px，来自 motionCache 材料位移代理）平移采样位置。 */
  private warpAt(xPx: number, boxX: number, colW: number, acceptedUVPx: number): number {
    const n = this.warpBase.length;              // 128
    // 显示 x → 材料坐标（减去材料净位移）→ 列索引（环形）
    const mx = xPx - boxX - acceptedUVPx;
    const colF = mx / colW;
    const i = ((Math.floor(colF) % n) + n) % n;
    return this.warpBase[i] * P012_C_WARP_RANGE;
  }

  /**
   * C 层理绘制（C1 分段平铺 + C3 低频明暗基底；§11.3）：
   * 1) C 先于 B（grain）绘制（design §3.1 时序：E底色 → C层纹 → B颗粒 → A散射）
   * 2) 每带：分段（相邻列 warp 差 >1/16 周期断段，K≤16）→ 每段 1 次
   *    clip（带曲线，与 B 档同骨架）+ ≤3 drawImage 平铺（warp 相位 = 横向
   *    采样偏移 dx = warp/warpRange×64）
   * 3) C3：C1 之上叠 2–4 个低频明暗带（clip 同带，shadeLut 采样，零新缓存）
   * 4) 消融 strat 跳过整段；空带（高度≤0.5px）跳过
   * 返回 draw 提交次数。
   */
  private drawStratification(ctx: CanvasRenderingContext2D, view: P012MaterialFrameView,
    heightToY: (h: number) => number, colW: number,
    boxX: number, boxY: number, boxW: number, boxH: number,
    geometryEpoch: number): number {
    const n = this.cols;
    // λpx / 位图烘焙（geometry epoch 变化重烘焙；pxPerMaterial = boxH/capacity
    // 是 heightToY 的逆变换系数）
    if (this.stratBakedGeometryEpoch !== geometryEpoch || this.stratBitmapH === null) {
      this.stratBakedGeometryEpoch = geometryEpoch;
      this.buildStratBitmaps(boxH / this.capacity);
    }
    // 三带位图 + 各自 warp 推进（acceptedUV：pairValid 时材料位移代理，否则 0 冻结）
    const pairValid = this.lastPairValid;
    const offH = pairValid ? this.motionCache.meanDeltaXPx(true) : 0;
    const offL = pairValid ? this.motionCache.meanDeltaXPx(false) : 0;
    const offM = (offH + offL) * 0.5;
    const bands: P012GrainBand[] = [
      { kind: 0, off: offH, bitmap: this.stratBitmapH },
      { kind: 1, off: offL, bitmap: this.stratBitmapL },
      { kind: 2, off: offM, bitmap: this.stratBitmapM }
    ];
    // 分段：sampler 在分段点调用（K+1 次/带，§11.2）
    const sampler = new P012MaterialColumnSampler(this.smoothBh, this.smoothMixStart,
      this.smoothSandTop, boxW);
    let draws = 0;
    let segTotal = 0;
    for (let bi = 0; bi < bands.length; bi++) {
      const b = bands[bi];
      const tile = b.bitmap;
      if (tile === null) {
        continue;                                 // Node/无位图：跳过（单测兼容）
      }
      // 带的实际屏幕纵向范围（空带跳过：复用 B 档 bandTopY/bandBottomY 逻辑）
      let minY = Number.MAX_VALUE;
      let maxY = -Number.MAX_VALUE;
      for (let c = 0; c < n; c++) {
        const t = this.bandTopY(b.kind, c, heightToY);
        const bo = this.bandBottomY(b.kind, c, heightToY);
        if (t < minY) {
          minY = t;
        }
        if (bo > maxY) {
          maxY = bo;
        }
      }
      if (maxY - minY <= 0.5) {
        continue;
      }
      // warp 逐列相位 → 分段（相邻列差 >1/16 周期断段）
      const segs: number[][] = [];                // [startX, endX, warpCenter]
      let segStartX = boxX;
      let warpPrev = this.warpAt(segStartX, boxX, colW, b.off);
      for (let c = 1; c <= n; c++) {
        const x = boxX + c * colW;
        const w = this.warpAt(c < n ? x - colW * 0.5 : x, boxX, colW, b.off);
        if (c === n || Math.abs(w - warpPrev) > P012_C_SEG_THRESHOLD || segs.length >= P012_C_SEG_MAX - 1) {
          segs.push([segStartX, x, warpPrev]);
          segStartX = x;
          warpPrev = w;
        } else {
          warpPrev = w;
        }
      }
      segTotal += segs.length;
      // 每段 1 drawImage/纵向块：源 = 位图 (v, 0, 1, H) 单列（warp 相位列），
      // 目标 = 段宽 × H 纵向块（横向拉伸源 1px 列 = 无损复制——源列内像素
      // 纵向 1:1；纵向按 H 平铺覆盖带高，λpx=H/k 保证相位无缝）。
      // draw 计数 = 段数 × ceil(带高/H)，位图宽 64 不再进入平铺循环
      // （首版按位图宽 64 循环平铺 + sx 不重置 bug → 净增量 27 > 预算 16）。
      ctx.save();
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
      ctx.globalAlpha = P012_C_ALPHA;
      const yTop = Math.floor(minY);
      const yBot = Math.ceil(maxY);
      for (let si = 0; si < segs.length; si++) {
        const s = segs[si];
        // 横向采样列：warp 相位（周期数）→ 位图 v 轴 px（v/64 = warp/warpRange）
        const dv = s[2] / P012_C_WARP_RANGE * P012_C_BITMAP_W;
        const sx = ((Math.round(dv) % P012_C_BITMAP_W) + P012_C_BITMAP_W) % P012_C_BITMAP_W;
        const dx = Math.round(s[0]);
        const dw = Math.max(1, Math.round(s[1]) - dx);
        // 纵向按位图高 H 平铺（相位无缝）
        for (let y = yTop; y < yBot; y += P012_C_BITMAP_H) {
          const dh = Math.min(P012_C_BITMAP_H, yBot - y);
          ctx.drawImage(tile, sx, 0, 1, P012_C_BITMAP_H, dx, y, dw, dh);
          draws++;
        }
      }
      // C3 低频明暗带（C1 基底之上，每带 2 个；shadeLut 采样按子区深度，
      // 零新缓存；2×3 带=6 fillRect 从 C 的 16 draw 预算出——3×3=9 会超）
      if (P012_C3_BANDS > 0) {
        const subH = (maxY - minY) / P012_C3_BANDS;
        for (let k = 0; k < P012_C3_BANDS; k++) {
          // 子区 shade（复用 E 档 8 档光照 LUT；带内深度越浅越亮）
          const shadeIdx = Math.max(0, Math.min(P012_LUT_STEPS,
            Math.round((1 - (k + 0.5) / P012_C3_BANDS) * P012_LUT_STEPS)));
          const shade = this.shadeLut[shadeIdx];
          const bandY = minY + k * subH;
          ctx.globalAlpha = P012_C3_ALPHA;
          ctx.fillStyle = this.rgb(255 * shade, 255 * shade, 255 * shade);
          ctx.fillRect(boxX, bandY, boxW, subH);
          draws++;
        }
      }
      ctx.restore();
      ctx.globalAlpha = 1;
      // 未用 sampler 的告警抑制（分段点采样在 warp 分段内已隐式完成：
      // warp 表本身是 128 列平滑函数，sampler 供 E3/未来加密采样复用）
      if (sampler !== null) {
        // 保持 sampler 引用供未来 E3 使用（避免 lint 未使用告警）
        const _probe = sampler.bhAtPx(boxX);
        if (_probe < -1e9) {
          hilog.error(0x0000, 'P012', 'strat: sampler probe impossible');
        }
      }
    }
    if (segTotal > P012_C_SEG_MAX) {
      hilog.error(0x0000, 'P012', 'strat: seg overflow ' + segTotal +
        ' > ' + P012_C_SEG_MAX + ' (budget violation → §11.5)');
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
    const fogR = P012_FOG_R;
    const fogG = P012_FOG_G;
    const fogB = P012_FOG_B;
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
   *  stretch=2.2 线性拉伸（clamp [0.2,2.0]）拉开 value-noise 钟形分布。
   *  §13.7.2 c 参数（clearMottleContrast 倍率，唯一实现点）：
   *  z(c,m) = clamp(1 + 2.2·c·(m-1), 0.2, 2.0)；c=1 恢复 s133 逐位基线。 */
  private staticTau(mottle: number): number {
    const c = this.activeAppearance.clearMottleContrast;
    const stretched = Math.max(0.2, Math.min(2.0,
      1 + (mottle - 1) * P012_A_MOTTLE_STRETCH * c));
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
        let alpha = this.fogAlpha(this.activeAppearance.fogDensityScale * tau);
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
        let alpha = this.fogAlpha(this.activeAppearance.fogDensityScale * tau);
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

  // ─────────────────── E3 I2 光学缓冲（§12.1-12.3） ───────────────────

  /**
   * N1 等效 mottle（§12.4 接入点）：e3n1 消融回旧 64 取模 mottleAt；
   *  默认走 N1 场连续采样（无 64 周期）。等效幅值 1.63（定标前置报告）。
   *  平流相位 aPhaseX：pairValid 时由 motionCache 材料净位移推进（与 B 档
   *  颗粒同一代理；旧 mottleAt 注释「运动代理接手」从未接线，E3 顺手补上
   *  ——雾斑驳跟随材料横向搬运）。烘焙循环用 fogPhaseX() 取本帧相位。
   */
  private mottleEquiv(xPx: number, yPx: number): number {
    if (this.frameAblation === P012Ablation.E3N1 || this.e3Noise === null) {
      return this.mottleAt(xPx, yPx);           // I1 消融档（旧路径）
    }
    const n = this.e3Noise.sample(xPx, yPx);
    return 1 + P012_E3_MOTTLE_EQUIV_AMP * n;
  }

  /** 雾平流相位（E3）：pairValid 时取 motionCache 重相净位移（与 B 档 offH
   *  同源）；断配对冻结（保持 aPhaseX 旧值，不跨失效帧累计）。 */
  private fogPhaseX(): number {
    return this.lastPairValid ? this.motionCache.globalPhasePx(true) : this.aPhaseX;
  }

  /**
   * I2 失效检测（§12.3 矩阵）：层高/q/phase 任一变化 → dirty；
   * scale/尺寸变化 → buffer 重建。返回 [waterDirty, ifaceDirty]。
   * 静止态全命中 → 双 false（零烘焙零上传）。
   */
  private e3Invalidate(view: P012MaterialFrameView, boxW: number, boxH: number): boolean[] {
    const n = this.cols;
    // scale 检测：缓冲宽高与 box 不匹配 → 重建（含首帧）
    if (this.e3WaterBuffer === null || this.e3IfaceBuffer === null ||
      Math.abs(this.e3WaterBuffer.height -
        Math.round(P012_E3_BUFFER_W * boxH / boxW)) > 0) {
      this.e3WaterBuffer = new P012E3FogBuffer(P012_E3_BUFFER_W, boxW, boxH);
      this.e3IfaceBuffer = new P012E3FogBuffer(P012_E3_BUFFER_W, boxW, boxH);
      this.e3PrevQH = new Float64Array(n);
      this.e3PrevQL = new Float64Array(n);
      // 重建后全 dirty（首次比较基线也重置）
      this.e3PrevSmooth[0].fill(NaN);
      this.e3PrevSmooth[1].fill(NaN);
      this.e3PrevSmooth[2].fill(NaN);
      this.e3PrevSmooth[3].fill(NaN);
      this.e3WaterBuffer.dirty = true;
      this.e3IfaceBuffer.dirty = true;
      this.e3NoiseX0 = null;          // 几何变化→窗口尺寸可能变化→强制重烘
    }
    // 层高/waterTop 增量检测：任一列 |Δ| > LAYER_EPS → dirty。
    // 阈值 0.25 物理格（视觉可辨下限之下的安全冗余；1e-9 会让沉降中
    // 每帧重烘——真机 ArkTS 烘焙 ~100ms/次，帧率崩到 5-20fps，s123 定位）。
    // phase 检测不变（0.5px 平流阈值保留）。
    const LAYER_EPS = 0.25;
    const passes: Float64Array[] = [this.smoothBh, this.smoothMixStart,
      this.smoothSandTop, this.smoothWaterTop];
    let layersChanged = false;
    for (let p = 0; p < 4 && !layersChanged; p++) {
      const cur = passes[p];
      const prev = this.e3PrevSmooth[p];
      for (let c = 0; c < n; c++) {
        if (Math.abs(cur[c] - prev[c]) > LAYER_EPS) {
          layersChanged = true;
          break;
        }
      }
    }
    // q 增量检测（q 量纲 0-1 悬沙浓度比）。
    // ★s134 渲染回归修复（任务 6adb3508…0b97）：原绝对阈 0.01 对 settle 类
    // 缓变场景每帧触发（thick preset 持续扬沙下 susp 缓变+sandTop/waterTop
    // 微变 → q 帧间差常 >0.01 → E3 双 buffer 每帧重烘 → 热态 126ms/帧）。
    // 修法=LAYER_EPS 的 q 侧等价物：
    // ① 阈值取「绝对 0.02 与相对 prev 的 20%」较大者（小 q 微抖不触发；
    //   真实悬浮变化 q>0.1 时相对 20% 仍快响应，q≤0.02 的静水保持稳定）；
    // ② q 快照只在重烘后更新（原无条件更新使小增量永不累计——与层高
    //   e3PrevSmooth 同语义修法：脏了才记账）。
    // q 阈不护雾带质量：重烘由「层高 or q or phase」任一驱动，真实沉降
    // （层高变化 >0.25）仍立即重烘；此处只拦 settle 微动的伪 dirty。
    let qChanged = false;
    for (let c = 0; c < n && !qChanged; c++) {
      const qh = this.suspQ(view.suspHAt(c), c);
      const ql = this.suspQ(view.suspLAt(c), c);
      if (p012E3QDirty(this.e3PrevQH![c], this.e3PrevQL![c], qh, ql)) {
        qChanged = true;
      }
    }
    // phase 检测：water-fog 跟 aPhaseX；interface 静止相位（不随平流）
    const phaseChangedWater = Math.abs(this.fogPhaseX() - this.e3WaterBuffer.sampleU) >
      P012_E3_PHASE_EPS;
    if (layersChanged || qChanged) {
      this.e3WaterBuffer.dirty = true;
      this.e3IfaceBuffer.dirty = true;
    } else if (phaseChangedWater) {
      this.e3WaterBuffer.dirty = true;
    }
    return [this.e3WaterBuffer.dirty, this.e3IfaceBuffer.dirty];
  }

  /**
   * I2 烘焙：water-fog 缓冲（§12.1 采样循环）。
   * 逐样本：局部 x（sampler 契约）→ sandTop/waterTop 连续层高 → q 标量
   * 插值 → N1 场（平流相位）→ fogAlpha（既有 65 点 LUT，零改动）。
   */
  private e3BakeWaterBuffer(view: P012MaterialFrameView, buffer: P012E3FogBuffer,
    heightToY: (h: number) => number, boxW: number, boxH: number,
    qHArr: Float64Array, qLArr: Float64Array): void {
    const w = buffer.width;
    const h = buffer.height;
    const halfW = P012_A_WFULL_PX / 2;
    const sampler = new P012MaterialColumnSampler(this.smoothBh, this.smoothMixStart,
      this.smoothSandTop, boxW);
    const waterSampler = new P012MaterialColumnSampler(this.smoothWaterTop,
      this.smoothWaterTop, this.smoothSandTop, boxW);
    // phase 提出循环（fogPhaseX 内含 128 列求和——每像素调用 = 62 万次
    // 浮点加法/帧，真机 s123 定位的主要成本）
    const phaseX = this.fogPhaseX();
    for (let u = 0; u < w; u++) {
      const x = (u + 0.5) * boxW / w;            // 局部坐标（相对 boxX）
      const qH = this.scalarAt(qHArr, x, boxW);
      const qL = this.scalarAt(qLArr, x, boxW);
      const gTopY = this.pxOfHeight(waterSampler, x, boxH);
      const gIfaceY = this.pxOfHeightSampler(sampler, x, boxH, 2);
      for (let v = 0; v < h; v++) {
        const yLocal = (v + 0.5) * boxH / h;     // 局部 y（相对 boxY）
        // 世界 y（N1 场采样坐标 = 屏幕 y 语义，局部即可——场与显示同尺度）
        const dist = Math.abs(gIfaceY - yLocal);
        const tauOpt = dist < halfW
          ? P012_A_TAU_INTERFACE * (1 - dist / halfW) : 0;
        const mot = this.mottleEquiv(x - phaseX, yLocal);
        const tauStatic = this.staticTau(mot);
        const tau = Math.max(0, tauOpt + P012_A_KH * qH + P012_A_KL * qL + tauStatic);
        let alpha = this.fogAlpha(this.activeAppearance.fogDensityScale * tau);
        alpha = Math.min(P012_A_FOG_ALPHA_MAX, alpha);
        if (alpha <= 0.004 || yLocal < gTopY || yLocal >= gIfaceY) {
          buffer.setPremul(u, v, P012_FOG_R, P012_FOG_G, P012_FOG_B, 0);
        } else {
          buffer.setPremul(u, v, P012_FOG_R, P012_FOG_G, P012_FOG_B, alpha);
        }
      }
    }
    buffer.sampleU = this.fogPhaseX();
    this.e3BakeCount++;
    this.e3UploadBytes += buffer.flushToUpload();
    // 注意：不调用 buffer.transferBitmap()——真机 OffscreenCanvas 在帧循环内
    // putImageData+transferToImageBitmap 单次耗时 ~600ms（s121 实测 52 倍帧时
    // 退化），降级上屏走 fillFogTiers 不读位图；transferBitmap 仅供未来上屏
    // 通道修复后按需启用（构造期/延迟帧）。
    buffer.dirty = false;
  }

  /**
   * I2 烘焙：interface 缓冲（sandTop±halfW 域，沙体之后绘制）。
   * alpha 上限守设计候选 0.55（与旧 drawInterfaceBand 同界）。
   */
  private e3BakeIfaceBuffer(view: P012MaterialFrameView, buffer: P012E3FogBuffer,
    heightToY: (h: number) => number, boxW: number, boxH: number,
    qHArr: Float64Array, qLArr: Float64Array): void {
    const w = buffer.width;
    const h = buffer.height;
    const wFull = P012_A_WFULL_PX;
    const halfW = wFull / 2;
    const sampler = new P012MaterialColumnSampler(this.smoothBh, this.smoothMixStart,
      this.smoothSandTop, boxW);
    // phase 提出循环（fogPhaseX 内含 128 列求和，见 e3BakeWaterBuffer 注释）
    const phaseX = this.fogPhaseX();
    for (let u = 0; u < w; u++) {
      const x = (u + 0.5) * boxW / w;
      const qH = this.scalarAt(qHArr, x, boxW);
      const qL = this.scalarAt(qLArr, x, boxW);
      const ifaceY = this.pxOfHeightSampler(sampler, x, boxH, 2);
      const bandTop = ifaceY - halfW;
      const bandBot = ifaceY + wFull * 0.25;
      for (let v = 0; v < h; v++) {
        const yLocal = (v + 0.5) * boxH / h;
        const d = ifaceY - yLocal;
        if (yLocal < bandTop || yLocal >= bandBot) {
          buffer.setPremul(u, v, P012_FOG_R, P012_FOG_G, P012_FOG_B, 0);
          continue;
        }
        const t = Math.max(0, Math.min(1, (d + halfW) / wFull));
        // 顶部 ramp 渐入（对齐旧 drawInterfaceBand 的 (s+0.5)/2.5 段语义：
        // 前 2.5/8 段高内线性渐入到 1）
        const rampIn = Math.min(1, (yLocal - bandTop) / (bandBot - bandTop) * 2.5);
        const waterWeight = t * t * (3 - 2 * t) * rampIn;
        if (waterWeight <= 0.01) {
          buffer.setPremul(u, v, P012_FOG_R, P012_FOG_G, P012_FOG_B, 0);
          continue;
        }
        const mot = this.mottleEquiv(x - phaseX, yLocal);
        const tau = Math.max(0, (P012_A_TAU_INTERFACE * waterWeight +
          P012_A_KH * qH * waterWeight + P012_A_KL * qL * waterWeight) * mot);
        let alpha = this.fogAlpha(this.activeAppearance.fogDensityScale * tau);
        alpha = Math.min(P012_A_BAND_ALPHA_MAX, alpha);
        buffer.setPremul(u, v, P012_FOG_R, P012_FOG_G, P012_FOG_B,
          alpha > 0.004 ? alpha : 0);
      }
    }
    this.e3BakeCount++;
    this.e3UploadBytes += buffer.flushToUpload();
    buffer.dirty = false;           // transferBitmap 不调（见 e3BakeWaterBuffer 注释）
  }

  /** 标量列采样：任意局部 x → 列值线性插值（与 sampler 同定位同权重） */
  private scalarAt(arr: Float64Array, x: number, boxW: number): number {
    const n = arr.length;
    const colW = boxW / n;
    const cx = Math.max(0, Math.min(n - 1e-6, x / colW));
    const i = Math.floor(cx);
    const t = cx - i;
    if (i >= n - 1) {
      return arr[n - 1];
    }
    return arr[i] * (1 - t) + arr[i + 1] * t;
  }

  /** 采样器高度 → 局部 y（px，相对 boxY）：heightToY 局部版（boxX=0 基） */
  private pxOfHeightSampler(sampler: P012MaterialColumnSampler, x: number,
    boxH: number, which: number): number {
    const hVal = which === 2 ? sampler.sandTopAtPx(x)
      : (which === 0 ? sampler.bhAtPx(x) : sampler.mixStartAtPx(x));
    return (this.capacity - Math.max(0, Math.min(this.capacity, hVal))) *
      boxH / this.capacity;
  }

  /** waterTop 专用：借用 sampler 接口（第一列数组即 waterTop） */
  private pxOfHeight(waterSampler: P012MaterialColumnSampler, x: number,
    boxH: number): number {
    return this.pxOfHeightSampler(waterSampler, x, boxH, 0);
  }

  /**
   * I2 绘制（§12.1 提交）：失效检测 + 按需烘焙（两 buffer 一次完成）；
   * 绘制按 ifaceOnly 分流——false=水体雾（沙体前），true=界面带（沙体后，
   * 与旧 drawInterfaceBand 时序一致）。每 buffer 一次 clip（层域曲线）+
   * 九参 drawImage（双线性硬件缩放）+ smoothing high。draw≤2/次 ≤6 预算 ✓。
   * e3i2 消融时调用方直接走旧 12 档路径（本方法不进）。
   */
  private e3DrawFogBuffers(ctx: CanvasRenderingContext2D,
    heightToY: (h: number) => number, colW: number,
    boxX: number, boxY: number, boxW: number, boxH: number,
    view: P012MaterialFrameView, ifaceOnly: boolean): number {
    // 失效检测先行（首帧在此创建双 buffer；后取引用）
    const dirties = this.e3Invalidate(view, boxW, boxH);
    const wb = this.e3WaterBuffer!;
    const ib = this.e3IfaceBuffer!;
    const n = this.cols;
    const qHArr = new Float64Array(n);
    const qLArr = new Float64Array(n);
    for (let c = 0; c < n; c++) {
      qHArr[c] = this.suspQ(view.suspHAt(c), c);
      qLArr[c] = this.suspQ(view.suspLAt(c), c);
    }
    if (dirties[0]) {
      this.e3BakeWaterBuffer(view, wb, heightToY, boxW, boxH, qHArr, qLArr);
    }
    if (dirties[1]) {
      this.e3BakeIfaceBuffer(view, ib, heightToY, boxW, boxH, qHArr, qLArr);
    }
    // 快照更新（失效检测基线）
    // ★s134 渲染回归修复（任务 6adb3508…0b97）第二处：原四层高+q 基线
    // 均无条件更新——增量永不累计（帧间差缓变 <阈值但累计已远超，tilt70
    // 雪崩 120 帧累计 1.51 物理格却零重烘=雾纹理滞后真沙形）。改为
    // 「重烘后记账」：层高与 q 基线只在 dirty 烘焙后更新，缓变累计语义
    // 恢复（每帧 0.012 的缓变 ~20 帧累计超 0.25 即触发；真实快变首帧
    // 即超阈）。smooth 序列低通特性保证记账滞后不振荡。
    if (dirties[0] || dirties[1]) {
      this.e3PrevSmooth[0].set(this.smoothBh);
      this.e3PrevSmooth[1].set(this.smoothMixStart);
      this.e3PrevSmooth[2].set(this.smoothSandTop);
      this.e3PrevSmooth[3].set(this.smoothWaterTop);
      this.e3PrevQH!.set(qHArr);
      this.e3PrevQL!.set(qLArr);
    }    // N1 场覆盖维护（§12.3：补边不重播种；覆盖 [−margin−aPhaseX, boxW+margin−aPhaseX]）
    // 保底门控：窗口平移 < P012_E3_NOISE_GATE_PX 跳过（治本稳定化后残余亚视觉
    // 位移不触发 110ms 全量重烘；genuine 搬运按 GATE 步距重烘，滞后落在余量内）。
    if (this.e3Noise !== null) {
      const needX0 = -this.fogPhaseX() - P012_E3_NOISE_MARGIN;
      const needW = boxW + 2 * P012_E3_NOISE_MARGIN;
      if (this.e3NoiseX0 === null ||
        Math.abs(needX0 - this.e3NoiseX0) >= P012_E3_NOISE_GATE_PX) {
        this.e3Noise.shiftAndBake(needX0, 0, needW, boxH);
        this.e3NoiseX0 = needX0;
      }
    }
    // 绘制（按 ifaceOnly 分流）——**平台降级路径**（2026-10-22 取证）：
    // ArkTS 真机 OffscreenCanvas 在帧循环内 putImageData 产出全白（与
    // 内容/alpha/尺寸无关，构造期调用正常——makeGrainBitmap 先例），
    // I2 的 RGBA 位图上屏链不可用。降级：I2 计算面全保留（N1 连续雾场、
    // 256×303 采样、失效缓存），上屏改「buffer 降采样 12 档矩形带 +
    // fillFogTiers 提交器」（与旧路径同 draw 预算；E3 的 N1 收益保留，
    // I2 视觉连续收益待 architect 重排上屏通道后恢复）。
    // 取证链：s112-s116 五轮真机诊断（E3BUF 日志）+ Node 通道对照。
    let draws = 0;
    if (!ifaceOnly) {
      // 水体雾：waterTop..sandTop 域，buffer premul alpha → 12 档矩形带
      const tierRects: number[][] = [];
      const alphaStep = P012_A_FOG_ALPHA_MAX / P012_A_ALPHA_TIERS;
      for (let t = 0; t < P012_A_ALPHA_TIERS; t++) {
        tierRects.push([]);
      }
      const gx0 = 0;
      const gx1 = wb.bufferWidth;
      // 上屏降采样步长自适应（E3 std 超界归因修复）：目标显示块 ≈8px。
      // 原 segU/segV=8 是 256 宽 buffer 的值（256/8=32 列块）；BUFFER_W 降到
      // 64 后沿用 8 只剩 8 列块（显示块 33px 平色大块，上屏档位从 12 塌缩到
      // 5——s132 设备 pxStd 9.38/13.71=0.684 超警戒带的直接机制，归因回执
      // 证据③）。按 buffer 宽等比缩放：seg = max(1, round(bufW/32))。
      const segU = Math.max(1, Math.round(wb.bufferWidth / 32));
      const segV = Math.max(1, Math.round(wb.bufferHeight / 38));
      const suMax = wb.bufferWidth - 1;
      const svMax = wb.bufferHeight - 1;
      for (let bu = 0; bu < gx1; bu += segU) {
        const x0 = boxX + bu * boxW / wb.bufferWidth;
        const x1 = boxX + (bu + segU) * boxW / wb.bufferWidth;
        for (let bv = 0; bv < wb.bufferHeight; bv += segV) {
          const su = Math.min(suMax, bu + (segU >> 1));
          const sv = Math.min(svMax, bv + (segV >> 1));
          const a = wb.readPremulAlpha(su, sv);
          if (a <= 0.004) {
            continue;
          }
          const alpha = Math.min(P012_A_FOG_ALPHA_MAX, a);
          const tierIdx = Math.min(P012_A_ALPHA_TIERS - 1,
            Math.floor(alpha / alphaStep));
          const y0 = boxY + bv * boxH / wb.bufferHeight;
          const y1 = boxY + (bv + segV) * boxH / wb.bufferHeight;
          tierRects[tierIdx].push(x0, y0, x1 - x0, y1 - y0);
        }
      }
      draws += this.fillFogTiers(ctx, tierRects, alphaStep, P012_A_BATCH_DRAW_BUDGET);
    } else {
      // 界面带：sandTop±(halfW..quarterW) 域，同降级提交
      const tierRects: number[][] = [];
      const alphaStep = P012_A_BAND_ALPHA_MAX / P012_A_ALPHA_TIERS;
      for (let t = 0; t < P012_A_ALPHA_TIERS; t++) {
        tierRects.push([]);
      }
      // 上屏降采样步长自适应（同 water 段；原 segU=8/segV=4 是 256 宽值，
      // 64 宽下水平仅 8 列块——见 water 段注释与归因回执证据③）
      const segU = Math.max(1, Math.round(ib.bufferWidth / 32));
      const segV = Math.max(1, Math.round(ib.bufferHeight / 76));
      const suMax = ib.bufferWidth - 1;
      const svMax = ib.bufferHeight - 1;
      for (let bu = 0; bu < ib.bufferWidth; bu += segU) {
        const x0 = boxX + bu * boxW / ib.bufferWidth;
        const x1 = boxX + (bu + segU) * boxW / ib.bufferWidth;
        for (let bv = 0; bv < ib.bufferHeight; bv += segV) {
          const su = Math.min(suMax, bu + (segU >> 1));
          const sv = Math.min(svMax, bv + (segV >> 1));
          const a = ib.readPremulAlpha(su, sv);
          if (a <= 0.004) {
            continue;
          }
          const alpha = Math.min(P012_A_BAND_ALPHA_MAX, a);
          const tierIdx = Math.min(P012_A_ALPHA_TIERS - 1,
            Math.floor(alpha / alphaStep));
          const y0 = boxY + bv * boxH / ib.bufferHeight;
          const y1 = boxY + (bv + segV) * boxH / ib.bufferHeight;
          tierRects[tierIdx].push(x0, y0, x1 - x0, y1 - y0);
        }
      }
      draws += this.fillFogTiers(ctx, tierRects, alphaStep, P012_A_BATCH_DRAW_BUDGET);
    }
    return draws;
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
    // E2 路线②原型（TOPLIGHT ON）：预烘焙光照图替代 shadeLut 档位查询。
    // 采样 z=带中点深度（游程按列合并结构下带内取中点——同列同带单档，
    // 带间深度差仍产生光照分层；任务书「同列同带插值」的最小实现）。
    const useTopLight = this.frameAblation === P012Ablation.TOPLIGHT;
    if (useTopLight) {
      this.e2EnsureLightMap();
    }
    // 逐列光照档（0..7）：s = (S[c+1]-S[c-1])/(2·colPx) 屏幕单位坡度
    // E2 TOPLIGHT：z=带中点深度（band kind 决定）——重沙最深/轻沙中/混合最浅
    const shadeOf = (c: number, bandKind: number): number => {
      if (useTopLight) {
        // 带中点材料深度（capacity 归一）：重沙带 z≈capacity-bh[c]+带半高，
        // 简化取三带中点深度的代表值（带内深度衰减是低频连续——中点代表）
        let zMid: number;
        if (bandKind === 0) {
          zMid = this.capacity - this.smoothBh[c] * 0.5;
        } else if (bandKind === 1) {
          zMid = this.capacity - this.smoothBh[c] - (this.smoothMixStart[c] - this.smoothBh[c]) * 0.5;
        } else {
          zMid = this.capacity - this.smoothMixStart[c] - (this.smoothSandTop[c] - this.smoothMixStart[c]) * 0.5;
        }
        const lmat = this.e2SampleLight(c, zMid);
        // Lmat → shade 档语义（bandColor 的 shade 参数：0=基色 1=提亮）
        const idx = Math.max(0, Math.min(P012_LUT_STEPS,
          Math.round(lmat * P012_LUT_STEPS)));
        return this.shadeLut[idx];
      }
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
        const shade = c < n ? shadeOf(c, band.kind) : -1;
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
    // §10.4 消融：ablation=SPARK 时整段跳过（连 clip 一起省）。
    if (this.frameAblation === P012Ablation.SPARK) {
      return draws;
    }
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
    // §10.4 消融：ablation=SPARK 时提亮系数退化为 1（不加亮粒）
    const spark = this.frameAblation === P012Ablation.SPARK ? 0
      : this.sparkTile[(col % P012_SPARK_TILE) * P012_SPARK_TILE +
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


/** B-route §2.3: occlusion view bound to the renderer's display
 * profiles (ArkTS: interfaces need class implementations, no object
 * literals with function properties). */
class RendererTracerOcclusion implements P012TracerOcclusionView, P012ParticleSampler {
  private readonly owner: P012WaterSandRenderer;
  private readonly heightToYFn: (h: number) => number;
  readonly fogBandHalfPx: number;

  constructor(owner: P012WaterSandRenderer, heightToY: (h: number) => number) {
    this.owner = owner;
    this.heightToYFn = heightToY;
    this.fogBandHalfPx = P012_A_WFULL_PX / 2;
  }

  sandTopAtPx(x: number): number {
    return this.owner.tracerSamplerRef.sandTopAtPx(x);
  }

  waterTopAtPx(x: number): number {
    return this.owner.tracerWaterTopAtPx(x);
  }

  heightToY(h: number): number {
    return this.heightToYFn(h);
  }
}

/** B-route §3: draw-context adapter (CanvasRenderingContext2D is not
 * structurally assignable to the tracer's minimal surface under ArkTS). */
class RendererTracerDrawContext implements P012TracerDrawContext, P012ParticleCanvas {
  private readonly ctx: CanvasRenderingContext2D;

  constructor(ctx: CanvasRenderingContext2D) {
    this.ctx = ctx;
  }

  get fillStyle(): string {
    return `${this.ctx.fillStyle}`;
  }

  set fillStyle(value: string) {
    this.ctx.fillStyle = value;
  }

  get globalAlpha(): number {
    return this.ctx.globalAlpha;
  }

  set globalAlpha(value: number) {
    this.ctx.globalAlpha = value;
  }

  fillRect(x: number, y: number, w: number, h: number): void {
    this.ctx.fillRect(x, y, w, h);
  }
}
