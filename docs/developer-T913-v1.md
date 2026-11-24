# T913 · DM2 和 R1 SOURCE 分段计量与等价候选

2026-11-24，按 [T911 工程修订](architecture-T911-v1.md) 交 Windows x64 Release。

## 实现

- 两族新增 `--source-timestamps-detail`。每个 SOURCE 槽各录一对输入就绪、dispatch 完成的原始设备 tick，带 opcode/role/ordinal。DM2 共 27 点，R1 共 41 点。保留原 coarse 查询和无查询两臂。同 fence 安全退役后才取拒根时间戳，仍为 `acceptedRoot=null`。
- `--source-prewarm` 在启动 pulse clock 和请求 grant 前分配 SOURCE descriptor set。只在全部 buffer 句柄、generation、offset/range 及 Common 都匹配时消费这份单次缓存。原命令录制、nonce、输入、Expected、提交和同 fence 仍在业务窗口。分记 reset、上传、copy 录制、descriptor、dispatch、EndCommandBuffer 与 prewarm 自身。
- DM2 候选把全 147,968 字比较分给同一 workgroup 的 64 lane，归约数字最小的原不符字并由原 lane 0 仲裁。SOURCE 的四个审核角色用稳定的单次槽扫描累加 128 列包库存，溢出仍在该列原查账位置拒绝。原来源表、来源归并、动量和审计顺序保留。
- R1 候选针对 ACTUAL 和 REPLAY 各自执行的两个 APPLY，用原四图分别建立按列/rank 的有效掩码，连接只访问有效行。按 32 槽生成 plan 掩码，覆盖 65,536 槽的全部 8 字，原顺序处理有效与损坏的空槽，再做原证书、transfer audit、marker 和 refill。基线的 QUANTUM 分支与 SOURCE 独立。R1 父 SOURCE 比较使用同一最早字归约。
- 角色表分别版化，实际 pipeline hash 仍由原工厂交给 Expected。SOURCE 仍录制 DM2 12、R1 19 个原槽，分别使用原 0xffff、0xcffff 资格、1,024 B 业务尾、250 ms 期限和 typed gate。

逐槽时间覆盖对应同步。DM2 槽 8 的 shader 内部 copy/replay/audit 和 R1 的 APPLY 内部 map、seal、marker/refill 暂共用该槽时标。完整分开须进一步版本化多派发角色及其接受计数。这轮 DM2 列优化集中在在途包审核，来源层的 checked 审计链保留。候选实际原字、首错和耗时的等价/收益由独立 PC 对照记录。

## 工件

每行目录在 `.codex/local-test/` 下。可执行文件是相应族的 `pc_sand_dm2_host.exe` 或 `pc_sand_r1_host.exe`，匹配同目录的 `sand_assets`。

下表是 v1 交付时的 PE 摘要。baseline 两目录已用 v2 同期源码重编译，当前 PE 摘要见后文。两族 baseline 角色表和 shader 文件的摘要沿原值。

| 目录 | PE SHA-256 | 角色集 SHA-256 |
| --- | --- | --- |
| `t913-dm2-baseline` | `149b1f4cd046507b4b5fe8bc0397a997168c0eb9ee35f1631391a7db1936b980` | `3639e57f834308bab001ca783f321b6a503c232caa924e04b1d8f1d7e3333ca6` |
| `t913-dm2-t913-v1` | `ef8ecd4ec3715d29b6ac036ee5783da687e081e4da5fa8eb4f7e38475a20230c` | `ada9bc45d057e8f819499232acbf5d62733d4929a87504f0896ad5f6a59453df` |
| `t913-r1-baseline` | `4803dd762ab762b79ff65b14d70ef50b2107c5d0840d072b14b5c7e994fea318` | `f7d812d64eba3c932fcfc41d70fbed48835349819f235b6d9d61fb58216641f3` |
| `t913-r1-t913-v1` | `72030900da4015004e3b8eb4cb38a71abf28f50fc810fcc354078bd1e7ac4c7f` | `86b479daf81320fef11080b742d20b3d9600e9c43fc7148f057adb595e98eb27` |

