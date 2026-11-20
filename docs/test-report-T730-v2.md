# T730 v2 · 目标 20 G13 单程序/拆分臂冷暖对照——**拆分臂首次编译成功 PERSISTED**

日期：2026-11-20，effector（tester 兼，含出包），真机（011124264R000337）。
被测：T730-mono（inputSha fd7ed272→signed 89be7cc5）、T730-split
（9edec170→707a751b），sign 均 PASS（T729 收据核对一致）。
证据：`test/reports/t491/t730b/`（m1–m6/s1–s4 布局、split-cache/ 两臂
T475 记录原文、v1-mono-arm.json、v2-split-arm.json）。

## MONOLITH 臂：RED（静默死亡复现）

- G13 页直达（全按钮），配置 REST → PROGRAM_BUILD/SHADER_COMPILE:ENTER
  （queuedTicket=177）
- 编译等待 ~5 分钟后**应用静默退出**——无新 jscrash、缓存仍 1 条
- 内存充裕（15.8G 总/10.5G 可用）——**排除 LMK**：单文件 259KB 大
  shader 触发驱动编译器内部中止并带走进程（T594 v8 结论细化：中止是
  monolith 专属，非通用）

## SPLIT 臂：**首次成功** ✅

配置 REST → T475 记录 13 条阶段事件，两大角色全链完成：

| 角色 | 链 | 墙时 | PERSISTED |
|---|---|---|---|
| **G13_COMPUTE**（259KB compute） | COMPILE→LINK→REFLECT→EXPORT_WRITE | **49.8s** | ✅ binaryBytes=301961 |
| G13_SCENE（vert+frag+link） | 编译→LINK_OK→REFLECT_OK→EXPORT | 秒级 | ✅ binaryBytes=11493 |

**设备上第一次大 shader 编译成功且落盘**——拆分架构击穿驱动中止：
MONOLITH 5 分钟截尾 vs SPLIT 49.8 秒完成，同一驱动同一 shader 族。

## 新阻塞点

PERSISTED 后 init tail：**INIT_ERROR_27**（INIT_TAIL_VALID:EXIT，
attempt 209）——编译门全过，编译后初始化尾部失败（新门，非编译域）。

## T727 测量合同记账

- 冷（MONOLITH）：墙时 ~5min 截尾（死亡），NOT_REACHED
- 冷（SPLIT）：compute 49.8s 至 PERSISTED；READY/Swap NOT_REACHED
  （INIT_ERROR_27 门）
- 暖命中：待重跑（PERSISTED 已在，重配置应走 binary load 跳编译）

## 回执

暖命中验证 + INIT_ERROR_27 归因 → architect/developer；SPLIT 臂编译
数据（49.8s/301KB binary）即目标 20 算法效果评估的输入。


## v2 追加（2026-11-20 17:10，暖命中验证 ✅）

**暖命中确认**：重配置后 T475 记录出现 5 条 LOAD 链——BINARY_LOAD_ENTER
→ LOAD_CHECKED → LOAD_LINK_OK → REFLECT_OK → REFLECT_PREPARE_OK，
**binary load 0.01 秒**（对比冷编译 49.8s，加速 ~5000 倍），binaryBytes
301961 复用。12 条记录（7 冷+5 暖）。

**INIT_ERROR_27 在暖路径同样出现**——init tail 门与编译无关：二进制
加载成功但初始化尾部失败。READY/Swap 仍被此门挡住（编译域测量已全，
初始化域归 architect/developer）。


## v5 追加（2026-11-20 17:50，effector；T732 FP32 修复复测——失败面推进，拒绝持续）

被测：T732-split（barrier+FP32 word-offset 修复，签名 PASS c18addcf）。
证据：`test/reports/t491/t730b/v5/`（l1/l2 布局、t732-diag.json、
v5-t732-retest.json）。

### 结果

- Boot/compile/link 全过（PROGRAM_BUILD→PROGRAM_LINK:ENTER 阶段链）
- 失败点 **INIT_FP32_RESULT:EXIT**（T732 插桩生效——从 INIT_TAIL_VALID
  推进到 op 66 FP32 拒绝点）——**但 FP32 拒绝本身持续**（attempt 250）
