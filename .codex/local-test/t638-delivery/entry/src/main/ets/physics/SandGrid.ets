/**
 * SandGrid - 元胞自动机流沙引擎（falling-sand cellular automaton）· 两相流麻版
 *
 * 复刻流沙画/流麻效果：两种密度的细沙在密封框内随重力方向流动，
 * 摇晃时混合、静置后按密度分层（重沙沉底、轻沙上浮），
 * 堆出自然安息角斜面，静止后完全稳定（零抖动）。
 *
 * 算法要点：
 * - 网格每格状态：EMPTY(0) / SAND(1) / WALL(2)，phase 区分两相（0=绿/重 1=蓝/轻）
 * - 每帧从重力"远端"往"近端"扫描，扫描顺序跟随主方向防连锁瞬移
 * - 沙粒尝试：主方向直落 → 液体置换（重沙可穿过轻沙下沉）→ 对角滑移 → 静止
 * - 两相密度差：重沙(phase0)可向下置换轻沙(phase1)，轻沙反向浮起 → 混合后静置自动分层
 * - 对角滑移带 slideChance 概率控制安息角
 * - 静止沙粒零操作 = 零抖动核心
 */
export class SandGrid {
  static readonly EMPTY: number = 0;
  static readonly SAND: number = 1;
  static readonly WALL: number = 2;

  readonly cols: number;
  readonly rows: number;
  private cells: Uint8Array;
  /** 每格颜色相位（0=绿/重沙 1=蓝/轻沙），渲染时区分色彩 */
  private phase: Uint8Array;
  /** 每格随机亮度扰动索引，渲染时制造细沙颗粒感 */
  private shade: Uint8Array;
  /** 扫描方向交替标志（垂直重力时消除左右偏差） */
  private scanForward: boolean = true;
  /** 对角滑移概率 [0,1]，保留接口兼容（当前滑移不设门槛） */
  private slideChance: number = 0.85;
  /** 重沙穿过轻沙的渗透概率 [0,1]——颗粒交换速率（分层/渗透速度），需压力条件成立才生效 */
  private sinkChance: number = 0.022;
  /** 液体模式（M1' 场景一）：true 时走液体相规则——类型不可变 + 单向置换
   *  （重粒下换轻粒，displaceChance）+ 水平铺平，关推挤链/安息角滑移。
   *  Node 标定（m1_calib3/final_check）：dc=0.5 时静置波动=0、零抖动、
   *  翻转回复 2.8s、摇晃混合→静置恢复、激烈摇晃期间分相守恒全 PASS。 */
  private liquidMode: boolean = false;
  /** 液体模式置换概率 [0,1]：重粒向重力方向置换轻粒的每帧概率（浮力驱动） */
  private displaceChance: number = 0.5;
  /** 黏性活动概率 [0,1]：每粒每帧尝试移动的门槛，统一放慢全部动力学
   *  （真实流麻是黏滞液体；1.0=全速 CA 沙粒，翻转重排 1s 内完成显跳变；
   *  0.12 ≈ 重排 2-3s，无跳变且倒立响应快） */
  private slowFactor: number = 0.12;
  /** 液体水平流动激活概率 [0,1] */
  private liquidChance: number = 0.8;
  /** 液体单步最大水平流动距离（格）——越大铺平越快 */
  private flowRange: number = 6;
  /** 是否已有沙粒移动（用于外部判断是否需要重绘） */
  private movedThisStep: boolean = false;

  constructor(cols: number, rows: number) {
    this.cols = cols;
    this.rows = rows;
    this.cells = new Uint8Array(cols * rows);
    this.phase = new Uint8Array(cols * rows);
    this.shade = new Uint8Array(cols * rows);
    for (let i = 0; i < this.shade.length; i++) {
      this.shade[i] = Math.floor(Math.random() * 8);
    }
  }

  idx(cx: number, cy: number): number {
    return cy * this.cols + cx;
  }

  get(cx: number, cy: number): number {
    if (cx < 0 || cx >= this.cols || cy < 0 || cy >= this.rows) {
      return SandGrid.WALL; // 越界视为墙
    }
    return this.cells[cy * this.cols + cx];
  }

  getPhase(cx: number, cy: number): number {
    return this.phase[cy * this.cols + cx];
  }

  getShade(cx: number, cy: number): number {
    return this.shade[cy * this.cols + cx];
  }

  setCell(cx: number, cy: number, type: number, phase: number = 0): void {
    if (cx < 0 || cx >= this.cols || cy < 0 || cy >= this.rows) {
      return;
    }
    const i = cy * this.cols + cx;
    this.cells[i] = type;
    this.phase[i] = phase;
  }

  setSlideChance(c: number): void {
    this.slideChance = Math.max(0, Math.min(1, c));
  }