构建时使用 `-DSAND_ROLE_VARIANT=baseline` 或 `-DSAND_ROLE_VARIANT=t913-v1`。完整模块 SHA 在对应 `roles.json`。四套构建与 SPIR-V 验证通过。T913 离线校核确认只改变 DM2 五个 SOURCE 角色、R1 两个角色，原表、原掩码与图形模块一致。最早不符、坏空槽和列账溢出模型的离线自检通过，T905 回归通过。共有 C4457 告警留在原 `parent_adapter.cpp`。集显和目标独显的业务运行待独立验证。

## PC 复采

固定 T904/T911 的源体 `.codex/local-test/t894-model-source.u32le`，SHA-256 `3580ccb1a4199f41708bd9a790f8997f77c4af87dac1743b08c636264e22f55a`。先按族交错执行基线和候选，再在同工件上分别加 `--source-timestamps-detail`、`--source-prewarm`、两者组合。原 `--source-timestamps` 可独立复跑 T904。

```powershell
$root = '.codex/local-test'
$inputBody = "$root/t894-model-source.u32le"
foreach ($family in 'dm2','r1') {
  foreach ($variant in 'baseline','t913-v1') {
    $build = "$root/t913-$family-$variant"
    & "$build/pc_sand_${family}_host.exe" --modules "$build/sand_assets" --source $inputBody --source-timestamps-detail --source-prewarm
  }
}
```

保每次首错、真实短等与同代退役。晚根时间戳继续只作诊断，接受仍以原门裁定。逐区间时间是同队列 GPU tick 差，宿主时标另算。

## SOURCE 槽内拆分 v2

构建开关 `-DSAND_ROLE_VARIANT=t913-split-v2`。保留 baseline 与 t913-v1 独立工件。旧 DM2/R1 槽位如下。

| 原槽 | v2 实际派发 | 保留的身份 |
| --- | --- | --- |
| DM2 8，R1 12 | BASE 拷贝 → 独立重放事务 → 重放 mass、momentum、stream 审计 | 事务胶接在同一 Work 的 StageDiagnostic 保留原 event count 和完成字，审计后清除。原事件 opcode 与首错职责沿父角色。 |
| R1 8、13 | 单 scope map 拷贝 → resolve / transfer 审计 → marker apply → refill / 封存 | 两次调用各用原 role、bank 及对应的独立状态。MAP 与 refill 之间保真实 compute 写读依赖。 |

物理计数 DM2 **14**、R1 **27**。新派发逐个推进 GPU 工作计数，host 记录真实派发和 barrier。Windows owner 通过版本化模块集合接入 T647 的本地 SOURCE 扩展行，绑定新合同摘要、原 audit mask、实际 tail 各计数与同 fence 准入。QUANTUM 表保持原字。期限、接受判据和 1,024 B 热尾大小照旧。 `ROOT` 在实际读到尾后输出尾执行/期望/barrier 三个值，晚完成沿原诊断。细时标新增阶段来自真实 dispatch 边界，DM2 31 点、R1 57 点。R1 resolve 与 transfer 审计仍共一个阶段，DM2 槽 10 保原完整打包。

| Windows x64 Release 目录 | PE SHA-256 | 角色表 SHA-256 |
| --- | --- | --- |
| `.codex/local-test/t913-dm2-t913-split-v2` | `2b13c952b40ad0c447ca032d2b46bbc0f4c5ed18740cec6ff1235d9b7910d2bf` | `f51229f446032a95144594151d75fd8500716086774e01ed630c6d06bcdf3f3f` |
| `.codex/local-test/t913-r1-t913-split-v2` | `e15d25813ae552b8c956293cfde843f82200f97653130485c4d5f9e19013f21a` | `58c999caf018194bb22917fce8668838e6381174ab53c708233332fb71787327` |

同期 baseline PE SHA-256 为 DM2 `c709d183777b3ac8781d1c118e1ae18e2059e529d0cd4f1aa08ef545c337af19`，R1 `5839b002649620c64d656a89330988cd804f84c4d28cc178ea9f7f93ff2c6a64`。

两族 v2 及 baseline Windows Release 编译通过。v2 测试对照 v1 角色原字、源槽映射、QUANTUM 表、掩码、原有模块 SHA 和新模块指纹。原 T905 负控及两个 v2 的 T890 owner 对照通过。T914 用新版独立判据接收 14/27 计数和新版角色标识，保 T905 历史 12/19 工件。实际原字等价和设备耗时继续由 effector 验证。
