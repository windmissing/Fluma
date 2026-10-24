// P-012 材料运动代理（architecture-P012.md §5 + algorithm-P012-render-v1.md §5.2）
// 问题 ID：P-012；方案版本：p012-render-material-v1（EAB-batch，D-B2）
// 职责：从前后两份只读相库存快照推导相净搬运的渲染位移代理（deltaX），
//   维护连续 UV 平流双缓冲（wrap 到 tile 周期，不漂移），提供 B 档全局无缝
//   偏移与 D 档未来 16 组局部偏移。
// 红线：不持有物理实例、不写回任何物理数组；无速度场、不读 P011 日志。
// 能力限制（设计稿 §5.2）：仅恢复同相沉积+悬浮合计的净搬运，不能分解底层/
//   表层/悬浮各自速度，也看不到净零反向循环；仅作弱纹理跟随与 D 候选方向。
import { P012MaterialFrameView } from './P012WaterSandAdapter';

/**
 * 相净搬运渲染位移代理。
 * - deltaX[c]：第 c 列的横向净位移（单位=列），正=材料向右净搬运。
 * - uvHpx/uvLpx[c]：第 c 列颗粒纹理的屏幕像素平流偏移（连续累积、wrap 到
 *   tile 周期），沙体纹理按此采样实现"纹理跟随材料"。
 * - valid=false（守恒失败/注入/重置/间隔断档/身份变化）：冻结——deltaX 清零、
 *   UV 保持不变，不跨失效帧累计伪造平流。
 */
export class P012MaterialMotionCache {
  private readonly cols: number;
  /** 重相（H=bh+mh+suspH）横向位移，单位=列 */
  private readonly deltaXH: Float64Array;
  /** 轻相（L=bl+ml+suspL）横向位移，单位=列 */
  private readonly deltaXL: Float64Array;
  /** 重相纹理平流偏移（px） */
  private readonly uvHpx: Float64Array;
  /** 轻相纹理平流偏移（px） */
  private readonly uvLpx: Float64Array;
  /** 前缀和临时缓冲（F[e]=-sum(deltaM[0..e])） */
  private readonly prefix: Float64Array;
  private tilePx: number = 256;
  /** 最近一次更新是否有效（驱动冻结/重建） */
  private lastValid: boolean = false;
  /**
   * 全局平流相位（雾代理）——未 wrap 的累计净搬运（px），重相/轻相各一。
   * 取代旧 mean(uv) 的「逐列 wrap 均值」：settle 下单列 UV 跨越 tile 边界使
   * 均值跳变（实测 phaseDelta 量子化于 {0,92,162,210}px）→ fogPhaseX 每帧跳
   * → E3 双缓冲每帧 dirty（20ms）+ 噪声场窗口每帧平移（110ms 全量重烘）。
   * 治本=在「未 wrap 的累计量」上计算后单次取用，绝不对均值施加 wrap。
   * 死区（②）抑制亚视觉增量累计；近空列钳幅（③）抑制 mbar→0 放大污染。
   */
  private globalPhaseHpx: number = 0;
  private globalPhaseLpx: number = 0;
  /** 近空列 deltaX 钳幅（px）：mbar→eps 时 deltaX 被放大，钳住防污染全局均值 */
  private static readonly DX_CLAMP_PX = 4;
  /** 亚视觉增量死区（px/帧）：低于此的逐帧净搬运不累计（抑制 settle 微抖） */
  private static readonly PHASE_DEADZONE_PX = 0.25;

  constructor(cols: number) {
    this.cols = cols;
    this.deltaXH = new Float64Array(cols);
    this.deltaXL = new Float64Array(cols);
    this.uvHpx = new Float64Array(cols);
    this.uvLpx = new Float64Array(cols);
    this.prefix = new Float64Array(cols);
  }

  /**
   * 每帧更新（§5.2 运动估计）。
   * @param prev 前一帧只读视图（adapter 配对 prev slot，仅在 valid 时有效）
   * @param curr 当前帧只读视图
   * @param valid 配对结论（adapter.pair.valid）—— false 即冻结
   * @param colW 每列屏幕宽（px），用于把列单位位移换算为纹理像素偏移
   * @param tilePx 颗粒 tile 像素周期（wrap 模数）
   */
  update(prev: P012MaterialFrameView, curr: P012MaterialFrameView,
    valid: boolean, colW: number, tilePx: number): void {
    this.tilePx = Math.max(1, tilePx);
    if (!valid) {
      // 冻结：deltaX 清零（本帧无平流），UV 保持不变（不清零，避免跨失效跳变）
      this.deltaXH.fill(0);
      this.deltaXL.fill(0);
      this.lastValid = false;
      return;
    }
    this.lastValid = true;
    this.computePhase(prev, curr, true, colW);
    this.computePhase(prev, curr, false, colW);
  }

