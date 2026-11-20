# T594 v3 · V5 独立入口真机直达——configure 接受后 M5_NOT_READY 持续原件

日期：2026-11-20，effector（tester 兼），真机（011124264R000337）。
被测：T669-v5-signed.hap（T653 unsigned 基+V5 native libgrsm1 3201504B/
BuildId 7f31c1de，T691 指定 V5 工件；signedSha ce9ba9af，sign=PASS）。
证据：`test/reports/t491/t594b/`（c1–cD 布局、v5-m3b-first/last.json、
v3-v5-entry.json）。

## 入口路径（T691 合同实测打通）

无参启动 → Index「**开启独立 GPU M2 V3 空场**」→ M2 V3 页「**打开 V5 独立
研究**」→ **GpuResidentM5 页直达**（V5 独立研究：真实来源层与活动粒子；
ROOT/ FULL/投沙/加水/合成信号/冷档状态/导出 V5 冷档 按钮全在）。

## 执行链与原件

| 步骤 | 实测 |
|---|---|
| ROOT 新空运行 | m5Configure **ok=true / expectedN=32**；reply state=**M5_NEW_EMPTY_RUN_REQUESTED** / requestTicket=1（M3B-last 原件） |
| 冷档状态 ×3（间隔 30s+） | **M5_NOT_READY / API_SEMANTIC / qualification=NOT_EVALUATED**，attemptId 2→3→5 递增 |
| boot 导出 | V5 页无 t456-boot（M3B-first/last 通道）；M3B-first state=COMPLETE / NO_FIRST_FAILURE |

## 判定

- V5 创建链：**configure 已被接受**（ok=true）但 m5ColdCapabilities 在
  attempt 2→5 间持续 M5_NOT_READY——创建臂未发布 READY，ROOT/FULL 冷档
  链卡在 **M5_NOT_READY 门**
- 与 T593 v5 CURRENT 窄窗、T541 SPF WAIT_RETIRED 构成第三条 create/ready
  路径流——三流汇聚回 architect
- RX shader 缺陷（T594-rx 树）与 V5 域**独立**：V5 包是另一 native
  （libgrsm1 3201504B），冷档验证可继续沿 V5 域推进（不等 RX 修复）

## 回执

V5 入口/页面/按钮/请求链原件齐；M5_NOT_READY 持续原件交 architect 定
V5 资格链缺口（API_SEMANTIC 域）。
