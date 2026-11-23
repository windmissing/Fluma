# T857 · OP17 三段写集与首错独立判据

2026-11-23 · tester · Windows PC 离线，按 [T854 工程合同](../../../docs/architecture-T854-v1.md)。

- Python 字节编译通过。21 组样本和 17 项反例通过，覆盖实际/重放两个 scope、J=1/2/4 的 14 个原派发槽、REST、匀转、RAMP 两支、MMS 窗内/窗末、整秒和非整秒 ledger、已选 trial/强制跳过/入口已有首错。正例含数学错误后仍执行账本及唯一 active trace、motion 首错先于 MMS 和 MMS 独立首错、撤权后退役。OP7 另有合成反例，保留 field 首错并允许后续数学诊断位累积。
- 按四缓冲 LE 原字和 SHA 检查三段写集。从原 WORK(0) 和 Layout 独立重算 64-lane FP32 账本归约，核 WORK 旧账快照及整秒 CTRL 更新。校验首错头、诊断字遮罩、错误累计、seal、logical/trace 计数及私有段身份。拒绝错槽、错代、错 nonce、旧段、缺 barrier、撤权后继续或私有发布。
- T844 OP7/OP8 与 T837 参考夹具回归通过。按 T854 摘要复核三份各 5,343,360 B 的 T833/T849 历史 OP7 归档，重算 SHA 和全部五个截面的实际四缓冲差分、首错头、计数和 audit。历史 OP7 原件在其原证据范围内有效。

## 运行与接力

`python -m py_compile test/t857/check.py test/t857/selftest.py`

`python test/t857/selftest.py --emit <synthetic.json>` 生成并检查样本，亦复用本地历史归档；归档位置和 SHA 见 [T854 证据](../../../docs/architecture-T854-evidence-v1.json)。

`python test/t857/check.py <synthetic.json> --synthetic` 独立读回。实际采证以 `python test/t857/check.py <normalized.json>` 执行，字段见 `selftest.py` 的 fixture。一个记录含 BEFORE 和 MOTION/MMS/LEDGER 后的整份四缓冲、Layout、实际范围/对象/owner/代、原六 push 和 root nonce。保原 queue/barrier/fence 与撤销记录及按发生顺序的阶段 mathDomain 位证据。原单派发对照须保相同 BEFORE 原字、layout、push 和末快照。只允许末端新增的 WORK 私段不参与单派发对照；全部原 WORK、CTRL、BASE、CANDIDATE 逐字对照。缺对照时输出 WRITESETS_ONLY。

样本中的 monolith 是合成对照，用来验证判定器。实际 Vulkan OP17 三段及原单派发原字、数值证书和产品发布证据仍待 T833。真实采证时应随标准化记录保存冻结 PE/模块、GPU 原字、模块资格、同步和原始进程收据。
