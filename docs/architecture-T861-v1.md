# T861 · G13 OP17 拆分版的证据与放行条件

2026-11-23 · architect · P3 · Windows PC · 离线复核

**工程风险已经从「单模块能否建管线」缩小到「三段能否在正式业务里正确接起来」。** 现有 PC 已跑通拆分 OP17 的一个真实槽和三个私有样本。完整 SOURCE→QUANTUM→ACTUAL/REPLAY→接受/发布仍缺实测，尤其是多微步的缓冲区身份、私有段复用、同步和撤权退役。算法方面，已采样的标量与独立参考吻合，完整速度、压力和黏性误差仍待测。开发机与目标独显机都走拆分版，旧单派发只保留为可选逐字回归。 [T833 实测](../test/reports/t833/t854-handoff.md) · [本轮复核明细](architecture-T861-evidence-v1.json) · [T854 路线](architecture-T854-v1.md)

## 已核原件及结论上限

复算 T833 的 PE、嵌入 SPIR-V、进程 JSONL 和原始冷档 SHA。T854 独立角色 `OP17_MOTION_CONTEXT`、`OP17_MMS_CONTEXT`、`OP17_LEDGER_ONLY` 实际建管线分别为 58.272 s、9.243 s、76.850 ms，submit/fence 返回 0，OP66 双轮原字掩码均 `[0,0]`。私有版本另在同一 owner 下完成三角色资格。13,894,400 B 原档用现有 `check_t854_capture.py` 接入 T857 重读，结果如下。 [T833 原收据](../test/reports/t833/t854-qualification/result.json) · [原字接入器](../pc/vulkan/check_t854_capture.py) · [T857 判定](../test/t857/check.py)

| GPU 实际样本，shader push `[17,0,1,0,0,10]` | 独立复读 |
| --- | --- |
| ACTIVE | `ACTIVE_WRITESETS_ONLY`。一次 active 计数、一次账本 trace，首错为空。用原 WORK 及 Layout 按 64-lane FP32 顺序重新归约，账本值和快照原字通过。 |
| ENTRY_NOOP | `NOOP_WRITESETS_ONLY`。入口一次 NOOP，后两段只推进私有阶段。既有 MMS 哨兵字未被覆写。 |
| MMS_FAULT | `ACTIVE_WRITESETS_ONLY`。首错 `[op=17,error=17,scope=0,J=1,micro=0]`，数学位 4，后续仍走入口 active 锁存和原账本/trace，首错保留。该样本把 tick 推至整数上限附近以触发域错，用于故障合同。 |

另外从 ACTIVE 的实际 BEFORE 原件取 tick、起点、`J=1/micro=0`、初角 `3/2`、角速度 `1`，用有理数独立算得样点 `t=1/240 s`、角度 `361/240 rad`。FP32 的 `c[624..628]` 逐字匹配，角度与角速度的 binary128 打包也逐字匹配。用 Decimal 90/125 位各独立求 `exp(-t)/4` 和 `exp(-2t)/8`，舍入的两字 `0x3e7eef80,0x3dfde023` 都等于捕获的 `c[642..643]`。样本的重力输入为零，结果仅覆盖该匀转点和窗内 MMS 系数，空间重力旋转、RAMP、完整速度压力场仍待观测。离线另重跑 T857 的 21 个样本/17 项负控、T837 独立基准自检和 T721 数值分类自检。 [精确输入和原字摘要](architecture-T861-evidence-v1.json) · [T837 基准](../test/t837/references.py) · [T721 判据](../test/t721/check.py)

现有 GPU 三段采样把所有 dispatch 和分段 copy 放入同一个 command buffer，取一个最终 fence。接入器按已知录制结构生成 `BARRIER` 行，按各阶段 CTRL 差值复核异常位。它支持该原档的写集和阶段首错，正式热路径在实际提交中的屏障、跨代拒绝、撤销和真实接受根仍需专门原件。收据的 `acceptedRoot=null`、`productionEquivalent=false` 继续按原含义登记。 [录制和 fence 位置](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t854_writes.inc)

## 原模块与独立参考的接法

- **交付以拆分版为主。** T833 可以接入 OP7/OP17 的完整候选，沿 T854 保原逻辑槽计数、三段时序、单 owner、写集和数值语义。当前阶段只编译和验证候选，生产映射仍是 SOURCE 16、QUANTUM 820。原整段没有实际输出时，记录 `MISSING_PC_REFERENCE`，沿 T837、T631、T721 及该槽的解析参考验正确性。T857 的 `WRITESETS_ONLY` 按实际含义保存，数值参考和整根通过另各记自己的结论。
- **可选回归。** 已匹配旧 T841 PE `ad40c7ce…` 内唯一的整段 `OP17` SPIR-V `cb5b3825…`。该构建按原 layout、默认 flags、空 pipeline cache，在开发机 `vkCreateComputePipelines` 进入后 60.100 s 截止，未取返回及业务原字。若有现成的 PC 或目标独显且愿做对照，先用同一 PE 和 `qualify_role.py ... OP17` 做有界资格。记录硬件、driver、FloatControls、layout、原 SPIR-V、QPC、API 返回和退役。该命令只跑建管线与 OP66。确认可执行后才值得另加同输入导出接缝，固定原四缓冲、同 push、同范围、同 device/driver 和原构建语义，比较同份 BEFORE 的末段原字，排除拆分版新增 128 B 私有 WORK 区。驱动替代结果只覆盖对应适配域，用户的目标机性能测试只跑拟交付拆分版。 [原整段停点](../test/reports/t833/t841-qualification/OP17.receipt.json) · [原 PE 指纹](../test/reports/t833/t841-qualification/build-and-result.json) · [Vulkan 观察器](../pc/vulkan/qualify_role.py)

