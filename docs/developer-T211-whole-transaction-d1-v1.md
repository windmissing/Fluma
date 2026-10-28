# T211 整体事务等价 D1 开发交付

日期：2026-10-28；developer；不使用真机。输入 T197/T205/T206。本卡紧接 T212 `a27d92c`，不改 GPU 桥模板、旧生成器/native/wire 或渲染。仅新 XComponent+shader 含前背景；禁止 Canvas 上屏/故障回退。生产默认不启用新 route。

## 实现

新增 `physics/tx/` 的 storage/view/owner/AVL/ID/ticket/cert-field/trace/capture/fixture 模块。`TxWholePairAdapter` 为稳定入口，实现在 `TxWholeTransactionCore.ets`；生产与隔离桥拷贝目录引用同一类身份，避免 ArkTS 的重复 nominal type。`WaterSandField.createWholeTransactionFixture` 提供独立同步 owner；显式 `attachWholeTransactionD1` 只用于冷配置差分。STEP、DIRECT_SOURCE、原 QUEUED_SOURCE、RESET、CONFIGURE、CAPTURE 命令保留同步原调用和原回执；queued 回执明确 QUEUED_NOT_ADMITTED，没有新 ACK/准入语义。

storage 按唯一 ArrayBuffer 注册并记录 alias；whole-bank 逐字节复制，scalar header 显式绑定池 live/free/nextId、树 root/idRoot 与有效位/canonical。opaque handle 以 owner 私有引用表校验，旧 handle/foreign view/read lease 拒绝；每次新 root/rollback/关闭推进控制 revision，内容恢复相同也不能继续使用旧 lease。配置容量8，超过受控深度标 UNCERTIFIED，不改原物理错误。实际 Hot 四个 snapshot bank 各有独立索引，物理 sourceBefore/stepBefore/field/budget/event/journal 仍由原 whole-bank、原检查和原恢复点权威维护，没有把只注册的新派生域冒充全部原业务状态。word-undo 是独立可测基础，未启入 D1；不启 D2 证书减算。

PairAdj AVL 每 slot 一个节点，完整 `(phase,sourceKind,sourceId,state,floorX,Y,id,slot)` 全序。邻接只证 eligible，root 取最小 eligible ID，再以原 RN64 差值判定做 rank/select + 完整区间最小 ID 的 b，排除自身。删除以旋转移动节点，不交换物理 slot/key。原 merge writer 的 F64 运算原样；写前摘除、写后按实际值插回，birth/merge/deposit/degrade/crossing/motion 都接 barrier；whole-bank 复制继承树，不逐 seed 重建。退役/复用更新 generation。ID 树保最先 physical slot；重复/坏值转有名 LEGACY_UNCERTIFIED，到原 cut 执行旧完整路径，不提前新增物理拒绝。

ticket ordinal/token 两树保 safe-integer 原 token；不改变 allocateParticleHotLease 次数、grant/denominator/debit/refund/close 图。close 留存历史，原 closed 检查仍权威，begin/rollback 与原预算同步清空树。非 Hot 的 allocate/retain/remove/merge/reset/restore/motion writer 使 canonical 失效，后续完整复制/rebuild 计 H_bulk，不假装已受控增量。

## 计量与残余

`P011_TX_WORK_TRACE_V1`：273 个原函数 scope，20 切点沿原函数和实际原调用落点，保留原 audit/guard/错误块。只插计量与 writer barrier，不更改原数值图/阈值。部分 scope 的 code 表示调用完成；Hot 原状态函数、原 audit、public step 的数值首码记录，SOURCE 的拒绝原因仍以原 receipt 为准，FIND noPair 另由 M_NO_PAIR 及原返回码解释。独立逐 cut 故障注入/首码矩阵由 T213 执行，本卡不宣称已覆盖该矩阵。

固定 counter 记录 birth/find/merge/build/insert/erase/neighbor/rank/range、ID/ticket visit、audit execute、copy、field/原循环、λ/seed/A-B；numeric audit 始终 execute，CERT_HIT=0。scope 保 entered=completed+failed+cancelled+active；每 scope 保存同步 inclusive/exclusive wall，父 scope 扣实际 child interval，未包裹的原 helper/callback 成本仍留在父 scope 中。默认 UTC wall clock，负值/非有限标 INCOMPLETE，不作设备 service 保证。详细32列 trace 默认 DISABLED；启用后满 ring 标 INCOMPLETE，不热扩容，不把缺尾当零。context 含 owner/run/step/attempt/substep/seed/params/input/sourceEpoch/dt/branch/role/ordinal/λ/row。

