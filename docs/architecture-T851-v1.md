# T851 · PC M1 冷角表之后的 SOURCE 描述符崩溃

2026-11-23 · architect · P5 · Windows x64 Vulkan

**首次归因：PC M1 SOURCE 宿主代码只分配 8 个 descriptor set，更新循环却覆盖两路共 16 个。** 冻结 PE 在完成冷角表提交和 fence 等待之后进入 SOURCE，向 `vkUpdateDescriptorSets` 传入空 `dstSet`。Intel Vulkan ICD 在应用输入的空句柄上触发读取异常 `0xc0000005`，进程以同一异常码退出。分流给 developer，修订 SOURCE 分配及发布前检查。 [原生异常栈](../test/reports/t851/pinned-debug-stack.txt) · [反汇编](../test/reports/t851/pinned-disassembly.txt)

## 原件与定位

| 证据 | 观测 |
| --- | --- |
| T847 三次交互启动 | 各 22 条原始记录。BOOT 的真实 fence 和 OP65/66 到达，SOURCE pipeline 返回成功，QUANTUM candidate/commit 建管线阶段到达。三次末条均为 `M1_QUANTUM_COLD_ANGLES / RUNNING`，未到 `M1_POINTER_READY`。历史三次均无原生异常栈。 |
| T847 BOOT 诊断对照 | 同 PE 不带 `--m1-window`，完成 BOOT 及窗口诊断，像素 `0xff00ff00`。该入口不执行 M1 SOURCE 描述符初始化。 |
| T851 冻结原件复现 | Release PE `4ab9ff59…`，三 SPIR-V 为 `1912af9e…`、`b8f0e327…`、`caf0614c…`，逐个对照 T830 收据。以 `--m1-window SOURCE_SPV CANDIDATE_SPV COMMIT_SPV` 启动，在日志到 `M1_SOURCE_MODULE` 时仅附加该诊断子进程。PID `28444`，异常线程 `21188` 与 JSONL owner 相同。日志再现同一 22 条截点。 |
| 原生异常及指令 | 首次异常为读地址 `0x55`，退出码 `0xc0000005`。`igvk64.dll+0x55b992` 从 `VkWriteDescriptorSet` 的 `dstSet` 槽取句柄，紧接的 `+0x55b996` 访问 `[rbx+0x55]`。上一应用栈帧的返回地址 `PE+0x17bab` 紧跟 `PE+0x17ba5` 的 `VulkanApi::UpdateDescriptorSets` 间接调用，后续指令正好检查 8 个 pass 和 2 个 bank。驱动版本 `31.0.101.2140`。 |
| 分配及句柄来源 | 同 PE `+0x177db` 将 `descriptorSetCount` 写成 8，`+0x17808` 调用 `vkAllocateDescriptorSets` 后检查返回值。冻结 `m1_source.inc:78–105` 的 `layouts` 和零初始化 `bindings` 各有 16 项。成功返回只填前 8 项。写入循环按 `base*passes+pass` 读取后 8 项时会取到空句柄。具体落入异常的 pass 未单独采样。 |

