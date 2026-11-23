# T894 · PC 跨材料恢复、D1 终态与样机宿主

2026-11-23 · developer · Windows x64 Release。A 与 B 独立提交和编译，B 工件叠加 A。DM2 与 R1 分别构建为 Windows 样机 PE。

| 工件 | 路径 | SHA256 |
| --- | --- | --- |
| A · 跨材料恢复 `8787f323` | `.codex/local-test/t894-a-release.exe` | `186a008eec9d7b52771bb9187a362702ccd41df68ef5c129984b76c92fa9f112` |
| B · D1 终态 `ba503ee5` | `.codex/local-test/t894-b-release.exe` | `f7dab474b8017899f6d3261fd0c96d979b6a8ef18137311e454d3ebdd4faa4af` |
| C · DM2 | `.codex/local-test/t894-dm2-release/pc_sand_dm2_host.exe` | `5788e027a94d37221ebf28333d85d17f425230df0d133d7ea09bdd3c37ca2653` |
| C · R1 | `.codex/local-test/t894-r1-release/pc_sand_r1_host.exe` | `4de99b3a0285f21e56e3a6243a081b45bf9cbf59e9a11d4f295fb04fc62d8cf0` |

## A · 同代恢复

普通 M1 窗口在业务超期后撤销旧 hold。实际提交根须同 fence 信号及 tombstone Poll 清到 Idle。未提交的根须由 owner 明确关账。资源仍在途、lease 未解、窗口换代或设备失效保失败终态。撤销旧 grant 后从同代真实新拍建立 baseline，再发 pointer READY。新 Down 才会绑定当前选择。原留存库存、接受 bank 和已接受计数贯通。已有画面保上一同代帧，恢复收据分记 `lastAcceptedRoot` 和 `lastPresentedRoot`。

沿原 `--m1-window` 加 `--m1-dual-source --m1-source-trace`。 `m1Recovery` 给出原拒因、nonce、真实退役 QPC 及额度销账，新的 READY 给出新 session 和 baseline 收据。 `MODE_CHANGED` 与 Down 原行关联选择、撤销 hold、绑定材料和 READY 时标。原阈值和预算沿 T891。离线自检通过，真实沙→水和水→沙仍按 T891 正反序复采。

## B · D1 跨 session 终态

只有实际 native 入队动作标 `DELIVERED`。每条 admitted pulse 给出唯一 `M1_D1_PULSE_OWNER`，保旧 scope/session/seq、callback、native receipt 和真实 owner 终态 QPC。重锚、关闭、队列满、时钟拒绝及封口分别保原原因。 `M1_D1_DRAIN.ownerCoverage` 按 session 核 queued/accepted/rejected/revoked 和序号掩码，资源退役和覆盖各自登记。实际 fence 分记 `BUSINESS_SIGNALED`、`TOMBSTONE_RETIRED`、`COLD_COPY_SIGNALED`。默认 D1 先走 shader 空箱准备和 WSI，再发实际 READY。 `--m1-d1-cold-reanchor` 可复采首帧后切换 session 的负控。

D1 仍用实际两拍与输入量。按 T891 先用同一 B 工件和原三 SPIR-V 复采 OFF×2。实际时域是否等价由原收据独立判定。离线 owner 覆盖与 T872 回归通过。

## C · Windows 样机

两个 PE 都装载各自目录 `sand_assets` 的实际 Vulkan 模块，在 device/queue 上创建 pipeline 并派发 BOOT、核原始审计词。输入模式显式为 `SYNTHETIC_RESEARCH`，用本机实际脉冲、原 grant 和同 fence 读回，经 T647 typed gate 后才晋升同代资源。DM2 可写原族 ROOT/FULL。R1 可采实际 WORK、同根旧/新 marker、plan、four maps、Completion 和 BOOT 视觉种子。 `--present` 以同代 lease 绘底材、glyph、真粒子、前景，记录实际 WSI 原码。R1 独立于 DM2。

以下命令在仓库根执行。只传 `--modules` 时执行 BOOT。示例输入 `.codex/local-test/t894-model-source.u32le` 采用 T670 已有研究参数，SHA256 为 `3580ccb1a4199f41708bd9a790f8997f77c4af87dac1743b08c636264e22f55a`。量与事件由 GPU writer 实际计算。

```powershell
$root = '.codex/local-test'
$d = "$root/t894-dm2-release"
$r = "$root/t894-r1-release"
$inputBody = "$root/t894-model-source.u32le"
& "$d/pc_sand_dm2_host.exe" --modules "$d/sand_assets" --source $inputBody --archive "$root/t894-dm2-root.bin"
& "$d/pc_sand_dm2_host.exe" --modules "$d/sand_assets" --source $inputBody --steps 1 --archive "$root/t894-dm2-full.bin" --full
& "$r/pc_sand_r1_host.exe" --modules "$r/sand_assets" --source $inputBody --steps 1 --angle-index 3840 --witness "$root/t894-r1-flow.bin" --present
```

两个 C PE 均为 Release， `--help` 与越界角、错族能力的离线拒绝通过。A/B PE 沿 T882 的 G13 构建基线。未执行设备测试。BOOT 资格数值、正式相同真实时域、接受根、真实通量、GPU 档和窗口可见原件待独立 PC 取证。
