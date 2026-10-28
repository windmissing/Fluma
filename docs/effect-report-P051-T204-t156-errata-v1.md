# P051 T204 T156 证据版本勘误回执 v1

日期：2026-10-28。责任：effector。任务卡：`6ae152f0e4b0c3103c056d6e`。消费：T116 V3 `16b41af`（`architecture-T116-T156-whole-transaction-v3.md` §2/§4.1）、T156 `952da4a` 报告与 raw。  
状态：**ERRATA_APPLIED / NEW_EVIDENCE_RECOVERED / OFFLINE_NO_DEVICE**。  
按 T116 V3 逐项版本化勘误 T156 报告；**并恢复出三臂本 run 的 ledger/admissions（T116 §2.1 标 NOT_CAPTURED 的关键缺项部分到件）**，其中含新的硬数据：**R1/R2 失败时池 live=512 满载**。

## 1. 版本化勘误（T156 报告按 T116 V3 修如下四点；原报告与 raw 不改）

| # | T156 原表述 | 勘误后口径（T116 V3 依据） |
|---|---|---|
| E1 | "R2 栈在 seedBody←liftByShake（无 merge 帧）" | R2 采样栈**含** `P011MergeExactIndex→mergeOldestHot→editPoolHot`（另有 audit APPLY_TAIL、sortActive→collectBirthHeaders→seed）；多个采样位置不能概括为"无 merge 帧"，也不能按采样次数求和（V3 §2.1） |
| E2 | "R3 CLOSED 的 runId 是 step3 旧 run（1793148867083）……页面复用该 run" | R3 有**独立 run** `p011-run-1793149047925-3`（PID 24636），本 PID 真实 CLOSED **274/0**；CLOSED 先于首 pan 3.689 秒。console 检到的 258/0 属 step3 旧 PID 21223 的文件（脚本 `Select-String -First 1` 未过滤 PID/run/time 所致）。"R3 复用旧 run"不成立（V3 §2.2） |
| E3 | "cmd-audit.log 241MB（含全部命令审计）" | 收窄：audit 仅覆盖 `Run-Cmd` 包装的操作；**裸 click/swipe 与两次 `hdc file recv`（0B）不在其中**且未保留退出码（V3 §2.2） |
| E4 | "gesture_sand 0 收据三次复现（shake=0 也现）——与卡死正交的独立问题" | R3 的 gesture_sand=0 可由**提前收尾**解释（captureFinished 先于 pan，`stageGestureSandAndFeed` 先检查后 inject；pan accepted 只过坐标门）。**不能据此宣"触摸注入三次未注入/独立 bug 已定"**；真实未收尾路径若仍失败须自行取证（V3 §2.2） |

另按 V3 §3 同分母修正：T156 §1 "聚合 O(N²logN) 比旧 O(N²) 更糟"混用聚合与单次分母——正确表述为**同 M 分母下旧项 O(MN²)、新项 O(M(N+L log L))**；B=Θ(N) 时旧可达 O(N³)、新 O(N² log N)；不证明所有负载实际更快（早命中负载旧扫描更便宜是可检验假设）。聚合结论以 T194 整体复杂度设计为准。

## 2. 新证据恢复（T116 §2.1 缺项部分到件）

既存检索（设备 cache，无需重跑）：**三臂本 run 的 ledger/admissions 全部存在并已归档**：

| 臂 | run | 本 run ledger | admissions | 内容 |
|---|---|---|---|---|
| R1 | `1793148908460-3` | **1,924,626B / 4 行** | 0B | substep1-4 全 committed 守恒 true |
| R2 | `1793148975272-3` | **1,924,426B / 4 行** | 0B | 同构 |
| R3 | `1793149047925-3` | **53,938,355B / 274 行** | 0B | 与 V3 记录的 274/0 一致 |

**硬数据（推翻/补强既有判断）**：
1. **R1/R2 失败时池 live=512 满载**（substep3 起 live 256→512、massH 0.0382；LIFT 事件批量 256 粒/子步）——T116 §2.1 "step3 Nmax512 不能替代失败时 N/L" 的缺项**部分到件**：失败时 N=512（非 T156 报告估的 192）、L=512；按 V3 §3.1 公式，单次 find O(N+L log L)、B 进循环次数由 256→512 的 LIFT 规模支撑。**T194 的 B/L 输入参数获得实测锚点。**
2. R1/R2 卡死前 4 子步全部 committed、守恒 true、`errorCode=null`——失败发生在第 5 子步（未及写 ledger），与 freeze 时点吻合。
3. R3 274 行全 committed、sourceEpoch=0（pan 全部在收尾后，与 E4 一致）、live=0。
4. admissions 三臂均 0B（空流）——双流 manifest 仍 INCOMPLETE（缺 admissions 实流），不改完整性判据（V3 §2.2）。

## 3. 判定与移交

| 项 | 判定 |
|---|---|
| T156 报告勘误 | **完成**（E1–E4 + 同分母修正，版本化附于 §1；原 raw/报告不删改，勘误以本回执为准） |
| 既存检索 | **超额到件**：三臂本 run ledger 53.9MB+1.9MB×2 归档（原判"仅 step3 一份"修正） |
| 新硬数据 | **失败时 N=512 满载、LIFT 256 粒/子步**——T194 designer 的 B/L 实测锚点；R1/R2 卡死前 4 子步健康 |
| T116 状态 | 首错原码/实际 M/A 计数仍缺（V2 诊断 T187 未触发，无导出）；本卡不移除"看评论"，交 T194 使用新锚点 |
| T187 衔接 | V2 通道未触发的窗口与本卡 N=512 数据共同缩小 T194 负载设计空间（满池 birth 场景可直接构造） |

## 4. 边界声明

- 离线+只读设备 cache 检索，未重跑测试；原 raw 与 T156 报告零改动，勘误以本回执版本化。
- 三臂本 run 文件 SHA 与行数见 `test/artifacts/t156-real-load-20261028/`（`*own-run-*` 六文件）。
- 滴答 T204 卡评论已发；清单行状态留 manager。
