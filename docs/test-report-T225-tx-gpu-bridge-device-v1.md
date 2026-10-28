# T225 整体事务与 GPU 桥真机全链 · 三 backend 对照报告 v1

- 角色：tester（真机 VYG-AL00，串行）· 日期：2026-10-28 · 状态：**结束待验收**
- 消费：T224 7 PASS 到件解锁；被测工程两份隔离包（各自完整性证据齐）：
  - **t212-h1-final**（T212 桥自测工程，sourceManifest=7540f964…，unsigned sha 3a26975d…）→ GPU_PAIR_V1 / LEGACY_SCAN
  - **t211-compile-final**（T211 差分工程，sourceManifest=e5f8f18c…，unsigned sha 444c5434…）→ TX_ADJ_AVL（该工程含 TxGpuTreeAdapter）
- 签名包：`.codex/local-test/t212-h1-final/entry-t225-signed.hap`、`.codex/local-test/t211-compile-final/entry-t211-signed.hap`
- 归档：`test/reports/t225/t225gpu1.json` / `t225legacy2.json` / `t225avl4.json`（+首次 avl3 作废记录见下）
- 执行入口：`aa start --ps txBackend <B> --ps txRunId <id> --ps txUnsignedSha <sha> --ps txFixtureId WET_192_64 --pi txSteps 32`

## 真机结果：三 backend 全 COMMITTED，三向状态逐字节一致

| run | backend | 包 | physicalStatus | states | physicalMs | cpuInclusive[0]/[1]ms |
|---|---|---|---|---|---|---|
| t225gpu1 | **GPU_PAIR_V1** | t212-h1-final | COMMITTED | 64/64 | **1380** | 212 / 332 |
| t225legacy2 | **LEGACY_SCAN** | t212-h1-final | COMMITTED | 64/64 | 2606 | 188 / 1594 |
| t225avl4 | **TX_ADJ_AVL** | t211-compile-final | COMMITTED | 64/64 | 3753 | 216 / 72 |

### 正确性（同 fixture WET_192_64、32 步、64 roots）

1. **三向 states 64/64 逐字节一致**（gpu==legacy 64/64；gpu==avl 64/64，跨两工程同 fixture 语义）——三 backend 物理结果等价。
2. **GPU_PAIR_V1 native oracle 64/64 全 AGREE**：每次 selectPair 的 native 结果与 CPU oracle 判定一致（status=PAIR 64）。
3. LEGACY_SCAN nativeRows 全空（正确：不走 native）；TX_ADJ_AVL nativeRows 全空（正确：tree adapter 不产生 native 行）。
4. GPU_PAIR_V1 三 shader（pair_build/query/reduce）真编译执行：expectedSha==actualSha 三枚全符、compileStatus=OK、log EMPTY、GPU dispatchCount=11、fence CONDITION_SATISFIED、GPU 真执行**非 HOST_DOUBLE**（retirement native 块全程落盘）。

### 成本（同 offered 负载 64 包、WET_192_64、32 步；clock=uptime ACTIVE）

- 物理总时长：GPU 1380ms < LEGACY 2606ms < AVL 3753ms（GPU pair 路径最快，≈1.9×/2.7×）。
- CPU 分段：LEGACY 的 cpuInclusive[1]=1594ms（扫描主导）远高于 GPU 332ms（GPU 桥 overhead）与 AVL 72ms（树最小）；AVL 树桥 CPU 成本最低但物理总时长最高（树构建/维护在 step 内）。
- gpuTimer=NOT_MEASURED（无 device GPU timer——如实不编 GPU 纯时延）。

## 诚实边界（不冒完成）

1. **首次 avl3 作废**：T212-h1-final 工程无 TxGpuTreeAdapter（worker 仅建 NativeTransport），TX_ADJ_AVL 注入断言失败 TX_GPU_ADMISSION_FAILED——**工程缺口非产品缺陷**，改用 T211 工程（含 adapter）重跑成功。两工程 sourceManifest 不同，avl 结果与 gpu/legacy 的对照是同 fixture 语义对照，不是同二进制对照。
2. **20 cut 逐一故障注入矩阵未在本卡达成**（T224 已声明）；本卡按 T225 合同补验设备可触及面：三 backend 正确性、GPU 真执行、同负载成本对照。满池 synthetic 锚点、B/M/A/λ 逐计数深审、freeze 两臂 PID 绑定复跑、服务 p50/p95 —— **未执行**，不在本报告宣称。
3. ledger 4096 槽 gpu vs legacy：both-null 256、one-null 2304（GPU 桥额外记桥内时延槽）、both-diff 98（记录语义差异：GPU 行含 bridge/prepare 时延，legacy 行只记扫描量，'49' 列 5 vs 32 为量纲不同非同义数值）——**states/nativeRows/after 才是等价性判据**，ledger 差异如实披露不隐藏。
4. 生产默认不迁移：本卡全部在隔离工程执行，未触碰生产 HAP/开关。

## 失败上报

无阻断。TX_ADJ_AVL 在 T212 桥工程缺 TreeAdapter 属工程侧已知缺口，已用 T211 工程闭环，建议后续工程合并时补齐。
