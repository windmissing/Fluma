# T252 明水 C 基础真机显示 · 报告 v1

- 角色：tester（真机 VYG-AL00）· 日期：2026-10-29 · 状态：**FAIL（根因确凿，交 architect）**
- 被测包：**s143/s144**（s142 树 + 最小 cwdemo want 路由增量 + T252DBG 诊断日志；unsigned `8bd2149f96ccf459c0cbb2982455e0b06a86a37b3721067d0cc69a13507040c8`，signed `53607ab3e079188b9f09caac37b926b381fe996453564689a77bf6f922d11b26`）
- 页面：`pages/ClearWaterDemo`（真实 B 耦合接入：`attachBInto` 真实 attach、SourceController 触摸驱动、`drawClearWaterSceneV3` native 六层）
- 归档：`test/reports/t252/`（t252-open/s144.jpeg + layout1/2/3.json）

## 真机结果：启动成功，渲染首帧即 FAULT，**六层从未上屏**

| 项 | 证据 |
|---|---|
| 页面/管线启动 | 标题"单沙＋明水·C V1"、沙/水/重置按钮、XComponent id=`p013_clear_water_surface`、runId=`cw-run-1793235611410-1` |
| init/session | `sess=1`（init 成功、ownerBound）、ctx=1、surf=2、w=1178 h=2026、cap=192 |
| **draw 首帧** | `code=100 INVALID_ARGUMENT stage=CW_V3_PARSE`，**每帧复现**（T252DBG 连续多条） |
| 画面 | 画布全白（六层从未绘制），状态文字 `FAULT_CW_V3_PARSE · 当前源: 沙` |

## 根因（确凿，非猜测）

**ArkTS 侧拼接的 `frameToken` 长度 999 字节，超过 napi 桥 `String()` 255 字节缓冲上限**：

- `P013ClearWaterTypes.ets` `bind()`：`configDigest = JSON.stringify(C-run-config) + ';' + JSON.stringify(CW_B-config)` ≈ 700+ 字节，全量拼进 frameToken；
- `napi_bridge.cpp` `String()`：`char b[256]`，`n >= sizeof(b)-1` 即返回 false；
- `DrawClearWaterSceneV3` 的 `MaterialText(e,a[0],"frameToken",p.token)` 失败 → `valid=false` → `INVALID_ARGUMENT @ CW_V3_PARSE`；
- **每一帧都被 parse 拒绝，六层/GL 从未执行**（非 GL 问题、非 surface 问题、非物理问题）。

T226 离线自测未暴露：host 测试用 `configDigest="offline-explicit-fixture"` 短串。T244 native-host 同用短 fixture。**这是 T226 交付时序中 ArkTS↔native 桥接层的产品缺陷**（tester 不修，按卡约定交 architect）。

修复方向（供 architect 裁决，tester 不实施）：①`String()` 缓冲扩容或动态分配；②frameToken 中 configDigest 改短哈希（但合同要求"full explicit identity, never hash-only"——需 architect 裁定 token 结构）；③MaterialText 对 token 走专用长字符串通道。

## T252 合同三项完成度（如实）

| 合同项 | 状态 |
|---|---|
| ①真实 window 六层 GL 编译同 token CURRENT | **未达成**：draw 在 parse 层拒，六层 shader 编译未发生 |
| ②swap 后同 token 像素有水面/床层 | **未达成**：画布全白无任何 GL 输出 |
| ③切源新 DOWN 两 tick 后真实 source ACK / 旧源终态隔离 / stale 后台 | **部分被阻**：页面 UI 上切源按钮可点（当前源显示切换），但 draw 链路断、物理帧不推进可见证据为全白；ACK 行为已在 T244 离线真实驱动覆盖（terminal ACCEPTED），设备侧被同一 parse 缺陷阻断 |

## 边界

- V1 水直进 W（无飞行水）、非 GPU M1——未冒称。
- 不重复离线 host 已证故障矩阵（T244 B 面 6/6 与本缺陷无关——host 用短 fixture 绕过了同一 bug，恰好说明缺陷在真实 ArkTS 数据接缝）。
- s143/s144 为隔离测试增量（cwdemo 路由 + T252DBG 日志），生产 SandCanvas 主路径未动。

## 失败上报

**FAIL → architect**：CW frameToken 超 napi String() 255B 上限，真机每帧 CW_V3_PARSE 拒绝。复现：装 s144 → `aa start --ps cwdemo 1` → 状态文字 FAULT_CW_V3_PARSE；hilog tag=T252DBG。
