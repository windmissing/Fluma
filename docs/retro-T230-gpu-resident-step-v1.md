# T230 GPU驻留整步方案复盘 v1

2026-10-28，designer；路线`gpu-resident-integer-step-v1`；不使用真机、未实现/运行产品或shader。主稿`algorithm-T230-gpu-resident-step-v1.md`，设计模型/证据另存。

## 候选与决定

1. **延续只读pair卸载：REJECTED作主线。** T212仍CPU物理权威、每find sort/SHA/upload/fence；T199未测得O2整事务收益，不能回应用户“状态尽量驻GPU/算法尽量迁移”。保留旧路径作对照，不回退在途修改。
2. **全F32移植：REJECTED作质量/guard authority。** 容量量级ULP超过原1e-6，非结合/FTZ/融合变化无保证；可以做明确视觉/运动实验，不能默认守恒等价。
3. **真实FP64直接移植：CONDITIONAL。** 本设备能力未测，ES3.2/整数shader成功不是FP64证据；即便有原生double也不证明JS Math函数/原图逐位等价。精确探针交T231，不挡整数主线。
4. **整数软件F64完整重现：DEFERRED备选。** 存bit型/基本算术可设计，transcendental/寄存器/多精度成本与旧引擎等价需要大量独立证据；不以“uint两word”就宣全F64等价。
5. **整数质量＋Q20运动、固定120Hz、GPU bank事务：SELECTED。** 内部质量转移exact0漂移，GPU只需32位整数，CPU不每粒往返；代价是网格量化/时间/guard/输入准入/床序/满池策略不同，全部新版本/新run，旧CPU保留。

## 推导中明确的取舍

- 不能把GPU在resident kernel里提前翻current指针后遇GL错误还保证rollback。选择GPU只封候选、尾fence后native校身份并切bank；host不做物理，只承担提交可见性。取消late候选不得复活。
- M0不等待所有旧channel迁完，但也不把单沙干场闭环称完整旧CPU/目标7/8。W/J/双相/merge等有明确实现位图和后续算法表，unsupported不悄悄CPU回退。
- 同列LAND和total预算有真实顺序依赖。首版GPU ID序resolver串行，床3色并行＋固定edge prefix，诚实计串行深度；不是atomic抢预算/任意线程先到先得。
- mass用Q20单列U32，但全域128列需U64；mass×position/velocity和DDA比例也需宽乘，不能“用了整数”就忽视wrap。
- 严格Hq为floor(tan30 Q20)再减1量子，覆盖tan表自身<1量子的误差；只用floorH会在部分反向边可能越真实角的原硬门。角度可行区间由GPU床归约，数值量化拒量和old F64 accepted差异必须报告。
- 动态merge保留逐次选对→真实merge→更新→birth的依赖；M2单workgroup+resident排序索引O(K)移动，每轮O(K)查找，不借旧CPU AVL当GPU索引，更不批选pair后乱序。
- 减少同步不等减少dispatch。M0最坏一frame268/306 compute dispatch与5次completion fence，可能在小池不划算，设备需同offered/accepted负载与UI竞争证据。
- 双frame事件arena有约13/35MB量级负担，不藏成零显存；正常不全量读回，需旧完整长期raw时启独立TRACE_CAPTURE信用/IO合同。context丢失不能从1024B摘要恢复pool。
- 消费T231已交正式平台/补正，改同XComponent GL owner/context为首选，共享context为探针后备选；本文256word=1024B completion/root与其建议256B/frame是明确接口/成本差异。source dedup改64word保存完整32word命令+32word终态，append-only有效prefix随bank选择提交，不漏算或先写后伪回滚。

## 验证范围与接续

本轮只有自持整数/Fraction设计核，核精确转移、guard区间、3色无冲突、prefix预算、时钟carry、排序pair引理/动态更新、模型root rollback/late、dispatch/资源式和输入指纹。不冒shader编译/GPU执行、产品性能、传感器/场景PASS。正式oracle由tester独立实现，不以本模型代产品正确性证据。

T231的平台工程与探针并行，不互设循环前置。后续architect落word/used-mask/offset/同步/lease与共享hunk，developer实现M0及真实构建/自测，独立headless/端到端/设备收益接力；失败先architect，工程改文档、设计问题再回designer。GPU最高优先，未改他人任务/业务/生产开关。
