# T236 GPU 驻留 M0 主核接续回执

2026-10-29，developer；原卡 `6ae1ea8be4b0c3103c13e7d0`。本次为 **PARTIAL / CORE_SLICE_DELIVERED / APP_ROUTE_NOT_CONNECTED / CONTRACT_QUESTION_OPEN**。不使用真机。原 `c7ab688` 基础报告与顶层历史证据保留；本轮证据在 `test/reports/t236/resume/`。不得把本报告登记为完整 M0、硬件 GPU、设备呈现、收益或 GR01–GR09 产品 PASS。

## 落地内容

- 消费 T241 `17d424c` 的 T233 manifest，重生成 C++/GLSL 字段、产品 wire 与逐 kind mask。K192 改为两份完整 pad256，载荷 15,060,496B；K512 为 36,997,904B。这是设计 payload，实际 alignment/program/全帧 target/driver 峰值另计。旧 K192/wire 两项工程缺口已解决，不再沿用旧阻断。
- `m0_source.comp` + `m0_source_root.glsl` + `m0_quantum.glsl`：四 SSBO binding、64 invocation，GPU SOURCE 扫描/拆包/空槽/ID/generation/来源，Q20 固定120积分、有理 DDA、ID 序跨列/接触/落床、三色床与严格 Hq guard、T/L 同额扣账。source32/34、quantum59/68 dispatch；未改变冻结质量、容量、守恒、预算或角度硬门。重力9.8、阻尼1、包 .125、flux24 为新路线候选参数，不冒旧 F64 或已标定速度律。
- GPU entry/candidate/spare 审核含 raw pool→PH/总量、非负/容量、预算、guard、ID/来源、reserved、派生值。先从 base 重算数学并逐 word核事件，再**重新从 base 按事件写集核 old/apply/new**；最终 candidate/spare 全 word比较。后一层避免“同一隐藏写入重复执行”蒙混。slot 全落床保留 generation；PH 包含全部在途沙；W/J/非H恒0。
- `product_wire`、`ProductCompletionPolicy`：按产品 SUCCESS / METADATA_REJECT / DEDUP_REPLAY 验证 receipt、source/physics/I/prefix、logical masks。正量 replay 只投影旧 receipt，不再发 sourceSuccess；拒绝/冲突/迟到不伪造新物理成功。真实 MAX_ID 排序与 padding 分离，但**分配最后一个 ID 的 wire 未定，当前不开放该出生分支**。
- `SourceGpuTransport`：冷 GL 实际能力门、真实固定 buffer 分配与 shader 编译、逐 pass barrier、真实 tail fence/flush、owner 线程及 EGL display/context/draw surface 校验。Poll 用零超时，signaled 后只 map1024B；每帧最多五 root，normal尾包上界5120B。command及实际 uniform payload 全计上传；冷全 bank诊断需显式 freeze，另计字节。可信接受前绝不改 current selector。取消迟到不 map；不可信/fatal completion 冻结继续提交，保留失败 arena；context loss 为 STATE_LOST，不从摘要恢复旧 run。
- `ResidentRun`：唯一 native owner 调用的异步调度接口。FIFO16，最多一个 source＋四量子，Pump一次 submit或非阻塞poll；dt0连carry/队列/帧号/receipt都不写，F0仅控制carry。source/前序量子独立保留；失败剩余量子记 discarded，不累入下次补跑。暂停清手势/时间并退休 late；暴露小型 inputGeneration 供下一代命令使用。
- `ResidentScene`：accepted publication→GPU PBO列纹理/VBO粒子属性，无 vertex SSBO / CPU整池回传。前背景、床、粒子、前景全 shader，完整 token绑定run/owner/context/surface/geometry/config/source/physics/frame/nonce/bank/publication及三个SHA和scene SHA。完整 draw与调用方swap均成功才提交 LastGood；失败为 FAULT。render-release fence保护双publication槽。`displayPresentVerified=false`。
- CMake嵌入物理/场景shader并链接隔离库。`GetFoundationInfo`为 `implementedFeatures=0, stepShaderImplemented=true, connectedToXComponent=false`，保持生产默认及旧 pair/T211 CPU通道不变。不引入 Canvas 或 CPU物理回退。M1水/J、M2/M3仍显式不支持。

