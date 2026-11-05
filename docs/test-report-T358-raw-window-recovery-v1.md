# T358 停源冻结原始窗口追回裁定（RAW 窗口缺失确认 + T350 身份勘误）（无真机，完成）

2026-11-05，tester。按 architecture-T353 §2/4 追回 T350 C run（pid 5352）从真实 UP 前至 frame 720 的原始文件/hash 与同 run 手势、T347STEP 行。终点=旧资产追回/缺失裁定，不重跑不等待新卡。

## 1. 旧资产盘点（全部存档 SHA256）

| 文件 | pid | T347STEP | T218DBG | p013_scene | SHA256 |
|---|---|---|---|---|---|
| `hilog_C_v2.txt` | **5352 ✓** | 3（frame 120/240/360） | 9（inject seq 1-9） | 56 | `965c9407d24d2cd8846e6ea0b974437e5bfe44e6d361a56c7a48a998232528bc` |
| `hilog_C_fgswitch.txt` | 5352（前后台段） | 0 | 0 | 19 | `1de099f1bd2f69e86e58a17fd335fedf693a01dfd2d6eec8562c29236f7135bf` |

- **pid 隔离核实**：B2（V1 停源，pid 2542）与 D（倾角，pid 8130）日志与 C run（5352）无混入——`hilog_C_v2/fgswitch` 全部目标行 pid 均为 5352 ✓。

## 2. RAW_WINDOW_NOT_SAVED（frame >360 窗口）

- `hilog_C_v2.txt` 是 20:00:5x 的 `hilog -x` 快照，**不含 frame 480/600/720 STEP 行**（逐字节检索 False）。
- T350 报告 §3 引用的 frame 480/600/720 三行来自采集后三次交互式 `hilog -x | grep T347STEP | tail`（墙钟 20:01:57 / 20:03:49 / 20:05:41），**原始全文从未落盘**——RAW_WINDOW_NOT_SAVED 成立，可证边界=**frame >360 的冻结证据无 RAW 原件，仅有会话转录文本**（转录值三方一致：pYmin=54.646947334308884 / pYmean=88.04859555536524 / motion=126）。
- 勘误 T350 报告 §3 证据栏：frame 480/600/720 行应标注"会话转录（RAW 未存档）"而非归档日志引用。保原报告版本不动，本文件即勘误件。

## 3. 同 run DOWN/MOVE/UP 原始行裁定

- **uitest 手势注入无 hilog 轨迹**（系统设计：uiInput 不产生应用侧事件日志）——DOWN/MOVE/UP 原始事件行**该证据通道不存在**，非未保存。
- 同 run 手势期锚点（唯一可用）：`T218DBG inject seq 1-9`（20:00:19.367 → 20:00:27.516，held 期采样）+ UP 后零新 request（T350 §R5）。**不拼 B2/D 日志**——pid 隔离已核实。

## 4. motion 语义核

wiring 源码（commit 41c5351 `P051DerivedV2Demo.ets`）：`Math.abs(ps.pVy[i])>0 → motion++`——**motion = 池内 pVy 非零粒子计数**，非 plan.motionCount。T350/T347 报告中 motion 字段语义按此读取。

## 5. 汇总 vs 逐 ID 位型

- 全部旧档仅含 STEP 汇总行（pYmin/pYmean/live/motion/mh），**无逐粒子 ID 位型数据**（T347STEP wiring 不输出 pState/pY 数组）。
- T350 §3"pY 逐位不变"指**汇总统计值 12 位一致**（pYmin=54.646947334308884、pYmean=88.04859555536524），不是逐粒子位型对比——**不冒充逐 ID 位型**；T353 判读须知此粒度限制（汇总冻结与逐 ID 冻结不等价，理论上存在内部互换而统计恒定的可能，但 12 位双统计同时恒定已强约束）。

## 6. 结论（回 T353）

1. **可追回**：C run UP 前窗口（inject 1-9 + frame 120/240/360 STEP + scene 56 行）RAW 在档（SHA256 上表）；motion 语义=pVy 非零计数；pid 隔离无混入。
2. **不可追回（RAW_WINDOW_NOT_SAVED）**：frame 480/600/720 冻结窗口 RAW 原件；DOWN/MOVE/UP 原始事件行（通道不存在）。
3. **T350 勘误两点**：§3 证据栏 frame 480/600/720 标注"会话转录"；§3/§8"逐位不变"限定为汇总统计粒度。
4. T353 归因可用的最強事实：**UP 前窗（RAW 在档）pY 正常下降**（frame 360: pYmin=114.51、motion=126）；**UP 后冻结仅有转录级证据**——归因若需逐 ID 位型或 frame>360 RAW，须新采集卡（非本卡范围）。
