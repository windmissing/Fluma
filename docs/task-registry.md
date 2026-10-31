# 后续工作清单 · 固定编号台账

> 建立人：manager。2026-10-25 09:10。
> 规则：**每个 task 一个固定编号，编号终身不变**（卡关闭后编号退役，不复用给别的卡）。
> 每轮清单**按优先级排序**（P5 → P4 → P3），因此**编号在清单里的出现顺序可以乱序**——编号只用于跨轮次指代同一件事，不是排名。

## 编号 → task 对照

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T01 | D1-R3 M1 生产热路径热接口实现（零分配收口）·S1 已过 | developer | P5 | 6adcc2c1…1cf05 |
| T02 | D2 M2 场集成（九接入点 + 账本 v7 + 13 项单测 = 隔离原型出口） | developer | P5 | 6adc52e4…2a86 |
| T04 | 终点指标 D-4/O-2 修复（D-4 已落，O-2 未落） | developer | P5 | 6adcd497…0569 |
| T05 | 掉沙接线修复入库（六项已落地并通过自测） | developer | P4 | 6adca854…cdcc |
| T06 | peak 协议（独立 protocol + 适配清单 + RED 基线） | tester | P4 | 6adcd3ba…f8c8 |
| T07 | peak 字段 revision（shake-on/off + 子步消费身份） | architect | P5 | 6adcd3d3…51f1 |
| T08 | peak 实现（采样 + 流式峰值 + 双算对照） | developer | P5 | 6adcd3e6…d26 |
| T09 | P1 CPU fixture + 五量 RED 基线（G1 解冻前置之二）·零交付待开工 | tester | P5 | 6adcc00b…0dda |
| T10 | 掉沙 V1-V4 真机返修复测收尾（真机队列第 1 位） | effector | P5 | 6adcd2dd…cb99 |
| T11 | 【T11·P6设备取证】effector：endpoint合规run与同账本peak双报 | effector | P4 | 6addadc4e4b06ee35727b319 |
| T12 | S4 sweep 三轮（先冻结 μ_y 量纲定义） | developer | 0 | 6adcd331…a94b2 |
| T13 | 方案3 b 起沙链 B-1/B-2/B-3（已落地待入库） | developer | P4 | 6adc6272…f9dd |
| T14 | 大角度 B：±45° 三测量对象形式化 + 300/s 可达性账本 | designer | 0 | 6adb6d3d…649b |
| T15 | 大角度 C：新 route 客观用例预注册 | tester | 0 | 6adb6d3d…88ef9 |
| T16 | 大角度 D：真实观感用例预注册 | effector | 0 | 6adb6d3d…538e |
| T17 | 大角度 E：新 route 工程化复审 | architect | 0 | 6adb6d3d…7d49 |
| T18 | 大角度 F：默认关闭新 route 实现 | developer | 0 | 6adb6d3d…649c |
| T19 | 大角度 G：只读审核与提交 | reviewer | 0 | 6adb6d3d…88efc |
| T20 | 【T20·D4/D5粒子视觉接线】developer：真实粒子快照渲染、落床可见性与B tracer退出 | developer | P5 | 6adda102e4b017ab33e90fbf |
| T21 | E2 出数 / 参数面板 / S3-S4 真机 | effector | — | 待建 |
| T22 | 方案3 悬浮平流启动裁决 | manager | P3 | 6ad5f713…9cb2 |
| T23 | 【T23·P1 G1放行裁决】designer：盘点G0必要证据、裁决可实施范围与门序 | designer | P4 | 6adda53be4b06ee357273211 |
| T24 | 【T24·P1材质离线准备】developer：G2/G3资产与shader接线准备，禁止提前产品启用 | developer | P4 | 6addb2fee4b06ee357280248 |
| T25 | 掉沙链主观验收闭环 | effector | — | 待建 |
| T26 | goals.md 四子项变更落地 + 连带口径登记 | designer | 0 | 6ad9af6a…055b |
| T27 | P-012 C 补采（层厚增减 mask 裁剪 + DPI 缩放呼吸检查） | effector | 0 | 6ad982ef…f0be |
| T28 | S4 物理定义（pEff(Pa)/rhoEff/参考压力 + 量纲闭合表） | designer | 0 | 6add5bda…8d7b |
| T29 | P1 G0 真机 probe（surface/sRGB/三层z序/透明/输入透传） | effector | P5 | 6add6012…830b |
| T30 | T01后续客观端到端验证（原T01-V-O） | tester | P5 | 6add769ce4b017ab33e71bd7 |
| T31 | T01后续主观端到端验证（原T01-V-E） | effector | P5 | 6add769ce4b06ee35724ea6b |
| T32 | G0帧桥拒绝与绑定时序修复 | developer | P5 | 6add77dfe4b03273ad6591a2 |
| T33 | G0叠层/alpha/色彩工程定案 | architect | P5 | 6add77dfe4b06ee35724fd94 |
| T34 | G0叠层/alpha/色彩实现 | developer | P5 | 6add77dfe4b012783274cf8e |
| T35 | G0补证复测 | effector | P5 | 6add77dfe4b06ee35724fd95 |
| T53 | 【P-013·G2测试先行】tester：G2上传器客观用例登记与冻结 | tester | P5 | 6addcb9d…9981 |
| T54 | 【P-013·G2上传与接口】developer：五接口+双代原子切换+限额pump | developer | P5 | 6addcbb3…87ae |
| T55 | 【P-013·G3完整材质pass】developer：material_pass+shader+H/L/M沙区 | developer | P5 | 6addcbc0…d431 |
| T56 | 【P-013·G4页面共存】developer：FrameToken共享+同代原子+整块回退 | developer | P4 | 6addcbd2…d6b1 |
| T57 | 【P-013·G5真机双测】tester+effector：P1 OFF/ON+消融+黄金场景 | tester/effector | P4 | 6addcbe1…bde4 |

## 已关闭退役编号（不复用）

2026-10-30/31：T319/T320/T322/T321/T323执行交付、T324合同交付、T317按T324完成partial-submit实现（84bf66e，五套非机证据，设备NOT_RUN）。T318解锁。退役：T319/T320/T322/T317（T321/T323/T325/T326/T327状态以滴答为准）。

2026-10-31：T318真机交付（fdfd9e3：分片状态机真机成立——逐批≤8ms全过最大1.712ms、单dispatch最大691µs；但130op表÷10op/批=13轮Pump×8ms tick≈104-127ms结构性超33ms CLOCK_GAP窗3-4倍，root在BATCH_ACTIVE被GAP取消封尾不可达，完整链NOT_RUN）→ 新T328 architect配比裁定+修订合同+手签口径/UINT32_MAX两项移交。退役T318。

2026-10-31：T328裁定交付（docs/architecture-T328-batch-gap-ratio-ruling-v1.md + T324 Amendment 1：唯一推荐方向(a) Pump内多批循环kPumpBudgetNs=8ms护栏——13批压缩到单轮Pump≈3.5ms结构性消除超窗，GAP判定不豁免不放宽，INV-5/6/7增补；移交裁定：手签包tester补签合法追认+SHA归档义务归签包者、UINT32_MAX首批补显式End(16)）→ 派T329 developer实施（run.cpp循环+UINT32_MAX修复+submitPumpWallNs+非机回归）+T330 tester真机验证（J1-J5判据）。退役T328。

2026-10-31：T323/T325/T326/T327四裁定交付验收（T323：包SHA差异=delivery清单引用错误非链断裂、复杂度走工程路线增量证书、冷采原子memcpy+序列化延后；T325：Date.now保留PROOF_GAP改判接受成本（manager裁决）、工具三项交tester；T326：observationRows空=注入时序必然非缺陷、撤销T319补序列化建议不派developer；T327：cw_diag.h:14两行互换实锤、最小修复合同+V2+四判据）→ 派T331 developer工程三件实施（增量审计证书+sort消除+预算披露+附注两件）+T332 tester工具三项+T333 developer最小修复+T334 tester真机复核（等T333，真机队列T330后）。退役T323/T325/T326/T327。

