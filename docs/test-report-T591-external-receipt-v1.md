# T591 v1 · M2 外部收据与创建原件独立续证（T588 合同）

日期：2026-11-18，tester，真机（011124264R000337，MLN-AL00，OS 7.1.0.566）。
被测：T579-m2fix-signed.hap（SHA a332c0a1…，包内 ELF 04b98996…，与 T588 记录一致）。
证据：`test/reports/t491/t591a/`（external.json + 时间线快照 3 件）。

## 1. external.json 独立收据（T588 §"T591 现在可直接完成"）

- HAP 完整 SHA256=**a332c0a1cd541e586a42984dc33bb353486c3852ec87b567b8a95606d13c44b3**
  （与 delivery.json wave1 逐字一致）；包内 `libs/arm64-v8a/libgrsm1.so`
  SHA256=**04b98996346e2706fd54e6d3da5e91fefd028d5180c408fb231473554f3c559f**（同）
- bm dump 安装核验、设备/OS/build、安装时间、当前 PID 在档
- **原 manifest 的 m2PackageDeclaration 保持 DECLARATION_MISSING 不改**（
  COLD_WANT_EXTERNAL_RECEIPT 留空是既有意图）——external.json 以原 SHA 为 link
  独立落盘，不冒设备加载证明

## 2. 同 attempt 二次只读 Create 诊断：不可得（如实）

- T461 原 attempt（LINK 开放区间 inCall≈41.15s）的进程早已不在；本轮回装+bm clean
  后冷启产生**新进程新 owner**（M0 页快照：epoch=4 / runLo=2 / surfaceLo=3 /
  contextLo=3 / STATE_LOST）——按 T588 规则"进程重启/换代必须新 attempt，不拼接
  旧 LINK 开始和新 Create 结束"，**同 attempt 后继诊断 NOT_POSSIBLE**
- 新 attempt 快照（185 行/47 页/missing=0/dup=0/GREW_DURING_EXPORT INCOMPLETE=
  活 owner 诚实截断）：lastOfferCode=NONE、typedType=-1、sourceCommit=0、
  firstFailureStage=CLOCK_GAP——新 attempt 无 SOURCE 输入、无 Create 活动，
  不构成对原 LINK 区间的任何后继证据

## 3. 判定

- external.json 独立收据 **交付**（文件 SHA 链闭环，原 DECLARATION_MISSING 保持）
- 同 attempt Create 后继：**NOT_POSSIBLE（进程已亡，不拼接）**——保
  nativeEndNs=0/NO_TRACE/DECLARATION_MISSING 真实状态；不因 T571 V5 FULL 交付猜
  M2 创建成功
- 如需 LINK 开放区间的真实后继：需要 developer 在 M2 Create 域提供可查询的
  terminal/stage 字段或跨重启的 Create 状态持久化（回 architect 精确缺口，不凭缺
  code 派物理返修）