  setSinkChance(c: number): void {
    this.sinkChance = Math.max(0, Math.min(1, c));
  }

  /** 开关液体模式（M1' 场景一）：模式切换由外部（SandCanvas）控制 */
  setLiquidMode(on: boolean): void {
    this.liquidMode = on;
  }

  isLiquidMode(): boolean {
    return this.liquidMode;
  }

  hasMoved(): boolean {
    return this.movedThisStep;
  }

  /** 用整块区域填充沙（保证初始排布紧密无缝隙） */
  fillRect(x0: number, y0: number, x1: number, y1: number, type: number, phase: number): void {
    for (let cy = y0; cy <= y1; cy++) {
      for (let cx = x0; cx <= x1; cx++) {
        this.setCell(cx, cy, type, phase);
      }
    }
  }

  sandCount(): number {
    let n = 0;
    for (let i = 0; i < this.cells.length; i++) {
      if (this.cells[i] === SandGrid.SAND) {
        n++;
      }
    }
    return n;
  }

  /**
   * 单步模拟。
   * @param gx 重力向量 x 分量（任意单位，内部归一化）
   * @param gy 重力向量 y 分量
   */
  step(gx: number, gy: number): void {
    this.movedThisStep = false;
    const mag = Math.sqrt(gx * gx + gy * gy);
    if (mag < 1e-6) {
      return; // 无重力，完全静止
    }
    const nx = gx / mag;
    const ny = gy / mag;

    // 主移动方向：按 |nx|:|ny| 概率比例随机选轴。
    // 固定取主导轴会让 45° 倾斜时沙完全锁死（斜格全被占）；
    // 概率分配让倾斜时垂直/水平滚动交替发生，沙堆才会塌向一侧形成斜坡流动
    let dx: number;
    let dy: number;
    if (Math.random() < Math.abs(nx) / (Math.abs(nx) + Math.abs(ny))) {
      dx = nx > 0 ? 1 : -1;
      dy = 0;
    } else {
      dx = 0;
      dy = ny > 0 ? 1 : -1;
    }

    // 两个对角滑移候选方向（主方向 + 侧向分量）
    const sideX = dx === 0 ? 1 : 0;
    const sideY = dy === 0 ? 1 : 0;
    const slide1X = dx + sideX;
    const slide1Y = dy + sideY;
    const slide2X = dx - sideX;
    const slide2Y = dy - sideY;

    // 扫描顺序：必须从重力方向的"远端"往"近端"扫，
    // 保证沙粒移动到的格子已被本轮扫描过，防止一帧内连锁瞬移
    const cyStart = dy === 1 ? this.rows - 1 : 0;
    const cyStep = dy === 1 ? -1 : 1;
    for (let cy = cyStart; cy >= 0 && cy < this.rows; cy += cyStep) {
      if (this.liquidMode) {
        // 液体模式：同扫描纪律，走 tryLiquidMove（关推挤/滑移）
        if (dx === 1) {
          for (let cx = this.cols - 1; cx >= 0; cx--) {
            this.tryLiquidMove(cx, cy, dx, dy);
          }
        } else if (dx === -1) {
          for (let cx = 0; cx < this.cols; cx++) {
            this.tryLiquidMove(cx, cy, dx, dy);
          }
        } else {
          const forward = this.scanForward;
          this.scanForward = !this.scanForward;
          if (forward) {
            for (let cx = 0; cx < this.cols; cx++) {
              this.tryLiquidMove(cx, cy, dx, dy);
            }
          } else {
            for (let cx = this.cols - 1; cx >= 0; cx--) {
              this.tryLiquidMove(cx, cy, dx, dy);
            }
          }
        }
        continue;
      }
      if (dx === 1) {
        for (let cx = this.cols - 1; cx >= 0; cx--) {
          this.tryMove(cx, cy, dx, dy, slide1X, slide1Y, slide2X, slide2Y);
        }
      } else if (dx === -1) {
        for (let cx = 0; cx < this.cols; cx++) {
          this.tryMove(cx, cy, dx, dy, slide1X, slide1Y, slide2X, slide2Y);
        }
      } else {
        const forward = this.scanForward;
        this.scanForward = !this.scanForward;
        if (forward) {
          for (let cx = 0; cx < this.cols; cx++) {
            this.tryMove(cx, cy, dx, dy, slide1X, slide1Y, slide2X, slide2Y);
          }
        } else {
          for (let cx = this.cols - 1; cx >= 0; cx--) {
            this.tryMove(cx, cy, dx, dy, slide1X, slide1Y, slide2X, slide2Y);
          }
        }
      }
    }
  }

