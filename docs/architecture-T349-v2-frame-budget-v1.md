# T349 · V2 帧率坍塌整体归因与 T351 工程合同 v1

2026-11-05；architect；不使用真机。依据 `docs/test-report-T346-v2-no-settle-offline-repro-v1.md`、`docs/test-report-T347-v2-settle-device-v1.md`、T347 所列签包/日志及当前生产源码。本文只裁定 CPU V2 性能工程和取证边界；GPU 作为备选，不新增投入。T347 的沉降、LAND、床增长及同帧 CURRENT 为功能证据，不等于流畅度通过。

## 1. 已证事实与不能推出的结论

T347 在真实设备上记录空场约 12.2 fps，注入后 5.1→2.8→2.0→稳定 1.3–1.4 fps；358 帧累计模拟 11.93 s，粒子 pYmean 124.315→22.247、pYmin 1.155、床 mh 0→1.1547，LAND 成立。170/170 条所核场景回执为 CURRENT、请求帧与实际帧相同、无 code100/STALE/FAULT。T346 的离线同源链约 6.5 模拟秒后也着床。因此慢帧使 T343 的短观察窗不足；不能重开物理沉降或新管线接线为本卡根因。

T346/T347 **没有**提供物理、V2 边界、发布复制、ArkTS 打包、native 绘制/提交、调度的互斥阶段耗时。报告把 `verifyPublicBoundary` 和 `matchesAuthority` 指为候选，但未量到它们的份额，更不能宣称这两处是唯一原因。空场已有约 82 ms/帧的观测基线；即使注入路径变成零耗时，也不能从现有证据推断达到 30/60 fps。T347 页面每 120 帧才做一次 `exportPayloadCold()`/`T347STEP`，它可能污染该采样帧，但不能单独解释连续稳定 1.3 fps。held 注入时的 `T218DBG` 是额外开销，源停后不再持续。没有证明 native 绘制或 GC 慢，也不能排除它们。

## 2. 源码调用图与累计界

页面 `P051DerivedV2Demo.tick` 以墙钟帧间隔计算 `dt=min(1/30,Δt)`，顺序为输入→`P051DerivedDemoV2.tick`（held 时可注入一次，再 step）→`copyPublicationInto`→`drawNewPipeline`→下一帧回调。`WaterSandField.stepWithTiltTrace` 在 `dt>1/60` 时选择两个子步，否则一个；休眠快路可能零子步。每个成功子步 `recordEndpointCommit` 调用一次 `DerivedCommitV2.prepareInactive('STEP')`；每次被接受的外源注入另有一次 `prepareInactive('EXTERNAL_ADMISSION')`。因此设成功子步数 Q∈{0,1,2}、成功外源发布数 A，则正常帧发布次数 P=Q+A，另有一份消费态复制和一次新管线绘制；异常/重试需按实际次数另计。T347 慢帧的 `dt` 上限令活跃 step 通常 Q=2，但原日志没有逐帧子步计数，不能写成测量事实。

每次 `prepareInactive` 顺序为 `fill`（`WaterSandField.copyStateTo`：10 组 128 列字段及池快照，并写元数据）→`verifyPublicBoundary(kind+':'+commit)`→`matchesAuthority`→封签→准备态，`commitPrepared` 才切换 active。每个新的 STEP/外源边界名称不同；`verifyPublicBoundary` 的 serial+boundary 快路不能跨这些边界生效。它检查 13 池缓冲绑定、扫描 free/live N、对 live ID 四趟 radix、`audit.build` 清空 256×70=17,920 个 exact70 limb 并扫 N 个槽、逐 live exact70 编码/累加、对 256 桶 round、逐 limb 比对 canonical、逐桶 count/cache 位型比对，最后严格检查 C=128 列。设容量 N=512、live=L，单次至少执行一套 `O(N+4L+256×70+C)` 的边界对拍，外加 live 的 exact70 编码/累加成本；这是源码工作界，**不是耗时测量**。逐行 hot 审计、preview/collect、事务跟踪另在 Q 个子步内重复，T311 已证逐请求 audit 的 Ω(R(N+C)) 下界，不能把帧耗全归边界函数。

