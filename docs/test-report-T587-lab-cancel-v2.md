# T587 v2 · lab 取消语义与 FILE/SHA 对取回（T579-lab 包）

日期：2026-11-19，tester，真机（011124264R000337）。
被测：T579-lab-signed.hap，bm clean 后冷启 PID 24902。
证据：`test/reports/t491/t587b/`（lab-record-cancel.json f101baee…、
lab-record-5scancel.json 0b8a7f1d…、布局快照）。

## 本轮实采（接 v1 四档分页 PASS）

### 1. 取消语义实锤（两轮独立）
- 完成后取消（FINISHED 后点取消）：outcome=**CANCELLED**、records=30 完整
  （COMPLETED_FIXTURE_NOT_PHYSICS 尾行不变）——取消**翻转 outcome 标签但不改
  records**
- **5s 内取消（RUNNING 期启动 4s 后）**：outcome=CANCELLED、records 仍 30 行
  完整走完——**取消不中止执行中的 job，只翻转结果标记**（两轮独立一致）
- 产品 lab 实际取消语义=标签覆盖式；是否符合 T531 合同预期交 architect 判
  （不自行判 PASS/FAIL）

### 2. FilesV2 FILE/SHA 对（本轮新增两对，累计三对）
| 文件 | SHA256 前 20 | state/outcome |
|---|---|---|
| resource-lab-1-1795050929495.json（v1 完整跑） | 43a9588091568a64…（v1 报告在档） | FINISHED/COMPLETED_FIXTURE |
| resource-lab-1-1795050929495.json 取消后覆盖导出 | f101baeeab7ead4d | FINISHED/CANCELLED |
| resource-lab-2-1795050982290.json（5s 取消） | 0b8a7f1dee7b83de | FINISHED/CANCELLED |

### 3. 释放屏障
FINISHED 后释放屏障无操作（屏障属 RUNNING 域）——FINISHED 后调用安全（不崩退）。

## NOT_OBSERVABLE（保原件回 architect）

- **同锁等位 fence 等位、BUSY 注入、stat 变化/过期/边界/reader 信用**：当前
  lab UI 无注入参数入口；取消不中止执行使"RUNNING 中 BUSY/fence 竞争窗口"不可
  稳定构造——需要 developer 提供 lab 注入参数（job 时长/注入点/强制 BUSY 开关）
  或独立注入 harness 后 tester 续测
- v1 四档分页 PASS 不变；fixture/physics 分域保持