2026-10-31：T329/T330接力交付验收（T329=e66f1c2按Amendment 1四件齐+非机全绿26/26×2·12/12·11/11·6/6·23/23设备NOT_RUN如实；T330=693f85a真机混合判定：J2/J4/J5 PASS封尾首达FenceEnter=2/首批code=16×39/GAP窗语义完好46/49ms原值触发，J1/J3 FAIL同根因=末批glFenceSync真机停顿37.65/41.28ms（12批未flush命令缓冲后建fence，循环本体仅2.2ms），击穿8ms预算与33ms窗→FENCE_PENDING被GAP取消code=2、accepted=0双源链NOT_RUN；六项已验证条款无回归、Amendment 1循环结构生效不需回退；NATIVE_DROPPED×2登记）→ 新T335 architect停顿机理归因+修复方向唯一推荐（分批flush/fence独立Pump/Amendment 2）+FENCE_PENDING×GAP核清+环扩容裁定（6ae55382e4b031ea82f3f856）。退役T329/T330。

2026-10-31：T335裁定交付验收（docs/architecture-T335-fence-stall-attribution-v1.md + T324 Amendment 2：机理(a)高置信采纳——驱动在sync创建时实现累计未flush命令缓冲（fence在flush前、127个未flush dispatch、296/325µs/dispatch线性指纹），不判驱动bug；唯一推荐(i)非末批尾glFlush——预期停顿37-41ms→~3ms、单Pump≈6.7ms；(ii)(iii)拒绝；INV-8 code=2取消语义文档化+INV-9 flush不改遥测；§2.5含fence口径预算/窗均不动；环1024→4096）→ 派T336 developer实施（flush一行+环常数+五套回归+flush计数新用例，6ae55673e4b0f69bc5818507）+T337 tester真机K1-K5（fence停顿≤5ms/Pump≤8ms/accepted≥1+双源链闭环/六项无回归/环导出COMPLETE，6ae55680e4b0efba6950e058，等T336）。退役T335。

2026-10-31：批量验收五交付+上轮T330/333补记（T333=0147095四判据非机全绿；T334=3435c28真机K1-K4全PASS——同run native src=4==TERMINAL#4三层闭环W=0.066773754、phy=266==header[5]、token无镜像、V1适配互证、V2生效；工装环256→4096变更如实登记、16384崩溃fail-closed在案；T332=2a5f955三项工具收口：39位点HOT=6/COLD_FAILURE_ONLY=32/COLD_EXPORT=1、净18对账36行闭合4+23-9=18、checkLong独立分母、静态FAIL有据维持；T331=3435c28混合落盘+归属注记26184ab：增量证书K=16+回退语义、sort消除R=320计数322→1、attemptCumulativeMs披露T311_HOT_WORK_V2、包SHA勘误、冷采分段化，非机全绿attempt68ms@R320；T336=8e2662a四件齐：非末批glFlush+环4096+flush计数CASE_F 12/14两配置断言过+INV-8注释，五套全绿32/32×2·12/12·11/11·6/6·23/23，无签包）→ T337真机验证仍待执行（无证据未关）；派T338 tester设备复核T331（M1死亡族存活/M2单事件≤600ms/M3冷采captureMs≤4ms/M4回退计数，6ae55f0ae4b069bbd884a5d2，等T337）。退役T331/T332/T333/T334/T336。

2026-10-31：T337真机BLOCKED（a10d4eb：T336环扩容4096但ArkTS校验器GpuBridgeTimelineCold.ets:24硬编码1024未同步→每页INVALID_PAGE导出0行，K1-K5全部不可判；fail-closed正确、native环4096健康（count=121 dropped=0）、包三件套1b5656e5在档；manager裁定=T336实施遗漏，非机替身不经过ArkTS导出路径是盲区）→ 派T339 developer一行修复（校验器1024→4096+pageBudget核实+单测四例防漂移+cold回归，6ae5622ee4b0f69bc58244ae）；T337保持BLOCKED不关，T339交付后沿原K1-K5换包重测；T338（等T337）顺延。

2026-10-31：T339修复交付（e8d70d1：校验器4096+页数上限256→1024+导出预算1MB→4MB两项环耦合预算同步+反漂移单测4/4+页预算16384核实够用）→T337重测（217d73d）混合判定关闭：K5 PASS（三run COMPLETE dropped=0，T339生效）；K2/K3 FAIL+逐批≤8ms回归——**Amendment 2真机证伪**：停顿精确转移到首批尾glFlush（33.660ms），10 dispatch vs T330的130 dispatch同为33-41ms固定税、与队列深度无关，T335摊销模型被证伪；无run到达末批fence（FenceEnter=0）；nonce/carry/取消/字典/submitting≠inflight五项无回归。**T338真机关闭（f6a1537）：M1/M2/M4 PASS**——R=148 attemptCumulativeMs=150ms（13×余量）、持续注入4分钟跨3run pid不变THREAD_BLOCK=0、maxMs≤5ms全stage（对照1449ms基线3个数量级改善）、auditFallbacks=0；M3 NOT_RUN如实（captureMs属t284生成树需独立harness）；工装T311_STAGE通道恢复×2如实登记（全历史未接线）。→ 派T341 architect固定税机理二次归因+修复方向二次裁定（预热flush/回退/GAP重议/驱动规避，6ae5749fe4b069bbd885cb60）+T340 tester冷采captureMs设备补测（t284独立harness，6ae5749fe4b031ea82f8647f，等T341包选择）。退役T337/T338/T339。

2026-10-31：manager方向变更裁定（用户指令）：**目标3提升为CPU仿真方案优先、GPU方案降为备选、渲染保持XComponent**——核实T202归因（微负派生残差跨模块合同不一致）+T207 exact70设计+T214工程化+T216实施（6份strict pack已过）+T217独立（8PASS/0FAIL）均在案，T218真机BLOCKED（床沙不可见）且复杂度障碍已由T331/T338解除→重测条件具备。派T342 developer code100修复（exact70 V2主链接入核验+strict pack复跑+code100反例用例+V1→V2生产切换准备，6ae5877fe4b0f69bc58437e8，P5）+T343 tester V2双态沙真机复测（D1-D6判据复用T218 DD12+床可见性链，6ae5877fe4b0efba695389b3，P5，等T342，真机队列T340后）。GPU线T279/T280/T281已交付部分保留现状不新增投入，待CPU线收口后manager重估。

2026-10-31：T340真机交付（43a12fa，P1 FAIL如实/P2 PASS/P3对照完成）——captureMs三锚点18.45/20.63/34.23ms超4ms预算4.6-8.6倍（rawBytes=0零payload仍耗18-34ms，T331分段化未达memcpy级），P2 fail-closed逐位正确，P3对照-26%但未达预算；T338 M3由NOT_RUN转FAIL闭环。**T342同步交付（8de02c9）**：exact70 V2主链N192/512×300步逐帧严格域断言（code100失败形态在V2下不复现）+code100反例（T180原快照解码为完整复现输入→V1形态仍被拒、V2链无法产生该状态）+T216套件7/7+6份strict pack复跑6/6+V1→V2切换就绪（S1-S4，切换决策DEFERRED_TO_T343）→T343真机解锁待包。→ 派T344 developer冷采归因修复（零payload超时→窗口覆盖调用方业务嫌疑，窗口收窄/移出/预算重估三方向唯一推荐，6ae5915de4b031ea82fbd6cc，P4）。退役T340。

