# T748 · DM BOOT + 非零 SOURCE

2026-11-21 · 目标20 · developer · 离线交付

T738-v5 已取得 DM BOOT 同次 PID/start、独立 driver 与 DSO 映射、完成 fence 和 WORK 8 B 原尾。T748 的新 ARM64 独立驱动从自己的 EGL pbuffer 启动，在同一真实 DM Controller/Transport 中先重跑 BOOT 审核，再创建并反射 SOURCE program。原 BOOT 和 SOURCE 生成字节分别为 22,353 B 和 41,802 B，均低于 49,152 B。BOOT 源沿 T734，SOURCE 源与 T736 已审原件匹配。新的 native 实现指纹和角色集 SHA 随构建收据发布。 [T738-v5 原件](../test/reports/t491/t738/v5/dm20-v5-final.jsonl) · [T736 合同](architecture-T736-v1.md)

- 驱动使用原 K192、S40 seed、不启用 FULL，以原 `OfferModelCommand` 和 `Pulse` 提交 `sourceId=2, mass=0.2, x=64, y=200, vx=0, vy=0`。Controller 仅允许首个具名 SOURCE 在中性角、fresh baseline 和 `pending=0` 下 claim。Transport 再核根 kind、ordinal、实际 SOURCE shader SHA 与绑定 program。该研究变体只授权一个 SOURCE 根；正常 pulse 在 claim 后取得的 pending 单列。
- 保留 12 次原 dispatch、12 次 dispatch barrier、3 次独立 copy barrier、250 ms 原 deadline 及 33,333,333 ns pulse freshness。原 1024 B WORK 尾在同根完成 fence 后读取。T647 的 shader SHA 来自 SOURCE program，`activePipeline` 独立绑定角色集 SHA。
- 真正取得 typed proof、fresh permit 和原 FIFO 队首后，先在局部 `nextRun` 标记 workspace retired，再预检零配额退役。原 `CommitAccepted` 重新验证后才发布钟、计划、队首与 bank。冷档复用 Controller 的既有 broker、credit 和只读 capture。

## 交给 effector 的工件

使用同目录下的新 `g9_dm_differential`、`libgrs_dm_t647.so` 和配套运行库。输入目录以 [原 K192 fixture](../test/reports/t491/t712b/dm20-input) 中的 `config.u32`、`settings.u32`、`identity.u32` 和 `seed.u32` 为起点。SOURCE 命令由驱动生成，逻辑 scope 在本进程新建。

```text
g9_dm_differential INPUT_DIRECTORY NEW_OUTPUT_DIRECTORY [CACHE_BASE_DIRECTORY]
```

保留 `execution.jsonl`、stderr、`boot-work-tail.u32`、`source-command.u32`、`source-input.u32`、`source-config.u32`、`source-completion.u32` 和 `source-cold.u32`。读回失败时以已有阶段、实际 GL/EGL 和原返回定位。同根冷档读取方式如下。

```text
python tools/g9dm/read_cold.py source-cold.u32 --source-manifest source_manifest.json --source-receipt execution.jsonl
```

设备判据沿 T736。用同一次 BOOT 和 SOURCE 收据核实际映射及作用域，核 12/12/12、0xffff、outcome 1、admitted 209,715 Q20、commit +1、quanta 不变、空 active quota、队首消费一次与候选 bank 晋升。冷档仍由独立 oracle 验事件和物理。单体冷档保留原 reader 入口。

离线复构命令为 `python test/t670/prepare.py dm .codex/local-test/t748-rebuild --boot-source`，随后在隔离项目运行 `hvigorw --mode module -p product=default -p module=entry@default assembleHap --no-daemon`。本轮未签包、未用真机，设备运行结果待 effector。 [编译与 ELF 收据](../test/reports/t748/compile.json)
