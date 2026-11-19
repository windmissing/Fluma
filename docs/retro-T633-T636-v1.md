# T635、T636、T633、T634 架构交付与复盘

2026-11-19，architect，四卡当前约定范围完成；不使用真机、未运行产品/GL/组件测试或构建。只有原件解析、ELF/dynsym/SHA与有限工程规格检查，不能冒产品PASS。

| 卡 | 交付结论和接手 |
|---|---|
| [T635](architecture-T635-v1.md) | 两job原生约0.35s，无执行中取消时刻证据；完成后Cancel改标签为确定工程缺陷。drain helper未接线且deadline/cancel唤醒有洞，FilesV2边界亦需修。developer修合同实现，tester原T587精准续测，v1窄PASS保留。 |
| [T636](architecture-T636-v1.md) | broker现有ABI可直接做独立跨DSO组件harness，无需双GL或新生产信用接口。旧包5符号/最终已交8符号分版本，tester选匹配原DSO与头、重新采本次maps，不能拼旧映射。 |
| [T633](architecture-T633-v1.md) | l3已捕获配置NAPI_DECODE，不能归冷GPU死锁；测试签了早期包。未配置激活/零scope进账与兜底CONFIG_PENDING误导需修，按ConfigTicket/阶段/seal补有界只读证据。tester包准备、T629/T630解码线与developer生命周期修订分流。 |
| [T634](architecture-T634-v1.md) | 有组合transport/VIS代码但产品owner未接入；还存在Completion64/1024B越界、CPU补审核位、材料producer/scene/真实cold缺口。完整W01–W12工程闭包先行，developer不能只加按钮；T600/T601等真实非空帧。 |

各卡同名JSON/对应evidence-v1.json及`analysis-T633-T636-engineering-v1.json`可复核。检查脚本默认只读核对，`--record`仅生成本批工程表和原件收据，不运行旧生成器、不修改产品源。

复盘：UI残留RUNNING不证明worker仍在跑；默认CONFIG_PENDING不证明Configure被接受；对象编译不证明shader被调用；同一DSO映射不证明信用API调用；符号名同样不能跨版本假设GPU ABI存在。下一轮报告应先给同attempt原调用/时刻、完整版本身份和唯一拒绝条件，再推根因。

本会话仅本人四行状态例外与四原卡回执，不创建/关闭别卡，不维护manager同名滴答清单。所有下游prompt受[共同执行边界](architecture-T633-T636-execution-boundaries-v1.md)约束：GPU驻留、CPU停投、新XComponent+shader唯一上屏、30fps、真实前置、开发只编译、tester/effector执行采证；水线13→14→12、目标10和旧湿M1暂停。