2026-10-31：T341裁定交付验收（docs/architecture-T341-flush-tax-reattribution-v1.md + T324 Amendment 3：T335摊销模型确认证伪——数量指纹是伪相关（同一常数除以不同分母）；机理二次归因(a)变体=GPU空闲后首次真实提交的DVFS频率爬升+驱动前端恢复（F1-F4判别链：同pid每run复现杀惰性初始化/零工作glFinish不触税/count无关/连续工作免税），不判驱动bug；唯一推荐(i)每run预热真实flush——Create尾3行+INV-10 warmupWallNs+stage 42字典42版+Amendment 2布局保留（机理表述修正）+降级预案登记；W1-W5真机判据；T340包裁定T338b即刻执行）→ **GPU备选线实施卡+真机卡待manager排程**（方向变更后不新增投入，Amendment 3合同文本维持有效）。退役T341。

2026-10-31：T343真机交付（6d906df，T343-signed.hap c15d1a4c）混合判定关闭——D1 PASS（V2页58+行全CURRENT code100=0，T342修复真机生效；EmptySandDemo的code100属V1链固有非D1失败）+D6 PASS（THREAD_BLOCK=0）；D2/D3 FAIL→**新断链发现**：V2粒子准入成功（28笔adm>0）、渲染可见但位置跨run静止、床量恒零、LAND无事件——重力→沉降→LAND→床写入链在V2断开，code100排除为根因（renderer零拒绝仍不沉降），与T218时代床全0同位但旧链未触及；D4不适用/D5部分。→ 派T345 architect V2粒子不沉降首次归因（重力消费核验/悬浮机理判别/V1V2分歧点定位，6ae5a6b6e4b0efba6955622c）。T344交付（6d906df同批，docs/developer-T344-capture-window-narrowing-v1.md）通过关闭：归因=窗口起表点过宽（18-34ms全为调用方渲染管线+大JSON构造，enqueue前预算已耗尽致rawBytes=0）→唯一推荐窗口收窄（惰性起表）→最小修复三处+窗口合同W1-W4 4/4+selftest 15/15+reassembly 4/4；T323预期矛盾如实登记并语义对位修正；复测归后续真机卡换包（t284生成树链自动携带）。退役T343/T344。

2026-10-31：T345归因交付（docs/architecture-T345-v2-no-settle-attribution-v1.md：CHAIN_TRACED_INTACT——七环节逐行核验静态链完整无单点断链；机理分列不冒唯一（(a)设备侧步进旁路/(c"")collect执行但池写入被V2包装层静默回退，静态不可分）；**决定性离线1:1复现实验设计**（Node移植五项转储判据，pY下降→设备层/pY恒220→复现三候选干净二分）；V1/V2分歧定位=物理内核零分歧（HOT collect重力数学:786-794逐字未动）、分歧面=V2包装层、T207/T214设计无冻结条款；唯一路由developer诊断先行）→ 派T346 developer离线复现诊断（1:1 harness五项转储+二选一结论+复现时最小修复，6ae5af24e4b0e9cb68f4bd3a）。T321交付（docs/developer-T321-source-clock-receipts-v1.md）通过关闭：GAP/UP/CANCEL/BACKGROUND/倒钟全显式终态+用户可见停源反馈+512+64×32固定有界源时钟收据环（checksum冷导出+丢失显式化）+观察不影响权威（ON/OFF逐位一致）；非机14/0+14/0+26/0+7/0四套件全绿设备NOT_RUN如实。退役T345/T321。

2026-10-31：**目标3验收盲区发现（用户亲手操作戳穿）**：EmptySandDemo（XComponent 生产页）触摸只调 model.start/move 记录诊断坐标——「触摸→生成沙粒」链路（stageGestureSandAndFeed）从未接线，只在旧 SandCanvas 页与测试夹具存在；T218/T343/T347 验证链全走 T218DBG 调试注入绕过此缺口；历史上唯一摸过触摸链的 T82 是 SandCanvas 旧页且结论 NOT PASS（落床未证+code5）。**目标3「物理主链已通」状态栏表述误导，改回未完成**（触摸入口缺失+空场缺失+落床连续性未真机证实+code100/多色配色）。→ 派T348 developer触摸倒沙接线移植（stage链从SandCanvas移植+空场场景选项+全链断言回归，6ae5e40fe4b031ea8306dd6b，P5）。目标1核心达成维持（注入路径验证与触摸无关），触摸入口缺失同步登记。

2026-10-31：T346诊断交付（a3e9602，docs/test-report-T346-v2-no-settle-offline-repro-v1.md）通过关闭——**OFFLINE_NOT_REPRODUCED**：Node 1:1 harness（直接 require 真实 ArkTS 源码 transpile，零重写偏差）证明 V2 全链物理正常（pY 215→0.577 单调下降、~6.5s 着陆、mh 床增长 0→0.577、motionCount 30 活跃、mediumMode 恒 DRY_AIR_V1、barrier 无 fault）；**设备层根因定向=帧率坍塌致模拟时间不足**（T343 双进程 hilog 重析：空场 12fps→注入后 0.5fps，观测窗模拟时间 ≤3.27s < 220/vTerm=3.44s 落差时延——粒子物理上尚未落完，s1≈s4「位置不变」=慢速下落被低采样冻结的视觉假象）；性能缺陷移交（V2 包装层 barrier verifyPublicBoundary 256×70 对拍+matchesAuthority 全量比较 O(大常数)/步）供 architect 派性能卡；非机回归全绿（harness 固化 2/2+T216 7/7+regression 10/10+strict pack）。→ 派T347 tester真机验证（E1观测窗延长至模拟时间≥7s hilog实时重建/E2 pY下降/E3床量非零LAND/E4双态同帧可见+fps坍塌曲线采集，6ae5b850e4b0e9cb68f537d6）。退役T346。

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T342 | 【T342·code100材质故障修复】developer：exact70 V2主链接入核验+strict pack复跑+code100反例用例+V1→V2生产切换准备（CPU线最高优先） | developer | P5 | 6ae5877fe4b0f69bc58437e8 |
| T348 | 【T348·触摸倒沙接线移植】developer：EmptySandDemo补stageGestureSandAndFeed链+空场场景选项+全链断言回归（目标3核心缺口） | developer | P5 | 6ae5e40fe4b031ea8306dd6b |
| T347 | 【T347·V2粒子沉降真机验证】tester：E1观测窗延长至模拟时间≥7s/E2 pY下降/E3床量非零LAND/E4双态同帧可见+fps坍塌曲线采集（T218 DD12复用） | tester | P5 | 6ae5b850e4b0e9cb68f537d6 |

2026-10-26：T68/T69/T67/T73完成，T66解锁；新增T75 | 【T75·G2合同适配补齐】developer：绑定配置基准、同代身份幂等与非法budget收据 | developer | P5 | 6adeb548e4b0e6ce82bea365，补齐G2合同而不重开T54。

2026-10-26：T59/T65/T70完成，新增：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T72 | 【T72·DEMO被动诊断接线】developer：触摸到准入、逐tick输入与同帧呈现证据 | developer | P5 | 6adeb1e9e4b01278329aa00f |
| T73 | 【T73·干场独立客观验证】tester：DRY_AIR_V1零水、触点出生与倾角事务对照 | tester | P5 | 6adeb1e9e4b017ab33f703df |
| T74 | 【T74·触摸零源设备补采】effector：干场候选事件链与新管线呈现逐级取证 | effector | P5 | 6adeb1e9e4b017ab33f703e0 |

2026-10-26：T64/T63/T54按通知完成；新增：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T68 | 【T68·纯新管线合同修订】architect：移除Canvas回退及背景前景依赖 | architect | P5 | 6adeac6ee4b017ab33f636c4 |
| T69 | 【T69·触摸零源归因】designer：T63事件到准入链分流与最小修复方案 | designer | P5 | 6adeac6ee4b03273ad764d35 |
| T70 | 【T70·触摸零源客观补证】tester：事件接收、源请求及准入拒因逐级证据 | tester | P5 | 6adeac6ee4b03273ad764d36 |
| T71 | 【T71·触摸零源最小修复】developer：按归因接通事件到真实粒子准入 | developer | P5 | 6adeac6ee4b03273ad764d37 |

