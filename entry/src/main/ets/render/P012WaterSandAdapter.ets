// P-012 双缓冲相库存适配器（architecture-P012.md §3）
// 问题 ID：P-012；方案版本：p012-render-material-v1（E-batch）
// 职责：独占双 slot 深复制、O(128) Kahan 相库存求和、H/L 独立守恒可信度
// 检查、前后配对状态机。红线：不持有物理实例引用，不写回任何物理数组。
import { WaterSandSnapshot } from '../physics/WaterSandField';
import {
  P012FrameContext,
  P012PairDecision,
  P012ConservationCheck,
  P012InvalidReason,
  P012_FRAME_GAP_MS
} from './P012RenderTypes';

/**
 * 单个 slot 的完整深复制（§3.1 字段表）。
 * 8 原始数组 + 4 派生数组全部 Float64Array[128]，Kahan 标量与身份字段显式存储。
 */
export class P012FrameSlot {
  readonly bh: Float64Array;
  readonly bl: Float64Array;
  readonly mh: Float64Array;
  readonly ml: Float64Array;
  readonly suspH: Float64Array;
  readonly suspL: Float64Array;
  readonly sandTopRaw: Float64Array;
  readonly waterTopRaw: Float64Array;
  readonly massH: Float64Array;
  readonly massL: Float64Array;
  readonly depositedH: Float64Array;
  readonly depositedL: Float64Array;
  totalH: number = 0;
  totalL: number = 0;
  absMassH: number = 0;
  absMassL: number = 0;
  fieldEpoch: number = 0;
  continuityEpoch: number = 0;
  geometryEpoch: number = 0;
  frameSerial: number = 0;
  sampleTimeMs: number = 0;
  runIdentity: string = '';

  constructor(cols: number) {
    this.bh = new Float64Array(cols);
    this.bl = new Float64Array(cols);
    this.mh = new Float64Array(cols);
    this.ml = new Float64Array(cols);
    this.suspH = new Float64Array(cols);
    this.suspL = new Float64Array(cols);
    this.sandTopRaw = new Float64Array(cols);
    this.waterTopRaw = new Float64Array(cols);
    this.massH = new Float64Array(cols);
    this.massL = new Float64Array(cols);
    this.depositedH = new Float64Array(cols);
    this.depositedL = new Float64Array(cols);
  }
}

/**
 * 只读帧视图（§3.1 MaterialFrameView）：标量 readonly、列值经访问器。
 * 不向外暴露可写 TypedArray；视图借用不跨帧保留。
 */
export class P012MaterialFrameView {
  private readonly slot: P012FrameSlot;
  constructor(slot: P012FrameSlot) {
    this.slot = slot;
  }
  get cols(): number { return this.slot.bh.length; }
  get totalH(): number { return this.slot.totalH; }
  get totalL(): number { return this.slot.totalL; }
  massHAt(c: number): number { return this.slot.massH[c]; }
  massLAt(c: number): number { return this.slot.massL[c]; }
  bhAt(c: number): number { return this.slot.bh[c]; }
  blAt(c: number): number { return this.slot.bl[c]; }
  mhAt(c: number): number { return this.slot.mh[c]; }
  mlAt(c: number): number { return this.slot.ml[c]; }
  suspHAt(c: number): number { return this.slot.suspH[c]; }
  suspLAt(c: number): number { return this.slot.suspL[c]; }
  sandTopAt(c: number): number { return this.slot.sandTopRaw[c]; }
  waterTopAt(c: number): number { return this.slot.waterTopRaw[c]; }
  depositedHAt(c: number): number { return this.slot.depositedH[c]; }
  depositedLAt(c: number): number { return this.slot.depositedL[c]; }
}

/** 本帧处理的公开结果：curr 视图 + prev 视图 + 配对结论（§5.2 运动代理输入） */
export interface P012AdapterResult {
  /** 本帧（curr）只读视图；curr 非有限时为 null（保留最后合法显示由 renderer 决定） */
  curr: P012MaterialFrameView | null;
  /** 前一帧（prev）只读视图；无合法基线时为 null（首帧/断配对后重建基线） */
  prev: P012MaterialFrameView | null;
  /** prev+curr 配对是否有效（无效时 renderer 冻结运动类状态） */
  pair: P012PairDecision;
  /** 守恒检查明细（H/L 独立） */
  conservation: P012ConservationCheck | null;
}

