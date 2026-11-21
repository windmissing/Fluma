# T788 v1 · 目标 20 DM SOURCE B 与 C 真实执行补证

日期：2026-11-21，effector，真机（011124264R000337）。
被测：T771 三件套（driver buildId 71b03f56，设备在位）。**T784 归因吸
收**：T775 三臂实为 A（argv `--timing` 省略时 T771 入口回退 A）。本轮
显式传 `--timing=B/C`，各新进程同 K192/S40 输入。证据：
`test/reports/t491/t788/`（B.jsonl/C.jsonl 各 28 条含 TIMING 原件）。

## B 臂（--timing=B 显式）

- TIMING schema **arm=B**，cadenceNs=**250000**，wake.count=**252**
- exitCode=6，firstGuardFailure=PUMP_ENTRY_GUARD DEADLINE @dispatch 12

## C 臂（--timing=C 显式）

- TIMING schema **arm=C**，cadenceNs=**250000**，wake.count=**246**
- exitCode=6，firstGuardFailure=PUMP_ENTRY_GUARD DEADLINE @dispatch 12

## 真实性确认

TIMING schema 分别报 arm=B/C（T775 三臂均报 A）+ 真实 cadence 250000ns
+ 独立 wake 计数（252/246）——**argv 生效，本轮 B/C 为真实执行臂**。

## 对照

两真实臂仍 exitCode=6 DEADLINE @dispatch 12——**deadline 越界与 timing
臂无关**（无论哪个臂窗口都太小）；wall/CPU 与 fence 模式同 T762 一致
（执行 fence 未 sig、退役复用已 sig）；typed/oracle NOT_REACHED（map
未返回）；原期限保持未放宽。

## 回执

归 architect：真实 B/C 臂确认 deadline 与 timing 臂无关——窗口放宽/
两段式 claim 决策不变；T773 reader 原件现含真实 B/C 时序。
