# T587 v3 · lab 排空真机复测——排空/终态/FilesV2 导出全 PASS（fixture 域）

日期：2026-11-20，effector（tester 兼），真机（011124264R000337）。
被测：T668-signed.hap（T668 lab 排空+FilesV2 离线夹具执行包，T531_RESOURCE_LAB
页直达，signedSha b1866e03，sign=PASS）。
证据：`test/reports/t491/t587b/`（lab-export.json 完整原件、v3-lab.json、
l1–l3 布局）。

## 执行链

启动 → 查询（UI poll）→ 释放屏障（排空）→ 导出（FilesV2）——四步全通，
页面契约自述"不创建 XComponent、物理 owner、SOURCE 或 Swap；不宣物理通过"。

## 验收核（T531_RESOURCE_LAB_RECORD_V3 原件，jobId=1/generation=1）

| 检查 | 实测 | 判定 |
|---|---|---|
| 排空 | 释放屏障后 `pendingResources="NONE"` | **PASS** |
| 终态先于读数 | terminalNs=327416765099377 < pollSampleNs=327427769007502 | PASS |
| 终态语义 | state=FINISHED / outcome=COMPLETED_FIXTURE / firstFailure 空 | PASS |
| 排空后取消域 | cancelRequestedNs=0 / cancelObservedNs=0（无取消介入） | PASS |
| FilesV2 导出 | files/resource-lab-1-*.json 落盘，recordsSHA256=8e19f2d2… | PASS |
| 身份 | scope pid=65246 / route=m2 / source=T531_RESOURCE_LAB_V1_FIXTURE_SYNTHETIC | 齐 |

## 判定

- **排空（释放屏障至 pendingResources=NONE）真机复测 PASS**；终态不可变
  （FINISHED 前无取消路径触发）与原文件分页导出全部成立
- fixture 域证据（FIXTURE_SYNTHETIC / physicsValidated=false / 不宣物理）——
  页面契约与记录一致，不冒产品 PASS

## 回执

T587 lab 域验收项全 PASS；此前 v2/v3（T640 lab V3）结论在设备 fixture 域
复现确认。无未完成项。
