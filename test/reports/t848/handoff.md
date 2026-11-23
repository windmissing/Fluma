# T848 · Windows M1 原生冷档读取

2026-11-23 · tester · Windows PC 离线。

运行 `python test/t848/check.py <m1-cold.bin> <同进程原始JSONL>`。工具按 T830 布局读取八段、保留原字节、报告每段长度和 SHA-256，并核对 SOURCE→首个 QUANTUM 谱系、原 Completion、owner/fence、同根 Present 和退役。适用 T830 首根冷档合同。

`python test/t848/selftest.py` 复用 T842 的原始 GPU Completion 与 T831 的有量、owner 接受前提。真实 SOURCE 为 131072 Q20。其原 Completion SHA-256 为 `3ef843ac316ba7834384fec5f1553cf5f991df4acfe9813e0d559bc6e846ece9`。人工构造的 QUANTUM 正例通过，十项负控覆盖截尾、尾随、段长、SHA、nonce、owner 拒绝、发布代、错几何代和未释放。

T847 的 `run1.jsonl` 至 `run3.jsonl` 当前末条均为 `M1_QUANTUM_COLD_ANGLES / RUNNING`，均未给出冷档 SHA 或同根 Present。待 T847 提供 `--m1-cold` 封存文件及对应进程 JSONL，由 tester 运行上述 reader 完成真档复核。
