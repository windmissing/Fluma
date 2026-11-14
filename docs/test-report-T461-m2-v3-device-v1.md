# T461 · GPU M2 V3 能力与整根真机验证（v1 阶段报告：真机首错 TRANSPORT_CREATE SHADER_FAILED，功能验证未达）

2026-11-10，tester。设备 62T0225B10010020，T452-final-complete 签包 SHA256 de316a648a17e86b21b5b7cb9debbde3ceadfc90cf2d20cef9ddd62a7f076959（与卡面一致）。

## 已证

1. **M2 V3 真机入口在位**：Index"开启独立 GPU M2 V3 空场"→独立页，头部 `T452_M2_EXPERIMENT_V3 · K192/Kw128 · GRS_GPU_SCENE_Q20_M2_V3`，revision/档位/token 全对；
2. **真机首错（两次复现）**：`FAULT TRANSPORT_CREATE #6`，TransportCode 6=`SHADER_FAILED`——`gpu.Create` 内 M2 V3 组合 shader 在真机 GPU（Maleoon 910）编译失败；点"新空场 V1"重试同码，确定性复现；
3. **锁存冷档 fail-closed 正确**：transport 未成功创建，"锁存 V3 冷档"→`NO_TRACE`，无伪造 trace；
4. 准入/拒量/在途水/落水全 0（未达运行态）。

## 未行使（如实 NOT_RUN）

能力探针四档明细（maxSsboBytes/bindings/shared 逐项值）、SOURCE 13 pass、部分准入/NO_PAIR/merge+birth、QUANTUM、短接受、同 token 场景/Swap、成功前缀与故障回滚、V2 实验切换、锁存/导出冷档内容与 reader 判读、实际耗时/物理速度/新帧——全部因 Create 阻断未达，不冒任何模型通过。

## 移交（回 architect/developer）

真机 SHADER_FAILED 与离线 SwiftShader 全绿（T452 80 roots）矛盾：组合 kernel 超出真机 GPU 编译能力（指令数/shared/barrier 结构）或触发驱动编译 bug。需 developer 提供真机 shader log（`shaderLog` 字段在 Create 失败路径未被填充——诊断为空）或拆分 kernel。**附发现**：Create 失败路径 `shaderLog` 为空字符串，首错诊断链在真机不可用，建议 developer 在 SHADER_FAILED 时回填编译 log。

证据：`test/reports/t461/`（4 截图 + 2 layout）。
---存在，追加---

---

## v1 补充（2026-11-14，T482-final coherent 修复包 300bd52a…）

来源更正后重测：`T482-final-signed.hap` SHA256 `300bd52a97e360cf533e261bf35433b465f560329f4a73232d8e119cdb51381e`（本地逐位一致；shaderSHA 新 b7d0a027…）。证据 `test/reports/t491/t461/`。

- **入口与身份在位**：`--ps grsm1 false` → 首页"开启独立 GPU M2 V3 空场"（真实 bounds `[254,1414][1027,1554]`）；进入后 `GPU M2 V3 · V2`，声明 `T452_M2_EXPERIMENT_V3 · K192/Kw128 · GRS_GPU_SCENE_Q20_M2_V3`（实际 revision 3 在位）；V3 专属按钮齐（锁存/导出 V3 冷档、导出 M2 创建原始诊断/上一创建诊断）。
- **整根 run**：新空场 → `PAUSED_CLOCK CLOCK_GAP / run 0:1`，四账全零，画布黑——M2 V3 与 M1 同落 CLOCK_GAP 域。
- **诊断 0 字节**："导出 M2 创建原始诊断"×2 产出 `t468-create-1-*.log` 均 0 字节（设备端 ls 实测）；V3 能力域 NOT_MEASURED。
- **NOT_REACHED**：四档验证、SOURCE 13 pass、部分/NO_PAIR/merge+birth、QUANTUM、短接受、同 token/Swap、成功前缀与回滚——全部因零 QUANTUM 未达。交 architect（呈现域根因归 T448/T490 链）。
