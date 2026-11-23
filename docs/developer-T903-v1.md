# T903 · Windows PC 开发交付

2026-11-24。工程依据为 [T902 修订](architecture-T902-v1.md)。

## A · D1 REPLAY 真实根冷档

提交 `017911b4`。空箱预览保留独立初始化 fence 和 `acceptedRoot=null`。首个 SOURCE 仍由原接受分支触发归档。首个 QUANTUM 在 `startBusiness()` 成功后、首次显示 lease 前归档。非预览启动与后续根保留各自的接受边沿。

归档入口核对接受根与业务成功 fence 的 nonce、run、geometry、bank、publication slot/generation、事件区间及各缓冲区范围，按 `(run, geometry, kind, nonce, generation)` 去重。复制提交、同 fence 等待、读回与 SHA 各自留证。提交前退出归还 pin。复制未完成时保留 pin，由 D1 退役路径收尾。文件沿用 `M1PROOT1`。

复采沿 T896 的三个 M1 SPIR-V 和原 52 动作计划，替换为下列 T903 M1 PE。分别用 `--m1-d1 REPLAY_OFF` 和 `--m1-d1 REPLAY_ON`，各给独立的 `--m1-d1-replay-prefix PATH`。记录首个 SOURCE 和 QUANTUM 档、`COLD_COPY_*`、owner 覆盖与 drain。用 `--m1-d1-cold-reanchor` 复核非预览入口。跨臂输入等价仍走 T891 的真实时域判据。

## B · DM2 / R1 SOURCE 截止与退役

两个样机独立保留 250 ms 原业务截止、原派发、1,024 字节热尾与 typed gate。新增 `ROOT_TIMELINE`，输出输入冻结、expected、录制、提交、原始 wait、pulse 年龄、读回、typed gate 和晋升。拒根的同代 fence 追加等待输出 `RETIRE`。仍在途时由 owner 输出 `DEVICE_DRAIN`，具体区分同 fence 信号、设备丢失与尚无退役证据。迟到 GPU 查询的 `acceptedRoot` 为 null。

`--source-timestamps` 在原 SOURCE submit 内增加查询。DM2 按 0–4、5–7、8–11 分段，R1 按 0–5、6–10、11–13、14–18 分段，另记上传和尾拷贝区间。时间段可包含屏障和复制，随原 tick、availability、队列位宽及设备周期一起保留。使用同一 PE、同一模块目录、同一输入分别执行无查询和有查询两臂。BOOT 收据含实际 GPU、驱动、队列及 UUID/LUID。开发机采功能与诊断时间，目标独显按实际场景另测持续帧率。

```powershell
$root = '.codex/local-test'
$inputBody = "$root/t894-model-source.u32le"
foreach ($family in 'dm2','r1') {
  $build = "$root/t903-$family-release"
  & "$build/pc_sand_${family}_host.exe" --modules "$build/sand_assets" --source $inputBody
  & "$build/pc_sand_${family}_host.exe" --modules "$build/sand_assets" --source $inputBody --source-timestamps
}
```

输入 SHA-256 为 `3580ccb1a4199f41708bd9a790f8997f77c4af87dac1743b08c636264e22f55a`。DM2 的 45 个 compute 模块与 R1 的 49 个 compute、6 个 graphics 模块逐文件与 T894 一致，编译角色集分别为 `3639e57f834308bab001ca783f321b6a503c232caa924e04b1d8f1d7e3333ca6`、`f7d812d64eba3c932fcfc41d70fbed48835349819f235b6d9d61fb58216641f3`。

## 编译与自检

| Windows x64 Release | 本地工件 | SHA-256 |
|---|---|---|
| M1 | `.codex/local-test/t903-m1-ninja/hello_world_pc.exe` | `e72b53c684a6757a366ac9a5a6316dc1d542ede6d6616fe9daef0d2635b5fa2e` |
| DM2 | `.codex/local-test/t903-dm2-release/pc_sand_dm2_host.exe` | `3ef1e99ebdca39a956bbe5e0f75a61f71c13218059a384244f9ff86e9e3d3602` |
| R1 | `.codex/local-test/t903-r1-release/pc_sand_r1_host.exe` | `694ad0c245a6ad86b890fc3f8bb79a86de90497f2ae06c93271487c512defc4e` |

- M1 冷档 backing 自检覆盖空根、SOURCE/QUANTUM、错 nonce/slot/generation/fence、非法事件范围。T894 跨 session 终态与溢出自检通过。
- DM2/R1 各自通过 T890 owner、生成表和采集范围自检。MSVC 构建保留原 `parent_adapter.cpp` 的 C4457 告警。
- 构建日志为 `.codex/local-test/t903-{m1,dm2,r1}-build.txt`。M1 复用 T882 的 8 个共享构建输入，按换行归一逐一相符。实际 REPLAY、写盘故障、在途结束、设备丢失、双样机 GPU 时标及数值门待独立 PC 复采。
- 滴答搜索 `T903` 无结果，清单登记为未建卡。本文件为本地交付回执。
