/**
 * HeightField - 高度场两相流体引擎（平行界面平衡模型）
 *
 * 物理模型（流体静力学平衡，等密度两相近似）：
 *   自由液面（总相顶）  ：斜线，斜率 tan(θ)，垂直于重力方向
 *   两相界面（重相顶）  ：与液面平行的斜线，斜率同为 tan(θ)
 *   即：轻相厚度沿全宽恒定，重相沉在界面之下 —— 这是连通器两液柱的静力学平衡解。
 *
 *   每列目标：totalTarget(c) = clamp(tanθ·c + C_T, 0, cap)   （液面线）
 *            heavyTarget(c) = clamp(tanθ·c + C_H, 0, cap)   （界面线，在其下方）
 *            lightTarget(c) = totalTarget - heavyTarget
 *   截距 C_T / C_H 由体积守恒二分求出（不做比例缩放——缩放会破坏斜面形状，
 *   使目标剖面与真实平衡态偏离，比例失衡时迁移游标直接锁死）。
 *
 * 实现：整数配对迁移（donor 高于目标 → receiver 低于目标，逐粒搬运），
 * 每步限流 maxPerStep 使流动渐进可视；体积结构性守恒，O(cols) 每步。
 */
export class HeightField {
  readonly cols: number;
  readonly cap: number;
  heavy: Int32Array;
  light: Int32Array;
  wavePhase: number = 0;
  private movedThisStep: boolean = false;
  /** 每步迁移上限（渐进流动速度，太大一步到位失去流动感） */
  private maxPerStep: number;
  private heavyTarget: Int32Array;
  private lightTarget: Int32Array;
  /** 体积缓存（液面/界面截距二分用，step 内多次 computeTargets 共享） */
  private volHCached: number = 0;
  private volLCached: number = 0;
  /** 平衡快速跳过：上一步的角度与是否仍有流动 */
  private lastTilt: number = 999;
  private movedLastStep: boolean = true;

  constructor(cols: number, cap: number) {
    this.cols = cols;
    this.cap = cap;
    this.heavy = new Int32Array(cols);
    this.light = new Int32Array(cols);
    this.heavyTarget = new Int32Array(cols);
    this.lightTarget = new Int32Array(cols);
    // 限流与规模挂钩：每步约迁移总量的 1/10，兼顾流动感与收敛速度
    this.maxPerStep = Math.max(4, Math.floor(cols * cap * 0.35 / 10));
  }

  total(c: number): number {
    return this.heavy[c] + this.light[c];
  }

  totalCount(): number {
    let n = 0;
    for (let c = 0; c < this.cols; c++) {
      n += this.total(c);
    }
    return n;
  }

  hasMoved(): boolean {
    return this.movedThisStep;
  }

  /**
   * 二分求截距 C：sum(clamp(tanEff·c + C, 0, cap)) == vol
   * 返回精确截距（单调函数，二分 60 次达 double 精度）
   */
  private solveLine(tanEff: number, vol: number): number {
    const n = this.cols;
    const cap = this.cap;
    let lo = -cap - Math.abs(tanEff) * n - 1;
    let hi = cap + Math.abs(tanEff) * n + 1;
    for (let it = 0; it < 40; it++) {
      const mid = (lo + hi) / 2;
      let s = 0;
      for (let c = 0; c < n; c++) {
        let t = tanEff * c + mid;
        if (t < 0) {
          t = 0;
        } else if (t > cap) {
          t = cap;
        }
        s += t;
      }
      if (s < vol) {
        lo = mid;
      } else {
        hi = mid;
      }
    }
    return (lo + hi) / 2;
  }

  /**
   * 计算两相目标剖面（平行界面模型）。
   * 先定液面线 C_T（总体积），再定界面线 C_H（重相体积，且恒在液面线下方）。
   * useCache=true 时用缓存的体积（同一 step 内 migrate 不改变总量，可跳过重扫）。
   */
  private computeTargets(tiltDeg: number, useCache: boolean = false): void {
    const n = this.cols;
    const cap = this.cap;
    const tanT = Math.tan(tiltDeg * Math.PI / 180);
    if (!useCache) {
      let volH = 0;
      let volL = 0;
      for (let c = 0; c < n; c++) {
        volH += this.heavy[c];
        volL += this.light[c];
      }
      this.volHCached = volH;
      this.volLCached = volL;
    }
    const CT = this.solveLine(tanT, this.volHCached + this.volLCached);
    const CH = this.solveLine(tanT, this.volHCached);
    for (let c = 0; c < n; c++) {
      const v = tanT * c;
      let ih = v + CH;
      if (ih < 0) {
        ih = 0;
      } else if (ih > cap) {
        ih = cap;
      }
      let it = v + CT;
      if (it < 0) {
        it = 0;
      } else if (it > cap) {
        it = cap;
      }
      this.heavyTarget[c] = Math.round(ih);
      this.lightTarget[c] = Math.round(it) - Math.round(ih);
    }
    // 整数残差修正（round 误差 < n，逐粒分配到未满列）
    this.fixResidual(this.heavy, this.heavyTarget, this.volHCached);
    this.fixResidual(this.light, this.lightTarget, this.volLCached);
  }