## 开发验证（最终原始结果以 evidence.json 为准）

- C++ Od/O2 各85项，0失败；显式 TEST mock与产品wire用例分开。
- Python int/Fraction 独立基础原语差分：每档82,205向量，0差异；这是基础算术，不是独立整步oracle。
- 自动生成 `--check`、T241 spec check；物理 ES310 与四个场景 ES300阶段离线语义编译通过。实际 Harmony native CMake使用的物理 source SHA 与离线展开 SHA一致。
- PC明确选择 ANGLE Vulkan **SwiftShader 软件实现**运行真实 GL产品shader/transport，而不是app fallback。K192/K512正常链各26项通过（最终重跑进程exit0），K512诊断显示G1+F4实际submitted=accepted=5、complete1/fault0。覆盖 source拆包/正量replay/冲突/不支持W/小于量子、stop late不map、四量子、LAND全/部分/generation、CROSS与三色BED、同token draw/swap、swap失败LastGood、FIFO16/dt0/F0/G1+F4。
- 单独 SHA 的测试shader故意在finalContact后做一个不记事件的 x+1：真实回放报 reason16、stage7、word192（67633153 vs 67633152）；native拒绝candidate、完整已接受state/source不变、冻结后续提交。4项通过。测试变体不进入产品CMake。
- 默认PC ANGLE D3D11只有ES3.0，能力门拒绝（CAPABILITY_UNSUPPORTED/rawGL1280），计 gate通过，sourceExecution NOT_RUN；不能记产品执行通过。
- 真实 `hvigorw default@PackageHap --no-daemon` 最终 BUILD SUCCESSFUL，native与ArkTS零错误。最终日志保留工具链 `--gcc-toolchain` unused参数警告，未声称零警告；未改他人代码。不使用真机。
- K512曾在测试程序对整tick设置的30秒软件等待期内未完成，触发断言；保留 `software-k512-harness-30s.txt`。后续加active/fault/提交数诊断并把**测试等待期限**延至120秒；未改变产品时间窗/物理门。最终结论只依据重跑结果，不抹去旧失败。

## 原卡待裁定与未完成项

1. **ID末值协议工程问题已评论原卡**（`6ae29016e4b0ab67702bde28`）：要求真实0xFFFFFFFF合法，又要求nextSandId单U32、不回绕。末ID分配后的nextIdAfter=2^32没有冻结表达。当前 `births > UINT_MAX-next` 明确拒绝，不能冒充完整合法域。需architect定义耗尽位/哨兵及后续拒绝wire后实现、测试；不私占reserved或饱和重用ID。
2. 真实 XComponent window 的 owner/NAPI/页面/路由尚未接通。本切片已有单context transport/scene/非阻塞run接口；不能将PC pbuffer当window。T242已明确工程接力，实际窗口及资源/offset256/颜色/resize/background/context loss设备证据另交相应developer/tester，不等设备来阻离线代码，也不把未实现接线写成设备等待。
3. 当前trace/performance导出、完整故障cut/背压矩阵、所有失败分支精确firstMismatch细节未齐；成功与已测反例字段有实证，其它不称全覆盖。GPU ID唯一性审核当前串行O(K²)，双重数学/写集回放亦有开销，未作收益声明。没有独立tester整步oracle对产品逐cut、没有设备或完整帧收益数据。
4. 不自行解锁/关闭T240，不将部分交付写成完整M0；manager据原卡真实回执推进。清单只把本人T236状态改“看评论”，不改其它行或滴答同名清单。

## 接口与复跑

`SourceGpuTransport::Create/Submit/SubmitQuantum/Poll/Acquire/ReleasePublication`、`ResidentRun::Offer/Advance/Pump/Pause`、`ResidentScene::Draw` 分别承担GL根、native控制、场景消费；均只在native GL owner上调用。外部窗口层负责创建current context、显式new-run身份及swap；不能在UI同步等待GPU完成。`Release`若render fence未退则BUSY，由owner loop再次poll，不busywait。

复跑命令见模块README。主核与故障变体均绑定独立完整SHA；原始日志、snapshot与摘要列在同目录evidence.json。构建日志原始gzip另存，便于无损追溯。
