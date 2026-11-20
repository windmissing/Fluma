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