T56原卡标题改为【P-013·G4纯新管线集成】developer：背景沙区前景同FrameToken、同代原子生效与显式故障态；用户禁止Canvas回退。

2026-10-26新增：T67 | 【T67·上传器合同裁定】architect：配置修订基准、身份冲突与非法budget返回码 | architect | P5 | 6adea928e4b06ee357347c76；T54部分交付待合同，未关闭。

2026-10-26：T44/T62/T58按通知完成。T44失败/阻塞后续新增：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T64 | 【T64·T44失败分流】designer：无沙画面、缺CLOSED及重复sequence根因与修复路由 | designer | P5 | 6adea6ece4b06ee35734245f |
| T65 | 【T65·T44失败客观补证】tester：渲染身份、收尾计数与原始账本唯一键 | tester | P5 | 6adea6ece4b017ab33f5712c |
| T66 | 【T66·T44装配收尾修复】developer：按归因修复实际无沙路径与完整落盘身份 | developer | P5 | 6adea6ece4b0127832968f6f |

2026-10-26：T61测试补证完成bb2b534，25/25PASS为反例证据而非产品PASS；T62修复解锁，T63仍等修复交付。

2026-10-26：T60归因完成；新增T63 | 【T63·DEMO拒绘修复后复测】effector：有效帧、粒子落床累积及手持重力演示 | effector | P5 | 6ade9e08e4b0e6ce82bb1a90，等T62修复与T61回归后真机复测。

2026-10-26测试报告闭环新增：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T60 | 【T60·DEMO拒绘归因】designer：NONUNIFORM_SCALE与INVALID_PARTICLE分流及最小修复方案 | designer | P5 | 6ade982ce4b017ab33f3c6b0 |
| T61 | 【T61·DEMO拒绘客观补证】tester：逐帧尺寸比例与同提交粒子拒因复现 | tester | P5 | 6ade982ce4b03273ad73dd71 |
| T62 | 【T62·DEMO拒绘最小修复】developer：按归因修复尺寸契约与粒子校验输入 | developer | P5 | 6ade982ce4b06ee357327073 |

T52执行完成但产品FAIL，归因/修复另卡；T53登记完成，预期RED转T54实现。

2026-10-25：T49/T48/T51/T50按用户通知完成。后续新增：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T58 | 【T58·干空容器准入方案】designer：无水外源粒子事务与空气触点出生接口 | designer | P5 | 6ade1b05e4b06ee3572f27d2 |
| T59 | 【T59·干空容器DEMO补齐】developer：无水启动与空气触点真实粒子注入 | developer | P5 | 6ade1b05e4b03273ad708147 |

2026-10-25用户空容器DEMO新增：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T49 | 【T49·新管线双场渲染】developer：同帧显示高度场沉积沙与真实粒子沙 | developer | P5 | 6addc96fe4b0e6ce82b176ec |
| T50 | 【T50·摇晃重力交互定义】designer：空容器DEMO的传感器到重力方向映射 | designer | P5 | 6addc96fe4b03273ad6a83c9 |
| T51 | 【T51·空容器触摸摇晃DEMO】developer：空场启动、触摸发沙、重力方向交互与新管线入口 | developer | P5 | 6addc96fe4b03273ad6a83ca |
| T52 | 【T52·空容器DEMO效果验证】effector：触摸粒子落沙累积与摇晃重力方向真机演示 | effector | P5 | 6addc96fe4b0e6ce82b176ed |

2026-10-25：T42完成（回执G0五项PASS）、T46归因完成（补证交T47）。新增：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T47 | 【T47·规划阻断补证】tester：step73最小复现、blockedReason与回滚逐值证据 | tester | P5 | 6addc614e4b017ab33eb33fe |
| T48 | 【T48·真实材质资产导出】developer：复用Canvas生成字节，接T24六图离线资产契约 | developer | P4 | 6addc614e4b0e6ce82b14735 |

2026-10-25全局盘点新增：T46 | 【T46·满池动态拒绝归因】designer：区分预期拒绝、算法缺陷与实现缺陷并给出修复路由 | designer | P5 | 6addc406e4b017ab33eb18dc。T25主观闭环范围并入T44，避免重复派工；T06已退役不列阻塞。

2026-10-25新增接力（T11/T24/T41/T20已完成）：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T43 | 【T43·设备采集链装配修复】developer：采样身份、worker打包及停摇命令接通 | developer | P5 | 6addc1e3e4b01278327e57bb |
| T44 | 【T44·双场产品与合规run复测】effector：真实粒子落床累积、停摇endpoint及peak双报 | effector | P5 | 6addc1e3e4b06ee357290762 |
| T45 | 【T45·G0返修结果与材质推进裁决】designer：接T42结果拆分G2/G3实施门序 | designer | P4 | 6addc1e3e4b0e6ce82b10a78 |

2026-10-25继续派工：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T41 | 【T41·G0帧长度返修】developer：定位TDATA_LENGTH两侧实值并最小修复 | developer | P5 | 6addb2fee4b01278327c0e67 |
| T42 | 【T42·有效帧与G0复测】effector：返修后draw/swap、透明、合成与色彩补证 | effector | P5 | 6addb2fee4b0e6ce82b0093d |

T23/T31/T35按用户通知完成，T40按滴答已完成同步；任务完成不自动等于指标通过。

2026-10-25新增：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T39 | 【T39·O-2计数实现】developer：成功准入独立累计与epoch一致性故障检出 | developer | P5 | 6adda513e4b06ee35727301a |
| T40 | 【T40·O-2口径补充与独立验证】tester：合法相等、独立计数来源和故障反例 | tester | P5 | 6adda513e4b0e6ce82af399e |

T04 architect口径分析完成，原卡关闭；O-2实现转T39，补充与独立验证转T40。T37方案完成交T20。

2026-10-25全局重排新增固定编号：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T36 | 【T36·触摸注沙速率修复】developer：统一时间单位、首帧与停止边界，消除43倍注入放大 | developer | P5 | 6adda18ae4b03273ad679625 |
| T37 | 【T37·双场到GLES接口适配】architect：同帧高度场和真实粒子shader数据通路 | architect | P5 | 6adda18ae4b0e6ce82af0d63 |
| T38 | 【T38·peak独立客观验证】tester：同账本双算、采样边界与反例回归 | tester | P5 | 6adda18ae4b03273ad679626 |

T02、T08、T13按滴答完成状态登记完成，不增加manager验收；端到端/后续验证另卡推进。