/**
 * 双 slot 相库存适配器（§3.3 每帧交换与失效策略五条规则）：
 * 1) 同步复制最新页面 snapshot 到空闲 slot；复制/检查完成前不发布视图。
 * 2) 首帧/连续性中断：只用 curr 绘当前静态材质，deltaX=0，重建基线。
 * 3) 合格配对：prev+curr 共同提供运动计算；静态相同快照保持相位不动。
 * 4) 本帧处理完成后交换 slot 索引；视图借用立即到期。
 * 5) curr 相总量不守恒：冻结运动，curr 作下一帧新基线；curr 非有限：
 *    不替换合法 prev，返回 null 视图。
 */
export class P012WaterSandAdapter {
  private slotA: P012FrameSlot;
  private slotB: P012FrameSlot;
  private currIndex: number = 0;       // 本帧写入并发布的 slot
  private hasPrev: boolean = false;    // prev slot 是否持有合法基线
  private lastDecision: P012PairDecision = {
    valid: false, reason: P012InvalidReason.NO_PREV
  };
  private readonly cols: number;
  private readonly capacity: number;

  constructor(cols: number, capacity: number) {
    this.cols = cols;
    this.capacity = capacity;
    this.slotA = new P012FrameSlot(cols);
    this.slotB = new P012FrameSlot(cols);
  }

  get columnCount(): number { return this.cols; }

  /**
   * 每帧入口：深复制 snapshot 到 curr 空闲 slot + 有限性/身份/守恒检查。
   * 返回结果中的视图仅本帧有效；下帧调用后不得继续使用。
   */
  update(snapshot: WaterSandSnapshot, context: P012FrameContext): P012AdapterResult {
    // ── 规则 5b：curr 非有限/损坏 → 不替换合法 prev，清 pairValid（保留最后
    //    合法显示由 renderer 决定）；返回 curr=null 视图。
    if (!this.validateSnapshotFinite(snapshot)) {
      this.lastDecision = { valid: false, reason: P012InvalidReason.NOT_FINITE };
      this.hasPrev = false;
      return { curr: null, prev: null, pair: this.lastDecision, conservation: null };
    }
    const curr = this.currIndex === 0 ? this.slotA : this.slotB;
    const prev = this.currIndex === 0 ? this.slotB : this.slotA;
    // prev 仅在本次更新前已有合法基线时持有真实前帧数据（首帧/断配对后为旧基线）
    const prevValid = this.hasPrev;
    const prevView: P012MaterialFrameView | null =
      prevValid ? new P012MaterialFrameView(prev) : null;
    this.copyInto(curr, snapshot, context);
    const conservation = this.conservationAgainstPrev(curr, prev);
    // ── 配对状态机（§3.2 配对要求 + §3.3 规则 2/5）──
    let decision: P012PairDecision;
    if (!this.hasPrev) {
      decision = { valid: false, reason: P012InvalidReason.NO_PREV };
    } else if (curr.fieldEpoch !== prev.fieldEpoch ||
      curr.continuityEpoch !== prev.continuityEpoch) {
      decision = { valid: false, reason: P012InvalidReason.EPOCH_CHANGED };
    } else if (curr.geometryEpoch !== prev.geometryEpoch) {
      decision = { valid: false, reason: P012InvalidReason.GEOMETRY_CHANGED };
    } else if (curr.runIdentity !== prev.runIdentity) {
      decision = { valid: false, reason: P012InvalidReason.IDENTITY_MISMATCH };
    } else if (curr.frameSerial !== prev.frameSerial + 1) {
      decision = { valid: false, reason: P012InvalidReason.SERIAL_GAP };
    } else if (curr.sampleTimeMs <= prev.sampleTimeMs ||
      curr.sampleTimeMs - prev.sampleTimeMs > P012_FRAME_GAP_MS) {
      decision = { valid: false, reason: P012InvalidReason.FRAME_GAP };
    } else if (!conservation.conservedH) {
      decision = { valid: false, reason: P012InvalidReason.CONSERVATION_H };
    } else if (!conservation.conservedL) {
      decision = { valid: false, reason: P012InvalidReason.CONSERVATION_L };
    } else {
      decision = { valid: true, reason: P012InvalidReason.NONE };
    }
    this.lastDecision = decision;
    // ── 规则 4：本帧处理完成 → 交换 slot，下帧覆盖旧 prev ──
    // curr 守恒失败（规则 5a）：仍把 curr 当下一帧新基线（hasPrev=true 覆盖），
    // 但本帧 pair 无效 → renderer 冻结运动并重置 UV 连续性。
    this.currIndex = 1 - this.currIndex;
    this.hasPrev = true;
    return {
      curr: new P012MaterialFrameView(curr),
      prev: prevView,
      pair: decision,
      conservation
    };
  }