  /** 整数目标与体积的残差修正（diff < n，向两侧端部分配） */
  private fixResidual(field: Int32Array, target: Int32Array, vol: number): void {
    let diff = vol;
    for (let c = 0; c < this.cols; c++) {
      diff -= target[c];
    }
    let gi = 0;
    while (diff > 0 && gi < this.cols * 2) {
      const c = gi % this.cols;
      if (target[c] < this.cap) {
        target[c]++;
        diff--;
      }
      gi++;
    }
    while (diff < 0 && gi < this.cols * 2) {
      const c = gi % this.cols;
      if (target[c] > 0) {
        target[c]--;
        diff++;
      }
      gi++;
    }
  }

  /**
   * 单场配对搬运：donor（高于目标）向 receiver（低于目标）逐粒迁移。
   * receiver 需满足 total+1 <= cap（轻相给重相腾位时自动受限）。
   */
  private migrate(field: Int32Array, target: Int32Array, maxPerStep: number): number {
    const n = this.cols;
    let moved = 0;
    let donorCur = 0;
    let recvCur = 0;
    let rescans = 0;
    while (moved < maxPerStep) {
      while (donorCur < n && field[donorCur] <= target[donorCur]) {
        donorCur++;
      }
      while (recvCur < n && (field[recvCur] >= target[recvCur] || this.total(recvCur) + 1 > this.cap)) {
        recvCur++;
      }
      if (donorCur >= n || recvCur >= n) {
        if (rescans > 200) {
          break;
        }
        rescans++;
        donorCur = 0;
        recvCur = 0;
        continue;
      }
      field[donorCur]--;
      field[recvCur]++;
      moved++;
      if (field[donorCur] <= target[donorCur]) {
        donorCur++;
      }
      if (field[recvCur] >= target[recvCur] || this.total(recvCur) >= this.cap) {
        recvCur++;
      }
    }
    return moved;
  }

  /**
   * 单步模拟
   * @param tiltDeg 倾斜角（度，正=右倾）
   * @return 本步迁移粒子数
   */
  step(tiltDeg: number): number {
    this.movedThisStep = false;
    this.wavePhase += 0.1;
    // 角度未变且已平衡 → 快速跳过（防静止场景白烧 CPU）
    if (tiltDeg === this.lastTilt && !this.movedLastStep) {
      this.wavePhase -= 0.1;
      return 0;
    }
    this.lastTilt = tiltDeg;
    // 迁移不改变各相总量，同一 step 内二分输入不变 → 目标剖面只需算一次
    this.computeTargets(tiltDeg);
    let total = 0;
    for (let round = 0; round < 4; round++) {
      // 先轻相腾位、再重相填充：大角度时重相目标列常被轻相占着，
      // 若重相先行会因 cap 约束卡住；轻相先行一步后空间即让出
      const movedL = this.migrate(this.light, this.lightTarget, this.maxPerStep);
      const movedH = this.migrate(this.heavy, this.heavyTarget, this.maxPerStep);
      total += movedL + movedH;
      if (movedL > 0 || movedH > 0) {
        continue;
      }
      // 两相均卡死 → 跨列交换：极端角度下会出现互锁
      // （列 i 需重相流入但其轻相无处去；列 j 需轻相流入但其重相无处去），
      // 单相 migrate 无法破局，需原子交换：i 的轻相换 j 的重相（total 双方不变）。
      let swapped = false;
      for (let i = 0; i < this.cols && !swapped; i++) {
        if (this.heavy[i] < this.heavyTarget[i] && this.total(i) >= this.cap
            && this.light[i] > this.lightTarget[i]) {
          for (let j = 0; j < this.cols && !swapped; j++) {
            if (this.light[j] < this.lightTarget[j] && this.total(j) >= this.cap
                && this.heavy[j] > this.heavyTarget[j]) {
              this.heavy[i]++;
              this.light[i]--;
              this.heavy[j]--;
              this.light[j]++;
              swapped = true;
              total++;
            }
          }
        }
      }
      if (!swapped) {
        break;
      }
    }
    this.movedLastStep = total > 0;
    if (this.movedLastStep) {
      this.movedThisStep = true;
    }
    return total;
  }
}
