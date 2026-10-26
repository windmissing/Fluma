# T143：整数条带独立交付复核（零字节差、双租约与资源预算）v1

日期：2026-10-26。角色：tester。任务卡 `6adf6a09e4b03273ad894550`。被验对象：T114 `1b2131f`（native pixel base strip V2 scene route）。执行器：`test/p143_base_strip_review.cjs`（**6/6 PASS**，独立期望非开发 13 例）+ native 复跑（`t143_core.exe`/`t143_atomic.exe`，用补齐 Windows Kits 环境的新脚本 `run_t143_core.cmd`/`run_t143_atomic.cmd` 编译——原 T114 cmd 缺 Kits 路径在本机编译失败，属脚本环境问题非交付缺陷）。raw：`test/reports/t143/p143_results.json`。不使用真机；完整 numeric 仍未标定（T121 线独立）；设备端到端归 T57。

## 1. 独立复核矩阵

| case | 内容 | 结果 |
|---|---|---|
| R1 零字节差 | **独立参数矩阵 6 组**（W∈{128,300,512,1178,3840,130}，含分数 box 10.5/255.25、镜像、spark、负 vX、越界槽位）逐 native j×三层与合同字节 oracle（T113 冻结操作序，H=[145,168,111]/L=[216,196,150]，clamp248+round-half-up）**Buffer.compare 全等** | PASS |
| R2 descriptor | byteLength=12W；payloadSha256 与标准 crypto 逐位一致 | PASS |
| R3 V2 路由/palette | `napi_bridge` 校验 `P013_SCENE_PACKET_V2_BASE_STRIP` + `renderContractRevision=P013_NEWPIPE_ONLY_V1` + `slopeKernelVersion=E2_GAUSS_COL6_R18_P1_V2`；evaluator 携带 `P013_BASE_PALETTE`/`paletteHash` 原值 | PASS |
| R4 资源预算 | uploader 强制 `budget∈[1,65536]`（`BASE_STRIP_BUDGET` 400 拒绝）；pending/uploadedBytes 分离（分段上传）；native core 实跑 PASS | PASS |
| R5 双租约原子 | 双纹理 `textures_[2]`+`activeSlot_/pendingSlot_`；native atomic 实跑 PASS（失败 swap 不提交两资源/成功双提交/身份错配拒绝） | PASS |
| R6 资源清单 | 两份 CPU 槽+两张三层 RGBA8UI GPU 纹理+ETS 可复用 12W scratch 显式存在；context_/discarded_/restoreCount_ 失效与恢复计数 | PASS |

## 2. 复跑记录（非开发 13 例作唯一证据的补充）

| 项 | 结果 |
|---|---|
| `node test/run_t114_base_evaluator_units.cjs`（开发 13 例） | 复跑 13/13 PASS（作交叉参照，不作本卡唯一证据） |
| `t143_core.exe`（预算非法/<4、分段、租约 abort/commit、双纹理上限、域拒绝、释放） | **T114 base strip core PASS** |
| `t143_atomic.exe`（失败 swap 不提交/成功双提交/身份错配拒绝） | **T114 scene dual lease atomic PASS** |
| 桌面 GLES 真实整数读回（`run_t114_base_strip_gpu.cmd`/`run_t114_scene_gpu.cmd`） | T114 已归档 PASS（本卡未重复执行 GL，属桌面真实整数 GL 证据链） |

## 3. 边界

- 桌面真实整数 GL 与桩分列：本卡 R1/R2 为 CPU evaluator 精确复核，GL 读回证据沿 T114 归档；设备端到端与色彩标定不在本卡。
- V1 历史保留；签名配置未改、不输出口令；离线验证不受 SignHap 既有配置阻塞（`assembleHap` 的 SignHap 失败系既有密码长度配置，非源码问题，T114 已声明）。
- 60W（W=3840）资源显式计数：R6 结构核验 + core 测试内含资源计数断言（T114 core PASS 覆盖）。
- 完整 numeric 仍未标定（MEASURED_NOT_CALIBRATED 保持）；不新增 ±1 容差/gamma。
