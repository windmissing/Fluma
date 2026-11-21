# T776 v1 · 目标 20 RX SOURCE 同票首拒时序补证

日期：2026-11-21，effector，真机（011124264R000337）。
被测：T766-rx 复用签包（0c980f70，T763 修订，映射 4bdd76d）。前置检查：
**归档无独立 g9rx-source JSON**（T766 轮采证为 dumpLayout 状态行+hilog）——
按合同"原件不足才复用 T766 签包走一次真实入口"。证据：
`test/reports/t491/t776/`（chain.txt 17 事件 ns 时序、l2.json 终态、
g9boot-retire.jsonl 退役段 8 事件、v1 判定）。

## 单次真实入口（pid 34145，ticket 1）

完整阶段链（ns 时序原件）：
- BOOT 编译 ~0.28s + 链接 ~0.3s
- SOURCE_BUILD 41767B（ee66f5f2，T736 冻结）
- SOURCE 编译 **84.35s**（2960796377032→3044556407657）
- SOURCE 链接 **235.0s**（3044556452188→3279478181563）
- SOURCE_REFLECT_EXIT qualified（program=4/40 uniforms）
- CONFIGURE_RESEARCH_EXIT result=0 → **APPLIED result=0**（3279540338125）
- → **PUMP_FAILED code=6 = Result::DEADLINE**（终态状态行采获）

## 首拒与时序

- **firstReject**：Pump（APPLIED 之后）
- **recentPass**：CONFIGURE_RESEARCH_EXIT result=0 + APPLIED result=0
  （紧邻其前）
- **pulse age**：APPLIED→Pump 间隔**分钟级**（编译墙时所致）——对照 DM
  T762 的 1.7μs 夹逼：同一新鲜度窗根因，RX 侧脉龄以分钟计

## 退役原件

force-stop 后 G9BootV1 退役段 8 事件（MAP→UNMAP seal 6559303→PREPARE→
BOOT_AUDIT_OK）。RX BOOT 事件无 fenceWaitRaw 字段（RX/DM schema 差异，
已记）。

## T765 reader 说明

RX 侧原件为 hilog+layout 格式（此构建未产出 g9rx-source JSON）——T765
reader 适用于 JSON 格式原件（DM 侧已用）。

## 回执

- RX 首拒=DEADLINE，同票时序链补证完成
- 归 architect：RX+DM 双族 DEADLINE 确认，脉龄（分钟）vs 窗口（250ms）
  量化对比完成——窗口放宽/两段式 claim 决策输入闭环