- sign_pair 反馈：T732 compile.json 的 scene.vert sha（478d2e92）与树内
  文件（bf7b11aa）不一致——回执缺陷，反馈 developer

### T727 表更新

冷编译 ✅（boot+compute+link）｜暖 load 0.01s ✅｜READY/Swap
NOT_REACHED（op 66 FP32 拒绝=唯一剩余门，数值/精度域）


## v6 追加（2026-11-20 18:35，effector；T737 归因后复测——次正规数清零稳定复现）

**T737 归因吸收**：attempt 250 实际首拒=op65（次正规数清零——次正规输入
加零与次正规结果生成均回读 0，本设备 BOOT GLES 程序 FTZ）；op66 两轮
独立探针同因失败；op65 sin(0)/cos(0)/sin(π/2) 哨兵全过=拒绝仅在次正规
域。**T730 v5 修订**：v5 记"已越过 op65 只剩 op66"——按 T737 原字，生效
拒绝仍是 op65（INIT_FP32_RESULT:EXIT 是最后诊断时点非首拒者）。

**sign_pair 通过**（T728 骨架+T732 身份值）：此前失败系我构造 roles 的
域错误——回执 scene.vert sha 478d2e92 为 LF 规范化域（LF 版+\0 实证在
libgrs_g13.so 内），回执本身无误。签包 T729-monolith（e1159ad7）/
T729-split（8df9bf7b）。

**复测**（T729-split 正签包）：G13 页→配置 REST→150s→**CONFIG_FAILED
INIT_ERROR_27 @ INIT_FP32_RESULT:EXIT，attempt 196，bootReadyNs=0**——
与 attempt 250 同签名。**次正规 FTZ 两轮稳定复现**（250/196），非瞬态。

**回 architect**：次正规处理为精度域设计决策——FTZ 安全测试向量（避开
次正规探针）/shader precise 模式/门放宽提案（MUST-PASS 语义审查），
tester 侧无设计决策不可自行变体。


## v7 追加（2026-11-20 18:45，effector；T739 资格插桩复测——三连稳定，原字全暴露）

**sign_pair 一次通过**（T739 回据完整：variant/project/buildId/roles 齐），
签包 T729-monolith（63e89f36，buildId 23209a13）/T729-split（9e35e801，
buildId 16704862）。部署 T729-split。

**复测**：G13 页→配置 REST→150s→**CONFIG_FAILED INIT_ERROR_27，
BOOT_EFFECTIVE_TERMINAL:EXIT，attempt 180**——T739
G13BootQualificationV2 插桩生效，`effectiveRejectStage=OP65`。

**op65 原字（WORK 五项实测 vs 门）**：
| 位置 | 门 | 实测 | 判 |
|---|---|---|---|
| +35 | 0.0 | 0.0 | PASS |
| +41 | 1.0 | 1.0 | PASS |
| +47 | 1.0 | 1.0 | PASS |
| **+53** | **1.0** | **0.0** | **拒绝**（次正规 half-ULP tie 被 FTZ） |
| +59 | 1.0 | 1.0 | PASS |

**op66 两轮封印**：ctrl200To215 双轮 [1+1ulp, 1, 0, 0, 7, 0, 0, FP32 签名]
——索引 2/3（最小次正规 0x00000001、0x00400000）均清零，mismatchMasks
[12,12]，封印双轮在位。

**结论**：第三次独立运行（250/196/180）三连确认 op65 +53 FTZ 与 op66
次正规探针清零——**确定性驱动行为**。T739 插桩完美（生效拒绝阶段、门位
原字、封印/掩码全暴露），但代码侧无法令驱动保留次正规。

**回 architect（设计决策）**：A) FTZ 安全门向量（改 +53 tie 与 op66 探针
为非次正规等价）；B) shader precise 编译模式；C) 门放宽（MUST-PASS 语义
审查）。tester 侧证据完备且稳定。
