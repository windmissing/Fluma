# P-013 G1放行技术裁决（T23）

日期：2026-10-25（Asia/Shanghai）  
责任：designer  
任务：6adda53be4b06ee357273211  
结论：**G1 不放行；G0 未通过，当前状态为 BLOCKED/FAIL。** 本裁决不更改已签收的 P1 数学、配方、坡度核、七项算法版本或 G0 判据。

## 1. 裁决口径

G1 的入口需要 T09 CPU/资产参考登记就绪，且 T29 所定义的必要 G0 平台探针全部由当前生产候选路径证明通过。任务“交付/关闭”只表示取证或开发工作已完成，不代表探针指标通过。T32、T33、T34 均已交付；T35 的 2026-10-25 复测原始产物可用于本裁决，但其 effector 正式报告尚未见到，须另由 T35 如实归档。

必须保持两种结论分开：T09/T29/T32/T33/T34 的执行任务完成状态按已交付记录；G0 测量结果仍包含明确失败和未证明项。

## 2. G0 必要项逐项状态

| 必要项 | 证据与当前状态 | 状态/责任 |
|---|---|---|
| CPU参考及配方 fixture | T09 记录 58/58 MATCH，材料/耦合的五个视觉量仍为诚实 RED / NOT_IMPLEMENTED；可支持参考和测试登记，不能证明渲染输出。 | PASS（CPU夹具）；RED（视觉量）/ tester T09 已完成 |
| 有效帧解析、提交、真实 draw 与 swap | T29 历史为 1742/1742 `code=100`；T32 增加拒因诊断并修复可复现的 onLoad 绑定时序。T35 最新 R1–R7 各臂仍记录 `code=100`、`swap=false`、`T_dataHash=0`、`uploadedFrameId` 未发布；R1遥测明确 `errorStage=TDATA_LENGTH`，profile 为 PENDING，attachment `UNPROVEN`。拒绝发生于可见上传/绘制之前，但此证据本身不证明ArkTS发出的实际buffer长度或类型为何不符合契约。 | FAIL / BLOCKED；developer 需对照 ArkTS 实际 Float32Array 的 length/byteLength/byteOffset 与 native 检查，并找出生产拒绝原因，不得臆测具体根因或放宽校验；T35 effector 负责正式报告 |
| 正确层级与前景 | T29 的旧 SURFACE 布局有 fg 被遮挡反例；T33 修订为 TEXTURE stack；T35 静态截图分析在 renderer 帧仍被拒的情况下判出 fg 像素可见，但不能据此证明沙层与前景处于同一真实生产帧且正确合成。 | UNPROVEN；T35 effector 归档证据 |
| 二值透明与诊断开关对照 | T29 因真实帧未绘制而 BLOCKED。T35 像素分析器对 R1/R3–R7 屏幕采样报告 alpha/背景 PASS，但相应 renderer 臂全部未提交有效帧；这只证明截图中的当时背景采样，不能证明 G0 沙 pass 的真实 alpha，也不能满足同 token diag on/off 对照。 | UNPROVEN；有效帧后由 T35 复测 |
| sRGB 请求、实际 surface/attachment 与读回 | T29 旧 profile 截图的颜色接近仅是屏幕编码旁证。T35 R1/R3–R7 遥测记录 `requested=SRGB`，而 `actual=UNPROVEN`、`attach=UNPROVEN`、`alphaSize=-1`、readback 在 `PRE_SWAP_DEFAULT_BACK` 且 `UNPROVEN`；分析器也明确要求 A/B/API/attachment证据，不能从 JPEG 推断。 | UNPROVEN；T35 effector 补证，失败项归 developer |
| 输入透传 | T29 对照证实 Default 吞输入而 None/Transparent 可收 Down；T35 R0、R1、R3、R4 采样亦有 Down，且 R4 使用 Default。视觉帧失败不抹去这个独立输入结果。 | PASS（None/Transparent 应用结构；Default 负对照）；T29/T35 |
| surface/context 生命周期 | T29 的 hello 路径前后台生命周期证据通过；T35 R0 hello 有持续帧、R1–R7 renderer 各臂被拒，故 renderer profile 重建/动态帧生命周期未被证明。 | PASS（hello路径）；UNPROVEN（生产renderer profile）；T35 |

## 3. 放行决定和实施边界

G1 **不得开启完整 P1 材质/六图消费、材质标定或宣称产品渲染通过**。阻断项是首要帧提交/实际 draw/swap 失败，并连带使透明、attachment色彩以及生产合成 token 的证据无效。不能以 T09 的 CPU MATCH、T29 的任务关闭、T34 的离线编译/host通过，或 T35 静态截图采样替代真实 GPU 帧证据。

现在可继续的工作：

