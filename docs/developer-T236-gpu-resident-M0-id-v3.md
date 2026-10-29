# T236 M0 末 ID 接续开发回执

2026-10-29，developer；不使用真机。本轮消费 T259 工程裁定，修复产品 source shader 对最后真实 ID `0xFFFFFFFF` 的出生准入：`next>0` 且 `births-1<=MAX-next`，最后出生使 nextSandId/terminal/sourceEvent/completion/header 统一为 0。0 仅作 run 永久耗尽哨兵，不作粒子 ID；新正量出生按原 reason 7 拒绝，同 key replay 的排序仍先于新准入。未修改质量、容量、guard 或阈值。

`RootOwner::ValidCompletion` 不再用 U32 单纯小于判定 nextSandId；源成功必须逐项核 `receiptBirthCount`、`receiptFirstId`、`receiptNextId`、完成包 nextSandId 与原 header，且仅末 ID 为 MAX 时允许 `>0→0`。量子、metadata reject、replay 原有 next 保持判定继续拒 `0→非0`。T259 manifest 重新生成布局 SHA；测试控制面 MAX→0 成功、0 后伪成功拒绝。Od/O2 foundation 各 87/0；shader ES310 语法编译通过；PC SwiftShader 常规链 26/0（软件 GL，非末 ID fixture）；Harmony native/ArkTS/PackageHap BUILD SUCCESSFUL。证据见 `test/reports/t236/resume/t259-*.log`。

本卡仍为 **部分交付 / 看评论**：实际 XComponent 产品 owner、独立 M0 APP_ROUTE、异步 UI pump/lease/swap 生命周期未接；逐阶段固定容量 trace 和完整失败矩阵未齐。末 ID 新修订尚未完成真实 shader 的末值 fixture，因为当前 Create 只创建冷 next=1、无受证既有历史注入入口；host completion 边界通过不冒整根产品验证。旧 PC SwiftShader/设备结果不能替代新 revision。无 CPU 物理或 Canvas 回退；真机由 tester/effector 另测。继续 T236 余项，不关闭完整 M0。
