# P-011 观测③ v3 单 run clean pilot 报告：A_selected 已绑定，真机执行被命令通道阻断

- **日期**：2026-10-21
- **执行**：effector
- **任务**：v3 单 run clean pilot（manager 10:46 批准，任务 6ad82794）
- **被测构建**：742efa6（D-1 整改版；`entry-default-s101-signed.hap` SHA256 `f2c5b06a…0d23`，从干净 worktree 检出 742efa6 重建签名）
- **结论**：**影子审计完成、A_selected=18.7° 已绑定；真机 pilot 未执行——命令通道结构性阻断（EXECUTION_CHANNEL_BLOCKED），按协议 §10 停止线上报 manager**

---

## 1. 前置①：A_selected 影子审计（协议 §4，已完成）

影子审计尚无产出，本次按 §4 用**实际产品物理代码**（`WaterSandField.ets` 经 Node 1:1 通道加载，同一 planner/逐 writer guard/预算逻辑，非去 guard 简化模型）执行：

**审计器**：`test/run_p011_obs3_v3_shadow_audit.cjs`（SHA256 `45862184…bd0c7`），对冻结网格 `A=15.1…29.9（149 点）`从注册初态逐 A 完整模拟九平台/八 leg，逐笔记录提交/阻断、decisionAlpha、有效角。候选阈值 26/20（v3 核验实测 executed=configured 的注册候选）。

**结果**：
- **SHADOW_SAFE 包络**：A∈[15.1, 27.1] 全部平台满足 §3.2 窗口（容差 0.20°、rate=0、≥3 提交子步），全程 **0 BLOCKED、0 回滚**（每平台约 306 个提交子步）；A=27.2 起出现平台窗口失败。注意 742efa6 上 **planning 闸门不再产生 v2 时代的整段冻结**（v2 pilot 的 ±30° BLOCKED 行为已消失）
- **SHADOW_COVERED 检查**（bracket 探针 `run_p011_obs3_v3_bracket_probe.cjs`，SHA256 `2d8cb57a…e73`）：四主 cell 的 §5.2 合格 bracket（相邻提交子步、跨阈、宽 ≤0.5°）：
  - A≤18.6：stop/+ 无合格 bracket（L2 下程 stop 跨越未出现）
  - **A=18.7：start/+ 6、stop/+ 2、start/- 7、stop/- 455，全部 width≤0.5°（典型 0.03–0.10°）→ 覆盖成立**
  - A≥18.7 覆盖持续成立（18.8/18.9/19/19.2/19.4/19.5/20 全部覆盖）
- **A_selected = 18.7°**（满足 `SHADOW_SAFE && SHADOW_COVERED` 的最小网格 A；非默认 30/29.9）

**审计证据**：`test/artifacts/p011-v3-shadow-audit/`（shadow-audit-results.json 全网格 + shadow-detail.json 6 点全转换记录 + bracket-probe*.json）。

## 2. 前置②：出包（已完成）

- worktree 检出干净 742efa6（主工作区 HEAD 已到 12ac332 且 entry/ 有未提交 WIP，不使用），hvigor assembleHap 编译通过（SignHap 00303116 为已知密码长度限制），hap-sign-tool 手动签名
- `entry-default-s101-signed.hap` SHA256 **`f2c5b06abc078cbd511e5c1ebbce61b936772fe7afca73e9cb8b0c2cf4880d23`**

## 3. 前置③：schedule 生成（已完成，§9 契约核对通过）

- `p011BuildScheduleV3(18.7, 0.5)` 语义生成：`calibration/schedule-obs3v3-A18.7-rate0.5.csv`
- **1513 条命令、总时长 196300ms**；九平台 H0..H8（5100ms hold）+ 八 leg L1..L8（100ms 步距、0.05°/步）
- §9 契约核对：UTF-8 无 BOM ✅ / LF 无 CRLF ✅ / 末行单 LF ✅ / 无科学计数法 ✅ / 列序 `commandSequence,offsetMs,segmentId,phaseKind,legId,targetTiltDeg,rateDegPerSec` ✅
- **SHA256 = `7b59b2e4c4f4d87e9390cfc85cc7fb7c4c94a9233d70389cc8fadcba37b6bd46`**
- leg 整除校验：(18.7−15)/0.05 = 74 步 ✅（0.5°/s 主档）

## 4. pilot 执行：被命令通道阻断（未执行真机采集）

runner（`tools/run_p011_v3pilot.ps1`，含 schedule/HAP 双摘要闸门、逐命令 v3legid/v3segmentid/v3phasekind want 绑定）就绪后发现：

**1513 条命令需按 100ms 节拍发送，但唯一可用执行通道 `aa start`（want）单条墙钟延迟约 200–600ms**——每条 ramp 命令 0.05°/100ms 的节拍物理上无法维持（实测 v2 runner 的 9 条相级命令无此问题）。

备选通道同样不可行：
- **手势注入**（uinput/uitest）：0.4°/px 灵敏度下 0.05° = 0.125px，远低于实测最小有效位移 ~50px；且 PanGesture 非线性衰减（标定记录 ~6°/800px 实收）完全无法解析 0.05° 步
- **应用内建 schedule 播放器**：`P011ScheduleV3.ets` 的生成/CSV/绑定函数已实现并通过测试，但**没有接入运行时播放循环**——scheduleId 目前只用于 ledger 相名映射，不驱动内部 tilt 序列

按协议 §10 停止线：**提交具名缺口 `EXECUTION_CHANNEL_BLOCKED`，不扩批、不逼迫闸门、不降低门槛**。真机未安装采集包（s101 已构建未部署）、未产生任何 ledger/run 数据。

## 5. 判定

| 项 | 状态 |
|---|---|
| A_selected 影子审计绑定 | ✅ 18.7°（证据完整） |
| 被测构建 742efa6 出包+防旧包 | ✅ s101 / f2c5b06a…0d23 |
| schedule 生成+§9 契约+摘要冻结 | ✅ 7b59b2e4…bd46 |
| v3 checker 六项预检 | 部分可离线验证（checker 已就位）；真机六项**未执行** |
| **单 run clean pilot** | **未执行 —— EXECUTION_CHANNEL_BLOCKED** |
| thresholdVerdict | **NOT_EVALUATED**（无 run 数据，不产生任何估计） |

## 6. 移交 manager（缺口与选项）

**具名缺口**：`EXECUTION_CHANNEL_BLOCKED` —— v3 协议 §3.2 的 100ms 命令步距超出 want 通道能力，应用缺内建 schedule 播放器。

**选项**（供裁决）：
1. **developer 增加 want 触发的内建 v3 schedule 播放器**（推荐）：want 传 `peakADeg/rate`，应用内部按 `p011BuildScheduleV3` 自驱 100ms 节拍（生成函数已实现并有测试覆盖），外部只管录屏+日志——改动小、协议零修改
2. designer 出新 protocol revision 放宽命令步距（改变已冻结方法，代价大）
3. 维持 BLOCKED（观测③继续挂起）

*红线自查：未放宽任何闸门、未改物理、未编造 run 数据；A=18.7 来自冻结网格最小覆盖规则而非默认值；影子审计明示不计真实 run 配额、不作真机安全证据。*
