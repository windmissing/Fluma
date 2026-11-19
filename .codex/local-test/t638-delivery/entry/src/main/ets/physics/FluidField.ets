/**
 * FluidField - 混合模型：高度场形态（平直界面）+ 渗透混合带
 *
 * 架构（CA 渗透的局限：颗粒界面天然锯齿，后处理拟合不出平直界面）：
 *   形态层：Float 高度场，全局目标剖面（平行界面平衡）+ 配对迁移 + budget 限速
 *           —— 界面天然平直（稳态粗糙度 0.000px），翻转渐变 2.2s（90% 重排）
 *   渗透层：ero[c] 混合带深度，独立速率增长（≈40s 渗透完成），与形态解耦
 *   cap 溢出修复：total>cap 的列把超出量挤到邻列（解除饱和锁死）
 *
 * 渲染：液面线 + 两相界面线都是平滑曲线；ero 决定界面附近的混合带颜色
 */
export class FluidField {
  readonly cols: number;
  readonly cap: number;
  heavy: Float64Array;
  light: Float64Array;
  /** 渗透混合带深度 [0, light]——界面附近蓝绿混合的比例依据 */
  ero: Float64Array;
  /** 黏性：每帧总迁移量 = 总误差 × slowRate（0.008 ≈ 翻转重排 2.2s） */
  private readonly slowRate: number = 0.008;
  /** 渗透速率：ero 每帧增长系数（0.0004 ≈ 40s 渗透完成） */
  private readonly eroRate: number = 0.0004;
  private heavyT: Float64Array;
  private totalT: Float64Array;
  private lightT: Float64Array;
  private movedThisStep: boolean = false;

  constructor(cols: number, cap: number) {
    this.cols = cols;
    this.cap = cap;
    this.heavy = new Float64Array(cols);
    this.light = new Float64Array(cols);
    this.ero = new Float64Array(cols);
    this.heavyT = new Float64Array(cols);
    this.totalT = new Float64Array(cols);
    this.lightT = new Float64Array(cols);
  }

  total(c: number): number {
    return this.heavy[c] + this.light[c];
  }

  hasMoved(): boolean {
    return this.movedThisStep;
  }

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

  private computeTargets(tiltDeg: number): void {
    const n = this.cols;
    const cap = this.cap;
    const tanT = Math.tan(tiltDeg * Math.PI / 180);
    let volH = 0;
    let volL = 0;
    for (let c = 0; c < n; c++) {
      volH += this.heavy[c];
      volL += this.light[c];
    }
    const CT = this.solveLine(tanT, volH + volL);
    const CH = this.solveLine(tanT, volH);
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
      this.heavyT[c] = ih;
      this.totalT[c] = it;
      this.lightT[c] = it - ih;
    }
  }

  /** 单列配对迁移（budget 限速：每帧总迁移量 = 总误差 × slowRate） */
  private migratePhase(field: Float64Array, target: Float64Array, budget: number): number {
    const n = this.cols;
    let moved = 0;
    let donorCur = 0;
    let recvCur = 0;
    let rescans = 0;
    while (moved < budget) {
      while (donorCur < n && field[donorCur] <= target[donorCur] + 0.05) {
        donorCur++;
      }
      while (recvCur < n && (field[recvCur] >= target[recvCur] - 0.05 || this.total(recvCur) >= this.cap)) {
        recvCur++;
      }
      if (donorCur >= n || recvCur >= n) {
        if (rescans > 30) {
          break;
        }
        rescans++;
        donorCur = 0;
        recvCur = 0;
        continue;
      }
      let amt = Math.min(field[donorCur] - target[donorCur], target[recvCur] - field[recvCur], budget - moved);
      amt = Math.min(amt, this.cap - this.total(recvCur));  // cap 硬约束（防浮点累积越界）
      if (amt <= 0) {
        recvCur++;
        continue;
      }
      field[donorCur] -= amt;
      field[recvCur] += amt;
      moved += amt;
      if (field[donorCur] <= target[donorCur] + 0.05) {
        donorCur++;
      }
      if (field[recvCur] >= target[recvCur] - 0.05 || this.total(recvCur) >= this.cap) {
        recvCur++;
      }
    }
    return moved;
  }

  /** cap 溢出修复：total>cap 的列把超出量挤到相邻未饱和列（解除饱和锁死） */
  private repairCapOverflow(): void {
    const n = this.cols;
    for (let c = 0; c < n; c++) {
      const over = this.total(c) - this.cap;
      if (over > 0.01) {
        let nb = -1;
        if (c > 0 && this.total(c - 1) < this.cap - 0.01) {
          nb = c - 1;
        } else if (c < n - 1 && this.total(c + 1) < this.cap - 0.01) {
          nb = c + 1;
        }
        if (nb < 0) {
          continue;
        }
        // 先挤 light（轻相浮到邻列更物理），不够再挤 heavy
        let move = Math.min(over, this.light[c]);
        this.light[c] -= move;
        this.light[nb] += move;
        const remain = over - move;
        if (remain > 0.01 && this.heavy[c] > 0) {
          const m2 = Math.min(remain, this.heavy[c]);
          this.heavy[c] -= m2;
          this.heavy[nb] += m2;
        }
      }
    }
  }

  /**
   * 单步模拟
   * @param tiltDeg 倾斜角（度，正=左倾即列号大处低）
   * @return 本步迁移量
   */
  step(tiltDeg: number): number {
    this.movedThisStep = false;
    const n = this.cols;
    this.computeTargets(tiltDeg);
    // 总误差 → 本帧迁移预算（指数趋近，天然渐变）
    let totalErr = 0;
    for (let c = 0; c < n; c++) {
      totalErr += Math.abs(this.light[c] - this.lightT[c]);
      totalErr += Math.abs(this.heavy[c] - this.heavyT[c]);
    }
    // 回正收敛加速：接近水平时 Budget 小收敛拖尾——提高速率
    const rate = Math.abs(tiltDeg) < 8 ? this.slowRate * 3 : this.slowRate;
    const budget = totalErr * rate + 0.02;
    const mL = this.migratePhase(this.light, this.lightT, budget * 0.5);
    // 重算 target（light 移动后 heavy 目标界面的列位不变，但预算重置）
    const mH = this.migratePhase(this.heavy, this.heavyT, budget * 0.5);
    this.repairCapOverflow();
    // 渗透层：ero 深度增长
    for (let c = 0; c < n; c++) {
      if (this.heavy[c] > 1 && this.light[c] > 1) {
        this.ero[c] = Math.min(this.light[c], this.ero[c] + this.eroRate * this.light[c]);
      } else {
        this.ero[c] = Math.max(0, this.ero[c] - 0.01 * this.ero[c]);
      }
    }
    if (mL + mH > 0.01) {
      this.movedThisStep = true;
    }
    return mL + mH;
  }
}
