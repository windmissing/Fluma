# T72 被动诊断接线交付

日期：2026-10-26。developer；不使用真机。开发完成，设备补采由 T74 另行执行。

新增固定 256 条 ring，单行最多两条事件，每次最多发八行；sequence/累计 dropped 明示缺口。覆盖原始 TouchType/local/ns→ms 时间、model.start/move/stop 原因、逐 tick source/累计 stale、sensor 投递计数、真实 wet sink 以及实际 dry request/receipt。dry 路径不会触发 wet sink，两者明确区分，不能把 sink 无回调误判成零准入。run/physicsCommit/frame 连接到新管线提交 code/swap；swap 只表示接口回执，不代表显示器 present fence。

触摸时序、stale 行为、准入/物理公式不变。新增只读 commit getter，诊断不调用改变身份的 getter。测试将同一输入分别送给开/关诊断的真实模型，对照场数组、粒子池质量与 sourceEpoch 完全相同。

验证：`test/run_t72_diagnostics_units.cjs` 3/0；证据 `test/reports/t72/passive.json`。ArkTS CompileArkTS 与 BuildNativeWithNinja 成功（公共构建证据 `test/reports/t75/build.txt`）。无真机/端到端结果。仅接新管线观测，不新增 Canvas 上屏或回退；完整场景能力依旧由 T71/T56 的专属实现承担。