- T20 按 T37 接口继续基础高度场与真实粒子 GLES 数据通路、落床/粒子视觉；不得引入 P1 完整材质或借此改 P1 配方。该基础闭环与 G1 独立。
- 可做不依赖产品指标的离线 shader/资产装配准备、CPU参考和测试脚手架；结果只标记 PREPARED/NOT_DEVICE_VALIDATED，不得声称 G1 实现或 PASS。
- T35 可整理本次运行的原始日志、图片、版本/设备元信息及判读，按实际失败和未证明项结束取证任务。**必要后续：** manager 按 R1–R7 原始拒因登记最小 developer 返修卡；修复并编译/离线自测后，由 tester/effector 重跑有效帧和其余 G0 项，再作 G1 复裁。现有证据没有给出唯一拒绝字段，因此返修卡需要求用本次 `errorStage`/拒因定位，不可预设字段或放宽 ParseFrame/Validate。

建议返修卡任务 prompt（**未建卡**）：

> 不使用真机。按 T35 的 R1–R7 原始日志定位 P-013 renderer 帧提交拒绝；重点检查 `errorStage=TDATA_LENGTH` 时 ArkTS 实际传入对象是否为期望的 Float32Array、`length/byteLength/byteOffset` 以及 native 读取到的长度，并记录两侧值、拒绝分支和最小根因。只修复可复现的生产桥接缺陷；保持 512 元素契约、typed-array/字段/Validate 检查及算法口径，不加 JSON 兜底、不放宽校验。补充针对根因的离线回归，提供编译与非真机自测结果、改动文件/提交。完成后交 T35 依设备窗口复测；developer 卡不承担真机验收。

## 4. T24 / G2–G5 后续卡拆分建议

T24 不是一个可一次性放行的“大实施卡”。以下为 manager 建卡 prompt 的拆分建议；依赖按顺序显式表示，禁止把 T35/T34 的既有范围重复派给下游。

| 阶段/角色 | 输入与输出 | 依赖 | 真机 |
|---|---|---|---|
| G2 developer：P1 shader/material接入 | 输入签收的 P1 rev2、T09 fixture、T34 composition接口；输出仅 Rmat/Lmat/ToneMap 的 typed 数据接线、CPU fixture逐例对照、编译与离线自测报告；不改配方或 profile probe。 | 新 developer 帧桥返修交付，且 G0 帧提交/draw/swap PASS 后才实施；目前只允许离线准备。 | 不使用真机 |
| G3 developer：纹理资产与材料参数消费 | 输入已批准六图/材质资产 schema 和 G2接口；输出确定性加载/采样/边界诊断、缺图拒绝和旧 Canvas fallback，不校准新配方。 | G2 开发交付；无真机指标的资产装配可准备，产品集成依赖 G0 PASS。 | 不使用真机 |
| G4 tester：材质客观验证 | 输入 T09 RED fixtures、P1冻结公式和 G2/G3 build；输出逐fixture对照、范围/边界/非法输入反例、颜色域与量化统计。 | G2/G3交付，G0有效帧通路已修复；本阶段不得重写阈值。 | 不使用真机 |
| G5 effector + tester：设备效果与性能 | 输入 G4客观报告及候选 build；输出真实设备截图/帧统计/合成与色彩证据、热图/材质观感及性能报告，问题分开归因后返修。 | G4客观结果无阻断、设备窗口排期；仅此阶段设备实测。 | 需要真机（加“真机”标签；离线准备与设备串行排队） |

## 5. 风险和反例

- **任务完成不等于门槛通过：** T29 已交付、T32–T34 已完成，但 G0 当前仍不通过；任何清单不得把它们写成 PASS。
- **截图误判：** R1–R7 的静态采样显示前景/背景颜色，不足以证明被拒的 renderer 实际贡献了像素。以截图 PASS 代替有效提交会把回退或独立 UI 误判为 P1 透明和 z 序通过。
- **请求不等于生效：** `requested=SRGB` 不等于 `actual=SRGB` 或 GL attachment 编码；`alphaSize=-1` 是未知而非通过。
- **错误修补扩大契约：** T35 已取得 `TDATA_LENGTH` 阶段证据，但尚未解释其与 ArkTS 实际数组长度的关系；需按字段/长度实值定位，不预设“构造长度错误”等根因，不放宽 typed array/字段/几何校验、不改已签收公式、不加 shader gamma。
- **历史 SURFACE 反例范围有限：** 旧布局失败不证明 TEXTURE 永远不可用；反过来，TEXTURE截图中前景可见也不证明真实 renderer帧已提交。
- **调度冲突：** T34 已完成的 profile、probe、层级实现与 T35 的设备取证不得复制进 G2–G5 卡；T20 的基础高度场/粒子 GLES 工作继续独立，不应被完整材质 G1 卡锁死。

## 6. 证据范围与后续

本裁决使用 T09 CPU fixture 报告、T29 回执、T32 开发报告、T33 工程 revision、T34 开发报告、T35 `test/artifacts/p013-p1g0-retest-20261025/` 原始 R0–R7 产物，以及只读分析器 `tools/analyze_p013_p1g0_retest.py` 输出。分析器运行仅读取已有设备采集文件；本轮未连接或操作真机，未更改业务代码、测试判据或既有执行者报告。

本轮决定：**保持 G1 关闭；基础 T20 可继续；本次 T35 复测揭示的帧拒绝应先形成 developer 最小返修卡，再重新完成有效生产帧和 G0 缺项复测。** T35 报告待 effector 归档；不以本裁决代写其设备报告。
