# T752 · RX BOOT + 非零 SOURCE

2026-11-21 · 目标20 · developer · 离线交付

T749 已将 RX ticket 1 的 BOOT、活进程 PID/startTick、实际 ARM64 映射及 fence 后 8 B WORK 原尾绑定。T752 增加独立 `BOOT_SOURCE` 构建，经原 RX 研究页、XComponent、Controller 和 GLES owner 运行。22,318 B 的 BOOT 沿用 T734 审核，41,767 B 的 SOURCE 与 T736 冻结字节一致。通过 BOOT 审核及 SOURCE 实际 program 反射后，原 Configure 才进入 `APPLIED`。两角色各自持有程序与缓存身份，SOURCE 的预期 proof 和冷档通过角色集 SHA 绑定。[T749 回执](test-report-T749-v1.md) · [T736 合同](architecture-T736-v1.md) · [T746 接续](architecture-T746-v1.md)

研究页在有实际 EGL extent 与合格 scene 后激活 scope，通过原 `g9RxSource` 提交 K192、sourceId 2、mass 0.2、x 64、y 200、vx/vy 0，之后发送首 pulse。owner 只允许首次、fresh baseline、队首 SOURCE、中性角且 `pending=0` 时 Claim。首次 Claim 前已有配额会撤权并保留原因。独立变体的 Touch ingress 禁用，Transport 也拒绝其余 kind。每根仍使用原 12 pass、12 dispatch barrier、3 copy barrier、250 ms deadline 和 33,333,333 ns pulse freshness。

通过真实 fence 后从 WORK 原位取得 1024 B。typed proof 和同 scope 通过后，先预检原 FIFO、命令 body 与局部零配额计划退役，再由原 `CommitAccepted` 晋升 bank。研究页冷档保存冻结的 INPUT、Config、Completion、状态、事件和同根设备凭据，`g9RxStatus` 随档记录实际 program、fence/GL/EGL、提交、roleSet、同票进程映射及可用时的首 Swap token。SOURCE 有真实正提交后才具备缓存准备条件，实际落盘由 T475 独立取证。

## 签包与设备交接

- 以[编译收据](../test/reports/t752/compile.json)中的未签名 HAP 签包，重新核包内 ARM64 ELF 与实机映射 BuildId。工件位于 `.codex/local-test/t752-final/entry/build/default/outputs/default/entry-default-unsigned.hap`。
- 启动原研究页，点一次「启用研究」。等页面显示 SOURCE outcome 和实际准入量后点「保存原件」，保存输出 JSONL，并采同 PID 的 `G9RxConfigure`、`G9Boot`、`G9RxSource` 日志。若失败，保原票据/最后阶段，启动新 scope 后才重试。
- 设备正控为 outcome 1、admitted **209,715 Q20**、commit +1、quanta 不变、原 FIFO 只消费一次、同根真实原尾和 `ROOT_BEGIN → SOURCE_BIRTH → ROOT_END`。用原 oracle 检查粒子、质量与事件。若采到同代 XComponent Swap，关联真实 token，并标记 SOURCE 的物理时间为零。

离线校核冷档使用随包编译收据给出的 `source_manifest.json`。

```text
python tools/g9rx/read_cold.py <archive.jsonl> --source-manifest <source_manifest.json>
```

重建使用 `node tools/g9rx/prepare_build.cjs .codex/local-test/t752-rebuild --boot-source`。在隔离目录运行 `hvigorw --mode module -p product=default -p module=entry@default assembleHap --no-daemon`。本轮开发未签包、未跑真机。

BOOT_SOURCE、BOOT-only 和默认 RX 均零编译错误。8 条离线准入断言通过，SOURCE 静态反射为 40 个 uniform 和四个 SSBO。协议尺寸及字段偏移一致。BOOT_SOURCE 的 Controller 和 Transport 私有尺寸因收据而增加，实际尺寸列在编译收据。
