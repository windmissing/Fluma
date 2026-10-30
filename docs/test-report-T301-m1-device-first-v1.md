# T301 GPU M1 产品真机首证（真机 BLOCKED：CLOCK_GAP 窗口）

2026-10-30，tester 真机（VYG-AL00 62T0225B10010020）。包：T281-signed.hap，SHA256 `2025d193f19bda4543354ec8c5f99cf6120d12d07d2884a9a7bac4ddf0a7c6a8`（实测一致）。pid 44723。

## 一、已达成的验证

### 1. grsm1=true 独立路由真机进入 PASS

- Want `--ps grsm1 true` 直达 `GpuResidentM1` 独立页面（标题 `GPU M1 · 新空场`），XComponent+shader 前景正常渲染，无 Canvas 回退。
- 页面控件齐备：沙/水/发射源选择、离线手动重力 Toggle、光学/诊断面板、K192/W128 与 K512/W64 两档入口、重置新空场。

### 2. READY 身份门真机行为 PASS（同 T282/T295 合同）

- `重置新空场` 点击生效：runLo 0→2→3→4 递增；无 FAULT、无崩溃。
- **READY 曾真实提交**：T+25s 快照 `permissionState=PAUSED_CLOCK / clockPhase=NEW_RUN_REQUIRED / clockDeltaNs=104896094`——READY 提交后因首个 wall delta（104ms，原值未钳）触发 CLOCK_GAP 撤权，与 M0/T283 行为完全一致。
- `firstFailureStage=CLOCK_GAP` 显式可读。

### 3. 冷参数能力门观察

- K512/W64 入口点击生效（runLo 保持、状态机正常进入重建序列，delta=105ms 原值记录）。四档仅此一档观察，K192/W64、K512/W128 未逐一巡检（窗口受限，如实登记）。

## 二、BLOCKED：双源整步闭环 NOT_RUN

5 轮快循环（重置→0.7s→沙+发射连点）全部：

- `lastOfferCode=NONE`——页面 source() 需 identity（READY 门），READY 窗口 <0.8s 未捕获；
- `acceptedCount=0`、`sourceCommitLo=0`、`physicsCommitLo=0`。

**沙/水源 command32→产品终态→QUANTUM→lease→同 token 床/活动沙/W 水体/J/ARRIVE 像素 NOT_RUN**。阻断因子与 T283/T273/T296 相同：测试通道（uitest/主机往返）无法在 READY 窗口内完成交互，非产品缺陷证据——M1 页面本身路由/状态机/原子权限在真机表现正常。

## 三、结论

- 产品真机首证的**路由与状态机层 PASS**；**数据链与像素层 NOT_RUN（BLOCKED）**。
- CLOCK_GAP 沿原合同保首码/delta（104896094/105338750 原值在案）；未伪 CPU 补算、未钳值、未静默改参数。
- 继续条件：亮屏人工保持前台（READY 稳定 ≥2s）后按本报告流程执行即可闭环；软 GL 离线 196/0 为开发证据，不代设备。

证据：`test/reports/t301/`（30 项快照 + 截屏 t301z.jpeg）。
