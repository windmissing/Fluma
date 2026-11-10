# T462 · GPU M2 Create 首错与诊断合同

2026-11-10，architect，P5，不使用真机。依据 T461 原布局、T452 最终签包及实际生成的 m2route，修订 [T445工程稿](architecture-T445-gpu-M2-V3-engineering-v1.md) 的创建失败诊断。证据哈希见 [本轮索引](analysis-T451-T462-T463-T464-T465-evidence-v1.json)。

## 1. 首次分流

T461 原件 `test/reports/t461/t461_l.json`、`t461_m2l.json` 支持 `FAULT TRANSPORT_CREATE #6` 和未生成冷 trace。重算最终签包 SHA256 为 `de316a648a17e86b21b5b7cb9debbde3ceadfc90cf2d20cef9ddd62a7f076959`，与 T257 收据一致，libgrsm1 SHA256 为 `37b27e0d8c36ff63bb8604d644bd6de55867480e7e666dc45d080c17842d8c6d`。对应冻结工程为 `.codex/local-test/t452-complete/`，不能只读共享 m1 或另一个中间生成版本。

`TransportCode::SHADER_FAILED=6` 是多个分支共用码。实际 Create 链为：

| 顺序 | 生成文件/对象 | 返回6的分支 |
|---|---|---|
| 1 | m2route/full_transport.cpp `ProbeCurrent` | 此处返回能力码4，不返回6 |
| 2 | source_transport.cpp `CreateUsing(FullSourceKernel)` | SOURCE V3 compile、link、uniform reflection Prepare |
| 3 | quantum_candidate.cpp `Create(QuantumKernel)` | 中间 QUANTUM kernel compile、link、uniform reflection Prepare |
| 4 | full_transport.cpp 删除中间 program 后创建 `QuantumCommitKernel` | 最终 QUANTUM V3 compile、link、uniform reflection Prepare |

因此现有原件只能确定进入 Create 且返回6，不能确定 SOURCE/中间QUANTUM/最终QUANTUM，不能断言组合 kernel 编译失败，更不能断言指令数/shared/barrier超限或驱动 bug。`UniformLocations::Prepare` 的 activeUniform 数量/名字长度上限也会产生6。首次失败可能发生在最终 program 之前；中间 program 的冗余存在不等于本轮失败原因，先保持原链做可比取证。

T452 生成 bridge 的错误发布只含 state/stage/code。上述三个编译链没有 `glGetShaderInfoLog/glGetProgramInfoLog`，失败先删除对象或 Release。T456 的 `boot_probe.h` 虽有512字节 shaderLog 容器，T452 未接入，T456生成器也未接通该日志采集。当前是**工程诊断未采集**，不是已证明驱动返回空 log。T461“Maleoon910”“两次同码”和驱动日志属于报告记录，现存布局未包含绑定当前 context 的 renderer/version/limits，能力不能从机型推定。

## 2. 冻结诊断修订

新增独立冷诊断 `T462_M2_CREATE_DIAGNOSTIC_V1`，绑定最终 package声明+host验签收据、lib ELF、build variant、layout hash、每个实际 shader SHA/UTF-8字节数、K/Kw/feature/revision/policy、bootSerial/epoch/window/surface/context/owner TID/candidate run。保留现有 TransportCode和冷trace协议，诊断不伪造 root，也不要求先 ARMED 才能读取。

在真正 makeCurrent 的 GL owner 冷阶段读取 EGL实际context client version、GL_VENDOR/RENDERER/VERSION/SHADING_LANGUAGE_VERSION，以及 ProbeCurrent 的全部数值与原始 glError：SSBO bindings、compute/fragment blocks、blockBytes、offsetAlignment、workgroup invocation/sizeXYZ/countXYZ、sharedBytes。列每档需求和 supported/reject reason；“数值准入可过”和“编译成功”分别记录。对不能查询的字段保留 API/error，不将0冒作有效能力。不在NAPI读侧调用GL。

