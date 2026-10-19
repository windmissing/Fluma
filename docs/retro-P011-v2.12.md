# P-011 v2.12 双角度滞回设计记录

> route：`natural-relax-hysteresis-v1`；状态：`DESIGN_PENDING_OBSERVATION`；日期：2026-10-19。

## 候选方案

以每条相邻边 `r=|S[c+1]-S[c]-tan(effectiveTilt)*dx|/dx` 为坡度残差。静止时 `r>=tan(thetaStart)` 开始滑；滑行时 `r<=tan(thetaStop)` 停止；中间区间保持当前 active 状态。滑动方向由有符号残差确定，通量沿用当前 `maxAvalancheFlux*dt`、donor/receiver 容量和既有 total/local 预算。状态不携带目标形状，不给任何 ROI 或列号优先级。

阈值的搜索中心暂记 `thetaStart≈30°`、`thetaStop≈20°`，只用于规划观测范围。第 1 步 natural-observation-v2.10 数据给出 ±15° relax 请求/应用总量（3125.60/718.10 与 3287.77/731.64），但没有逐边坡度残差转折记录，无法从 aggregate flux 唯一估计阈值。故本轮不填写实值；需要后续逐边记录起动/停止前后残差、有效倾角、授予/限流和状态转移，并在留出运行复核置信区间。

## 约束与未决项

双角度只是状态阈值，不改变 `thetaHardMax=30°` 的现行硬门槛，也不覆盖 thick 的运行参数 `angleDeg=4°`；这三个量须在架构审查中保持语义分离。每次应用后仍审计 `r<=tan(thetaHardMax)`。所有改变 `sandTop` 的算子必须受不增大越界残差的 admissibility guard；若倾角变化或剩余预算导致超限不可恢复，则报告 FAIL/BLOCKED，不放宽预算。

`sliding[cols-1]` 在连续 run 内保留，预算限流不清状态；reset、几何重建、TEST/EXPERIENCE 切换清空。守恒、stock/capacity、每列 `vSandTopMax*dt`、bottom `10*dt`、total `300*dt` 和无目标形状原则全部沿用。每步 `O(cols)` 时间与 `O(cols)` 状态。

## 进入实现前条件

1. 取得起动与停止的逐边观测转折区间及留出验证；再以观测值替换两个占位符。
2. architect 审核 angle hard guard 对每个改变 `sandTop` 的通道都闭合，且不引入额外预算/几何/优先形状。
3. 若任一条件不满足，维持 `DESIGN_PENDING_OBSERVATION`，不交 developer、不启动 tester。代码和测试用例未修改。


## 候选取舍

- **逐边双阈值（选择）**：`sliding[c]` 对应局部坡面残差，状态只持续到该边低于 stop；空间上保持现有局部规则，不需目标形状，额外内存 `O(cols)`。
- **全局单一滑动状态（拒绝）**：任一边越过 start 就令整场崩塌持续到全局最小/最大残差满足 stop，会让远离超差边的接口也参加崩塌，扩大预算竞争并可能制造长波变化。
- **输入角平滑/单阈值死区（拒绝）**：只平滑 `effectiveTilt` 或在一个阈值附近加 deadzone，无法表达“高阈值启动、低阈值停止、区间保持”的路径依赖；还会混淆传感器滤波与材料状态。
