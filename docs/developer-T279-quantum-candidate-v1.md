# T279 GPU M1 QUANTUM 候选核交付

2026-10-30，developer；原卡 `6ae34b05e4b0efba6933b57d`。消费 T237/T238 与 T240 source-closure-v4。不使用真机。

## 交付范围

新增 `gpu_resident/m1/quantum_{core,t16,motion,stages,event_masks}` shader 与 `QuantumCandidate` native 执行器。四 binding 分别为不可写的接受基线、候选、workspace、event64。实际执行入口库存/来源检查、合法角投影、两色 head2、冻结 eta/medium/bed/head、GPU bitonic ID 排序、水 AIR/沙 AIR-WET T16 运动、TURN、约束 WAIT、整包 CROSS、LAND、床三色有序预算、末沙接触、GPU `(column,y,id,slot)` 排序与单扫 ARRIVE。全部库存计算为 Q20 整数与宽乘除/RNE。

质量包含 W/J 与 S/P，容量包含四项；严格 Hq605394 与 300/s、28/s 预算原值不变。water CROSS 不扣固体预算；sand CROSS 扣 total 一次、local 两端各一次；LAND local 一端；BED 有序 total grant 的 before/after 逐行记录。head2 同色提案读取同一状态，prefix 分配事件，再并行 apply；计数使用原子操作，避免并行边竞争。碰墙/顶/床/侧阻挡消费全部余 tick，力只积分 `ceil(segmentTicks*lambda)`；不对等待余时重复加力。已 WAIT 每 quantum 重新评估，失去支撑保留 y 释放。整包到达将 J 精确转 W，来源、源成功序号与 I 不变。

event64 V2 掩码直接由 T238 manifest 生成；无关 word 为 0。每 slot 沙48/水16含收尾，frame arena 使用 `4*(48K+16Kw+254)+K`。预留失败不删事件、不继续无日志运行。诊断失败最终 GPU pass 将整个候选恢复为只读基线；之前成功 SOURCE bank/owner 不变，已复制 SOURCE 事件前缀保留。失败尝试行仍是诊断，不构成成功事件。

`SourceCoreTransport` 仅增加私有 candidate pin，持有期间拒绝 source submit/release；普通 source 路径不变。候选消费接受 bank 和 source ledger，通过 GPU copy 保留同 frame SOURCE 前缀；前缀长度使用已校验 completion word53，不能使用 native owner 未投影的 header38。新 frame 不复制旧 frame 事件。Poll 仅 fence 后映射64B诊断；完整 bank/events 仅显式 Freeze/Capture 冷导出。测试 fixture 上传接口只在 `GRS_T279_HOST_TEST` 编译，HAP 不含该入口。生产基线仅由 SOURCE transport 新空 run 建立，不支持 snapshot 导入；M2/M3/非零 bh 拒绝。

## T280 接口边界

`QuantumCandidate::Submit(SourceCoreTransport&, QuantumParameters, RootRequest)` 只产生候选；Poll OK 不等于 accepted。它不切换 SourceRootOwner、不递增其接受计数、不生成 publication、completion256 seal 或 CURRENT。候选 header 中物理/quantum/arrival 等计数为拟议结果；bank selector、publication generation 等接受/发布字段由 T280 后续统一封存，不得拿候选直接发布。SOURCE shader identity 保留为 lineage；候选 kernel SHA 单独列入交付证据，T280 需冻结组合 shader 身份。完整逐 word 全相 audit、event replay、提交/失效/late owner 生命周期仍属 T280；scene 与产品 route 属 T281。默认 route OFF，`Plan` 不宣 full M1 可用。

候选阶段编号为0 copy、1入口、2/3/4水提案/prefix/apply×2、5冻结、6/7沙索引/排序、8/9水索引/排序、10水运动、11沙运动、12/13/14床提案/prefix/apply×3、15末沙接触、16/17到达索引/排序、18 ARRIVE、19候选计数、20失败恢复。每 dispatch 一次 storage/update barrier。planeTag 为内部候选编码：BED `256+column`（高床为目标列），WALL `512+boundary`（顶为641），CROSS `768+boundary`，SURFACE `1024+column`，TURN `1536+column`，无平面终段 `1792+column`；source/field 对应槽按掩码清零。T280 replay 应显式消费此编码，不套 M0 私有编号。

g512、emit16、settle8、kAir/kFlight0、kWet24、kappa24 均是 T237 待定标候选参数，不宣称已验证速度律。soft 使用冻结角表 tan28 的557537。

## 验证与实际资源

PC ANGLE/SwiftShader 执行真实 GLES compute，不是 OHOS 真机。四档每档38例，合计 **152/0**。独立 Python 整数 oracle 不导入 designer model，逐段验证 AIR/WET 速度、RNE/ceil、WAIT 时间/法向速度、完整两色 head2 提案/写集、合法角投影；并验证库存/容量/guard/预算身份、unused words、跨列、wet/surface/TURN、墙/顶、LAND局部部分准入、同色多床边 prefix、多包到达 y/ID反序、事件上限和入口损坏恢复。每档另在 native 中实际执行 SOURCE→GPU ledger/事件前缀 copy→候选，断言 source pin、成功不切 owner、失败 bank逐 word回滚/前缀保留、新 frame 隔离。

| K | Kw | 候选实际 glBufferData 字节 | dispatch / quantum |
| --- | --- | --- | --- |
| 192 | 64 | 11,341,392 | 105 |
| 192 | 128 | 12,394,832 | 119 |
| 512 | 64 | 27,174,736 | 114 |
| 512 | 128 | 28,228,176 | 128 |

dispatch 为 `27+L(nextPow2(K))+2L(Kw)`；本表仅候选+workspace+单 frame event arena，排除独立 SOURCE transport、driver/JIT/RSS、T280双 arena/replay/audit/publication、T281渲染资源。不能对比完整 M1 模板后宣称内存收益或设备吞吐。

现有 SOURCE native transport 回归 **562/0**（K192/Kw128）；本轮 `default@PackageHap` **BUILD SUCCESSFUL**，零构建错误。逻辑 LF kernel SHA256 `54946590ca762cc8d9bb2e153e3fbab839b74bdf70fd474ad7b94b621f580f96`。unsigned HAP SHA256 `4d5e3bfe474e303e898567b8cba7a7ae14e9bdac94a91332a2ff733f81dd5a75`。未安装、未用机、未宣设备/独立 tester/scene PASS。

原始证据：`test/reports/t279/delivery.json`、四档 `gpu-*.txt`/`quantum-*.json`、`source-regression.txt`、`package.log.gz`。复跑顺序：`python test/gpu_resident/prepare_t279_quantum.py` → `test/gpu_resident/build_t279_host.cmd` → `python test/gpu_resident/check_t279_quantum.py`。`archive_t279.py` 归档输入/输出 SHA、实际资源与构建结果；`T279_VERIFY_ONLY=1` 仅重新校验已生成输出，不能当新 GPU 执行。
