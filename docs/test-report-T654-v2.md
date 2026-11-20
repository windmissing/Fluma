# T654 v2 · G13 同页离线设施真机执行——四页 raw 原件全取得（8192×4）

日期：2026-11-19，effector（tester 兼），真机（011124264R000337）。
被测：T672-signed.hap（T672 G13 同页诊断 collector 离线修订+执行包，
signedSha c9a274db，sign=PASS）。
证据：`test/reports/t491/t654b/`（t672out/ 六文件原件、v2-samepage.json、
l1–l5 布局）。

## 执行链（T672 设施，G13 页内同进程）

1. 冷启直达 G13 页（新增"完整初始化诊断"按钮=T672 设施）
2. 配置 REST → UNCONFIGURED 恒持（attempt0——GL 消化未达，如实记录）
3. **完整初始化诊断按钮** → files/t672-1795093174273/ 产出 6 文件：
   status-before.json / r1-status.json / **r1-page0..3.json（8192B×4 raw+
   base64，sampleSeq=16 同代原子快照）**

## 四页原件

| 页 | rawBytes | sampleSeq | offsetBytes | sha256 |
|---|---|---|---|---|
| 0（decode 段） | 8192 | 16 | 0 | 6e5aba26… |
| 1 | 8192 | 16 | — | 126cbee7… |
| 2 | 8192 | 16 | — | 8b9f7ed9… |
| 3（control 段） | 8192 | 16 | — | 981d92cda… |

status：fieldToken G13F1:287785986262658…、coldToken G13R1:…076a27c7、
consumedSourceSeq=0——身份域齐。

## 判定

- **T654 核心目标（g13InitDiagnostics 四页 raw 原件）达成**：T664 家族
  "attempt 232 已死不可回读"的替代路径成立——T672 同页设施在同进程内采样，
  无需外部 seam
- page0 前段 int32 序列含 216/512/128/512 等（GL 能力域候选值）；阈值逐项
  核对（blocks>=4/bindings>=4/invocations>=64/precision>=23/laneX>=64/
  shared>=4096/maxBlock）按 T649 合同 decode 段布局在下轮解析（本报告先锁
  raw 原件 SHA）
- UNCONFIGURED/attempt0 如实：configure GL 消化未发生（同 T654 v1 阻塞域），
  四页为此刻快照（能力查询未跑 = decode 段为初始态，不做资源通过推断）

## 回执

四页 raw + status + SHA 全归档；UNCONFIGURED 域继续（GL 消化问题独立于采集
设施）；阈值解析下轮按 T649 合同 decode 段布局执行。


## v3 追加（2026-11-20，effector；阈值域状态锁定）

日期：2026-11-20。依据：v2 四页 raw 原件+T649 合同 decode 段布局 [0,4096]。
证据：`test/reports/t491/t654b/v3-threshold-status.json`。

### 阈值域判定：NOT_EVALUATED（如实）

- page0 为 **UNCONFIGURED 快照**（能力查询未执行）——blocks/bindings/
  invocations/floatPrecision/laneX/shared/maxBlock 八阈值在配置化 G13 attempt
  快照到手前不可评估
- T649 合同阈值表已备齐（blocks≥4/bindings≥4/invocations≥64/precision≥23/
  laneX≥64/shared≥4096/maxBlock≥profile.ww*4/rawGL==0）
- GL configure 臂阻塞与 T594/T711 同 create/ready 族（上游同源），非采集
  设施缺陷（T672 同页设施已证可用）

### 回执

阈值解析通道就绪（decode 布局+阈值表+解析脚本域）；配置化快照到手即
出阈值判定表。
