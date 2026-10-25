import { P011ParticleRenderProjection } from '../common/P011ParticleRenderProjection';
// P-012 表现层公共类型（architecture-P012.md §2 frameContext + §3 slot 字段）
// 问题 ID：P-012；方案版本：p012-render-material-v1（E-batch）
// 红线：只读物理场——本文件不 import 任何物理模块，不使用 any。

/** 外观档位：OFF=原 renderWaterSand（canonical）；E/A/B 为第一梯队材质层 */
export enum P012AppearanceTier {
  OFF = 0,
  E = 1,
  EA = 2,
  EAB = 3
}

/** 外观技术路线身份（与物理 routeId 完全隔离，仅进配置/视觉证据 manifest） */
export const P012_APPEARANCE_ROUTE_ID = 'p012-render-material-v1';

/** §10.4 逐项消融开关：off=全部启用（默认）/ grain=跳过颗粒层 /
 *  fog=跳过雾层 / spark=跳过亮粒 / strat=跳过 C 层理（§11.4）/
 *  e3i2=跳过 I2 光学缓冲回旧 12 档雾路径（§12.5）/
 *  e3n1=N1 噪声场回退旧 64 取模 mottle（§12.5）/
 *  dyn=F1 静态/动态占比测量档（跳过静态层：底色渐变/水体雾/液面高光/
 *  C 层理/B 颗粒/边框；只保留沙体三带+界面带——F1 消融，不进产品）/
 *  toplight=E2 路线②原型开关（ON=预烘焙光照图替代 shadeLut 档位查询；
 *  默认 OFF=现状基线，可一键回退——E2 最小可撤销原型，不进产品）。
 *  外观诊断域，不进物理 checker/遥测。 */
export enum P012Ablation {
  OFF = 0,
  GRAIN = 1,
  FOG = 2,
  SPARK = 3,
  STRAT = 4,
  E3I2 = 5,
  E3N1 = 6,
  DYN = 7,
  TOPLIGHT = 8,
  // falling-sand B-route: the tracer spray layer (anchor-measure OFF arm;
  // default ON — product tier).
  TRACER = 9
}

/** §10.4 fillRect 策略：tight=精确平铺边界（默认，交付后）/
 *  full=旧整盒外扩一整圈（v1 A/B 回归对照）。 */
export enum P012FillBox {
  TIGHT = 0,
  FULL = 1
}

/** adapter 校验失败原因（INVALID_RENDER_INPUT 细分，不回写物理） */
export enum P012InvalidReason {
  NONE = 0,
  /** 首帧 / 尚无有效 prev（正常冷启动，非错误） */
  NO_PREV = 1,
  /** 快照非有限 / 长度不匹配 / schema 不符 */
  NOT_FINITE = 2,
  /** 身份（runId/inputMode/sourceKind）不一致 → 断配对 */
  IDENTITY_MISMATCH = 3,
  /** 帧序号不连续 / 时间倒退 */
  SERIAL_GAP = 4,
  /** 显示帧间隔 >100ms（v1 视觉连续性中断） */
  FRAME_GAP = 5,
  /** 相总量守恒检查未过（H/L 分别判定） */
  CONSERVATION_H = 6,
  CONSERVATION_L = 7,
  /** 已知 reset/inject/invert 事件（fieldEpoch/continuityEpoch 变化） */
  EPOCH_CHANGED = 8,
  /** 几何代次变化（resize/DPI） */
  GEOMETRY_CHANGED = 9
}

/** 输入模式（P011 既有语义的只读副本） */
export type P012InputMode = 'TEST' | 'EXPERIENCE';
/** 输入来源（P011 既有语义的只读副本） */
export type P012SourceKind = 'sensor' | 'gesture' | 'script';

