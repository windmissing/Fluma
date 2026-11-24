# T911 · DM2 / R1 SOURCE 超期归因与工程修订 v1

2026-11-24 · architect · Windows Vulkan 集显 · 离线。**DM2 的约 241 ms GPU 路径与约 41 ms 录制叠加越过 250 ms，设备优先缩审计段，同时压缩首根录制。R1 的 GPU 根约 413 ms，主要预算在实际 writer 和独立 replay，需同时缩两边。** 四臂均在业务截止被拒，随后同 fence 退役成功。保原业务门、1,024 B 热尾和 typed gate。 [独立复算原字](architecture-T911-evidence-v1.json) · [T904 原报告](../test/reports/t904/report.md) · [T902 原合同](architecture-T902-v1.md)

## 1. 四臂复算

T914 后续的 split-v2 启动故障、时钟封装及续根证据见 [T919 工程修订](architecture-T919-v1.md)。T919 补充发布包校验和首根到续根的复测入口。

证据钉住 T904 `e04697a8` 和 T903 B `cafcfe9d`。实际为 `8086:3e98` Intel UHD 630、驱动 `1656924 / 31.0.101.2140`、queue family 0，各族独立 PE 和 role set。复核本机 PE SHA 和相应产物、512 B 的同一 `t894-model-source.u32le`（`3580ccb1…`），里面的 synthetic 业务体为质量 0.2、x=64、y=200、初速零。各进程的实际 frozen `inputSHA` 仍不同，身份和 permit 随 run 封装。T904 有无查询各一例，能说明边界与量级，无法单独量出查询开销或稳定分布。非查询两臂无 query pool、无点位，`queryCreateVkResult=1` 是未创建分支的初值。各原件最后一行是实际进程错误文本 `<family> ROOT_NOT_ACCEPTED`。 [T903 构建与复现命令](developer-T903-v1.md)

ROOT、ROOT_TIMELINE、RETIRE 和 ROOT_GPU_TIMESTAMPS 按 `family/run/nonce=1/fence/generation=2` 联接，再核 device/queue/plan。四份 BOOT 的原 VkResult 为 0，DM2 seal 6559303、R1 seal 3223126018。随后 SOURCE 分别录制 12/19 个派发、submit=0。真实 business polls 均为 `VK_TIMEOUT=2`，stop 记录的 pulse 当下仍 fresh，firstStop 为 `GRANT_DEADLINE`。业务 readback/gate/promote 没有开始，`TAIL_NOT_READY` 是未到门，ROOT 的零 outcome/reason 为默认字。独立 RETIRE 在同 fence 返回 0，所有迟到查询均 `acceptedRoot=null`。

| 族/臂 | BOOT 宿主资格 ms | grant→submit 退出 ms（录制 ms） | 退出时剩余预算 ms | submit 退出→stop ms（超出 deadline） | poll 次数/中位实耗 ms | 独立 retire wait ms | GPU 根 ms |
|---|---:|---:|---:|---:|---:|---:|---:|
| DM2 关查询 | 140.053 | 37.753（37.675） | 212.247 | 213.755（+1.508） | 14 / 15.725 | 195.049 | — |
| DM2 开查询 | 92.091 | 41.514（41.436） | 208.486 | 212.258（+3.772） | 14 / 15.974 | 28.527 | 240.994 |
| R1 关查询 | 194.078 | 30.061（29.990） | 219.940 | 232.014（+12.075） | 15 / 15.863 | 168.238 | — |
| R1 开查询 | 194.896 | 34.769（34.668） | 215.231 | 217.961（+2.731） | 14 / 15.654 | 195.231 | 413.412 |

冻结输入到 Expected 完成只有 0.0258–0.0728 ms，宿主前段主要在 `recordEnter→recordExit`。请求的 2 ms wait 实测中位 15.65–15.97 ms。单次等候粒度能吃掉临界根余量，不能解释 R1 的 GPU 路径。实际驱动/调度原因尚待同臂复测。退役耗时是拒绝后另一段 host 等候，可能与 GPU 根重叠。以查询点的相邻差值和 `((end−begin)&(2³⁶−1))×83.333 ns` 重算 GPU 区间。起点在 upload 前 TOP_OF_PIPE，后续用 BOTTOM_OF_PIPE。覆盖本命令的工作与同步，无法按现有点位剥出纯 shader 或单个 barrier 时间。两套时钟分别算时长，禁止把原 tick 当绝对 QPC 对齐。 [原时标与录制边沿](../pc/vulkan/sand/owner.cpp)