`matchesAuthority` 在成功路径比较 13 个池 buffer、8 个 field buffer，逐 buffer 新建两只 `Uint8Array` 包装，共 42 个视图对象；先核池 header。按源码字段长度，比较约 `55N+16C+8×C×8` 字节，N=512/C=128 时约 38,400 字节/发布。`copyStateTo` 的源→inactive bank 复制、`copyPublishedInto` 的 active bank→页面 slot 复制、`P013ScenePreparation.pack` 的 ArkTS→native payload 准备是不同工作，不能合算成一次。`copyDerivedPayloadV2` 返回的 bytes 计数公式为 `13×128×8+55N+16×128`（N=512 时 43,520），而实际 `.set` 的 128 列 Float64 数组为 12 组（shakeSource 两组条件性），计数公式与实际条件路径不严格一致；T351 应按真实复制字节核准计数，不能以该返回值充当实测带宽。

`P013SceneController.drawNewPipelineFrame` 每帧验证封签身份，执行 surface 查询/prepare、`P013ScenePreparation.pack`，再调 native `drawNewPipelineFrame`，并有可能做配置绑定及日志；成功态是 XComponent+shader。仅封签匹配为身份检查，不证明 render/pack 低耗。若配置每帧变化或 native 阻塞，需阶段收据才能定位。没有 Canvas 失败回退空间。

整体可见工作按一帧表示为 `T = T_input + ΣQ(T_preview+T_hot(R,N,L)+T_field+T_pubCopy+T_verify+T_match) + ΣA(T_admission+T_pubCopy+T_verify+T_match) + T_consumerCopy + T_pack + T_native + T_callback/GC`；此式是分段计账，不允许将嵌套 inclusive timer 相加。Q、A、R、N、L、失败/重试次数、各段 wall 耗时均要同一 frame token 对齐。`T_hot` 至少含 T311 保留的逐请求 full audit；先前 T308 的旧路径复制次数不能直接挪作当前 T347 的实测。

## 3. T351 实施合同：先量整帧，再做有证等价化

### 3.1 固定、互斥的帧收据

在页面/模型层以同一个 `runId/frameSerial/physicsCommitId` 串联以下顶层互斥区间：输入/注入、step、consumer copy、scene preparation/pack、native draw/提交、回调尾。step 内细分每子步 preview/collect/hot/field/发布，其中发布细分 fill copy、结构+exact70 对拍、authority byte compare、seal；记录 Q、A、R、N、live、每类调用/复制真实字节、阶段累计与最大耗时、fault/rollback/STALE。T311 已有的 20 类 hot stage 收据可复用，但计时为 nested inclusive，必须与互斥顶层和子步序号对齐，不能把 inclusive 和当整帧。时钟只作诊断，不改变 physics dt；若仅 `Date.now()` 可用，应记录精度/零值，不能宣称亚毫秒分辨率。固定大小环、标量计数、≤1Hz/按需冷导出；禁止每帧同步 JSON、全池导出或大量 hilog。插桩本身对照关闭版应有有界开销，给出丢样/溢出标志。记录帧回调间隔和物理 dt 两个不同数，帧率不可用模拟秒反推。

先用无源空场、保持源输入、源停后 0/低/中 live 三档比较，每档同一设备/签包/渲染参数和固定短窗；开发本角色只做非机重复/阶段计数和签包编译，真机由 T352 执行。比较 p50/p95 帧间隔与各段分布；低帧率候选段须占对应退化量，才能对其做性能归因。若 native/pack 或空场基线占主导，给出精确 tester 阶段补证，不能把边界优化强称完整修复。

### 3.2 安全的工程改造顺序