每个对象区分 `SOURCE_V3`、`QUANTUM_INTERMEDIATE`、`QUANTUM_FINAL_V3`，每个阶段区分 CREATE_SHADER、SOURCE_UPLOAD、COMPILE、CREATE_PROGRAM、ATTACH、LINK、REFLECT、ALLOCATE、PREPARE。编译/链接前写 enter，返回后立即记返回状态与紧邻的 raw GL 错误，失败对象删除前读取对应 info log。成功对象的 warning 可保留，首个失败不可被清理错误覆盖。reflection失败记录明确子因、n/capacity/index/nameLength，禁止冒报为 compile。

`glGetError`具有消费性，诊断helper必须把消费到的首错继续送入原firstGlError/返回码判定，不能因“收日志已读走错误”让原失败变成功。日志查询自身错误单列，不冒充原编译/链接失败的错误；不增加hot-path查询或锁内GL。

每对象最多4096字节原始日志，整个尝试最多16384字节；同时记 reportedLength/copiedLength/truncated/queryError/emptyLog。原字节落本地冷文件，JSON显示安全转义，不凭截断末尾拼接新诊断。GL调用必须发生在原 owner/context，不能为收 log 重编译。定长结构先锁存再清理；有界不可变快照供页面导出，换代不覆盖上一首错。编译若长期未返回，读侧可以导出 enter/inCallDuration，不能宣称编译FAIL或完成，更不能强杀GL线程后复用context。

不对 info log 做“必须非空才成功交付”的假门。`compileStatus=false, reportedLength=0` 本身是合法待分析证据；必须证明已调用日志API并记录其真实返回。Create失败后禁止 READY、root接受、trace/scene token生成或Swap伪成功，允许 FAULT 或标STALE旧完整有效帧。

## 3. developer与tester接力

developer 不使用真机，修改独立生成 m2route 的实际三个 program链、诊断锁存/导出与页面入口，保留既有kernel数学和wire。必要非机检查：三个对象各 compile/link/reflect失败的区分、空log/长log/查询失败、失败前对象仍存活、清理不改首因、换代旧快照不被覆盖、读侧无GL、成功warning不作FAIL。使用实际helper和生成路径，不能只测另写模型；完成ArkTS/native编译和签包。原 T452 已交付，不重开旧卡。

后续 tester 带真机标签，同设备串行，以新签包一次冷创建和一次明确新空场复现；保存 raw诊断、完整布局、包/ELF/source hash、真实context能力。先定位首个 program与阶段。实际 shader 语法/扩展问题或reflection/接线问题回architect更新工程再developer；确证设备资源/编译能力无法支持冻结算法后才交designer评估GPU驻留切分。证据仍不能归因则精确补最小独立shader复现，保留原GLSL字节/context/驱动身份；软件GL通过只作离线参考，不是设备支持证明。

执行者要么完成本卡约定范围及角色必要检查，要么具体阻断才停止，局部受阻先完成其它项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改滴答同名清单。成功原卡回执且本人行结束待验收；失败/缺证写未完项、复现原错、证据、尝试、不能继续原因、前置和接手角色，本人行看评论。CPU停投。仅XComponent+shader，背景前景禁止Canvas或软件GL回退，失败FAULT/标STALE旧完整有效帧。独立真机功能/性能卡不作开发关闭门。

## 4. 本轮结论

本卡完成工程诊断缺口首次归因与可实施修订，交developer新增诊断、tester再取设备首错回architect。当前 shader具体失败阶段和驱动原因登记 UNDETERMINED，T461未行使功能继续 NOT_RUN；不把本次归因交付写成产品PASS。

## 2026-11-10 用户要求补齐编译产物持久化

此前仅安排日志落盘，遗漏成功program binary复用。新增 [编译产物持久化工程合同](architecture-T462-program-binary-persistence-v1.md)，明确自动落盘、跨启动加载、环境/代码失效及开发/独立验证prompt。manager据此新增P5实施，不扩正在执行的T468诊断关闭门；当前尚未实施，不能称为已缓存。