/** 页面→renderer 的显示契约（§2；全部为页面既有字段的只读副本） */
export interface P012FrameContext {
  renderFrameId?: number;
  physicsCommitId?: number;
  particleSnapshot?: P012ParticleRenderSnapshot;
  particleLayerEnabled?: boolean;
  /** 物理实例重建/reset/注入/倒立等既有处理点递增；只用于断开视觉配对 */
  fieldEpoch: number;
  /** 连续性代次（reset/inject/invert/后台断档） */
  continuityEpoch: number;
  /** 几何代次（resize/DPI 变化） */
  geometryEpoch: number;
  /** 页面实际采样序号（帧递增） */
  frameSerial: number;
  /** 单调时间 ms */
  sampleTimeMs: number;
  /** 本帧间隔 ms */
  frameDeltaMs: number;
  inputMode: P012InputMode;
  sourceKind: P012SourceKind;
  /** 物理路线身份只读副本（外观开关不得改变） */
  physicalRouteId: string;
  runIdentity: string;
  /** 容器显示区域（px） */
  boxX: number;
  boxY: number;
  boxW: number;
  boxH: number;
  pixelRatio: number;
  /** 视觉倒立（180° 事件已由页面处理过） */
  visualInverted: boolean;
  /** 外观配置 */
  appearanceTier: P012AppearanceTier;
  /** 外观独立随机种子（不调用/消耗物理随机源） */
  renderSeed: number;
  /** 外观配置代次（tier/seed 变化时递增） */
  configRevision: number;
  /** §10.4 消融开关（want p012ablate；默认 OFF=全部启用） */
  ablation: P012Ablation;
  /** §10.4 fillRect 策略（want p012fillbox；默认 TIGHT） */
  fillBox: P012FillBox;
  /** 物理证据采集强制 OFF（wssnapshot 标志/采集入口/手动开关任一为真） */
  canonicalEvidence: boolean;
  /** falling-sand B-route §2.1: display-space gravity unit vector,
   *  R(tilt)·(0,+1), derived from the SAME attitude source as the
   *  sensor path (no second attitude source). 180° invert negates it. */
  gravityDirX: number;
  gravityDirY: number;
  /** §3 tracer layer switch (default true=product; ablation=TRACER is
   *  the anchor-measure OFF arm — shared mechanism with existing
   *  ablations). */
  sandTracerEnabled: boolean;
}

/** adapter 校验结论（布尔化：pairValid 才允许运动类计算，E 静态材质不受限） */
export interface P012PairDecision {
  valid: boolean;
  reason: P012InvalidReason;
}

/** 守恒检查结果（H/L 独立报告，互不抵销） */
export interface P012ConservationCheck {
  conservedH: boolean;
  conservedL: boolean;
  totalH: number;
  totalL: number;
  tolH: number;
  tolL: number;
}

/** 渲染层 draw 预算（E-batch：E 档合计 ≤16 次 fill/stroke 提交） */
export const P012_E_BATCH_DRAW_BUDGET = 16;
/** EA-batch：A 档云雾新增 draw 预算（算法稿 §8.2：A 档 ≤48） */
export const P012_A_BATCH_DRAW_BUDGET = 48;
/** EA-batch：E+A 合计 draw 预算（16+48=64；公共容器绘制含于 E 侧） */
export const P012_EA_BATCH_DRAW_BUDGET = 64;
/** EAB-batch：B 档颗粒新增 draw 预算（算法稿 §8.2：B 档 ≤48；v1 三带全局
 *  平流 pattern fillRect 实际仅 3，结构预留 16 组×3 材质供未来 D 各向异性） */
export const P012_B_BATCH_DRAW_BUDGET = 48;
/** EAB-batch：第一梯队 E+A+B 合计 draw 预算（16+48+48=112；公共容器另列） */
export const P012_EAB_BATCH_DRAW_BUDGET = 112;
/** 第一梯队含公共合计上限（≤160 中 112 留给 E/A/B 三档绘制） */
export const P012_FIRST_TIER_DRAW_BUDGET = P012_EAB_BATCH_DRAW_BUDGET;

/** 显示帧间隔阈值：超过即视为 v1 视觉连续性中断（§3.2） */
export const P012_FRAME_GAP_MS = 100;

// ───────────── §13 参数可调化：P012AppearanceParams schema（architecture §13.2/13.7）─────────────

/** schema 版本（迁移失败→丢弃用版本化推荐） */
export const P012_APPEARANCE_SCHEMA_VERSION = 1;
/** 风格族 ID（不匹配→拒绝整包） */
export const P012_STYLE_FAMILY_ID = 'soft-water-sand-v1';
/** E3 验证构建绑定的版本串（s133 = 9d11849 构建 hash 前缀） */
export const P012_APPEARANCE_RENDERER_VERSION = 's133-9d11849';