| 编号 | task | 关闭时间 |
|---|---|---|
| T30 | 集成客观端到端验证（ddf8787） | 2026-10-25 |
| T157 | GPU信封与shader正式修复（b21d0e5） | 2026-10-27 |
| T151 | FIELD_COLUMN原值真机补采（失败链定位：预览COLLECT期间col48，report P051-T151） | 2026-10-27 |
| T159 | GPU完整事务测试包接线（c8dff53，离线9/9+生成项目构建全绿） | 2026-10-27 |
| T161 | GPU修复后完整事务真机复测（566da9d，信封消费10/10 F-T142-1真机闭环；三路收益BLOCKED→T163/T164） | 2026-10-27 |
| T158 | GPU真机采集器修订（cf1d416，6/6） | 2026-10-27 |
| T160 | 支持纯核独立验证（620ad6e，15/15） | 2026-10-27 |
| T42 | G0有效帧与复测（ef0e7a7，G0五项PASS） | 2026-10-25 |
| T43 | 设备采集链装配修复（13c7690） | 2026-10-25 |
| T45 | G0返修结果与材质推进裁决（decision-P013-T45-material-rollout-v1） | 2026-10-25 |
| T38 | peak独立验证（ddf8787） | 2026-10-25 |
| T39 | O-2计数实现（2f864f0） | 2026-10-25 |
| T34 | G0合成开发；按滴答已完成登记 | 2026-10-25 |
| T36 | 触摸速率修复；按滴答已完成登记 | 2026-10-25 |
| T37 | 双场到GLES接口架构（6089f98，交原T20/T34接力） | 2026-10-25 |
| T32 | 帧桥修复开发；按用户通知完成登记 | 2026-10-25 |
| T07 | peak字段revision三项对齐（61e5ac4） | 2026-10-25 |
| T33 | G0叠层/alpha/色彩工程定案（e287c47） | 2026-10-25 |
| T01 | M1开发、编译、离线自测（端到端转T30/T31） | 2026-10-25 |
| T09 | CPU fixture与五量RED基线 | 2026-10-25 |
| T10 | 掉沙返修复测取证交付 | 2026-10-25 |
| T29 | G0探针取证交付（G0未全过，后续T32–T35） | 2026-10-25 |
| R01 | 终点指标双报告 | 10-24 22:50 |
| R02 | E4 沙丘占比量测方案 | 10-24 22:50 |
| R03 | 终点指标缺陷修复 D-1/D-3/D-4/D-5 | 10-24 23:30 |
| R04 | 终点指标协议勘误 errata1 | 10-24 23:29 |
| R05 | 终点指标字段对齐（D-2/D-6 裁定） | 10-24 23:32 |
| R06 | 终点指标双报告补充1 | 10-24 23:58 |
| R07 | D1-R3 迁移设计（architect） | 10-24 23:41 |
| R08 | t90 epoch 修复 | 10-24 23:41 |
| R09 | S4 代理升级建议（architect） | 10-24 23:41 |
| R10 | E1 C 层理锚点 v2（designer） | 10-24 23:18 |
| R11 | 大角度 A：新 route 登记（manager 自办） | 10-24 23:48 |
| R12 | peak 协议（独立 protocol + 适配清单 + RED 基线 + §15 用例条目） | 10-25 09:30 |
| R13 | 掉沙接线修复入库（六项 + 单测） | 10-25 09:44 |
| R14 | 终点指标 D-2/D-6 实施（诊断三具名 + 事件级校验归 checker） | 10-25 09:46 |

## 未编号的一次性工作

不占 T 编号（无独立跟踪价值，或完成后即消失）：
- 终点指标双报告勘误与补充（已并入 R04/R06）
- 大角度 A 卡正文改写与 sortOrder 修正（manager 自办）

## 使用规则

1. 每轮输出清单时，**沿用本表的编号**，不重新编号。
2. 清单**按优先级排序**（P5 在前），所以编号顺序与优先级顺序**不一致是正常的**。
3. 新卡入表时**取下一个未用的最大编号 +1**，退役编号不复用。
4. 卡被关闭 → 在「已关闭退役编号」表补一行，编号退役。
5. 用户按编号指代时（如「T05 验收」），直接查本表定位卡。


2026-10-26：T136/T137/T114/T123/T121完成关闭；GPU最高优先原型接力，T128解锁。新增：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T138 | 【T138·GPU配对原型工程】architect：T137算法、SSBO布局与CPU权威事务 | architect | P5 | 6adf6a09e4b017ab3407f962 |
| T139 | 【T139·compute隔离harness修订】developer：barrier、wait原码与持久资源计时 | developer | P5 | 6adf6a09e4b03273ad89454d |
| T140 | 【T140·GPU配对独立oracle先行】tester：稳定merge、分段查询与逐pair基线 | tester | P5 | 6adf6a09e4b06ee35747d303 |
| T141 | 【T141·GPU确定配对原型实现】developer：整数分段索引与完整merge请求 | developer | P5 | 6adf6a09e4b017ab3407f963 |
| T142 | 【T142·GPU原型真机差分与收益】tester：完整事务、同步恢复与CPU索引对比 | tester | P5 | 6adf6a09e4b017ab3407f964 |
| T143 | 【T143·整数条带独立交付复核】tester：零字节差、双租约与资源预算 | tester | P5 | 6adf6a09e4b03273ad894550 |
| T144 | 【T144·色彩roundtrip探针修复】developer：真实纹理采样与完整fixture摘要 | developer | P5 | 6adf6a09e4b017ab3407f965 |
| T145 | 【T145·色彩roundtrip定点复测】tester：实际采样链与6144B摘要 | tester | P5 | 6adf6a09e4b017ab3407f966 |


2026-10-26：T131/T126/T118/T132/T134完成关闭，T119/T133解锁、T121可继续。新增固定编号：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T136 | 【T136·compute真机到件裁定】architect：能力边界、同步成本与GPU原型门 | architect | P5 | 6adf6602e4b0e6ce82d0734c |
| T137 | 【T137·GPU确定配对算法证明】designer：整数分段索引、最小pair与串行事务 | designer | P5 | 6adf6602e4b03273ad88f511 |


2026-10-26：T130/T117/T124/T125/T127完成关闭，T118解锁。新增固定编号：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T132 | 【T132·向外算术测试先行】tester：定向舍入、affine白名单与局部证书 | tester | P5 | 6adf48d1e4b0127832c37ab9 |
| T133 | 【T133·向外算术与局部证书实现】developer：定容原语、affine与NoWrite框架 | developer | P5 | 6adf48d1e4b017ab34058d26 |
| T134 | 【T134·严格纯核测试先行】tester：settle、top分支与CONTACT_WAIT LAND | tester | P5 | 6adf48d1e4b017ab34058d27 |
| T135 | 【T135·严格态支持纯核实现】developer：settle、top与等待态LAND影子事务 | developer | P5 | 6adf48d1e4b03273ad86acdb |


2026-10-26：compute shader从附注评估拆成独立性能路线：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T130 | 【T130·GPU计算性能路线】architect：compute shader任务边界、事务与收益判定 | architect | P5 | 6adf42e9e4b06ee35744ccae |
| T131 | 【T131·真机compute能力探针】tester：GLES计算着色器、SSBO与同步成本 | tester | P5 | 6adf42e9e4b03273ad8646a7 |


2026-10-26：T112/T120/T110完成关闭；T116/T121保留看评论待证。新增固定编号：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T123 | 【T123·粒子合并精确索引实现】developer：定容分组、Y范围最小ID与计数 | developer | P5 | 6adf424be4b017ab34051fa3 |
| T124 | 【T124·设备复发证据纠正】tester：PID隔离、提前CLOSED与真实负载缺项 | tester | P5 | 6adf424be4b03273ad863bfe |
| T125 | 【T125·dyadic独立测试V2】tester：真实位型、有效反例与基础模块验证 | tester | P5 | 6adf424be4b0127832c267fb |
| T126 | 【T126·色彩证据V4重建】tester：标准SHA、真实raw与实际执行矩阵 | tester | P5 | 6adf424be4b03273ad863c01 |
| T127 | 【T127·严格态逐核工程接力】architect：T110支持域、包络原语与分阶段接入 | architect | P5 | 6adf424be4b0127832c267ff |
| T128 | 【T128·精确索引独立差分】tester：无匹配上界、连续merge与事务等价 | tester | P5 | 6adf424be4b0127832c26800 |
| T129 | 【T129·精确索引设备稳定性复测】effector：PID分臂、调用负载与真实收尾 | effector | P5 | 6adf424be4b0e6ce82cdad2c |


