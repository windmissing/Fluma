# T865 · PC D1 动作与 D2 热页离线交接

Windows PC tester，2026-11-23。沿 [T858](../../../docs/architecture-T858-v1.md) 与 [T860 交付](../t860/handoff.md)。

- `python test/t865/check.py emit` 输出 T496 编排的 PC dry 版本，48 pulse、2 干沙 SOURCE、2 capture control，共 52 动作。 `drive` 按其相对时间将 JSONL 送给外部适配器。正式 run 应由实际 READY/owner checkpoint 启动，适配器把动作送入同一真实 OfferSource/预约入口，并保存声明时钟、实到时钟和终态。外线程适配与证据采集仍待实际接口接入。
- `python test/t865/check.py ingress <T860 --m1-source-trace JSONL>` 检查序列连续、溢出、offer 身份和终态。还需适配器将 ingress/pulse/capture 终态映射回固定 actionId。 `compare <四臂证据.json>` 按真实终态、drain、Completion、command32、producer 双 SHA 和限定的已核身份字段对比，并检查隔离重放的 bank/event 原字、backing 与分区预算。时域等价或原字有缺口时保留实际差异。
- `hot <证据.json>` 读 T860 的预约、FROZEN 页及终态行，独立绑定同进程原接受根及完整 Completion SHA，判超时、撤销、重复和错根。归档采证方提供同进程原 JSONL及接受根锚，不用首根 M1PCOLD1 代替跨根协议。
- `python test/t865/selftest.py --emit <目录>` 可生成独立 CLI 样例。Python 编译、四臂 52 动作合成验证、12 项负控与历史 T496 原动作表复读通过。现有 PC 热页 reader 已准备，实际四臂、全量 D2 和 32 轮待相应接口和 PC 原件。
