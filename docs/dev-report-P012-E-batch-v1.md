# P-012 E-batch 开发报告 v1（developer）

- **日期**：2026-10-20。责任：developer。依据：architecture-P012.md §9.3 E-batch + algorithm-P012-render-v1.md §3 + 滴答任务 6ad71fabe4b05503d543ff4f。
- **技术路线**：`appearanceRouteId=p012-render-material-v1`（E 档）。状态：E-batch 代码交付 + 单测通过 + 构建验证（见 §4）；SV-07 色调锚点初评与性能预检空跑属 effector/tester 职责，本报告不代填。

## 1. 修改文件清单

| 文件 | 操作 | 内容 |
|---|---|---|
| `entry/src/main/ets/render/P012RenderTypes.ets` | 新增 | 外观档位枚举/InvalidReason/frameContext 接口/draw 预算常量（§2/§3 显式字段，无 any） |
| `entry/src/main/ets/render/P012WaterSandAdapter.ets` | 新增 | 双 slot 深复制 + Kahan 相库存求和 + H/L 独立守恒检查（tol=64·N·EPS·max(1,\|M_old\|+\|M_new\|)）+ 配对状态机（§3.3 五条规则） |
| `entry/src/main/ets/render/P012WaterSandRenderer.ets` | 新增 | E 档渲染：预计算 LUT（T=x/(1+x) 压缩+sRGB，去 127 钳位）/伪法向光照（La=0.85 Ld=0.15 左上主光）/液面宽软高光（alpha 0.12）/确定性亮粒 tile（LCG seed 20261020，三档合并 Path）/OFF 分支决策纯函数 |
| `entry/src/main/ets/pages/SandCanvas.ets` | 修改 | 薄分发（§9.2：<60 行，仅分发+epoch 递增）；p012FieldEpoch/ContinuityEpoch/GeometryEpoch/frameSerial 递增点；aboutToDisappear dispose；面板外观档位按钮（仅 WATERSAND 模式）；want 参数读取 |
| `entry/src/main/ets/entryability/EntryAbility.ets` | 修改 | want 参数 `p012tier`（0..3）/`p012seed` 解析进 AppStorage |
| `entry/src/test/P012WaterSandAdapterTest.ets` | 新增 | 11 个 hypium 用例（守恒/注入/同快照/非有限/间隔/epoch/serial/身份/alias/Kahan/OFF 决策） |
| `entry/src/test/List.test.ets` + `entry/src/ohosTest/ets/test/List.test.ets` | 修改 | 注册 P012WaterSandAdapterTest |
| `test/run_p012_render_units.cjs` | 新增 | Node 1:1 通道验收 runner（长跑守恒/H-only/L-only 注入/mock-canvas draw 计数/确定性/决策表/slot 隔离 + hypium 全套） |

## 2. 每处改动的理由

1. **独立 renderer 模块**（架构 §1 模块选型表"选用"行）：页面只控制显示选择/生命周期，材质逻辑零侵入；原 `renderWaterSand` 保留为 OFF/canonical。
2. **双 slot + Kahan**（§3）：页面 snapshot 每帧 `copyStateTo` 原地覆盖（SandCanvas:1385 已核对），adapter 深复制是唯一安全方案；两 slot 合计 24576 bytes。
3. **薄分发约束**（§9.2）：①一次帧只走一个分支（if/else 互斥）；②frameContext 全部复用页面既有字段；③`canonicalEvidence=waterSandSnapshotEnabled`（物理证据采集期间 checker 消费 snapshot，强制 OFF）；④renderer 延迟创建（首次启用非 OFF 档），dispose 挂 aboutToDisappear。
4. **epoch 递增点**：initField（fieldEpoch）/invert180（continuityEpoch）/模式 boundary（经 applyInputMode 内既有逻辑，p012ContinuityEpoch 由 invert/reset 覆盖主路径）。E-batch 无运动平流消费方，epoch 先行落位供 EA/EAB 使用。
5. **E 档材质数值**（设计 §3.1）：基色取 effector 目标 ROI（重沙 145,168,111 / 水 192,202,196 / 轻沙暖浅）；LUT 去除旧渲染 127 亮度上限，高光肩部到 ~248；亮粒 tile 确定性 LCG（不调用/消耗物理随机源）。
6. **draw 预算工程化**：亮粒不逐块 fillRect（最坏 20+ 次超预算），按 3 个固定 alpha 档合并为单 Path fill——mock-canvas 实测最坏 9 draw ≤ 16 预算。

## 3. 单元测试结果（Node 1:1 通道，实际产品 .ets 经 TS 编译执行）