## 最小 PC 接续计划

| 观察 | 范围与停止边界 |
| --- | --- |
| 独立阶段覆盖 | 复用已有实际 slot 10。优先补 slot 126（ACTUAL J2 最后微步，MMS 窗末无写）、186/354（ACTUAL J4 首尾，RAMP 两侧或整秒账本）、415（REPLAY J1，入口已有首错）、475/531（REPLAY J2 首尾和窗内外）、647/759（REPLAY J4 内部及末步，重力非零和正常/域错）。用同套冻结输入/位型和现有独立参考，跨两 scope 比对共享物理值，并保持本应不同的身份字段。补 forced skip、运动首错后实际发生 MMS 错及 OP7→OP8 原时序回归。 |
| 全根槽位 | 首个业务候选采实际计划的所有 14 个 OP17 逻辑槽，逐个区分 active/NOOP，记录 `scope,J,micro,ordinal`、实际角色、绑定的 bank/range、nonce/permit 和原位型。选择 J=1/2/4 的 active 样本各保实际/重放证据。已选 trial 的后续 NOOP 只登记真实 NOOP 覆盖。边界提交前复用 owner gate，禁止把比较结果拼成合成 GPU 原字。 |
| 撤权与资源 | 两类真实边沿各取一次。提交前撤权时，后续段没有 queue submit，也没有新 Completion/accepted root。提交后撤权时，已经提交的命令可以跑完；候选进入隔离退役，保当代 fence 和锁存原错，禁止接受或发布，重新开代前保真实退役。另取错 nonce/slot/范围或旧私有段负控。T857 现有 `revoked=True` 适用于后续消费者未提交的记录，已提交后撤权另按 T828/T837 的根级原件判定。 |
| 工程正控 | 用实际业务入口取非空 SOURCE 和同谱系非零 QUANTUM，保两 scope、比较、同根 1024 B Completion、原 250 ms 和 33,333,333 ns 许可、真实 Vulkan 队列、独立资源/代际收据及发布。逻辑计数仍 16/820，物理 dispatch/barrier/copy 单独计，不能把私有 copy 夹具当正式热路径。实际候选的每个已用角色都核对应 SPIR-V 与 OP66。 |
| 数值正控 | 第一轮用 F02 私有投影快速验证压力规范和散度，用 F05 实际非零闭盒收完整速度、动态/物理压力和证书。F02 走专用投影判据，T837 的 FULL 物理根入口用于 F05 等真实接受的业务根。若差异集中在黏性，再用 F03 解析剪切衰减隔离。按 T631 的网格/时刻/单位和参考参数，T721 的散度、黏性、压力及投影证书阈值原样执行。需要跨网格/步长的项取实际收敛样本。保同根 FULL、真实势场来源、原 u/v/π、均值规范和误差指标；T837 输出 `CASE_REFERENCE_PENDING` 时按对应病例补齐本判据的剩余项。 |

“未接受、错代、绑定错、屏障或 REPLAY 差异”先按首个实际失败边沿归入工程，修接线与复测。“真实接受且输入、原字、单位与取样都核对一致，实测误差仍稳定超门”才进入数值归因。按 F02/F03/F05 及步长、网格、求解迭代或精度受控对照，定位到投影、黏性、离散或舍入，再由 designer 修改对应算法。阈值、原物理步骤及 GPU 驻留/渲染路径沿既有合同。仅 Windows PC 验收，禁止 Canvas 回退。 [正式 PC 验收](architecture-T828-v1.md) · [全根判据](../test/t837/README.md) · [T631 独立数值样本](test-report-T631-v1.md)

## 接续与放行

Developer 按 T854 接正式形态的私有候选、提交/撤权边沿和分开计数，交 Windows x64 编译回执、可独立运行的候选 PE、映射及实际角色 SHA，保持正式映射冻结。Tester 补上表缺失夹具和独立判据后读真实归档，可复用 T857 的 14 槽 schema 与 T837/T721/T631。Effector 在开发 PC 上取得真实 GPU 和窗口原件，供 tester 做离线判断和 architect 首次归因。先推进可验证的候选，目标独显环境在同路线业务包上量实际场景帧耗。只有完整根、数值和独立负控过门后才按实际候选考虑物理 850 或后续 836 的发布资格。目标 14 继续等目标 13 的可信 `GridView` 和速度、压力、黏性实测。 [T854 接线/计数](architecture-T854-v1.md)