| 族 | UPLOAD ms | PREPARE ms | 写入与核验 ms | 尾传输 ms | 相对实际剩余预算的缺口 |
|---|---:|---:|---|---:|---:|
| DM2 | 0.013083 | 39.249843 | 5–7 SOURCE_WRITER **70.993299**，8–11 AUDIT_COMPARE_PACK_SEAL **130.730810** | 0.007250 | 32.508 ms |
| R1 | 0.008667 | 32.648536 | 6–10 ACTUAL_WRITER **182.605686**，11–13 REPLAY_AUDIT_APPLY **166.590250**，14–18 COMPARE_PACK_AUDIT_SEAL 31.551540 | 0.007500 | 198.182 ms |

DM2 尾审计组占 54.25%，仅缩录制可用的理论余量也只有约 9.006 ms，须给排队、fence 与最终接受留实际预算。R1 的两组主体合计 349.196 ms、占 84.47%。若余段及宿主前段用时保持本臂，两个主体合计须降到约 151.014 ms 才平账，尚未留最终接受余量。仅消除全部宿主录制或其中一个主体，仍过不了按本臂计的预算。两族复制 1,024 B 的实际尾区间均约 7–8 µs，所以优先保现合同。诊断读回始终与业务判定分离。 [原 BOOT、ROOT 与 retest](../test/reports/t904/)

## 2. DM2 · 先改反复全量扫的审计与封装

实际表：0–4 初始化、输入、去重/量化/信用。5–7 私有出生、来源证书、账。8 使用同一真实 BASE 重做 SOURCE 及审计，9 对比 CANDIDATE/REPLAY，10 完整根审计以封场景资格，11 封 Completion256。B 路径是 SOURCE-only，不进入 DM 扩散动员、中点、2404-slot QUANTUM 或 FULL discovery。前 5 槽中包含设备 BASE copy、候选复制及清 arena，现标记无法拆。 [冻结派发表](architecture-T554-g9-dm-dispatch-v2.json) · [实际 shader](../entry/src/main/cpp/gpu_resident/g9dm/pipeline.comp)

- 量级记 N=128、K=192、每列床/流层来源上限 L=64、全局来源容量 O=4096、活包 A、实际 EVENT/PATH 行 E/P。一次 `g9AuditMassGeometry` 下限含 (N·K=24,576) 次逐列槽扫描、两套 (N·L) 来源图、逐槽证书/保留字、全 O 来源的 16 字记录审计（空来源也验零），最坏另有 (A·K) 的 ID 重复扫描。正常无早错路径在原槽 1、6、8、10 重复这类审核，且动量/事件游标各有独立边界。逐字比较单 lane 扫 147,968 字（591,872 B state），槽 1 的清 arena 写 **12,767,232 B**，读到的事件可少于容量。算术、审计和复制由相同设备 workgroup/队列执行。现有组时标给优先级，未证明哪个循环独占 130.731 ms。
- **第一增量**保原 12 个逻辑槽，先量 1、6、8、9、10、11，细化槽 8 内 BASE copy 与 replay/audit，槽 10 内审计和 pack 入口/出口可用临时分段模块，另计其派发和 barrier。同步核 CmdResetDescriptorPool/CommandPool、上传录制、descriptor alloc/update、dispatch 录制、EndCommandBuffer。只允许把不依赖 nonce/arena/动态输入且对象代隔离的模板、descriptor、预热移到 grant 前；原实际 command/Expected/输入冻结及时效仍在同一业务窗口。用同模块 cold/warm 和前置录制模板开关证明收益。隔离实际 fence 等待粒度时记录原实参、已耗时及 pulse 采样，任何替换不能改原 deadline/fresh/current 检查。
- **第二增量从尾审计做等价并行。** 先把全 state 比较按字块划分，所有块覆盖含 reservation/保留零，确定归约选原先最早不符字。再用稳定按列/来源分桶、实际活包 ID 索引和整数 checked 归约把列扫池及重复查找换为一次扫描和定序归集。空槽、空来源及未用图的零仍验，来源账以原 `(origin, sourceId)` 对齐。来源的有符号动量/能量与 RNE 保原算序或证明重排下无中间溢出、同符号及同首错。ACTUAL / REPLAY 从真实同 BASE 分别计算，禁止把候选 audit memo 复制成 replay 证书。跨 workgroup 用实际派发间依赖，原 error 仲裁按逻辑槽和原检查顺序兑现。稳定 bin 的扫描目标为 (O(K+N·L+O+E+P)) 加确定排序和 (O(stateWords)) 全比较。再按测量替换槽 1/6 同类全量工作。切 arena 清零方案仅在独立原字等价能覆盖未用范围、arena generation、写前覆盖、拒绝及输出摘要时开放。可先试等覆盖的全范围原型。

