# T955 · Windows Release 开发交接

2026-11-24 · 开发交付。G13 F02/F03、G15 干前沿和 G16 晚尾取证各有独立入口。业务 250 ms 门和原状态提升条件继续生效。

## G13 · 819 封存与求解分段

`--g13-seal-stages` 将 seal-v1 的 819 单个逻辑槽拆为三次 GPU 派发。PREPARE 并行清零后按原字序填 Completion，同时消费一次逻辑槽。SHA 从同一 canonical state buffer/range 按原 SHA-256 顺序计算。PUBLISH 并行复制两个 scope 的 Completion 并写封印。三个派发之间有明确计算屏障。F02 保留 820 个逻辑槽，物理派发由 850 增为 852。封存阶段可分别计时，本版未测得 SHA 耗时下降。

`--g13-cycle-scope actual|replay` 单臂记录 J1/micro0 的逐周期或块边界。F02 记录压力 16 个周期，F03 记录真黏性 8 个块。另圈出 ACTUAL/REPLAY 的 J2/J4 预排范围。实际执行和收敛由冷档的对应 scope trace 与槽快照判定。

开启上述任一诊断时，若业务和常规退役等待超时，最多再等 5 s 确认原 fence，然后用独立冷拷代保存 Completion、BASE、CANDIDATE、REPLAY、WORK、CTRL、PACK0/1。归档记录 Completion 与各段 SHA。超期 F02 保留私有诊断状态及 `acceptedRoot=null`。校验者可由归档重算 canonical state SHA，逐字比较两个 scope，并复算体场。

```powershell
$l = ".codex/local-test"
$g13 = "$l/t955-ninja/hello_world_pc.exe"
$f = "test/reports/t868/fixtures"
$common = @("--g13-diagnostics","hot","--g13-tail-timing","--g13-slot-snapshot","--g13-split-op48")
& $g13 --g13-candidate "$l/t955-f02-a" --g13-fixture F02 "$f/F02-config128.bin" "$f/F02-state.bin" @common --g13-cycle-scope actual --g13-seal-v1
& $g13 --g13-candidate "$l/t955-f02-b" --g13-fixture F02 "$f/F02-config128.bin" "$f/F02-state.bin" @common --g13-cycle-scope actual --g13-seal-stages
& $g13 --g13-candidate "$l/t955-f02-replay" --g13-fixture F02 "$f/F02-config128.bin" "$f/F02-state.bin" @common --g13-cycle-scope replay --g13-seal-stages
& $g13 --g13-candidate "$l/t955-f03-actual" --g13-fixture F03 "$f/F03-config128.bin" "$f/F03-state.bin" @common --g13-cycle-scope actual
```

按 A/B/B/A 复测全 SHA 段与两侧边沿。同一 F03 夹具可改用 replay 单独计量。seal-v1 SHA 保持 `8561de20…`，三阶段模块依次为 `b9a999c6…/9f24f251…/52343783…`。

## G15 · 开放干前沿

新增角色包 `g15-dm2-dry-front`。预测和校正的静止试量，对冻结 F 至少一侧为零的内面使用共享零冲量。来源预检通过后从最终批准 q 重算干前沿冲量，以左到右的前缀 RNE 分配供体 pEntry。由原 pEntry、最终共享通量、原床修正和外力重建入账动量。实体壁、两侧实际湿库存的压力、候选回滚和 reason 7 继续沿原门。WORK 面内字 24 为干边分类，25 为原 q，26–29 为原冲量，0/4–7 为实际批准 q/入账冲量。列 stage 24–27 保留静止掩码试量。对原 T942 包，预期仅 25、27、29 角色改变。

```powershell
$g15 = "$l/t955-g15-package"
& "$g15/pc_sand_dm2_host.exe" --modules "$g15/sand_assets" --g15-dune-mirror --g15-tilt-q8 -11520 --steps 1 --quantum-timestamps --raw-envelope "$l/t955-g15-neg"
& "$g15/pc_sand_dm2_host.exe" --modules "$g15/sand_assets" --g15-dune --g15-tilt-q8 11520 --steps 1 --quantum-timestamps --raw-envelope "$l/t955-g15-pos"
```

T952 离线原件复核覆盖旧负向门、镜像、q=0 的干边及合法湿边。候选实际业务、来源和能量证书由 tester/effector 复核。

## G16 · B13 余时与 B52 晚尾

新增 `diagnostics` 入口。查询在原业务提交中记录 ROOT、FREEZE、每 32 微步、SEAL 和热拷边沿。超期根的回执字段在业务日志标为缺值，原 fence 退役后才在 `late-completion.u32le` 留原字和冷诊断。不会提升 private commit 或 scene。冷热 fence 代分开记录。B13 继续保持两段上限和 11/9 失败语义，持续承托策略等待算法裁决。

`remaining-v1.u32le` 的前 32 字包含两个 16 字记录，分别给出 micro、segment、microStart、dt、段起点、remainingIn、hit、remainingOut、原 journal 起止和候选量，word31 为完成微步数。其后 32 字为独立冷提交的重扫，状态 1 为按当前 swept 候选与判据未发现新事件，2 为事件见证，3 为不确定或检查失败。原 journal 中保留两个真实事件组的键和 Q24 括号。重扫在保存原候选、原接触和 witness 后执行，只复用私有 scratch，保原 receipt；不漂移、不求第三次冲量。其他诊断根输出 `time-v1.u32le`。

```powershell
python test/t928/fixtures.py --out "$l/t955-g16-seeds"
$g16 = "$l/t955-g16-release/t909_angle_probe.exe"
$spv = "$l/t955-g16-shaders/g16_multi_v1.spv"
& $g16 g16-multi $spv "$l/t955-g16-seeds/G16-B13.disk64" static 128 2 "$l/t955-b13" diagnostics
& $g16 g16-multi $spv "$l/t955-g16-seeds/G16-B52.disk64" static 256 1 "$l/t955-b52-static" diagnostics
& $g16 g16-multi $spv "$l/t955-g16-seeds/G16-B52.disk64" fast 256 1 "$l/t955-b52-fast" diagnostics
```

G16 扩展模块 SHA 为 `6d4beb5e51e665fb0c853c7f411f85a61e6d1ff9ecd09bef632913d50c396a52`，完整源码 39,749 B。

## 工件收据

| Windows x64 Release | PE SHA-256 |
| --- | --- |
| G13 `hello_world_pc.exe` | `eb0e841e19a2eeda2ca390302206ee6a4c176ca1443f0706991d1ee53064c475` |
| G15 `pc_sand_dm2_host.exe` | `bd66e8416e5506850876e0d1a394030f5fdcc94128f5c1180dbf93298797110c` |
| G16 `t909_angle_probe.exe` | `4e619c8e01e1ed5eeb0a788d9a4313c4acc218b41891ab440238111a0f4bf24d` |
| G16 `t936_g16_freeze_fixture.exe` | `ce844beda87b9cb6948525196a672caba46c436ccc4fb76411b11207338440a5` |

三套 Release 编译、SPIR-V 验证及 T952 冻结样本离线复核通过。G15 `roleSetSHA=7d92f63e…`，只改变 25/27/29，角表与派发计划全等，最大源码 43,369 B。G13 已核三阶段的原 SHA 输入和封印位置。UHD 630 端到端数据由 effector 采集。
