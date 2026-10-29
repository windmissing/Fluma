# T260 GPU M0 主核独立差分报告（v1）

日期：2026-10-29；tester；**不使用真机**。消费 T236 core-v2 正式交付（`d8fd6bc`，PARTIAL/CORE_SLICE_DELIVERED）与 T234 独立 GR 规格。原卡 `6ae2a5c5e4b0d25fef8d659e`。

## 结论

**6 PASS / 0 FAIL，总差分 0 word**——自持 BigInt/Fraction 整步 oracle 对产品 shader（PC SwiftShader 软件 GL 实际执行的 `m0_source.comp`，SHA `290eb07e…78d3d`）的可观测输出（state 6144 word × 5 capture + completion256 × 1）逐 word 全部一致。独立非设备：**不把 SwiftShader 收益当真机，不把本差分记为设备 PASS**。

## 独立性声明

- oracle（`test/p260_oracle_core.cjs` + `test/p260_oracle_source.cjs` + `test/p260_differential.cjs`）自持 BigInt/Fraction/RNE/有理 DDA/整数 guard/角度表二分，**不调用产品 helper、不引用 designer 模型**；T230 规格文本与 T233 冻结 manifest/角度表作为数据输入（SHA 绑定核对）。
- GL host（`test/p260_host/t260_host.cpp`，独立于 developer 的 `t236_source_host.cpp`）只做场景编排与证据导出；被测对象是产品 `SourceGpuTransport/RootOwner/m0_source.comp` 全链。
- 复用 T234 已交付 oracle 基础原语结论，未重复 82,205 向量基础算术差分（任务书允许）。

## 分项

| ID | 内容 | 结果 |
|---|---|---|
| D-SRC | source root 状态 word-diff：oracle 预测 vs GPU bank（准入 .25→2 births、ID 1/2、x=67633152、y=104857600、PH=262144、header 全字段） | PASS（0 diff / 6144 word） |
| D-SRC-C | source completion256 word-diff（receipt/admitted=262144、birthCount=2、prefix 0→1、capMax=262144、dispatch=32） | PASS（0 diff / 256 word） |
| D-Q1..Q4 | 4 量子 word-diff：重力积分 vy 序列 104857600→…→104850464（Δy=-7136/量子）、motion=2 事件/量子、eventEnd=2/4/6/8、angle=0、guard=[0,0]、bank 选择器逐 root 轮换 | PASS（各 0 diff） |
| E-CNT | 事件计量交叉核对：srcEv=2（source）、motion=2×4、bed=0、eventEnd 累计 2→8 | PASS |
| SNAP-X | 交叉复核 T236 归档快照：oracle 对 developer `source-state-192.bin` 亦 0 diff（6144 word，baseBank=1 路径）；fault 快照差异仅 shaderSha 8 word（身份域，物理字恒等） | PASS |

## 边界（如实）

- 场景覆盖：source 全成功 + 4 量子自由落体（零初始速度、angle=0、无床/无 CROSS/LAND/BED 触发——两粒悬空 y=100，床高 0，未到底）。LAND/CROSS/BED/replay/拒绝路径的整步差分**本轮未覆盖**，oracle 语义已实现待下一轮场景扩展。
- MAX_ID 分支协议未齐（T236 待裁定 1）：oracle 未实现出生分支，与产品一致，**不判预期缺陷**。
- 窗口/XComponent 接线未接（T242）：本卡纯离线，不涉及。
- K512 与 fault-shader 拒绝路径未在本 host 重跑（developer 证据已在案，oracle 快照交叉复核覆盖其状态面）。
- 真实 GPU/真机执行、设备呈现、收益数据：NOT_RUN，不冒充。

## 证据

- `test/reports/t260/host_trace.txt`：GL host 原始 trace（SwiftShader OpenGL ES 3.1.0 / ANGLE Vulkan，显式软件平台）。
- `test/reports/t260/differential.json`：差分结果机器可读（revision T260_DIFFERENTIAL_V1）。
- `test/reports/t260/source-state-192-xcheck.json`：对 T236 归档快照交叉复核结果。
- 执行器：`test/p260_oracle_core.cjs`、`test/p260_oracle_source.cjs`、`test/p260_differential.cjs`、`test/p260_host/`。