/** 六字段外观参数（§13.2 schema；域与步长冻结，禁 clamp——非法拒绝整包） */
export interface P012AppearanceParams {
  schemaVersion: number;
  styleFamilyId: string;
  rendererVersion: string;
  /** 颗粒粗细：三档离散 {2,4,6}（盒滤波半径 R=1/2/3） */
  grainFeaturePx: number;
  /** 颗粒对比：[12,24] 步长 1，默认 20 */
  grainSigma: number;
  /** 水雾浓度倍率：[0.5,1] 步长 0.05，默认 1；cap=0.78 只读不随 f 变 */
  fogDensityScale: number;
  /** 层理疏密（名义 λ，材料深度单位）：[16,32] 默认 24——控件档按 effectiveλ 去重 */
  strataLambdaMaterial: number;
  /** 层理强度倍率：[0.5,4/3] 默认 1（ampOrdered=6·s / ampMixed=3·s） */
  strataStrengthScale: number;
  /** 清水斑驳对比倍率：[0.5,1.25] 步长 0.05，默认 1 */
  clearMottleContrast: number;
  source: 'factory-recommended' | 'user' | 'preset';
  /** 应用成功单调 +1（§27.4 模式） */
  configRevision: number;
}

/** 页面聚合配置：physics 与 appearance 两域，物理 configure 前必须剥离 appearance（§13.1） */
export interface WaterSandScreenConfig {
  physics: Record<string, Object>;
  appearance: P012AppearanceParams;
}

/** 工厂推荐外观包（六参数默认 = s133 冻结基线，§13.7.1 数值绑定表） */
export function p012FactoryAppearance(): P012AppearanceParams {
  return {
    schemaVersion: P012_APPEARANCE_SCHEMA_VERSION,
    styleFamilyId: P012_STYLE_FAMILY_ID,
    rendererVersion: P012_APPEARANCE_RENDERER_VERSION,
    grainFeaturePx: 2,
    grainSigma: 20,
    fogDensityScale: 1.0,
    strataLambdaMaterial: 24,
    strataStrengthScale: 1.0,
    clearMottleContrast: 1.0,
    source: 'factory-recommended',
    configRevision: 0
  };
}

/**
 * λ 同 k 去重表（§13.7.3 控制层去重——不动 lambdaPx 对齐算法）。
 * 对名义域 [16,32] 以 0.1 步进枚举，按 effectiveλ 的整周期数 k 分组，
 * 每组取代表名义值。**默认值 24 强制为所在组代表**（§13.7.4-1「默认值域
 * = 锚点已验域」：默认配置必须通过自身校验，否则恢复推荐会被拒绝）。
 * UI 只暴露去重后档位，避免「滑动无变化」。pxPerMaterial 为当前几何定值。
 */
export function buildStrataLambdaDedupTable(pxPerMaterial: number): number[] {
  // 先按 k 分组（保留组员列表）
  const groups = new Map<number, number[]>();
  for (let v = 16; v <= 32.0001; v += 0.1) {
    const nominal = Math.round(v * 10) / 10;
    const k = Math.max(1, Math.ceil(256 / (nominal * pxPerMaterial)));
    if (!groups.has(k)) {
      groups.set(k, []);
    }
    groups.get(k)!.push(nominal);
  }
  // 默认值 24 所在组：代表改为 24（若 24 在域内）
  const kDefault = Math.max(1, Math.ceil(256 / (24 * pxPerMaterial)));
  if (groups.has(kDefault)) {
    const members = groups.get(kDefault)!;
    members.sort((a, b) => Math.abs(a - 24) - Math.abs(b - 24));
    if (members.length > 0) {
      // 组代表 = 最接近 24 的成员（即 24 本身，24 ∈ [16,32] 且 0.1 步进可达）
      const def = members[0];
      groups.set(kDefault, [def, ...members.filter(m => m !== def)]);
    }
  }
  // 每组取第一个成员（代表）
  const keys: number[] = [];
  for (const members of groups.values()) {
    keys.push(members[0]);
  }
  keys.sort((a, b) => a - b);
  return keys;
}

/** λ 名义值是否属于去重表键集（validate 用） */
export function strataLambdaInDedupTable(nominal: number, pxPerMaterial: number): boolean {
  const table = buildStrataLambdaDedupTable(pxPerMaterial);
  for (const k of table) {
    if (Math.abs(k - nominal) < 1e-9) {
      return true;
    }
  }
  return false;
}

/**
 * schema 校验（§13.2 fail-closed）：返回 null=合法；返回原因串=拒绝整包
 * （UI 显示边界与原因，不 clamp 不部分应用）。pxPerMaterial 为当前几何
 * 定值（λ 去重表键集校验用）；传 0 跳过去重表校验（Node 单测便捷路径）。
 * α 不入首版：持久化 JSON 出现 alpha 字段由调用侧整包拒绝（前向防御，
 * 本函数只校验接口字段本身）。
 */