2026-10-26：T94/T109/T111/T113/T115/T106完成关闭，T112/T114解锁；失败回执改“看评论”。新增：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T116 | 【T116·池优化设备复发归因】architect：T94堆栈、真实负载与精确索引路由 | architect | P5 | 6adf3c1ce4b017ab3404b7c5 |
| T117 | 【T117·COLLECT首错误原码诊断】developer：事务阶段、审计漂移与清理身份 | developer | P5 | 6adf3c1ce4b06ee3574451f5 |
| T118 | 【T118·首错误诊断独立验证】tester：三类99、分片身份与副作用对照 | tester | P5 | 6adf3c1ce4b0127832c15f8f |
| T119 | 【T119·设备首错误原码补采】effector：COLLECT99与AUDIT首因分离 | effector | P5 | 6adf3c1ce4b017ab3404b7c6 |
| T120 | 【T120·严格态oracle到件核对】architect：ULP位型与测试合同一致性 | architect | P5 | 6adf3c1de4b03273ad85d1a8 |
| T121 | 【T121·色彩V3到件分流】architect：常量对照范围、错误常量与归档缺口 | architect | P5 | 6adf3c1ce4b017ab3404b7cc |
| T122 | 【T122·单沙明水方案交接补齐】designer：已完成方案文件与三线合同回执 | designer | P3 | 6adf3c1de4b0e6ce82cd4117 |


2026-10-26：T97/T103/T104/T105完成关闭，T94解锁。新增固定编号：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T109 | 【T109·设备COLLECT失败归因】architect：beginCollect状态与首失败事务路由 | architect | P5 | 6adf3613e4b0127832c06d35 |
| T110 | 【T110·严格态writer证明补篇】designer：操作图、舍入包络与分支域 | designer | P5 | 6adf3613e4b0e6ce82cce07b |
| T111 | 【T111·严格态基础测试先行】tester：精确dyadic oracle与证书边界 | tester | P5 | 6adf3613e4b0127832c06d38 |
| T112 | 【T112·严格态基础模块】developer：定容dyadic比较与状态证书框架 | developer | P5 | 6adf3613e4b03273ad857175 |
| T113 | 【T113·整数条带测试先行】tester：原生像素F64零字节差与资源合同基线 | tester | P5 | 6adf3613e4b0127832c06d39 |
| T114 | 【T114·原生像素整数条带实现】developer：F64求值、整数上传与scene双租约 | developer | P5 | 6adf3613e4b0e6ce82cce07c |
| T115 | 【T115·色彩目标身份纠正补证】tester：同SRGB8附件对照与真实SHA清单 | tester | P5 | 6adf3613e4b017ab3404595a |


2026-10-26：用户新增第六大目标单沙＋明水（排在前五目标之后），新增固定编号：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T106 | 【T106·单沙明水场景方案】designer：沙水耦合、清水渲染与掉沙掉水交互 | designer | P3 | 6adf343de4b06ee35743c769 |
| T107 | 【T107·单沙明水工程方案】architect：双场水库存、同帧渲染与注入模式接口 | architect | P3 | 6adf343de4b06ee35743c76a |
| T108 | 【T108·单沙明水测试先行】tester：水沙守恒、双注入切换与明水场景基线 | tester | P3 | 6adf343de4b06ee35743c76b |


2026-10-26：T96/T98/T99/T100/T101/T102完成关闭；T97/T103解锁；新增固定编号：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T104 | 【T104·严格合法态工程落地】architect：真实patch证书与writer内缩接口 | architect | P5 | 6adf3250e4b06ee35743a638 |
| T105 | 【T105·整数条带工程与色彩到件分流】architect：原生像素三带资源与T101证据 | architect | P5 | 6adf3250e4b017ab34041a9b |


2026-10-26：T92/T93/T90/T91完成关闭；新增固定编号：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T96 | 【T96·粒子池同步热点优化】developer：等价选对短路、birthOnly与有界诊断 | developer | P5 | 6adf27a0e4b03273ad846e0e |
| T97 | 【T97·池优化独立等价与有界执行】tester：对抗池、三臂轨迹与零源链 | tester | P5 | 6adf27a0e4b0e6ce82cbe08c |
| T98 | 【T98·规划合法态数值修订】designer：安息角边界、零目标与容差一致性 | designer | P5 | 6adf27a0e4b017ab340363c8 |
| T99 | 【T99·材质离散表示修订】designer：逐像素baseByte与F64／GPU边界 | designer | P5 | 6adf27a0e4b017ab340363c9 |
| T100 | 【T100·规划对照证据纠正】tester：真实θ5轨迹与浮点辅助标签 | tester | P5 | 6adf27a0e4b03273ad846e0f |
| T101 | 【T101·色彩采集精确续证】tester：同目标生产对照、连续量与缺失矩阵 | tester | P5 | 6adf27a0e4b017ab340363cb |
| T102 | 【T102·设备首失败诊断导出】developer：code5有界trace与快照身份 | developer | P5 | 6adf27a0e4b0e6ce82cbe08d |
| T103 | 【T103·DEMO首个code5设备补采】effector：候选trace、池状态与回滚 | effector | P5 | 6adf27a0e4b06ee35742e945 |


2026-10-26：T82/T78按评论执行完成关闭（产品失败保留），T88完成关闭；T57实质阻塞。新增：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T92 | 【T92·双场主线程卡死归因】architect：shake-lift池合并与零源收据分流 | architect | P5 | 6adf1e5ee4b03273ad83857d |
| T93 | 【T93·材质整数补证分流】architect：baseByte直证与采集修订缺口 | architect | P5 | 6adf1e5ee4b0127832bb7274 |
| T94 | 【T94·双场卡死修复后复测】effector：三臂稳定性、真实注入与双流CLOSED | effector | P5 | 6adf1e5ee4b06ee357420099 |
| T95 | 【T95·DEMO物理阻断后复测】effector：LAND长窗与手持重力缺口 | effector | P5 | 6adf1e5ee4b017ab3402929f |


2026-10-26：T86/T87/T89/T56完成关闭；T82/T78/T57解锁。新增：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T90 | 【T90·规划机制到件分流】architect：空区间证据与安息角合同边界 | architect | P5 | 6adf0846e4b06ee3573f40a3 |
| T91 | 【T91·新scene几何独立回归】tester：SCENE-GEO-01至05与历史断言版本化 | tester | P5 | 6adf0846e4b0e6ce82c849fb |


2026-10-26：T71/T85/T80/T81按用户通知完成关闭，T86解锁；新增固定编号：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T87 | 【T87·色彩矩阵到件分流】architect：宿主目标能力与剩余色彩证据路由 | architect | P5 | 6adf041ee4b0127832b2c3fc |
| T88 | 【T88·材质剩余色彩补证】tester：半透明合成与baseByte精度分叉 | tester | P5 | 6adf041ee4b06ee3573ec0b4 |
| T89 | 【T89·渲染回归断言归因】architect：T62旧入口断言与新scene合同分流 | architect | P5 | 6adf041ee4b03273ad805146 |


2026-10-26：T79/T83/T84按用户通知完成关闭（b71a015），T71解锁；新增固定编号：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T85 | 【T85·规划预览只读诊断】developer：候选调用链、通道增量与逐边区间 | developer | P5 | 6adef88fe4b0127832aed275 |
| T86 | 【T86·规划预览机制补证】tester：step73候选序列与安息角区间证据 | tester | P5 | 6adef88fe4b0127832aed276 |


2026-10-26最新归因路由调整：T79原卡改architect，完整标题为【T79·安息角规划阻断归因】architect：step73边界可行性与最小修复路由。已完成历史卡不重开。新增：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T83 | 【T83·触摸接线工程归因】architect：接收失败分流与输入节点工程文档 | architect | P5 | 6adef4bee4b03273ad7e4c91 |
| T84 | 【T84·材质色彩工程分流】architect：目标编码与取整问题的工程设计用例路由 | architect | P5 | 6adef4bee4b0127832ada18c |