## 3. R1 · 复用父审计优化，独立降低两份视觉计划成本

实际表 0–5 含父准备和 `VIS_ROOT_COPY`。6–10 依次为 ACTUAL `VIS_PLAN_BEGIN / SOURCE_PRIVATE_BIRTH / VIS_PLAN_APPLY / SOURCE_ORIGIN_CERTIFICATE / SOURCE_LEDGER`。11–13 是 REPLAY `VIS_PLAN_BEGIN / SOURCE_AUDIT_REPLAY / VIS_PLAN_APPLY`，旧 map 来自真实 BASE。14–18 才父 compare/pack、VIS_ROOT_AUDIT、GLYPH、seal。两段 writer 划分包括父算子、屏障和各自视觉工作；当前臂没有 WORK witness 或 WSI 测量。 [R1 冻结表](architecture-T563-g11-r1-dispatch-v1.json) · [视觉角色](../entry/src/main/cpp/gpu_resident/g11vis/visual_stage.glsl) · [来源计划](../entry/src/main/cpp/gpu_resident/g11vis/material_plan.glsl)

- 父物理同样是 N128/K192、state 591,872 B、父 arena 11,718,656 B，另有视觉 V2048。视觉逻辑保三份 marker（每 bank 262,144 B）、四张 map（合计 524,288 B）、2,097,152 B plan。ROOT_COPY 和两个 PLAN_BEGIN 各清全 plan，构成 **6,291,456 B 固定写量**。两个 BEGIN/APPLY 合计写四张 old/new map 两遍，即 **1,048,576 B**，ROOT_COPY 另写两份 marker。两个 APPLY 各在 lane 0 枚举 65,536 plan 槽，包括空槽余字校验，另有 (O(N·L)) map 扫描、有效 key 的查找及来源传输检查，最坏随 (O(N·L·(L+K)+N·K)) 增长。marker apply 走固定 V，出生列 refill 还扫描 V，实际次数随已封存 SOURCE 出生/暴露列和占用变化。后段独立双 bank 逐 bit 比较、ghost 排序和 glyph/Pstyle 同样有固定工作。应分别实报固定容量、active/visited 及每根候选/拒绝，生产 SOURCE 的量级不能按 marker 全满假定。
- **第一增量**在 2、6/7/8/9/10、11/12/13 与 16/17/18 补边沿。8/13 按 old/new map、BuildMapRefs、resolve/空槽审计、transfer audit、marker apply、refill 分臂细测。用同一 R1 父输入/seed 可运行的父物理诊断臂测父操作自身成本，组合路线仍须完整视觉审核。作为阳性对照，父源审核失败应该阻断组合，视觉旧 map 来自错代要报实错。区分首次资源建管线、录制与 GPU 处理，按实际对象和范围冻结证据。
- **第二增量同改 ACTUAL 与 REPLAY 的稀疏工作，数据独立。** 先平行分块做全覆盖清零、map 复制、plan/map 合法性和 marker 比较，锁定旧序号和首错，再用当前根 GPU 实际 SOURCE 更新集合驱动索引。只读的 old/new 四图保持独立来源，按 `(column,origin,sourceId,map,rank)` 稳定连接已变或需验的范围。SOURCE 的无输运退化须由原事件、已合法的 BASE 与新状态的实际写集证明，既有地图的物理内容和未用区仍可独立验证。保持 sparse descriptor 的每个未触及槽语义为已验证零，witness、hash、compare 和有效前缀读取到的原字一致。在需要完整 witness 时在 GPU 封存后实际物化同一 ABI 的零域和四图。任何重用缓存/戳的方案必须防同一 generation 重入、拒绝、取消和旧代读。两份计划/Marker 各从各自真实角色独立生成，出生列按原事件/slot 与 ID 分配顺序 refill，比较实际全 32 marker words，glyph 和 Pstyle 能由同一 accepted lease 消费。
- **第三增量依据热点缩减余段。** 复用 DM2 的父 SOURCE 等价分块方案，针对 R1 的实际字典和组合 Expected 另行证明。视觉 root copy 可试只读 CURRENT 和有界私有写块，需保三个 bank 的原语义及读范围，必要时物化整块。保 ghost 代表、排序 tie、密度受限及物理到视觉的映射字段。目标成本为固定范围的设备初始化或分块核验，加实际 map/plan 活行与 marker 范围的确定连接、原序归约及 glyph。两份真实视觉阶段的结果继续供 typed gate 消费。 [组合资源/工作与 witness 合同](architecture-T563-g11-accepted-flux-visual-v1.md) · [联合发布](architecture-T839-v1.md)