上表依据 PE 收据固定源码快照 `819088e7` 及角表接入 `12759bca`。初查 Windows Application 1000/1001 无匹配。首次从进程启动就附加调试的观察在 BOOT 建管线阶段触及 60 秒上限，由监督器收回，未取得异常。上表采用随后延迟附加取得的实际异常和进程退出状态。收据的文本 SHA 采用 Windows 原始 CRLF。Vulkan `VUID-VkWriteDescriptorSet-dstSet-00320` 要求 `dstSet` 为有效句柄。 [T830 冻结工件和命令](../test/reports/t830/m1-held-handoff.md) · [T847 原始取证](../test/reports/t847/) · [T851 原始 JSONL](../test/reports/t851/pinned-debug.jsonl) · [SHA 和捕获收据](../test/reports/t851/capture.json) · [Vulkan 描述符更新合同](https://docs.vulkan.org/refpages/latest/refpages/source/VkWriteDescriptorSet.html)

## 冷角表边界

`M1_QUANTUM_COLD_ANGLES` 在 `m1_quantum.inc:91` 发出，后面依次是 command pool reset、回收已完成的 BOOT fence、创建本次 fence、录制角表更新、transfer→compute barrier、submit 和等待。本快照在等待成功后仅缓存 `m1ColdAngleUploadBytes` 字段。`PrepareM1Quantum` 返回后，`m1_source.inc:72–105` 紧接着创建 SOURCE pool、分配和更新描述符。此后才有 `M1_POINTER_READY`。T851 的 SOURCE 异常栈确认角表上传调用已通过 submit/fence 检查并返回，修正 T847 按最后一条日志推定的崩溃段。 [QUANTUM 源码](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_quantum.inc#L91) · [SOURCE 源码](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc#L72)

冻结 PE 内角表仅匹配一次，文件偏移 `2516464`。`grs::kAngleWords` 为 61,444 个小端 `uint32_t`，共 245,776 B，SHA-256 `23b5210055d0b3f51892967170134a6cad14360579b038e26ee48f3ea5bbb368`。`kM1QWork` 占 552,272 B，具备 `STORAGE_BUFFER | TRANSFER_DST`，角表字节区间为 `[13632,259408)`。四次写入的 `(偏移,长度)` 分别为 `(13632,65536)`、`(79168,65536)`、`(144704,65536)`、`(210240,49168)`，满足四字节对齐、单次最多 65,536 B 和边界要求。静态数据由 `vkCmdUpdateBuffer` 录入命令，无角表的宿主映射。reset 函数按当前 device 解析并检查，沿本 owner 的 device、command pool、queue 及新 fence 完成提交和等待。本轮未采 GPU 角表回读。 [角表核对收据](../test/reports/t851/capture.json) · [vkCmdUpdateBuffer](https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdUpdateBuffer.html)

## Developer 修订合同

1. 从两路 bank 和每路 8 个 pass 得出同一个 set 数量。用它声明 `layouts`、`bindings`、pool `maxSets` 和 `descriptorSetCount`，分配和遍历均覆盖 16 个集合。每组为 4 个 storage 和 1 个 uniform，共需 64 个 storage、16 个 uniform。维持 bank 交换及 `base*passes+pass` 的已有索引。QUANTUM 的独立池已按 `count*2` 分配，维持原布局和路由。固定 SOURCE 与交互 SOURCE 都走这段宿主初始化。
2. 按 `VkResult` 检查分配，并在第一个 `vkUpdateDescriptorSets` 前核完整数组的成员均非空。异常处报告预期数量、实传数量、实得非空数量或第一个空项。更新及后续绑定使用当前 owner、device、pool 和几何代的有效句柄。失败直接沿现有故障/退役路径，当前初始化不发 `M1_POINTER_READY`。保留 resize、关闭与输入撤销的判定，已提交资源等待实际 fence 后回收，swapchain 仍沿独立的退役边界释放。上一有效帧按既有故障策略保留至窗口代退役。
3. 为角表等待成功、SOURCE 描述符分配返回、全部 SOURCE 写入完成各发一次可独立识别的检查点。沿用同一进程/owner 的原 JSONL，分配记录实际 `VkResult` 和数量，角表完成记录实际 fence 返回与字节数。`vkUpdateDescriptorSets` 的完成点在实际调用返回后发出，该 API 为 `void`。本次修订保持冻结三 SPIR-V、角表字节与 SHA、资源范围及 barrier，保持已有 SOURCE/QUANTUM 帧预算、真实 256 字 Completion、1,024 B 热路径单尾回读、REPLAY、代际撤销与原子发布合同。所有产品渲染继续使用现有 shader 管线，不允许 Canvas 回退。 [既有架构与实现交接](developer-T830-v1.md) · [窗口及调度合同](../test/reports/t830/m1-held-handoff.md)

## 复测与放行

- Developer 提供新 PE 的指纹、构建回执和同三份 SPIR-V 的指纹。就地核对 2×8 与每项非空。对少配 8、分配失败和 readiness 前代已撤销分别做负控，前两项必须在第一次描述符更新前退出，撤销路径必须阻止 readiness 并沿既有退役。验证固定 SOURCE 和交互入口均可通过初始化。若使用 Vulkan validation layer，记录实际启用状态。
- 独立 PC 复测先核实际角表 fence 成功 → SOURCE pool 分配和非空数均为 16 → 更新完成 → `M1_POINTER_READY`。再实发 Down/held pulse，核非空 SOURCE root、真实 Completion 及 owner 接受、同谱系首个 QUANTUM 与 REPLAY、shader 的同根 Present/像素。用实际 Up 验证后续 SOURCE grant 停止，已获授权的 QUANTUM 按原释放合同继续，再核真实退役。固定 SOURCE 同时保其原独立回归，提交新 PE 的实测原件。 [T847 业务终点范围](../test/reports/t847/v1-segfault.json) · [T840 固定 SOURCE](../test/reports/t840/v2-m1-rerun.json)
- 用 `--m1-cold` 保留该进程的 `M1PCOLD1` 原文件与 JSONL，交 T848 原 reader 复核八段及同根谱系。T851 取得崩溃归因，业务首帧、交互终点和正式冷档仍需上述复测。 [T848 冷档交接](../test/reports/t848/handoff.md)
