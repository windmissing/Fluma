# T590 v1 · M3 与 G9 同 attempt 拒因续证（T582 修复消费，T579-lab 后新包）

日期：2026-11-18，tester，真机（011124264R000337）。
被测：T582-g9-final-signed.hap（T582 修复消费包，bm clean 后冷启）。
证据：`test/reports/t491/t590a/`（first/last 四件+M3 拒因 JSON）。

## M3A 域（m3Configure N16 链）

- UI 原始词（同 attempt 显示）：**M3_CONFIG_REJECTED / ELEMENT_COUNT /
  route=M3A / bootStamp=202612073253127 / diagGeneration=1 / attemptId=1 /
  qualification=VALID / diagnosticRevision=GPU_INPUT_DIAGNOSTIC_V1**
- 原件 `gpu-input-M3A-first.json`（1638B，sha 3afef15a…）：snapshot
  **expectedN=16 / reason=ELEMENT_COUNT**；`gpu-input-M3A-last.json`（3638B，
  sha 9715dac2…，UI 显示 SHA 9715dac2791e1… 一致）：request
  **schema=M3_CONFIG16_V4 / length=16 / byteLength=64 / words[4,15,192,64,7680,
  8388608,1048576,65536,…]**——N16 配置词全量在档
- T582 修复的 correlated safe reply + bound-input 诊断域**真机生效**（reject 时
  first/last 双文件自动落盘，state=COMPLETE，无崩退）

## G9 域（g9ResearchConfig64 N1 链）

- UI 原始词：**G9_REAL_TOUCH_START_FAULT：G9_CONFIG64_NAPI_ERROR:
  G9_REAL_TOUCH_CONFIG_REJECTED**
- 原件 `gpu-input-G9-first.json`（1652B）：**api=g9ResearchConfig64 /
  expectedN=1 / reason=ELEMENT_COUNT / qualification=VALID**；
  `gpu-input-G9-last.json`（3618B）：request **schema=G9_RESEARCH_SEED_R2_REQUEST1 /
  length=1 / byteLength=4 / words=[2]**
- G9 真实拨动 R2 的 CONFIG64 N1 拒因在 T582 修复包下可显示、可落盘、不崩退

## T590 判定链对照

| 域 | 结果 |
|---|---|
| 标准 JSON 失败（M3/G9） | ✓ REJECTED 结构化（GPU_NAPI_ERROR_V1） |
| UI 同 attempt 原始词 | ✓ 两页均有（bootStamp/diagGeneration/attemptId 原词） |
| 诊断首拒可导出 | ✓ first/last 四件自动落盘（files/ 可取回） |
| 不崩退 | ✓ 两页 REJECTED 后进程/页面存活 |
| 首拒根因 | **ELEMENT_COUNT 双页一致**：NAPI 传参元素数与 native expected 不符——
  G9 expectedN=1（words=[2] 收到 length=1 ✓ 但仍拒）与 M3 expectedN=16（收到 16 ✓
  仍拒）——**计数匹配下仍 REJECTED**，说明 native 侧 expectedN 校验词序/语义不匹配
  或解码合同与 UI 侧 schema 不一致——按 T590"首拒仍 UNKNOWN 先取证再归因"如实
  保留 UNKNOWN→architect |
| T545 四档 SOURCE/QUANTUM/250ms 续证 | NOT_RUN（入口被首拒阻断，诚实记录） |

## 边界

- 不把安全失败（结构化 REJECTED 不崩退）扩为物理 PASS
- 固定两文件 16KiB 原件与 SHA：first/last 四件均 <16KiB（1638–3638B），SHA 在档