## 4. Developer / tester 可证伪交接

1. 每族分别保 T903 原 PE、生成表、参数/seed、编译角色、设备和实际输入 body，采用同 GPU 下交错运行的基线/单项候选，开/关时标再对照。收集首次/预热后至少各三次和一个连续热窗，报告原排序和分布。固定 semantic seed 和 body，分别保 root scope、冻结 inputSHA、permit 和对象代；独立比较按现有规范化规则重绑不同 run 原字段。阶段查询拆成多臂限量使用，首次分段主要定位最大稳态区间；经同臂证实时标不会改变失败相位再比较。计实际 workgroup/派发、访问字数、事件/plan 种类与实际有效数量，另保传输/同步/录制、queue 校准（设备支持才用）、业务和退役原 QPC。截止、废弃与窗口/安全退役用原同一 fence。
2. 双族最小正控为同一研究 seed 的新非零 SOURCE，辅以 duplicate、q0、满槽/源表、generation/来源坏值、稀疏和近容量，以及已退役但晚 fence。物理核实际 admission/拒绝、状态全部原字与首错、逐来源 S/F/P、EVENT/PATH 前缀、能量/动量账。R1 另核 map/plan 的各角色和 glyph/Pstyle，存于独立诊断/可读关的档位，保 source 为零物理时间。等价基线若集显无及时接受，可用独显原工件的真实接受根或经过独立核验的阶段原字做参考。晚退役的有限诊断拷贝只用于裁剪差分，禁止发布或顶替业务 reference PASS。接受只在原 `250 ms + freshness/current + typed gate` 全部通过时记。计划可用 `40 ms host 前段 + 195 ms GPU + 15 ms 等待观测/reader/gate/提交` 作为协作预算，实际均以真实时间和原门裁定。
3. 对应 `DM2 0xffff` 与 `R1 0xcffff` SOURCE 资格保全，DM 的动员/预测 bit 不给 SOURCE 冒证，R1 bit18/19 必须来自真正的物料计划审核和两角色字级对照/图元。计划多一派发时独立版本化角色表和模块 SHA、按可信工厂从真实 production pipeline/role 建 Expected，严禁伪造旧计数。保持现有 4 compute SSBO 及配置 UBO 策略，checked 范围、同一原业务 submit/fence/1,024 B 读回与已付作用域；临时 staging 和诊断另记，shader 单编译输入上限仍 49,152 B。四臂配对的 T905 checker 和 12 项自测负控已复跑通过，四份检查回执均 `accepted=false`。实际 accepted SOURCE 到达后才接 DM2 的 QUANTUM/ROOT/FULL，及 R1 的实际 Q 非零通量/WORK witness/联合 shader 帧。释放退役资源后再递交新输入，迟到原字只作诊断。开发集显给功能和诊断分段，持续性能依用户目标独显现场实测。 [typed gate 合同](architecture-T647-v1.md) · [T905 验证入口](../test/t905/check.py)

本轮已定位到分族阶段预算，并交付可实施的等价裁剪与同输入对照。尚无按槽细分后的 GPU 占比和优化实测结果。使用 GPU 驻留和 shader 呈现，禁止 Canvas 回退。