  private tryMove(cx: number, cy: number,
                  dx: number, dy: number,
                  s1x: number, s1y: number, s2x: number, s2y: number): void {
    const i = cy * this.cols + cx;
    if (this.cells[i] !== SandGrid.SAND) {
      return;
    }
    // 黏性门槛：统一放慢所有动力学（液体观感核心）
    if (Math.random() >= this.slowFactor) {
      return;
    }
    const myPhase = this.phase[i];

    // 1. 主方向可走 → 直落
    const belowType = this.get(cx + dx, cy + dy);
    if (belowType === SandGrid.EMPTY) {
      this.moveTo(cx, cy, cx + dx, cy + dy);
      return;
    }

    // 2. 主方向被沙挡住 → 推挤：仅同相推挤（异相推挤会整块顶开对方，
    //    破坏"慢慢渗透"效果）。渗透只走 canEnterSlide 的颗粒交换通道
    if (belowType === SandGrid.SAND) {
      const pushOk = myPhase === this.getPhase(cx + dx, cy + dy) && Math.random() < 0.9;
      if (pushOk && this.tryPush(cx + dx, cy + dy, dx, dy, 0)) {
        this.moveTo(cx, cy, cx + dx, cy + dy);
        return;
      }
    }

    // 3. 滑移/流动（重力无关：垂直重力时对角滑移，水平重力时向斜下/侧向滑，
    //    修复旧版水平重力只有推挤一条路导致的锁死）
    {
      const first = Math.random() < 0.5;
      const ax = first ? s1x : s2x;
      const ay = first ? s1y : s2y;
      const bx = first ? s2x : s1x;
      const by = first ? s2y : s1y;

      if (this.canEnterSlide(cx + ax, cy + ay, myPhase)) {
        this.moveToOrSwap(cx, cy, cx + ax, cy + ay);
        return;
      }
      if (this.canEnterSlide(cx + bx, cy + by, myPhase)) {
        this.moveToOrSwap(cx, cy, cx + bx, cy + by);
        return;
      }

      // 4. 液体水平流动（仅垂直重力时侧向=水平才有意义；
      //    水平重力时侧向是竖直方向，直落已覆盖）
      if (dx === 0 && this.flowRange > 0 && Math.random() < this.liquidChance) {
        const dir: number = Math.random() < 0.5 ? 1 : -1;
        const lx = this.tryLiquidFlow(cx, cy, dir, myPhase);
        if (lx !== cx) {
          this.moveTo(cx, cy, lx, cy);
          return;
        }
        const rx = this.tryLiquidFlow(cx, cy, 0 - dir, myPhase);
        if (rx !== cx) {
          this.moveTo(cx, cy, rx, cy);
          return;
        }
      }
    }
    // 5. 各方向均堵 → 静止（零操作，零抖动核心）
  }

  /**
   * 液体模式单粒移动（M1'，Node 1:1 移植自 m1_liquid_test2.js 终版规则）：
   * 1. 直落全速（CA 每帧 1 格离散，无瞬移风险；黏滞观感由界面渐变+铺平承担）；
   * 2. 单向置换：仅重粒向重力方向换轻粒，压力门控（上方是同相液体）+ displaceChance，
   *    换位不改变类型 → 分相守恒恒等式；豁免 slowFactor（浮力交换是快物理，
   *    slowFactor 会把翻转重排压到 19s+，Node 实测 FAIL）；
   * 3. 水平铺平：复用 tryLiquidFlow，受 slowFactor+liquidChance；
   * 4. 静止零操作。
   */
  private tryLiquidMove(cx: number, cy: number, dx: number, dy: number): void {
    const i = cy * this.cols + cx;
    if (this.cells[i] !== SandGrid.SAND) {
      return;
    }
    const myPhase = this.phase[i];
    // 1. 直落：全速
    if (this.get(cx + dx, cy + dy) === SandGrid.EMPTY) {
      this.moveTo(cx, cy, cx + dx, cy + dy);
      return;
    }
    // 2. 单向置换（仅重粒发起）：重力方向是轻粒 + 上方是同相液体（压力门控）
    if (myPhase === 0 &&
        this.get(cx + dx, cy + dy) === SandGrid.SAND &&
        this.getPhase(cx + dx, cy + dy) === 1 &&
        this.get(cx - dx, cy - dy) === SandGrid.SAND &&
        this.getPhase(cx - dx, cy - dy) === myPhase) {
      if (Math.random() < this.displaceChance) {
        this.swap(cx, cy, cx + dx, cy + dy);
        return;
      }
    }
    // 3. 水平铺平（仅垂直重力时侧向=水平才有意义）
    if (dx === 0 && Math.random() < this.slowFactor && Math.random() < this.liquidChance) {
      const dir: number = Math.random() < 0.5 ? 1 : -1;
      const lx = this.tryLiquidFlow(cx, cy, dir, myPhase);
      if (lx !== cx) {
        this.moveTo(cx, cy, lx, cy);
        return;
      }
      const rx = this.tryLiquidFlow(cx, cy, 0 - dir, myPhase);
      if (rx !== cx) {
        this.moveTo(cx, cy, rx, cy);
        return;
      }
    }
    // 4. 静止零操作
  }

