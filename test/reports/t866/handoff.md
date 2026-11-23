# T866 · G13 完整候选根独立判据

Windows PC tester，2026-11-23。沿 [T861](../../../docs/architecture-T861-v1.md)、T857、T837 与 T631。

- `python test/t866/check.py slots <候选.json>` 校验 14 个原槽及两 scope，锁生产逻辑 SOURCE 16/QUANTUM 820。候选物理计数单列，当前检查形态为完整 OP17 三段并可含 OP7 拆分。复用 T857 四缓冲与 128 B 私段逐字判据；实际 commandBuffer/queue、绑定、六 push、各角色摘要及两道 compute 依赖须来自原实际录制收据。VkMemoryBarrier 或覆盖同 WORK 范围的 buffer barrier 可入证。撤权分别检查提交前零 submit 与提交后同 fence 退役，拒绝接受、发布或资源泄漏。原单派发缺输出时记录 MISSING_PC_REFERENCE。
- `project <F02.json>` 检查私有投影、Q32 均值规范、离散压力公式、投影残差及空间阶。 `full <manifest> --archive <FULL.jsonl> --receipt <原收据>` 用 T837 复核原 u/v/pi、根/代、物理单位、OP66 和数值门，再给出 F03 黏性衰减或 F05 制造解的单样本结果。
- `matrix <清单.json>` 逐份重读 manifest/archive/receipt 后检查 F03 网格、J、倍增黏性、时间阶与连续场偏差，以及 F05 两种运动的网格矩阵和空间阶。离散后向欧拉一致与连续场阈值分开报告。 `laplace <F03原字样本.json>` 独立读专用周期算子输出、SHA 及空间阶。正式运行还需保 phi 的真实来源、QPC、单位和候选全链实际原件。
- `python test/t866/selftest.py --emit <目录>` 生成可独立读回的样本。Python 编译和独立 CLI 通过。14 槽、两类撤权、F02 三档、F03 18 组、F05 6 组及 11 项负控通过。另以 T854 原件重读三个实际私有单槽样本 ACTIVE/ENTRY_NOOP/MMS_FAULT，均符合写集判据。该实测只覆盖 slot 10，正式完整根及数值实测待 T833。
