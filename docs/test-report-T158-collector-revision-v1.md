# T158 GPU 真机采集器修订（tester 报告 v1）

日期：2026-10-27。角色：tester。不使用真机。按 T152 §1/3/6 第二步。结果：`test/reports/t158/p158_results.json`（6/6 PASS）。

## 结论

**6/6 PASS。tester 采集器（diag/timing 导出器）修订完成并离线独立验证：JSON 合法性、逐样本新 request 身份、parse/schema 校验、exec/consume/oracle 分类全部就位；旧非法 JSON 原样归档（不就地修复、不冒签 PASS）；旧设备 compile/link info log 搜遍全部合法采集位置确认 NOT_CAPTURED（不编造、不重跑设备）。供 T161 设备正式复测使用。**

## 修订内容（tester 自有文件，生产/共享源零改动）

1. **数组首项逗号 bug 修复**：`t142_device_diag.cpp`（tester 自有诊断导出器）中 `runIdx++?",":","` / `sampleIdx++?",":","` 前置自增导致首项也输出逗号——改为首项判断（`runIdx==0?"":","`），任意嵌套层级均不再产生前导逗号。
2. **逐样本新 request 身份**：5 轮采样不再复用同一 identity——每样本 `identity(0, gpu.contextEpoch(), cap*100+rep+1)`，serial 按 cap+rep 唯一。
3. **exec/consume/oracle 分类 + protocolPass 标志**：每样本显式分类 `EXEC_CONSUME_OK / EXEC_CONSUME_NO_PAIR / EXEC_ORACLE_MISMATCH / EXEC_CONSUME_PROTOCOL_ERROR / EXEC_FAILED` 与 `protocolPass` 布尔——**consume 失败永远不被计为协议 PASS**。

## 离线独立验证（L1–L6）

| 用例 | 实测 | 结论 |
|---|---|---|
| L1 旧 JSON 非法确认 | timing/fixture_diag 两文件 JSON.parse 均失败（`[,`），byteLength+SHA256 归档 `legacy_audit.json`；**原文件未修复** | PASS |
| L2 内存转换 | 仅删除"紧跟 `[` 的逗号"（通用正则，覆盖 runs/samples 两级嵌套）即解析成功，各 2 runs；timing 10 样本 consume 全部=PROTOCOL_ERROR——**分类失败如实呈现** | PASS |
| L3 schema 校验 | 必需字段（exec/dispatchCount/5 项时延/waitCalls/waitReturn）齐全、waitReturn 枚举合法、dispatchCount 11/12 与 Nmax 对应 | PASS |
| L4 V2 导出器源检查 | 逐样本新身份语句、首项无逗号形态、classification/protocolPass 标志在位 | PASS |
| L5 旧编译日志 | 搜遍 test/reports 全树+t141 采集目录：**NOT_CAPTURED**——设备编译错误（`Expected token … found 'half'`）当时只打在 hdc shell stdout 未重定向到文件；记录于 `compile_log_archive.json`，**未重跑设备编造证据**；T161 正式跑必须捕获 compile/link stdout+info log | PASS |
| L6 来源标记 | 导出器区分 T140 fixture（argv）与内置 synthetic 池（argc≤3）；leafTopSample 是 CPU leaves dump，不冒 GPU 逐层表回读 | PASS |

## 声明

- 修订只涉及 tester 自有 `t142_device_diag.cpp` 与本卡新执行器/归档；T141 交付源、T152 分流文件、生产代码零改动。
- 注意：T141 共享源 `pair_executor_v1.h/.cpp`、`pair_wire_v1.h` 等工作区存在并行会话（T157 schema/改名）未提交改动，本卡编译设备 diag 时这些头文件以工作区现状参与编译；本卡产出 `t158_diag` 二进制 SHA 归档时按实际链接内容记录。本卡未触碰也未回退这些并行修改。
- 真反例协议问题（F-T142-1 echo 错位）仍按 T152 §2 等 developer 正式 shader 修订；本卡不判 PASS、不改 owner。
- 旧文件保留：`test/reports/t142/t142_device_timing.json`、`t142_device_fixture_diag.json` 原字节不变。

交 manager 验收；T161 按 L4/L5 约定执行正式设备复测。