1. **收紧发布复制并降低确定的包装开销。** `DerivedCommitV2.sameBuffer` 不应在每次比较新建 42 个视图；在冷态为受控 13+8 buffer 绑定视图，热入口仍检查当前 buffer 对象、byteLength、别名/容量、detach，不符走原 fault/不发布。核实实际复制字节并修正 diagnostic counter 的定义/计算。对 `copyStateTo`→`matchesAuthority` 的双遍池/字段读取，只有在同步不可重入、源/目的 buffer 私有且中间无可达写入的证据齐备后，才允许合并为一次逐字节 copy+verify，保留每字节精确比较、原错误阶段及提交前最后检查；若这些前提不能证明，保留完整 `matchesAuthority`，仅做预分配/顺序读优化。不可凭 contentSerial、checksum 或抽样跳过 byte compare。
2. **重组边界扫描而不削弱证明。** `verifyStructure` 与 `audit.build` 当前分别全池扫描；可以用一个遍历计划收集 free/live、核全部非法值并构造 exact70 audit scratch，但必须在原 structure 错误优先级完成后才发布 audit 结果，保留 4-pass ID 去重、每公共边界 exact70 全量重建、256×70 limb/位型对拍及 `verifyStrict`。若合并改变故障码、first-fault 冻结字节、canonical 事务状态，则不合并。对每子步的其它 full-pool copy/audit，按 T311 的 R/N/attempt 收据找可去重的同一快照；不得跨外部 writer、guard callback、rollback 或新 public boundary 复用旧证明。
3. **按真实占比处理 pack/native。** 如果互斥收据显示它们是主要预算，T351 应在原新管线中减少相同 frame 的重复打包/上传或配置工作，保持 `run/epoch/commit/frameToken`、CURRENT/STALE 故障语义、像素层完整性与资产版本。无法在现有职责内改 native 时，报告精确调用、时长和接口前置，交 manager 派对应工程卡；不能仅优化 V2 audit 后宣布帧率修复。

不可改物理 dt 上限、终速、落床判据、外源速率、endpoint 协议或渲染为 Canvas。无法证明合并校验所需不变量时，不在 T351 偷换 `verifyCertificate`/`contentSerial` 为充分证明；由 architect 将“私有缓冲所有权、写入闭包、可重入性、错误序、rollback、exact70 逐边界证明”提交 designer 设计证书。当前无需先等 designer 才开展上述工程测量和无证书安全优化。

### 3.3 预算与交付判据

以 30 fps 为本轮工程预算参考线 33.3 ms/展示帧，60 fps 参考线 16.7 ms；二者是**目标预算，不是当前承诺或 T347 测得耗时**。T351 应给空场、注入中、源停后各段 p50/p95、每帧 Q/A/R/live/N 与可加总的互斥阶段占比，说明预计可省的毫秒与剩余下界。若 33.3 ms 不可达，清楚报主导阶段、证据和所需后续工程/设计，不用减少物理子步或丢 CURRENT 伪达标。T352 真机按同签包、稳定观察窗验证帧间隔与模拟秒进展、真实触摸、LAND/床量、同 frame token CURRENT 和无新 fault；不能把 T347 原慢包的功能 PASS 当性能 PASS。

开发本角色检查：ArkTS/原生两工程可编译签包；非机覆盖 N=0/512、live=0/低/上沿，Q=0/1/2、A=0/1、重复/失败输入、替换/detach/alias、外部 writer、rollback、publication seal/stale；新旧逐位对照 exact70 acc/count/cache、source/commit/elapsed/particle payload、第一错误码及冻结证据；预算计数在不同 R/N 下线性闭合，采样关闭/开启不改变结果。测试不需要真机作为 developer 关闭门。若 T351 任何前提失败，保留原全量验证，报告局部结果和具体阻断，不得删校验换性能。

## 4. 分流结论

首次归因为**有确定重复工作与缺失整帧分段证据的工程问题**。`verifyPublicBoundary` 全量 exact70 对拍、`matchesAuthority` 逐字节比较及每发布复制确实每个活跃提交重复；T311 逐行 full audit 和新管线 render/pack 也在同一帧预算中。现有证据不足以排序耗时或宣布唯一根因。T351 可依本合同立即实施测量与安全工程改造；T352 消费新签包做真机性能/功能续证。若严格边界与 33.3 ms 冲突且合并证明无法在工程层完成，回 architect 明确不变量后交 designer，而不是降低 exact70、守恒、事务或非法输入检测。T347 功能结论保留，产品流畅性仍未通过。