2026-10-26：T47/T74/T76/T77按通知完成关闭；T71解锁。新增固定编号：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T79 | 【T79·安息角规划阻断归因】designer：step73边界可行性与最小修复路由 | designer | P5 | 6adef380e4b0127832ad40f3 |
| T80 | 【T80·材质色彩客观补证】tester：目标编码矩阵与GPU取整中间量 | tester | P5 | 6adef380e4b017ab33fdad3b |
| T81 | 【T81·最新材质桥执行补证】tester：T55桥入口与T75合并执行器覆盖缺口 | tester | P5 | 6adef380e4b0e6ce82c51e1e |
| T82 | 【T82·触摸接线修复后DEMO复测】effector：真实注沙落床、停止边界与手持重力 | effector | P5 | 6adef380e4b06ee3573cc257 |


2026-10-26：T72/T75/T66/T55按用户通知完成关闭；T74/T56解锁。新增固定编号：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T76 | 【T76·开发交付独立复核】tester：上传合同、采集收尾与被动诊断反例 | tester | P5 | 6adeedbce4b0127832ab72a4 |
| T77 | 【T77·材质色彩差异归因】designer：默认pbuffer与SRGB8 FBO偏差及补证路由 | designer | P5 | 6adeedbce4b06ee3573c1957 |
| T78 | 【T78·双场采集修复后复测】effector：完整新管线可见沙与双流CLOSED证据 | effector | P5 | 6adeedcee4b0127832ab780e |


2026-10-26：T119/T138/T128/T133完成后接力（P5，不使用真机）：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T146 | 【T146·FIELD_COLUMN首错误归因】architect：现场分量、快照覆盖与前序写场 | architect | P5 | 6adf6d30e4b06ee357480fa9 |
| T147 | 【T147·向外算术独立差分】tester：affine边界、局部证书与NoWrite | tester | P5 | 6adf6d30e4b06ee357480faa |

2026-10-27：批量验收T129/T141/T145/T147后接力：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T148 | 【T148·向外除法与纯核缺口分流】architect：T147方向反例与T135实施接力 | architect | P5 | 6adfe799e4b03273ad8c38b6 |
| T149 | 【T149·FIELD_COLUMN原值诊断V2】developer：分量bits、badMask与有限阶段窗口 | developer | P5 | 6adfe799e4b017ab340ad992 |
| T150 | 【T150·FIELD_COLUMN诊断V2独立测试】tester：原bits、首错与观测无副作用 | tester | P5 | 6adfe799e4b017ab340ad993 |
| T151 | 【T151·FIELD_COLUMN原值真机补采】effector：最后合法到首次非法的完整失败链 | effector | P5 | 6adfe799e4b0e6ce82d3c570 |

2026-10-27：T142/T148交付与T116增量接力固定编号：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T152 | 【T152·GPU真机失败工程分流】architect：wire回显、shader编译与完整事务部署 | architect | P5 | 6adff62ce4b03273ad8d5fc0 |
| T153 | 【T153·向外除法oracle纠正复测】tester：精确商包含、全样本与区间除法 | tester | P5 | 6adff62ce4b0127832d27964 |
| T154 | 【T154·audit索引负载诊断】developer：真实乘数、阶段时钟与有界进度 | developer | P5 | 6adff62ce4b0127832d27965 |
| T155 | 【T155·audit负载诊断独立测试】tester：排序终止、seed放大与观测等价 | tester | P5 | 6adff62ce4b06ee3574bf340 |
| T156 | 【T156·真实负载与收尾窄补证】effector：PID锁定、先注入后finish与原码拉取 | effector | P5 | 6adff62ce4b0127832d2796a |

2026-10-27：T152/T135/T153/T150完成接力固定编号：

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T157 | 【T157·GPU信封与shader正式修复】developer：SHA回显、避名与错误原码 | developer | P5 | 6adffd57e4b03273ad8e476e |
| T158 | 【T158·GPU真机采集器修订】tester：合法JSON、请求身份与旧日志归档 | tester | P5 | 6adffd57e4b017ab340ce10e |
| T159 | 【T159·GPU完整事务测试包接线】developer：Worker所有权、请求身份与可安装工程 | developer | P5 | 6adffd57e4b0127832d47f20 |
| T160 | 【T160·支持纯核实施独立验证】tester：逐node包络、INWARD与原子影子事务 | tester | P5 | 6adffd57e4b0e6ce82d5da1f |
| T161 | 【T161·GPU修复后完整事务真机复测】tester：信封消费、三路收益与故障恢复 | tester | P5 | 6adffd57e4b0e6ce82d5da20 |
| T162 | 【P-011·T154门禁分流】architect：诊断OFF控制流+门禁探针扩展方案+baseline路由 | architect | P5 | 6ae00c6d…9c6a |
| T163 | 【P-011·worker abc打包修复】developer：T159生成项目Worker设备加载失败根因与生成器最小修复 | developer | P5 | 6ae0179b…c90a |
| T164 | 【P-011·GPU三路收益真机复测】tester：完整attempt收益+注错四类+N相关性复核 | tester | P5 | 6ae017b3…cef8 |


## 2026-10-29 接续编号（恢复远端固定卡，禁止复用）

| 编号 | task | 角色 | 优先级 | 卡 ID |
|---|---|---|---|---|
| T263 | 【T263·GPU M0全阶段独立差分】tester：LAND、床审核、拒绝与末ID重放 | tester | P5 | 6ae2d367e4b0c3103c20440a |
| T264 | 【T264·GPU M0产品整步真机验证】tester：同token提交、故障生命周期与完整收益 | tester | P5 | 6ae2d367e4b0c3103c20440b |
| T265 | 【T265·T243工程交接恢复】architect：补回已交付v2归因与诊断修订 | architect | P4 | 6ae2eb52e4b0c1a6bd710fdd |
| T266 | 【T266·V2有界诊断与启动修订】developer：独立run计时、阶段成本与合法路由 | developer | P4 | 6ae2eb52e4b0c1a6bd710fde |
| T267 | 【T267·V2落床定向续证】tester：本run物理时长、冻结阶段与报告勘误 | tester | P4 | 6ae2eb52e4b0d25fef9d6eeb |


| T268 | 【T268·明水可见性与ACK缺证分流】architect：库存到像素及设备回执边界 | architect | P3 | 6ae2f99ae4b0ab67703581ef |


| T269 | 【T269·GPU产品桥接失败首次归因】architect：命令入队、ADVANCE与时钟生命周期 | architect | P5 | 6ae30629e4b0c3103c250489 |
| T270 | 【T270·明水逐笔诊断实现】developer：有界receipt、水库存与同token层采样 | developer | P3 | 6ae30629e4b0ab677036c41b |
| T271 | 【T271·明水逐笔与像素真机补证】tester：正量水ACK、W到层像素及旧代终态 | tester | P3 | 6ae30629e4b0ab677036c41c |


| T272 | 【T272·GPU产品桥接修复】developer：128B命令、身份代次与CLOCK_GAP恢复 | developer | P5 | 6ae32a72e4b0efba6930d40a |
| T273 | 【T273·GPU产品桥接修复真机复测】tester：K192源终态到整步同帧上屏 | tester | P5 | 6ae32a72e4b0e9cb68d51e84 |
| T274 | 【T274·V2续证阻断首次归因】architect：首生计时冻结、锚点与零床量 | architect | P4 | 6ae32a72e4b0e9cb68d51e85 |
| T275 | 【T275·事务AFTER切点合同裁定】architect：原拒绝消费者、观测边界与首码归属 | architect | P4 | 6ae32a72e4b031ea82ab720c |


| T276 | 【T276·V2诊断导出修复】developer：短时钟日志、分片冷档与提交锚点 | developer | P4 | 6ae32e53e4b0efba69310d8f |
| T277 | 【T277·V2同run落床锚点续证】tester：首生八秒、完整分片与库存像素链 | tester | P4 | 6ae32e53e4b0efba69310d90 |
| T278 | 【T278·事务AFTER字典实施】developer：NEXT原消费者、OBS快照与NA边界 | developer | P4 | 6ae32e53e4b0f69bc561d7b1 |