  /** 单相位：deltaM → 前缀 F → deltaX（列）→ UV 平流 wrap */
  private computePhase(prev: P012MaterialFrameView, curr: P012MaterialFrameView,
    heavy: boolean, colW: number): void {
    const n = this.cols;
    const eps = 1e-6;
    const deltaX = heavy ? this.deltaXH : this.deltaXL;
    const uv = heavy ? this.uvHpx : this.uvLpx;
    const getM = (v: P012MaterialFrameView, c: number): number =>
      heavy ? v.massHAt(c) : v.massLAt(c);
    // ① deltaM[c] = M_new[c] - M_old[c]
    // ② F[e] = -sum_{c=0..e} deltaM[c]（闭边界外侧通量=0）
    let prefixSum = 0;
    for (let c = 0; c < n; c++) {
      const dM = getM(curr, c) - getM(prev, c);
      prefixSum += dM;
      this.prefix[c] = -prefixSum;   // F[c]
    }
    // ③ deltaX[c] = (F[c-1] + F[c]) / (2·max(Mbar[c], eps))，Mbar=(M_old+M_new)/2
    for (let c = 0; c < n; c++) {
      const fPrev = c > 0 ? this.prefix[c - 1] : 0;
      const fCur = this.prefix[c];
      const mbar = (getM(curr, c) + getM(prev, c)) * 0.5;
      const denom = 2 * Math.max(mbar, eps);
      deltaX[c] = (fPrev + fCur) / denom;   // 列单位
    }
    // ④ UV 平流：uv_new[c] = uv_old[c] - deltaX[c]·colW（纹理采样左移=材料右移）
    //   wrap 到 [0, tilePx) 防止长期累积溢出（不改变显示相位）
    const tp = this.tilePx;
    for (let c = 0; c < n; c++) {
      let u = uv[c] - deltaX[c] * colW;
      u = ((u % tp) + tp) % tp;
      uv[c] = u;
    }
    // ⑤ 全局平流相位（雾代理，未 wrap）：mean(δx·colW) 累计。deltaX 本身不 wrap，
    //   故均值无跳变；近空列钳幅（③）+ 亚视觉增量死区（②）双保险。
    //   这是 fogPhaseX 的真实来源——取代旧 mean(uv) 的 wrap 均值（跳变源）。
    let sumPx = 0;
    for (let c = 0; c < n; c++) {
      let dxPx = deltaX[c] * colW;
      if (dxPx > P012MaterialMotionCache.DX_CLAMP_PX) {
        dxPx = P012MaterialMotionCache.DX_CLAMP_PX;
      } else if (dxPx < -P012MaterialMotionCache.DX_CLAMP_PX) {
        dxPx = -P012MaterialMotionCache.DX_CLAMP_PX;
      }
      sumPx += dxPx;
    }
    const incrPx = sumPx / n;
    if (Math.abs(incrPx) > P012MaterialMotionCache.PHASE_DEADZONE_PX) {
      if (heavy) {
        this.globalPhaseHpx += incrPx;
      } else {
        this.globalPhaseLpx += incrPx;
      }
    }
  }

  /** B 档（无 D）：全局平流偏移 = 沙区 mean(UV)（px）。所有列共享 → 无缝。 */
  meanDeltaXPx(heavy: boolean): number {
    if (!this.lastValid) {
      return 0;
    }
    const n = this.cols;
    const uv = heavy ? this.uvHpx : this.uvLpx;
    let s = 0;
    for (let c = 0; c < n; c++) {
      s += uv[c];
    }
    return s / n;
  }

  /**
   * 全局平流相位（雾代理，未 wrap）——E3 fogPhaseX 改用此值（治本修复）。
   * 返回累计净搬运（px），settle 亚视觉增量被死区抑制→稳定；真实搬运累计→
   * 平滑驱动雾流。不 wrap：wrap 会破坏连续漂移并引发跳变（旧 mean(uv) 之弊）。
   */
  globalPhasePx(heavy: boolean): number {
    return heavy ? this.globalPhaseHpx : this.globalPhaseLpx;
  }

  /** D 档未来：第 g 组中心列偏移（px）—— 各列独立偏移支撑 16 组局部各向异性 */
  groupDeltaXPx(heavy: boolean, groupCenterCol: number): number {
    const uv = heavy ? this.uvHpx : this.uvLpx;
    const c = Math.max(0, Math.min(this.cols - 1, groupCenterCol));
    return uv[c];
  }

  /** 单相位 deltaX（列单位）读取（D 档/诊断用） */
  deltaXColumnsAt(heavy: boolean, c: number): number {
    const arr = heavy ? this.deltaXH : this.deltaXL;
    if (c < 0 || c >= this.cols) {
      return 0;
    }
    return arr[c];
  }

  get isValid(): boolean {
    return this.lastValid;
  }

  /** 生命周期：清连续性（reset/inject/invert/后台恢复/外观开关） */
  reset(): void {
    this.deltaXH.fill(0);
    this.deltaXL.fill(0);
    this.uvHpx.fill(0);
    this.uvLpx.fill(0);
    this.globalPhaseHpx = 0;
    this.globalPhaseLpx = 0;
    this.lastValid = false;
  }
}
