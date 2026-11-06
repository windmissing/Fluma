# T360 停源悬空链定点真机补证（T359 装置，两臂+沉降完成态）（真机，完成——冻结未复现，证据回 T353）

2026-11-06，tester 真机（VYG-AL00 62T0225B10010020）。验证对象：**T359-handoff 签包 SHA256 `cd9fbef5143100bc28f4c2d49d36a8af9bb876e18e67208e8acd8da614358316`**（开发交付 8202810，verify-app PASS；与 T350 旧包 SHA 完全不同——新包身份如实登记）。诊断装置：cuts ring（256 行×48 词，stage 0-8 枚举，yLo/yHi F64 位型双词）+ 256 子步预算档（Date.now 整数 ms）。

## 1. 包与臂执行记录

- 入口固定 V2（`--ps p051v2 1`），pid 60247 全程不变（含 Home 往返），零 jscrash/THREAD_BLOCK。
- **臂1**：真实触摸 6×1.5s → UP → 前台持续 → 右倾 setTilt → 前后台 → 导出冷档 `v2-budget-t360-arm1.json`（SHA256 `a8a81138…950a`）。
- **臂2**（复现 T350 条件 9 笔快注）→ **UP 后立即导出** `v2-budget-t360-arm2.json`（SHA256 `2b69a6ac…9ee31`，捕获 UP 切点窗）→ 90s 后再导出沉降完成态 `v2-budget-t360-arm2b.json`（SHA256 `f9d2d09a…cd27f`）。
- 顺带追回 T352 时代遗留冷档 `v2-budget-1793882460817.json`（SHA256 `f320e6a6…f0ee3`，一并归档）。

## 2. 核心证据：UP 后沉降链正常（冻结未复现）

### 臂1（cuts 窗 commit 881-890 / frame 437-441；substeps commit 634-889）

- **live 94→90→86→83 at commit 834/836/838**——UP 后粒子**分批着陆/退役**（substeps 逐子步 live 曲线）。
- cuts 窗内 id1/2/3 全部 **y=1.1547、vy=0.0**（床面值，F64 位型双词重组后）——已落床静止。
- UP 后模拟时间：frame 441→1050 = **20.3 物理秒 ≥8s 判据 PASS**。

### 臂2（UP 切点在窗内！cuts commit 2367；substeps 2112-2368）

- **UP 切点精确定位：commit 2367**（held 1→0 边界，134 行注入期在窗内）。
- live 曲线：83 →（注入期 birth）**124** at commit 2365（UP 前 9 笔快注新粒子出生）→ UP 后保持 124。
- cuts 窗（frame 1177-1180）：id1/2/3（旧粒）y=1.1547 已落床；UP 后新粒子 stage2 预测位 y=1.218/1.282/1.346 **仍在下落**（窗尾截止）。

### 臂2b（90s 后沉降完成态；substeps commit 2679-2934）

- **live=124 恒定**（无变化）——UP 后出生的 124 粒新粒子**全部沉降完成落床**，静止稳定。
- cuts 窗内被覆盖的 id1/2/3 y=1.1547 vy=0（ring 覆盖限制如实：256 行仅 3 id）。

## 3. 判定与移交 T353

1. **T350"UP 后悬空冻结"在本臂组合（6 笔/9 笔快注、run 单次、UP 后 90s 观测）未复现**——T359 逐 ID 位型装置下 UP 后物理链完整：沉降→着陆→床增长→静止稳定。
2. T350 冻结证据维持**转录级**（T358 RAW_WINDOW_NOT_SAVED 已裁定）——冻结为**不可复现/条件特异**现象（候选条件差：T350 冻结时 run3 已多次 run 重启 + 4 分钟长注入积累；本卡全部单次 run 短注入）。归因交 T353 architect，可判读方向：多次 run 重启后的状态残留 vs 注入节奏。
3. setTilt 流向：acceptedTiltRad 30°→42°（右倾 +15 生效，89° 目标按限幅合同钳位——与 T350 R8 一致，限幅归 T353）。
4. 装置语义限制（供 T353 消费）：cuts ring 256 行仅覆盖最近 ~5 帧（3 id）；yLo/yHi 为 F64 位型双词需重组；stepReceipt（stage8）ID=0 为命令回执非粒子。

## 4. 边界遵守

未改 dt/物理/业务代码；未冒观察完成（臂2b 以 90s 真实等待覆盖沉降窗）；旧窗口未重建不冒复现旧 run；三入口不重复（单 V2 入口按卡）。

证据：`test/reports/t360/`（4 份冷档 JSON+SHA256、hilog_arm1.txt 21773 行、hilog_arm2.txt 47132 行、layout1.json）；签包 `.codex/local-test/t257-sign/T359-handoff/T359-handoff-signed.hap`。
