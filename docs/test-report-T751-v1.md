# T751 v1 · 目标 20 DM 非零 SOURCE 签包与真机验证

日期：2026-11-21，effector（tester 兼，含出包部署），真机（011124264R000337）。
被测：T748 driver（d9dcd3a2 交付：g9_dm_differential 962704B buildId
0a496a94 + libgrs_dm_t647 1353304B）——设备 md5 验证后复跑（首次 dm21-out
误用 T745 旧 driver【md5 738388e7，file-send 静默失败】，强制 rm+重发后
dm21-out2 用正版【md5 08044402】）。证据：`test/reports/t491/t738/v7/`
（dm21-v2-final.jsonl 26 条原件、dm21.jsonl、v7 判定）。

## BOOT 段：全过（25 条记录）

ATTEMPT_IDENTITY（pid 4219/startTick 5850766/driverMappedBuildId
a2f4a69e/so 映射 9b6cc802）→ BUILD（BOOT 源 22353B）→ COMPILE→LINK→
REFLECT→ROOT_INIT→WARM→FINISH（fenceWaitRaw 37146）→ MAP/UNMAP（8B 尾
[0,6559303] 再次确认）→ **BOOT_AUDIT_OK** → CREATE_EXIT result=1638。

## SOURCE 段：控制层拒绝（如实）

SOURCE_ROOT 事件（T748 合同全字段）：SOURCE program 41802B（<49KB）创建
反射成功；sourceId=2/mass=0.2/x=64/y=200 按 T748 提交；**12 dispatches+
12 barriers+3 copyBarriers 已提交**；passes 9 元组索引序列完整。

**拒绝点**：result=6 / controlFault=9 / typedAccepted=False /
preflight=False / fenceCreated=False / mapReturned=False /
admittedQ20=0 / committed=False——**typed proof、1024B WORK 尾、原
oracle 非零 209715 Q20 未到达**（mapReturned=False 后无 map）。

## 工件核对

- 映射 BuildId：driver a2f4a69e + so 9b6cc802（同票记录）✔
- scope/ticket：scope 58820616386719/ticket 1 ✔
- 原输入及 SHA：inputSHA 568c126c/parameterSHA 9b9160f4/layoutSHA
  ae83a418/seedSHA 17c88ede ✔
- 12 pass/3 copy ✔（提交侧）
- pbuffer：eglContext/eglDisplay/eglSurface 原件 ✔（实际 GPU 根证明）
- 1024B WORK 尾/oracle/typed proof：**NOT_REACHED**（SOURCE 拒绝）

## 附带发现

so md5 失配：设备 b99227cc vs T748 回执 780cc7c1——已标记，归
developer 核对（可能 handoff 目录 so 与回执生成时点不同）。

## 回执

- BOOT 段：PASS（含 8B 尾再确认）
- SOURCE 段：控制层拒绝（result=6/controlFault=9）——**首次归因归
  architect**（controlFault 9 语义）；so md5 失配归 developer
- READY/有效 Swap：NOT_REACHED（按实证）
