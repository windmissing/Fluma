# T73 干场独立客观验证 v1

日期：2026-10-26。责任：tester。任务：[6adeb1e9…03df](https://dida365.com/webapp/#p/6ac8a7e3e9adb60000000419/tasks/6adeb1e9e4b017ab33f703df)。状态：**OFFLINE_OBJECTIVE_PASS / SCREEN_EFFECT_NOT_COVERED / DEVICE_NOT_USED / CALIBRATION_NOT_CLAIMED**。

对 T59 交付（commit `ddc6947`，DRY_AIR_V1 干场实现）做**独立**客观验证：用例从 T58 设计稿 §8（D01–D15）重新设计，不复用、不重跑 T59 开发自测（`run_p059_dry_admission_units.cjs`）作为证据；包络等式、λ 数学、守恒与回滚身份均从原始数组独立重算。仅新 XComponent+shader 管线视角（D15 走 `COLUMN_PRESERVING_F32_RENDER_V1` 适配器），无 Canvas 上屏/回退路径。

## 0. 证据文件与目标身份

| 项 | 值 |
|---|---|
| 执行器 | `test/p073_dry_air_verification.cjs`（纯 Node，只读；17 例） |
| 执行证据 | `test/reports/p073_dry_air_verification.json`（**17/17 PASS、0 FAIL**） |
| 目标身份 | 7 个干场实现文件与 `ddc6947` 构建提交 **EOL 归一化后逐字节一致**（`IDENTICAL_EOL_NORMALIZED`；差异仅为 git autocrlf 换行符，内容零差异）——结论对 T59 构建成立 |
| 判据来源 | T58 设计稿 §3–§8 文本（期望值/拒绝码/不变量均取自设计，未新设容差线） |

## 1. D01–D15 逐组结果

| 设计组 | 验证内容 | 结果 |
|---|---|---|
| D01 | 干冷启 W/H/L/P/U 全零、cold waterTop 逐列 0、EMPTY_BASELINE、空 tick、reset 清准入计数 | PASS |
| D02 | 干场上旧湿场 `injectGestureSand` 仍 NO_WATER（包络非零时也一样），库存/发布零变动 | PASS |
| D03 | 空气触点 (20.25, 220)：DRY_POINT 精确出生（pX/pY 原值、初速 0、不夹水面/床面/列中心），床不增，W 恒 0，包络恰增 admittedH，单次 publication | PASS |
| D04 | 一触点两相同点出生、IH/IL 分列、epoch/count 各 +1 一次；注入第二相故障→field/pool/allocator/nextId/源/发布**全量回滚**（poolDigest 逐字节一致） | PASS |
| D05 | 统一 λ：列占 255 时 λ=0.25（独立复算 1/4 精确），H/L 同 λ 各得 0.5；单 slot 双相 → 整笔 CAPACITY_BLOCKED，状态零变动 | PASS |
| D06 | x=128/负、y=257/0、NaN/负质量/溢出和 → 明确拒绝不夹取；Q=0 → NO_OP；全部拒绝零状态变动 | PASS |
| D07 | 全程干场出生→连续 step 到 LAND：600 步内每步包络等式+守恒复核，W 恒 0，H=0.3/L=0.2 精确落床 | PASS |
| D08 | 零预算 CONTACT_WAIT（state2 保留）→ 有预算 step：库存精确、无质量丢失 | PASS |
| D09 | 导出 sink 故障 → count=1、INCOMPLETE、exportFailures=1、事务不撤销；后续物理 step 失败不回滚外源成功 | PASS |
| D10 | 重复 sequence → INVALID_IDENTITY；真实 DEMO 模型 stop/后台/重进无追赶注入 | PASS |
| D11 | E=0：仅 H/仅 L/双相源均不重置 initial 基线；drift 全有限；零相库存严格为零 | PASS |
| D12 | dry 空气加速度 ±15° 符号正确（9.8sinθ / −9.8cosθ），0° 退化为旧候选式；**冷预测器与 HOT 双 collect 逐值一致** | PASS |
| D12b | 满池摇晃保床退款、永不降级 U（suspH/suspL 恒 0） | PASS |
| D13 | snapshot 往返恢复 medium/源/计数/包络；篡改 medium/包络/缺字段在写 live 前**全部拒绝且零部分写** | PASS |
| D14 | 模式门：dry 拒 invert（DRY_INVERT_UNSUPPORTED）、拒写水（MODE_CONFLICT）、拒卸载 route；wet 场拒 dry 注入；各拒绝后状态完整 | PASS |
| D15 | dry 同 commit 发布（medium 随行）通过现有适配器 COLUMN_PRESERVING_F32_RENDER_V1，包 commit/particleCount 一致 | PASS |
| X01（T73 追加） | 第二次准入包络恰增第二次 admitted 量、publication 再 +1；**被拒请求不推进 physicsCommitId** | PASS |

## 2. 独立性声明

- 期望值取自 T58 设计文本（回执字段、W=0、包络=O 等式、DRY_POINT、统一 λ、EMPTY_BASELINE、dry 重力向量），不从开发断言抄写；
- 包络等式、λ 上限、跨 step 守恒均由执行器从 `snapshot()` 原始数组独立重算（非读开发 digest 函数）；
- 追加用例 X01（被拒不发布 / 二次准入包络增长）为开发自测未覆盖的反例方向。

## 3. 边界与移交

- **不覆盖**：屏幕效果、真机行为、标定状态；T58 §8 的设备侧验证归 T74（真机，等 T72 诊断与本稿硬闸门）；本稿不宣称干场 DEMO 产品通过。
- **旧协议**：endpoint/peak 协议适用域未动；dry medium 对这些协议的适用性须另行登记。
- **旧湿场回归**：D02/D14 确认 NO_WATER、η/λ 数学、invert、水写入门全部保持原语义；包络 `waterTop=O` 是占据表示，不冒充自由水（`getDryDiagnostics().freeWaterTotal` 恒 0 已验证）。
- **失败责任**：本稿 0 FAIL，无归因项；若 T74 设备取证发现与本稿离线结论矛盾的行为，交 designer 归因（离线/设备环境差异优先排查）。

## 4. 变更记录

- 2026-10-26，v1：首次发布。17/17 PASS（D01–D15 + X01）；T59 构建字节 EOL 归一化一致；独立重算包络/λ/守恒；无 Canvas 回退路径参与；未改产品/判据，未用真机。