```
test/run_p012_render_units.cjs：18/18 PASS
  p012_adapter_longRun_neverConservationFails   conservationFails=0（120 帧倾斜往复）
  p012_adapter_injectHOnlyBreakAndRecover       reason=CONSERVATION_H(6) → 下一帧恢复
  p012_adapter_injectLOnlyBreak                 reason=CONSERVATION_L(7)（H/L 独立）
  p012_renderer_drawBudget_worst60Frames        worstDraws=9 ≤ budget=16
  p012_renderer_deterministicSameSeed           两实例同 seed 渲染样式序列一致
  p012_offBranchDecision_table                  null/OFF/canonical/disposed → false；E/EA/EAB → true
  p012_adapter_slotIsolation                    页面 snapshot 突变不泄漏进 slot
  + hypium 11 用例（含 not-finite 清配对两帧恢复、frame-gap>100ms、epoch/serial/身份断配对、Kahan 与 getTotals 一致 <1e-6）

test/run_p011_input_units.cjs：67/67 PASS（既有 P-011 无回归）
```

## 4. 构建与运行验证

- **构建**：hvigorw assembleHap 编译通过（3 轮 ArkTS 语法修正后：匿名对象字面量→显式类 P012SandBand、类型标注补全）。SignHap 00303116 为已知结构性问题（GUI 加密串），手动签名绕过。
- **签名**：`entry-default-s81-signed.hap`（hap-sign-tool，dev-cert-chain.pem 三证书链）。
- **部署**：hdc install -r 真机 VYG-AL00（62T0225B10010020），安装成功。
- **运行验证环境**：真机 VYG-AL00 / OpenHarmony 7.0（API 26）/ 分辨率 1280×2832。
- **运行结果**：
  - `p012tier=1`（E 档）启动正常、无闪退、渲染循环稳定（WS_FRAME 持续输出，dormant 状态正常）；
  - E 档量化（P4 附表同口径 ROI，effector 冻结脚本复核为准）：**水体 p50 65→187**（目标 ~200；OFF 基线 64.9）；**沙 p95 221 / max 222**（OFF 钳位 127 已解除；目标 max ~240、p99 221~248）；沙体 std 15.5（亮粒+光照基础）；倾斜手势注入后渲染正确响应（水体 185~187、沙面浅黄绿 179,199,150、沙丘亮面 221）。
  - `p012tier=0`（OFF 档）同 HAP 启动：**完全恢复原渲染**（水体 36,71,113 深蓝 / 沙 35,85,34 暗绿，与历史 OFF 基线一致）——OFF 分支回归通过。
  - 截屏证据：`test/artifacts/p012-e-s81-on.jpeg`（E 静置）、`p012-e-s81-tilt.jpeg`（E 倾斜）、`p012-off-s81.jpeg`（OFF 回归）。
- **修复记录**：首版 s80 发现 want 参数路径未创建 renderer（延迟创建只在面板切换入口），s81 修复（aboutToAppear 统一经 setP012AppearanceTier）。

## 5. DoD 对照（E-batch 四项）

| DoD | 状态 | 说明 |
|---|---|---|
| ① OFF 分支像素级回归 | **初验通过，正式判定归 tester** | OFF 分支字节级走原 renderWaterSand（未改动其任何一行）；真机 OFF 启动配色与历史基线一致；同物理 seed 逐帧像素比对由 tester 采集流程执行 |
| ② E 档 SV-07 色调锚点初评 | 代码就绪，**判定归 effector** | E 档经 want 参数 `p012tier=1` 或面板按钮启用；初步量化水体 187/沙 221 在锚点区间附近，正式初评用冻结脚本 |
| ③ adapter 守恒检查用例 | **完成**（18/18） | H/L 独立/注入断配对/同快照零平流/非有限保护全过 |
| ④ 性能预检空跑（OFF 档采集链路） | 代码就绪，**执行归 tester/effector** | 采集脚本 p012_perf_capture.ps1 属 tester 交付；同 HAP OFF/档位切换入口已提供 |

## 6. 红线自查

- 只读物理场：P012 模块零 import step/inject/invert180 写路径；adapter 深复制，不 alias 页面 snapshot（有单测）；不写回 WaterSandField/FluidField。
- 外观物理验收隔离：appearanceRouteId 独立；不进 checker/P011 遥测/WS 日志（本批未加任何遥测字段）。
- 一次帧一个分支：if/else 互斥，无叠加。
- 物理零改动：WaterSandField.ets 未动（注入用例仅测试侧调用既有 inject API）。
