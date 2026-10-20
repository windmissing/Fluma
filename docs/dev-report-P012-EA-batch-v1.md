# P-012 EA-batch 开发报告 v1（developer）

- **日期**：2026-10-20。责任：developer。依据：architecture-P012.md §9.3 EA-batch + algorithm-P012-render-v1.md §4 + 滴答任务 6ad71fabe4b05503d543ff4f。
- **技术路线**：`p012-render-material-v1`（E+A 档）。前置：E-batch commit `0120fe6`。
- **状态**：EA-batch 代码交付 + 单测 22/22 + 真机三锚点达标（见 §4）；帧时配对首轮归 tester。

## 1. 修改文件清单

| 文件 | 操作 | 内容 |
|---|---|---|
| `entry/src/main/ets/render/P012WaterSandRenderer.ets` | 修改 | A 档云雾：两尺度 value-noise tile（n1 细团 16px / n2 大团 56px，零均值归一化，LCG 确定性）+ alphaFog LUT + 水体雾 drawWaterFog（逐列竖段采样，tau = tauOptical 界面支撑 + kH·qH + kL·qL + staticTau 静态斑驳）+ 界面过渡带 drawInterfaceBand（沙体之后绘制，smoothstep 包络 t²(3-2t) + 顶部 ramp 渐入 + 8 段竖向采样） |
| `entry/src/main/ets/render/P012RenderTypes.ets` | 修改 | A 档 draw 预算常量（P012_A_BATCH_DRAW_BUDGET=48 / EA 合计 64） |
| `test/run_p012_render_units.cjs` | 修改 | 新增 4 用例：EA mist draw 预算（实测最坏 32≤64）/ fog alpha 上界 / mist 只在 EA+ 档（E 档零雾提交）/ EA 确定性（双实例 fill 序列一致） |
| `test/calib_p012_tau_static.cjs` | 新增 | TAU 标定仿真脚本（真实 tile 采样 → 参数扫描 → 水体 std 预测） |

## 2. 关键实现决策与理由

1. **界面过渡带在沙体之后绘制**（首版画在水侧被沙体 fillRect 硬边截断，y=1945→1950 一行跳变 12 亮度）：带范围 sandTop 上方 halfW（水侧）+ 下方 wFull·0.25（沙侧收尾），smoothstep 包络使沙面上缘被雾柔和覆盖。
2. **水体雾逐列计算（弃 16 组分块）**：组代表值在组边界产生水平硬跳变（实测 x=580→582 跳 38 亮度，maxGrad 锚点无法达成）；逐列后 128 列 × ~40 段 = O(5000) 采样仍远低于预算，draw 数只取决于 alpha 档数（12 档）。
3. **静态斑驳标定链**（test/calib_p012_tau_static.cjs）：alpha 上限 0.55 时水体 std 数学天花板 ≈10.8（range 30.8 × value-noise 钟形形状因子 0.35）→ 无法达 P4 锚点 std>15。三步突破：①a1/a2 从 0.45/0.35 提到 1.3/1.0（mottle std 0.12→0.35）②mottle 线性拉伸 stretch=2.2 拉开钟形分布③水体雾 alpha 上限 0.78（界面带仍守设计候选 0.55）。仿真预测 std=15.3，真机实测 17.1。
4. **NaN 修复**：`seed ^ 0x5f5f` 对大整数异或产生 NaN 传播进 n2 tile（大半雾层静默缺失）→ 改 `(seed|0)+1/+2` 线性派生。
5. **maxGrad 5.6 达标**（<8）：竖向 8 段采样（段内阶梯 ≤2.2 亮度）+ 界面带顶部前 2 段 ramp 渐入（消除带顶硬起始）。

## 3. 单元测试（Node 1:1 通道）

```
test/run_p012_render_units.cjs：22/22 PASS
  新增：p012_ea_mistDrawBudget_worst60Frames   worstTotalDraws=32 (预算 64)
       p012_ea_fogAlphaBound                   水体雾 ≤0.78 ✓
       p012_ea_mistOnlyInEATier                eMist=0 eaMist=22（E 档零雾提交）
       p012_ea_deterministicSameSeed           双实例 fill 序列一致
  既有 18 用例无回归（E 档/E-batch 状态机全过）
```

## 4. 真机验证（VYG-AL00，s87）

**EA 三锚点全部达标**（P4 附表冻结口径 ROI，effector 冻结脚本复核为准）：

| 锚点 | OFF 基线 | EA 实测 | 锚点要求 | 结果 |
|---|---|---|---|---|
| 水体 std | 1.13 | **17.1** | >15 | ✅ |
| 界面 maxGrad | 116.5 | **5.6** | <8 | ✅ |
| 过渡宽度 10-90% | 1 行 | **159 行** | 数十 px | ✅ |

- **静态无闪烁**：两帧差分——纯水体区（远离界面）mean=0.0036≈0（静态斑驳完全静止）；diff 峰集中在 y=1692 界面带与沙面区（物理场 t90 演化带动，合法）。
- **E 档回归**：tier=1 水体 p50=186.9（s81 基线 187，一致）——A 改动对 E 档零影响。
- 截屏证据：`test/artifacts/p012-ea-s87-on.jpeg`（+`-b` 第二帧）、`p012-e-s87-regression.jpeg`。

## 5. DoD 对照（EA-batch 三项）

| DoD | 状态 |
|---|---|
| ① E+A 联合验收（maxGrad<8 / 过渡数十 px / 水体 std>15） | **developer 侧三锚点实测达标**；effector 冻结脚本正式复核待做 |
| ② 静态无闪烁 / 运动无贴纸 | 静态：developer 差分验证通过；运动无贴纸需录像判定（effector） |
| ③ 帧时配对首轮（OFF vs E+A） | 待 tester（p012_perf_capture 通道） |

## 6. 红线自查

- 只读物理场：A 档仅读 snapshot（suspH/suspL 经 view 访问器）；q clamp 不改库存。
- aPhaseX 静止 =0（无运动推进）；EAB-batch 运动代理接手时才更新。
- 一次帧一个分支：EA/EAB 档才调 drawWaterFog/drawInterfaceBand；E 档零雾提交（有单测）。
- 设计候选参数偏离记录：水体雾 alpha 上限 0.78（设计候选 0.55）——P4 冻结锚点 std>15 在 0.55 上限下数学不可达（天花板 10.8，标定脚本可复现）；界面带仍守 0.55。此项交 designer 复核确认。