| T279 | 【T279·GPU M1整步候选核】developer：T16水沙运动、ARRIVE与侧扩散 | developer | P5 | 6ae34b05e4b0efba6933b57d |
| T280 | 【T280·GPU M1整步提交核】developer：全相审核重放与native接受 | developer | P5 | 6ae34b05e4b0f69bc5642ef9 |
| T281 | 【T281·GPU M1明水同帧接入】developer：publication水scene与产品路由 | developer | P5 | 6ae34b06e4b069bbd86615b9 |


| T282 | 【T282·GPU READY原子发布修复】developer：epoch门、队列与一致状态快照 | developer | P5 | 6ae3e0eae4b0efba6938f71b |
| T283 | 【T283·GPU READY修复真机续证】tester：一致权限到K192整步上屏 | tester | P5 | 6ae3e0eae4b0efba6938f71c |
| T284 | 【T284·V2冷采总预算修复】developer：锚点限次与延后分批导出 | developer | P4 | 6ae3e0eae4b0e9cb68dbe0ea |
| T285 | 【T285·V2有界冷采真机续证】tester：单run首生八秒与落床索引链 | tester | P4 | 6ae3e0eae4b069bbd86bf5a8 |
| T286 | 【T286·明水实际覆盖区重采】tester：正确ROI、自然水量正控与run勘误 | tester | P3 | 6ae3e0eae4b0f69bc5697a38 |
| T287 | 【T287·GPU M1候选核独立验证】tester：T16、到达序与侧扩散写集 | tester | P5 | 6ae3e0eae4b069bbd86bf5ab |


| T288 | 【T288·M1独立事件解析首次分流】architect：event64布局、Capture与oracle边界 | architect | P5 | 6ae3f6c7e4b031ea82c25c23 |
| T289 | 【T289·明水正控与量化续证分流】architect：单列覆盖、源窗口与sRGB判线 | architect | P3 | 6ae3f6c7e4b0f69bc56bc7a9 |
| T290 | 【T290·T251执行报告交接】tester：已有真机原档、结论与缺测登记 | tester | P5 | 6ae3f6c7e4b031ea82c25c24 |


| T291 | 【T291·GPU CLOCK_GAP持续阻断归因】architect：真实调度时间线与首帧推进 | architect | P5 | 6ae3fc72e4b031ea82c40fd9 |
| T292 | 【T292·M1独立oracle步长纠正】tester：256B事件行与T16到达全流复核 | tester | P5 | 6ae3fc72e4b0e9cb68de9d5c |
| T293 | 【T293·明水同run可辨正控补证】tester：动态列ROI与源终态保留 | tester | P3 | 6ae3fc72e4b0f69bc56c71e6 |
| T294 | 【T294·事务设备runner缺口首次分流】architect：T251入口与合包原档接线 | architect | P5 | 6ae3fc72e4b031ea82c40fda |


| T295 | 【T295·GPU首帧分段时间线】developer：wake、Pump、Draw与CLOCK_GAP原时序 | developer | P5 | 6ae4033de4b0efba693caf5b |
| T296 | 【T296·GPU首帧真实时序采证】tester：工作等待调度分账与K192闭环 | tester | P5 | 6ae4033de4b031ea82c60542 |
| T297 | 【T297·整体事务既有runner真机执行】tester：三backend两臂、故障与服务成本 | tester | P5 | 6ae4033de4b0f69bc56d30bb |

| T298 | 【T298·GPU M1整步接受独立验证】tester：审核重放、尾包与lease原子性 | tester | P5 | 6ae406b7e4b0f69bc56d8dde |

| T299 | 【T299·事务backend受阻首次归因】architect：AVL与GPU停顿、cut命中及归档 | architect | P5 | 6ae40da4e4b0f69bc56e4d51 |
| T300 | 【T300·GPU M1场景独立验证】tester：双态水沙同token像素与lease边界 | tester | P5 | 6ae40da4e4b069bbd870db67 |
| T301 | 【T301·GPU M1产品真机首证】tester：双源整步到明水同帧上屏 | tester | P5 | 6ae40da4e4b0efba693dcf3b |


| T302 | 【T302·GPU真机时间线出口归因】architect：冷导出接线与M0/M1超窗证据 | architect | P5 | 6ae41460e4b0f69bc56ef2c5 |
| T303 | 【T303·事务runner阶段观测接线】developer：worker收命令、原错与冷阶段档 | developer | P5 | 6ae41460e4b0e9cb68e0eb89 |
| T304 | 【T304·事务阶段锚点真机续证】tester：真实backend入口与缺档定向补齐 | tester | P5 | 6ae41460e4b0f69bc56ef2c6 |
| T305 | 【T305·明水长窗收据缺口分流】architect：定容环保留与同run像素证据 | architect | P3 | 6ae41460e4b031ea82cab2a0 |


| T306 | 【T306·GPU时间线冷出口实现】developer：M0/M1分页文件与完整性清单 | developer | P5 | 6ae4275ee4b031ea82cd9e33 |
| T307 | 【T307·GPU冷时间线定点真机续证】tester：M0/M1原始文件到首帧分账 | tester | P5 | 6ae4275ee4b0f69bc57022ad |
| T308 | 【T308·V2 watchdog粒子路径首次归因】architect：buffer绑定、patch规模与累计复杂度 | architect | P4 | 6ae4275ee4b031ea82cd9e34 |
| T309 | 【T309·明水critical收据保留实现】developer：双lane、冷分段与源帧身份 | developer | P3 | 6ae4275ee4b0e9cb68e1fbca |
| T310 | 【T310·明水长窗源到像素真机闭环】tester：critical零丢失与同run水面 | tester | P3 | 6ae4275ee4b0e9cb68e1fbcb |


| T311 | 【T311·V2累计复杂度整体修订】developer：绑定入口、全池工作与阶段预算 | developer | P4 | 6ae4324ae4b0e9cb68e2a96c |
| T312 | 【T312·V2整体复杂度真机续证】tester：双路径阶段账与落床同帧 | tester | P4 | 6ae4324ae4b0f69bc570ebd8 |


| T313 | 【T313·GPU首root超窗首次归因】architect：原始时间线、提交耗时与时钟边界 | architect | P5 | 6ae43b70e4b0efba69413c99 |
| T314 | 【T314·事务冷启与故障语义分流】architect：Want复用、阶段分母与cut终态 | architect | P4 | 6ae43b70e4b0efba69413c9d |
| T315 | 【T315·明水长按源量与像素链核对】architect：有效窗口、两笔水量与同帧身份 | architect | P3 | 6ae43b70e4b0e9cb68e36607 |


| T316 | 【T316·V2零分配静态失败首次归因】architect：基线146与新增18证明缺口 | architect | P4 | 6ae43ce9e4b0efba69415fd1 |

| T317 | 【T317·GPU首root同步超窗修订】developer：初始化前置、Submit分段与有界提交 | developer | P5 | 6ae44d63e4b0efba6942fb6a |
| T318 | 【T318·GPU有界首root真机闭环】tester：Submit分账与双源同token上屏 | tester | P5 | 6ae44d63e4b031ea82d675fc |
| T319 | 【T319·事务原档定向checker】tester：三cut原码、前缀与报告勘误 | tester | P4 | 6ae44d63e4b031ea82d675fd |
| T320 | 【T320·零分配定向证明补证】tester：Date原语、净18记录与旧39位点 | tester | P4 | 6ae44d63e4b0efba6942fb6b |
| T322 | 【T322·明水原账与commit标签勘误】tester：有效33ms、源物理代与swap证据边界 | tester | P3 | 6ae44d63e4b0f69bc5737c1a |


| T324 | 【T324·GPU分片提交合同接力】architect：continuation、撤权封尾与总墙钟 | architect | P5 | 6ae45280e4b0e9cb68e5599a |