原完整 field/eta/water/correction/planning/observation/publication 工作保留在 scope/原循环与基线 inventory，不以 pair 节省宣称整链线性。IO/renderer/driver/VM 内部未测量项不填虚假零；T212 Worker 已提供独立 encode/write/fsync 归档 scope，T211 fixture 的冷 JSON 导出另记编码 scope 与 UTF8 字节，物理回执与归档状态分开。没有启用生产异步 owner、后台迁移或新拒绝策略。

`resource-ledger.json` 逐项保留 T205 的473静态分配点及1097风险点，新增 bank/index/trace/ticket/scalar/reference/handle 分项和 cold profile。4×96N 的 pair+ID 与原索引/回退索引并存，ticket48Q，whole-bank8份实际 registered unique backing 另列。原容器、临时对象、字符串、lazy/capture/外部快照未删；extraI/copyIndex reference 容器不是零分配。总量 NOT_TOTALLED，runtime PARTIAL_BOUND，外部 UNBOUNDED_EXTERNAL，无虚假总 MB 或零分配声明。

## 离线证据与编译

- `run_t211_core_units.cjs`：1,804 次独立嵌套 oracle + AVL/ID 结构检查，含±0/subnormal/overflow差/MAX_ID/跨组/重定位/复用；find 与 erase+insert 的 node envelope 同步断言。
- `run_t211_bank_units.cjs`：6/6，raw NaN payload/-0、alias/scalar、child 成功 parent 失败、stale/foreign/read lease、深度溢出、逐字 undo、ticket 历史与 trace 闭合。
- `run_t211_fixture_units.cjs`：N192/512 同输入 LEGACY/TX 原回执与物理状态相同；各64 find，4个实际 bulk build（包含原非Hot恢复失效），不是每 birth/seed 重建；scope 最大深度15是函数调用深度，不冒充 view 深度。audit 闭合、birth 分母身份成立。
- `run_t211_baseline_units.cjs`：以修改前 `a27d92c` 源码为原基线，N192/512 × OFF/LEGACY/TX，每组6条命令（36条），6次中间完整 snapshot 的数组 raw bytes 与回执一致，含重复源、非法时间和 NaN dt；时钟冻结只为同输入差分，不是性能结果。
- `run_t211_commands_units.cjs`：两 backend 各5条 queued/step/capture/reset/configure 命令一致；queued 不算 admitted。
- `run_t211_gpu_tree_units.cjs`：新生成隔离工程真实编入 T211 树适配器，N192/512 各64 pair及完整最终原档与 LEGACY 一致。T212 9/9桥回归也通过，native 是 HOST_DOUBLE，不是 GPU 验证。旧 T212 单测输出 treeIntegration 是其历史固定文字，实际树到件以本卡 tree-bridge 和 source manifest 的 T211_CORE_COMPILED 为准。
- 既有 Hot/M2/particle/gesture/planning/s3/s4/input/advection/clear-water-B 共10通道 exit=0。

主工程及隔离 STRICT_O2 `default@PackageHap` 均 BUILD SUCCESSFUL，零错误，见 `test/reports/t211/{root-package,package}.txt`。签名/安装/真机/E2E NOT_RUN。最终隔离源 manifest、四TU实际编译宏/命令/链接、ELF及 unsigned HAP 身份与复现 zip 同目录归档；生成与复现命令见下。T212 post-build manifest 的 productionTouched=false 是生成器不改根工作区的声明；本卡对七个生产物理文件的改动以本卡 source-hunks manifest 为准，不能解读为 T211 没改生产源码。

复现生成：`node test/p013_compute/tools/generate_tx_gpu_project.cjs .codex/local-test/t211-final STRICT_O2`（使用新的空目录）。JBR PATH 下执行 `hvigorw --mode module -p product=default -p module=entry@default default@PackageHap --no-daemon`；再运行 `archive_tx_gpu_post_build.py <project> <destination>`。baseline 差分先把 `git archive --format=zip a27d92c entry/src/main/ets` 安全解压到 `.codex/local-test/t211-baseline`。

本卡交付受控同步 D1 与具名旧完整覆盖；未声称设备收益、完整产品稳定性、所有 fault-plan PASS、driver/VM 总内存上界、owner 新行为或 D2 优化。后续独立证据按 T213 接力。
