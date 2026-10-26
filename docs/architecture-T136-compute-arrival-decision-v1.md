# T136：compute到件裁定与受控原型边界 V1

2026-10-26；architect；原卡 `6adf6602e4b0e6ce82d0734c`。消费T131提交 `1a4cffd`、`docs/test-report-T131-compute-capability-v1.md`、`test/p013_native/t131_compute_probe.cpp`、`test/reports/t131/`现有归档，承接T130第7节。不使用真机，不重跑测试，不改生产代码。

## 1. 裁定

**设备compute基础能力已观察成立；允许进入隔离原型准备，实际pair算法实现仍以T137证明交付为前置。同步合同先按本文修正，再产生有效原型性能证据。生产迁移与加速收益均UNPROVEN。** 不因尚无pool收益而重新索取已交GL版本/map输出；也不把能力PASS扩成恢复、reduction、池事务或产品PASS。T123 CPU精确索引继续并行。GPU路线保持最高调度优先级，不省略算法和证据前置。

T137设计、独立harness同步/归档修订可以并行；未取得T137确定性/复杂度/边界证明前，不实现自行推测的pool并行算法。设备能力不是当前算法证明的阻塞项。恢复/graphics竞争/完整attempt收益是原型阶段的既定测试范围，不要求先把完整收益测完才能准备原型。

## 2. 已有能力及准确范围

VYG-AL00，设备 `62T0225B10010020`，归档实际GL ES3.2、GLSL3.20、EGL1.4，major/minor查询3/2；`#version 310 es`编译/link和N192/512/4096 map输出、同输入重复输出匹配。独立64×64 pbuffer成立，不代表XComponent窗口或与渲染并发已测。

实际kernel是 `out[i]=in[i]*3u+1u` 的uint32逐元素map，local_size_x=64，有length越界保护；**没有reduction、多级SSBO消费者、shared-memory通信或context恢复测试**。不能用报告中map/reduction合称扩展它的覆盖。

| 归档值 | 可消费的含义 |
| --- | --- |
| workgroup count各轴65535；size各轴1024 | 各轴上限，且每组invocations总积仍受1024限制，不能1024³ |
| maxInvocations=1024；shared=32768B | 查询上限，未实际压力分配/通信验证 |
| SSBO max block=1073741824B | 查询上限，不是成功分配1GiB；当前每个buffer仅768/2048/16384B |
| `ssboBindings=16` | 源码查询的是GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS，不能当全局binding point数 |

通用harness将max block用64位查询/存储，并分别归档compute blocks与全局SSBO bindings；当前1GiB在有符号32位内，不能据此否定当前设备查询。

## 3. 同步工程问题：先修合同再交实现

源码dispatch后没有glMemoryBarrier；fence仅标完成进度，不能替代shader写的访问类别可见性。按[Khronos GLES3.1规范§7.11，PDF第133–134页](https://registry.khronos.org/OpenGL/specs/es/3.1/es_spec_3.1.pdf)，shader写后CPU映射前使用GL_BUFFER_UPDATE_BARRIER_BIT，后续shader读取前使用相应GL_SHADER_STORAGE_BARRIER_BIT。每层原型索引/reduction依数据消费建立barrier，不以重复map恰好匹配证明通用同步正确。

两次glClientWaitSync返回值均未消费：必须记录ALREADY_SIGNALED/CONDITION_SATISFIED、TIMEOUT_EXPIRED、WAIT_FAILED，超时/失败停止消费；建立有界deadline，记录每次wait及GL错误。map失败、glUnmapBuffer返回FALSE、fence创建失败均为失败，禁止沿用旧输出；各阶段错误先归档再清理，不能先清空错误后报NONE。补齐对象live count和销毁/重建context后的epoch失效验证，迟到结果不得提交。

这属于**探针工程问题→developer独立harness修订→tester定点补证**，不是算法设计失败，也不是要求tester整体归因。旧设备输出保留为正向观察，旧计时不升级为修正同步后的数据。

## 4. 成本与样本身份

`gpuFenceWaitMs`是CPU单调时钟测得的fence创建/flush/clientWait段，含驱动排队和线程调度，不是GPU timer。`dispatchMs`是CPU提交段。`totalMs`仅t0到t4，排除CPU输入/expected准备、context/compile、genBuffers、校验/第二次dispatch及清理；readback还含输出vector分配。没有测完整pair或完整物理attempt。

`hot`参数只写JSON标签；所有bench都会gen并重新glBufferData，两类均非持久buffer warm路径。一个进程共用context/program，不能把各N的cold都称独立冷启动。后续分列context/compile首次、buffer首次、持久资源warm，保留真实样本次序，不重命名旧数据。

当前`t131.json`六组total分别2.4744/0.2520/0.3542/0.2727/0.3534/0.2508ms；`t131_run5.json`也是一次六组快照，不是五轮原始数组。现归档只有上述两份、stats_summary、environment和compute_shader；摘要声称其余组5样本、4096hot10样本，尚不能从归档重建五轮及p95方法。初始192cold dispatch=1.355ms，摘要192c p95=.38ms，需说明是否另属预热/采样集合，不能静默删样本或据此断言数据造假。

摘要512hot CPU wait p50=.271/p95=.322ms可标“报告统计，样本待关联”，不能推出每子步≈1ms可承受、每merge不可行、成本与N无关。map的负载不能代替T130的多层索引、CPU F64预处理和逐merge重建。

## 5. 最小下游交接

1. developer（不使用真机）：独立测试target修订barrier/wait/map错误合同；hot改持久资源且冷/暖身份显式；限制字段命名/64位查询；原始逐轮JSON含run/build/driver/contextEpoch/输入hash/资源生命周期。无T137前只修通用harness，不开发pool算法。已有缺失轮次先归档，不强制重测已有可验证map能力。
2. tester（真机标签、设备串行）：按修订harness补同步/超时与context重建；归档每轮原始计时、失败/outlier、冷暖定义、分位数算法与聚合脚本。reduction/层间可见性随已证明原型执行，不把map重复当reduction。GPU timer缺失明确NOT_MEASURED；禁止用wait填GPU runtime。不额外编造百轮或固定收益百分比门。
3. T137证明到件后，developer仅按T130测试专用接口实现：CPU保留原F64排序/精确范围与唯一权威merge/预算/ID/ledger；GPU只产有request identity的整数索引建议；每次merge失效重建。与T123/原扫描逐次差分及完整attempt端到端p50/p95、最坏、错误率、UI/渲染竞争、恢复资源按T130执行，所有准备/上传/同步/读回/验证均计成本。实际采用另据这些结果裁定，不升级生产renderer context。

通用prompt约束：不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅自己状态有限例外；不改滴答同名清单。所有渲染含背景前景只走新XComponent+shader，禁止Canvas上屏或失败回退；故障明确FAULT或标明STALE的上一有效新管线帧。设备卡串行；无设备卡明确“不使用真机”。

本轮交付为能力边界/工程修订/原型门裁定完成；不是补证已执行，也不是compute收益或产品通过。
