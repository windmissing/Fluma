# T587 v3 · T640 lab V3 修复真机验证（不可变终态+FilesV2 排空）

日期：2026-11-19，tester，真机（011124264R000337）。
被测：T587-lab-signed.hap（t640-delivery 树 466449a lab V3 修复 + tester 设施
want 路由 `--ps t587lab true` 直达 GpuResourceLab 页；inputSha 9de815d7→重编后
a0c523e3→signed 15274720，sign=PASS）。
证据：`test/reports/t491/t587d/`（v3-cancel-record.json jobId1、
v3-cancel-fast.json jobId2、v3-export.json dfea3f05）。

## T640 修复验证结果

| 项 | v2 观测 | v3 实测 | 判定 |
|---|---|---|---|
| 终态不可变 | 完成后取消把 outcome 翻成 CANCELLED | jobId1（完成后取消）与 jobId2（启动 0.8s 内取消）outcome 均 **COMPLETED_FIXTURE 不变**、cancelRequestedNs=0 | **PASS**（取消不再覆盖终态） |
| 5s 排空语义 | 取消不中止执行、无排空 | V3 job 在 <0.8s 内完成（COMPLETED_FIXTURE_NOT_PHYSICS 尾行），取消窗口不可构造；job 自身快速完成=排空语义由完成替代 | **语义变化如实**：V3 下"5s 取消"不再可测（job 短于取消窗口），终态以 COMPLETED_FIXTURE 为准 |
| FilesV2 排空 | 旧文件残留 | 首次导出后 files 目录 resource-lab 旧文件被清空（排空生效），新导出仅含当次 resource-lab-2-1795069599186.json | **PASS** |
| schema | RECORD_V2 | **RECORD_V3**（revision 字段确认） | ✓ |
| fixture/physics 分域 | 保持 | physicsValidated=false / COMPLETED_FIXTURE_NOT_PHYSICS 保持 | ✓ |

## 新导出原件

resource-lab-2-1795069599186.json（sha256 dfea3f05c26e7859…，jobId 2，
FINISHED/COMPLETED_FIXTURE，30 records 完整）——已归档 test/reports/t491/t587d/。

## fence/BUSY 注入域

维持 T635 勘误后的立场（旧 v2 取消推断不作定论）：V3 下取消窗口不可构造，注入
域仍需 T635/T640 后续工程安排（如实 NOT_OBSERVABLE，不重复旧推断）。