  /**
   * 推挤链：询问 (cx,cy) 的沙粒能否沿 (dx,dy) 挪一格。
   * 能则先把它挪走并返回 true（调用者随后占据该格）。
   * depth 限制链长，防止全屏连锁；遇墙/越界/超深返回 false。
   */
  private tryPush(cx: number, cy: number, dx: number, dy: number, depth: number): boolean {
    if (depth > 8) {
      return false; // 链长上限：避免 O(n) 全屏传递
    }
    if (Math.random() >= this.slowFactor) {
      return false; // 推链同样受黏性约束
    }
    const t = this.get(cx, cy);
    if (t === SandGrid.EMPTY) {
      return true; // 前方是空位，直接可挪
    }
    if (t !== SandGrid.SAND) {
      return false; // 墙
    }
    // 前方是沙：递归推它
    if (!this.tryPush(cx + dx, cy + dy, dx, dy, depth + 1)) {
      return false;
    }
    this.moveTo(cx, cy, cx + dx, cy + dy);
    return true;
  }

  /** 对角滑移进入判定：空格直接进；重相进轻相格按 sinkChance 概率渗透（颗粒交换，
   *  蓝被压在绿下时绿沙缓慢下沉/蓝沙慢慢上浮的核心通道）+ 压力条件（正上方是沙） */
  private canEnterSlide(cx: number, cy: number, myPhase: number): boolean {
    const t = this.get(cx, cy);
    if (t === SandGrid.EMPTY) {
      return true;
    }
    return t === SandGrid.SAND && myPhase === 0 &&
           this.getPhase(cx, cy) === 1 &&
           this.get(cx, cy - 1) === SandGrid.SAND &&
           Math.random() < this.sinkChance;
  }

  /**
   * 液体水平流动：从 (cx,cy) 沿 dir 方向寻找最远可达格（最多 flowRange 格）。
   * 途经格须可进入；且只有当流动终点的"下坡路"通畅（终点侧下方非实心沙）时才流动，
   * 这使液体只往更低处流——液面趋于水平的宏观表现。
   */
  private tryLiquidFlow(cx: number, cy: number, dir: number, myPhase: number): number {
    let best = cx;
    for (let step = 1; step <= this.flowRange; step++) {
      const nx = cx + dir * step;
      // 遇到任何非空格立即停止——液体不能穿过其他液体（否则视觉瞬移）
      if (this.get(nx, cy) !== SandGrid.EMPTY) {
        break;
      }
      best = nx;
      // 流动中途发现下坡（侧下方为空）→ 到此为止，滑下去更自然
      if (this.get(nx, cy + 1) === SandGrid.EMPTY) {
        break;
      }
    }
    return best;
  }

  /** 该格是否可进入：空格，或（重沙时）轻沙格 */
  private canEnter(cx: number, cy: number, myPhase: number): boolean {
    const t = this.get(cx, cy);
    if (t === SandGrid.EMPTY) {
      return true;
    }
    return t === SandGrid.SAND && myPhase === 0 && this.getPhase(cx, cy) === 1;
  }

  private moveToOrSwap(x0: number, y0: number, x1: number, y1: number): void {
    const t = this.get(x1, y1);
    if (t === SandGrid.EMPTY) {
      this.moveTo(x0, y0, x1, y1);
    } else {
      this.swap(x0, y0, x1, y1);
    }
  }

  private moveTo(x0: number, y0: number, x1: number, y1: number): void {
    const i0 = y0 * this.cols + x0;
    const i1 = y1 * this.cols + x1;
    this.cells[i1] = SandGrid.SAND;
    this.phase[i1] = this.phase[i0];
    this.shade[i1] = this.shade[i0];
    this.cells[i0] = SandGrid.EMPTY;
    this.movedThisStep = true;
  }

  private swap(x0: number, y0: number, x1: number, y1: number): void {
    const i0 = y0 * this.cols + x0;
    const i1 = y1 * this.cols + x1;
    const tmpCell = this.cells[i0];
    const tmpPhase = this.phase[i0];
    const tmpShade = this.shade[i0];
    this.cells[i0] = this.cells[i1];
    this.phase[i0] = this.phase[i1];
    this.shade[i0] = this.shade[i1];
    this.cells[i1] = tmpCell;
    this.phase[i1] = tmpPhase;
    this.shade[i1] = tmpShade;
    this.movedThisStep = true;
  }
}
