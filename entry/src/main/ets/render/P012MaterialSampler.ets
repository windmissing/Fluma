// P-012 C/E3 共享连续材质采样器（architecture-P012.md §11.2；design §4）
// 问题 ID：P-012；方案版本：p012-render-material-v1（C-batch，C1）
// 职责：「任意显示 x（px）→ 连续材料层高」一层。输入 renderer 已有的
//   smooth 数组（buildDisplayProfiles 每帧更新的 128 列平滑值），输出
//   bh / mixStart / sandTop 的连续插值。
// 红线：纯函数式、只读，不做二套平滑/无约束高阶样条；物理列 128 不变，
//   轮廓绘制（drawSandColumns 逐列 fillRect）不经本类。
// 共享边界（§11.2）：C 在分段点调用（K+1 次/带）；E3 未来雾 alpha/材质密度
//   采样直接 import 本类，禁止第二套插值实现。
// 保层序证明（非负权重线性插值）：t∈[0,1)、权重 (1-t,t) 非负 →
//   每条边界曲线是列值的凸组合 → 逐点保持 bottomH ≤ mixStart(x) ≤ bh(x)
//   ≤ sandTop(x)（因列级 smooth 数组本身满足该序，凸组合保序）。

/** 插值定位结果（ArkTS 禁止数组解构，用显式类） */
class P012SamplerSplit {
  i: number = 0;
  t: number = 0;
}

export class P012MaterialColumnSampler {
  private readonly cols: number;
  private readonly colW: number;
  private readonly bh: Float64Array;
  private readonly mixStart: Float64Array;
  private readonly sandTop: Float64Array;
  /** 复用定位对象（避免逐采样分配；单线程渲染路径内安全） */
  private readonly splitResult: P012SamplerSplit = new P012SamplerSplit();

  constructor(bh: Float64Array, mixStart: Float64Array, sandTop: Float64Array,
    boxW: number) {
    this.bh = bh;
    this.mixStart = mixStart;
    this.sandTop = sandTop;
    this.cols = Math.max(1, bh.length);
    this.colW = boxW / this.cols;
  }

  /** 列索引与插值权重（x clamp 到 [0, boxW)）。写进复用对象 splitResult。 */
  private split(x: number): P012SamplerSplit {
    const cx = Math.max(0, Math.min(this.cols - 1e-6, x / this.colW));
    let i = Math.floor(cx);
    let t = cx - i;
    // x 贴右缘：clamp 后 i 可能 = cols-1 → i+1 越界，退化为 (cols-2, 1)
    if (i >= this.cols - 1) {
      i = this.cols - 2;
      t = 1;
    }
    this.splitResult.i = i;
    this.splitResult.t = t;
    return this.splitResult;
  }

  /** 重沙层顶（连续高度，材料单位） */
  bhAtPx(x: number): number {
    const s = this.split(x);
    return this.bh[s.i] * (1 - s.t) + this.bh[s.i + 1] * s.t;
  }

  /** 轻+重沙分界（bh+bl，连续高度） */
  mixStartAtPx(x: number): number {
    const s = this.split(x);
    return this.mixStart[s.i] * (1 - s.t) + this.mixStart[s.i + 1] * s.t;
  }

  /** 沙顶（sandTop，连续高度） */
  sandTopAtPx(x: number): number {
    const s = this.split(x);
    return this.sandTop[s.i] * (1 - s.t) + this.sandTop[s.i + 1] * s.t;
  }
}