export function validateP012AppearanceParams(p: P012AppearanceParams,
  pxPerMaterial: number): string | null {
  if (p === null || p === undefined) {
    return 'appearance 参数缺失';
  }
  if (p.schemaVersion !== P012_APPEARANCE_SCHEMA_VERSION) {
    return `schemaVersion=${p.schemaVersion} 不受支持（当前 ${P012_APPEARANCE_SCHEMA_VERSION}）`;
  }
  if (p.styleFamilyId !== P012_STYLE_FAMILY_ID) {
    return `styleFamilyId='${p.styleFamilyId}' 不匹配（须 '${P012_STYLE_FAMILY_ID}'）`;
  }
  if (p.grainFeaturePx !== 2 && p.grainFeaturePx !== 4 && p.grainFeaturePx !== 6) {
    return `grainFeaturePx=${p.grainFeaturePx} 非法（离散档 {2,4,6}）`;
  }
  if (!Number.isFinite(p.grainSigma) || p.grainSigma < 12 || p.grainSigma > 24 ||
    Math.abs(p.grainSigma - Math.round(p.grainSigma)) > 1e-9) {
    return `grainSigma=${p.grainSigma} 非法（[12,24] 步长 1）`;
  }
  if (!Number.isFinite(p.fogDensityScale) || p.fogDensityScale < 0.5 - 1e-9 ||
    p.fogDensityScale > 1.0 + 1e-9 ||
    Math.abs(p.fogDensityScale / 0.05 - Math.round(p.fogDensityScale / 0.05)) > 1e-6) {
    return `fogDensityScale=${p.fogDensityScale} 非法（[0.5,1] 步长 0.05）`;
  }
  if (!Number.isFinite(p.strataLambdaMaterial) || p.strataLambdaMaterial < 16 - 1e-9 ||
    p.strataLambdaMaterial > 32 + 1e-9) {
    return `strataLambdaMaterial=${p.strataLambdaMaterial} 非法（名义 [16,32]）`;
  }
  if (pxPerMaterial > 0 && !strataLambdaInDedupTable(p.strataLambdaMaterial, pxPerMaterial)) {
    return `strataLambdaMaterial=${p.strataLambdaMaterial} 不在去重表键集（同 k 已有代表档）`;
  }
  if (!Number.isFinite(p.strataStrengthScale) || p.strataStrengthScale < 0.5 - 1e-9 ||
    p.strataStrengthScale > 4 / 3 + 1e-9) {
    return `strataStrengthScale=${p.strataStrengthScale} 非法（[0.5,4/3]）`;
  }
  if (!Number.isFinite(p.clearMottleContrast) || p.clearMottleContrast < 0.5 - 1e-9 ||
    p.clearMottleContrast > 1.25 + 1e-9 ||
    Math.abs(p.clearMottleContrast / 0.05 - Math.round(p.clearMottleContrast / 0.05)) > 1e-6) {
    return `clearMottleContrast=${p.clearMottleContrast} 非法（[0.5,1.25] 步长 0.05）`;
  }
  if (p.source !== 'factory-recommended' && p.source !== 'user' && p.source !== 'preset') {
    return `source='${p.source}' 非法`;
  }
  if (!Number.isFinite(p.configRevision) || p.configRevision < 0 ||
    Math.floor(p.configRevision) !== p.configRevision) {
    return `configRevision=${p.configRevision} 非法（非负整数）`;
  }
  return null;
}

/** Reused post-commit particle view; candidate capacity/appearance are not frozen. */
export class P012ParticleRenderSnapshot implements P011ParticleRenderProjection {
  physicsCommitId: number = 0; particleRevision: number = 0; routeId: string | null = null;
  active: boolean = false; count: number = 0; capacity: number; cols: number = 128;
  readonly x: Float64Array; readonly y: Float64Array;
  readonly vx: Float32Array; readonly vy: Float32Array; readonly mass: Float64Array;
  readonly phase: Uint8Array; readonly state: Uint8Array; readonly particleId: Uint32Array;
  constructor(capacity: number) {
    this.capacity = capacity; this.x = new Float64Array(capacity); this.y = new Float64Array(capacity);
    this.vx = new Float32Array(capacity); this.vy = new Float32Array(capacity);
    this.mass = new Float64Array(capacity); this.phase = new Uint8Array(capacity);
    this.state = new Uint8Array(capacity); this.particleId = new Uint32Array(capacity);
  }
}