  /** 最近一次配对结论（供 renderer 诊断） */
  get lastPairDecision(): P012PairDecision {
    return this.lastDecision;
  }

  /** dispose：丢弃全部 slot 引用（§3.3 末条） */
  dispose(): void {
    this.hasPrev = false;
    this.currIndex = 0;
  }

  // ───────────────────────── private ─────────────────────────

  private validateSnapshotFinite(snapshot: WaterSandSnapshot): boolean {
    if (snapshot.schemaVersion !== 2 || snapshot.cols !== this.cols ||
      snapshot.capacity !== this.capacity) {
      return false;
    }
    const arrays: Float64Array[] = [snapshot.bh, snapshot.bl, snapshot.mh, snapshot.ml,
      snapshot.suspH, snapshot.suspL, snapshot.sandTop, snapshot.waterTop];
    for (let i = 0; i < arrays.length; i++) {
      const arr = arrays[i];
      if (arr.length !== this.cols) {
        return false;
      }
      for (let c = 0; c < this.cols; c++) {
        if (!Number.isFinite(arr[c]) || arr[c] < 0) {
          return false;
        }
      }
    }
    return true;
  }

  /** 深复制 + 派生字段 + Kahan 求和（单循环 O(128)，§3.2） */
  private copyInto(target: P012FrameSlot, snapshot: WaterSandSnapshot,
    context: P012FrameContext): void {
    const n = this.cols;
    let sumH = 0, compH = 0, absH = 0;
    let sumL = 0, compL = 0, absL = 0;
    for (let c = 0; c < n; c++) {
      const bh = snapshot.bh[c];
      const bl = snapshot.bl[c];
      const mh = snapshot.mh[c];
      const ml = snapshot.ml[c];
      const sh = snapshot.suspH[c];
      const sl = snapshot.suspL[c];
      target.bh[c] = bh;
      target.bl[c] = bl;
      target.mh[c] = mh;
      target.ml[c] = ml;
      target.suspH[c] = sh;
      target.suspL[c] = sl;
      target.sandTopRaw[c] = snapshot.sandTop[c];
      target.waterTopRaw[c] = snapshot.waterTop[c];
      const mH = bh + mh + sh;
      const mL = bl + ml + sl;
      target.massH[c] = mH;
      target.massL[c] = mL;
      target.depositedH[c] = bh + mh;
      target.depositedL[c] = bl + ml;
      // Kahan 补偿求和（H/L 分别）
      let yH = mH - compH;
      let tH = sumH + yH;
      compH = (tH - sumH) - yH;
      sumH = tH;
      let yL = mL - compL;
      let tL = sumL + yL;
      compL = (tL - sumL) - yL;
      sumL = tL;
      absH += Math.abs(mH);
      absL += Math.abs(mL);
    }
    target.totalH = sumH;
    target.totalL = sumL;
    target.absMassH = absH;
    target.absMassL = absL;
    target.fieldEpoch = context.fieldEpoch;
    target.continuityEpoch = context.continuityEpoch;
    target.geometryEpoch = context.geometryEpoch;
    target.frameSerial = context.frameSerial;
    target.sampleTimeMs = context.sampleTimeMs;
    target.runIdentity = context.runIdentity;
  }

  /** tol_p = 64·N·EPS·max(1,|M_old|+|M_new|)（§3.2；渲染代理数值可信度门槛） */
  private conservationAgainstPrev(curr: P012FrameSlot,
    prev: P012FrameSlot): P012ConservationCheck {
    if (!this.hasPrev) {
      return {
        conservedH: true, conservedL: true,
        totalH: curr.totalH, totalL: curr.totalL,
        tolH: 0, tolL: 0
      };
    }
    const n = this.cols;
    const base = 64 * n * Number.EPSILON;
    const tolH = base * Math.max(1, prev.absMassH + curr.absMassH);
    const tolL = base * Math.max(1, prev.absMassL + curr.absMassL);
    return {
      conservedH: Math.abs(curr.totalH - prev.totalH) <= tolH,
      conservedL: Math.abs(curr.totalL - prev.totalL) <= tolL,
      totalH: curr.totalH, totalL: curr.totalL,
      tolH, tolL
    };
  }
}
