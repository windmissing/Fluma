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
  /** 物理证据采集强制 OFF（wssnapshot 标志/采集入口/手动开关任一为真） */
  canonicalEvidence: boolean;
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
