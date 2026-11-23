# T877 · Windows Vulkan 候选时序与取证交接

Windows x64 Release 编译通过。候选诊断 PE 为 `.codex/local-test/t877-release.exe`，SHA256 `ce824652d3d24408b50a55f0de1522fd6eefd63cbaa1fa5f603f35451c66e7f1`。

## 两个对照入口

| 参数 | 业务 fence 内 | 业务 fence 后 |
| --- | --- | --- |
| `--g13-diagnostics full` | 1,024 B Completion、14 个槽的 2,352 B latch/首错、V2 七段原字 | 校验、接受、读 GPU 查询、写档 |
| `--g13-diagnostics hot` | 原派发、原屏障、相同 Completion 和槽证据，共 3,376 B | 接受或确认私人投影后，在下一根录制前用独立命令与 fence 复制实际七段，并核对完整档的热前缀 |

表中槽证据为 QUANTUM 的范围，SOURCE 的热前缀只有 1,024 B。在同一 T833 fixture 上另加以上参数，每臂使用不同档案 basename。两臂留相同 16/820 逻辑计划及 850 次 QUANTUM 候选派发，按实际句柄避免重复绑定。独立自测从原计划复现 QUANTUM pipeline bind 160 次、descriptor bind 6 次。默认 T833 命令继续走既有整段取证与原绑定方式。生产映射保留。

`hot` 已有热前缀但缺安全冷窗口时，档名以 `.hot` 结尾，JSONL 保对应 SHA 和接受身份。可导出 ROOT/FULL 的完整档仍沿 `PC_G13_T833_RAW_V2`。仍有 in-flight 的拒绝或冷读超时不进入下一根。

## 收据与解释

- `T877_ROOT_WAIT` 保同根、设备、队列、fence 和原始 QPC。分别记录 grant、录制、submit、全部有界短轮询、首退出原因及独立 retire wait。短轮询请求为 `min(2 ms, 剩余期限)`；退役 signal 不能用于原许可接受。Win32 pulse 保序号、callback 与 native receipt 原时刻，保最后有效值及触发失效的观察值。 [Vulkan fence 参考](https://docs.vulkan.org/refpages/latest/refpages/source/vkWaitForFences.html)
- `T877_GPU_TIMESTAMPS` 保实际 queue 的有效位数、周期、查询可用位与 ticks。ACTUAL、REPLAY 以及压力和黏性各有分段标记，其中压力和黏性 span 会包含其间的其他工作。设备时标用于分段，期限使用 QPC。缺能力或对应 fence 未完成的查询保明确缺项。 [Vulkan 查询参考](https://docs.vulkan.org/spec/latest/chapters/queries.html)
- `T877_ROOT_ACCEPT`、`T877_COLD_COPY`、`T877_FILE_WRITTEN` 与 `T877_DEVICE_DRAIN` 分别保接受、冷读、写档与异常退役。绑定、数值门、33,333,333 ns 和 250 ms 截止按原合同。

离线 C++ 自测覆盖短轮询剩余期限、业务与退役返回不同、首因不覆盖、定容截断、真实脉冲顺序与失效以及 850/160/6/14 计划计数。实际 GPU 时间分布、整段复制成本和 F02/F03/F05 数值由 T878 等独立 PC 证据判定。
