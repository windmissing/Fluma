# T848 · 冷档谱系与 T856 原件复核

2026-11-23 · tester · Windows PC 离线。接续 [T858 双分支合同](../../../docs/architecture-T858-v1.md) 与 [T856 原件](../t856/report.md)。

| 原件 | 严格容器 | 原载荷 | SOURCE → QUANTUM |
| --- | --- | --- | --- |
| T856 run2，原档 93872 B | TRUNCATED_PAYLOAD | 原始 payload 和归一化副本逐字节相同。完整 SHA 与原 JSONL 匹配。 | frame 1→2，permit 1→2，arena 1→0，SOURCE eventEnd=1，QUANTUM events=[0,1)。根 15648396347025，发布代 2，槽 0。 |
| T856 run3，原档 94384 B | TRUNCATED_PAYLOAD | 原始 payload 和归一化副本逐字节相同。完整 SHA 与原 JSONL 匹配。 | frame 1→2，permit 1→2，arena 1→0，SOURCE eventEnd=2，QUANTUM events=[0,2)。根 15652757277577，发布代 2，槽 0。 |

两次均由原 SOURCE 和 QUANTUM 的 256 个字、源与物理提交、bank 谱系、owner/fence 与原窗口回执独立核对。队列 Present 和退役均对到同一 QUANTUM 根。T856 的短头只有 8 个摘要字节，少 24 B。归一化副本可用于载荷复核，诊断结果标记 `T856_PAYLOAD_ONLY_ORIGINAL_REJECTED`。严格入口仍要求完整 SHA、真实文件长度和原同进程收据一致。

## 复现

- `python -m py_compile test/t848/check.py test/t848/selftest.py`
- `python test/t848/selftest.py`。原 10 项负控、15 项新增负控通过。覆盖同帧前缀内容和起点、两分支 permit、错误 frame/arena、下一帧非零起点、旧代、原短档、坏归一化对应关系和跨进程收据。
- 历史诊断命令 `python test/t848/check.py test/reports/t856/m1pcold1-run2-normalized.bin test/reports/t856/run2.jsonl --t856-short-original test/reports/t856/m1pcold1-run2.bin`。run3 替换相应文件名。严格检查原档会返回 `TRUNCATED_PAYLOAD`。直接搭配归一化副本和旧 JSONL 则返回 `COLD_RECEIPT_SIZE`。
- T859 修订后的原生完整容器待 effector 从新 PE 取得，随后运行 `python test/t848/check.py <新原档> <同进程原始JSONL>`。待补项为完整容器的真实生产与严格回读。实际同帧前缀分支本轮有合成正负控，两份实证均属下一帧。
