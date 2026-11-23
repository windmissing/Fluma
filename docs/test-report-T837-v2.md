# T837 目标13 PC 纯流场判定器续办

本轮按 T605/T631/T828 补 F01–F16 的有限参考样本和逐例负控。原 ROOT/FULL reader、Completion256、GridView 和 OP66 自测复跑通过。坏归档被拒绝。实际字段比较新增 F03 的 Fourier 模态幅值与三个 J 的离散参考，以及 F06 的起动级数误差、稳态误差和尾界。

`python test/t837/references.py` 输出参考样本。实际 FULL 入口仍为 `python test/t837/check.py check MANIFEST ARCHIVE RECEIPT`。当前依据仅覆盖有限解析与合成诊断。完整场收敛、壁剪切、工作账及 PRIVATE_PROJECT 等实际材料须按独立用例分别取证。

T833 的现存收据只有 BOOT 和稳态管线阶段，欠实际接受的 QUANTUM、冷档与 GridView。实测子项记待前置，未经测量的数值与物理条件保留未验证。[离线回执](../test/reports/t837/references.json) [T833 边界](developer-T833-v1.md)
