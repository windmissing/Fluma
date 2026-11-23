# T848 · T863 原生完整冷档严格复核

2026-11-23 · tester · Windows PC 离线。独立运行 `python test/t848/check.py test/reports/t863/m1pcold1.bin test/reports/t863/run1.jsonl`，直接读取未改写原件，通过 `FULL_SHA256`。原档 94408 B，含 72 B 封套和 94336 B 载荷。八段长度及逐段 SHA、完整载荷 SHA、原档与 36 行 JSONL 的文件 SHA 均登记在 [独立收据](t863-strict.json)。

SOURCE 与 QUANTUM 的原 Completion、提交、bank 谱系、实际 owner/fence 和同进程 WSI 回执通过。实际为下一帧，frame 1→2、permit 1→2，QUANTUM event 范围 [0,2)。同根 15700005297258、发布代 2、槽 0 的 Present 与退役均对齐。同帧前缀覆盖沿 v2 合成证据。

Python 编译及既有 25 项拒绝用例通过。基于 T863 的临时副本另验证摘要尾部改字、截尾、错 nonce、owner 拒绝与进程起点错配，均如预期拒绝。

T863 摘要的 SOURCE SPIR-V 摘要栏多抄了两个字符。实际模块文件 SHA 与原 JSONL 及 T859/T856 固定指纹一致，为 `1912af9ed97e30d9eb158f0af2d6eaf5dcbabe6fc32bc04bd57b8226fc4c7162`。T859 PE 实际 SHA 也与交付匹配。原档、原日志和 T863 汇总均原样保留。
