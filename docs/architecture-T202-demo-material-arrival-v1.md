# T202：DEMO 材质原值到件与真实分流

2026-10-28，architect；原卡 `6ae152f0e4b0d25fef57f110`。消费 T176、T180/T187 `0a6fe16`、四臂原raw、T178/T179及现行源码。不使用真机、不重跑测试、不改业务/阈值。本稿接续T176，原报告/raw不覆写；核证摘要与重组原包分别在 `analysis-T202-demo-material-capture-v1.json`、`analysis-T202-reassembled-material-v1.json`。

## 1. 裁定：有完整导出，零行结论是报告/检索错误

C3-full.txt按PID40176解析，实际有 **24个 `WS_EVENT scene_material_rejection` 分片**，在原行198674–198697，part1..24无缺号/重复，complete=true，总49063字符/UTF8字节，原重组文本SHA256 `0f3cedbf65e2c69697fe7de43b8d670531a4bb629b9836916737d257fdd3e39f`。20项数组的原hex重算SHA全部匹配，未执行项为0B/null并明确NOT_VALID_CURRENT；不是丢行后猜出的bits。

该包绑定run `p011-run-1793151013110-3`、stateEpoch2、commit826、page/contextFrame410、sceneFrame409、token `p011-run-1793151013110-3:2:826:409`。token与原p013_scene及包内native回执一致。**不派“补DEMO导出接线/放宽触发条件”的开发返修**：源码已有接线，设备本次也确实执行了导出。

T180/T187报告标题“全窗口未复现”应限双场页；DEMO材质故障已复现且有原值。按`.rows[].event`解析被动诊断，C3 present为0×414、100×63、冷启动−1×1，63个reason均SCENE_MATERIAL_DATA。p013_scene另有CURRENT33、STALE2、FAULT68，这是不同日志采样分母，不与63相加或硬配行数。首故障原p013_scene在09:30:46.166，demo present记录到09:30:46.492才输出；用token关联，不用墙钟邻行替代。

## 2. 已定位的真实拒绝谓词

firstFailure.point：MATERIAL_DATA_PROFILE / SOURCE_VALIDATION / column57 / row0 / channel=null / predicateMask128，Error.message=MATERIAL_DATA_PROFILE。

| 第57列字段 | F64实际存储值 | little-endian原8B |
|---|---:|---|
| sandTop、bh、bl、mh、ml、suspH/L、hs/hl | +0 | `0000000000000000` |
| waterTop | −1.6653345369377348e−16（−3×2^-54） | `000000000000a8bc` |
| colParticleMassH | 同上 | `000000000000a8bc` |
| colParticleMassL | +0 | `0000000000000000` |

本包128列中只有57列满足top>water。`P013FrameAdapter.pack`源码位128恰为top>water，因此这次CPU准备拒绝的直接原因**已闭合**。它发生在source验证，displayF32/profileF64/materialStagingF32当前有效长度均0，不是F32量化、shader/GL上传或资源纹理错误。零床下负waterTop与负列缓存对应；不是此列的S结合顺序差异，不能套用T176历史mixed候选当本次原因。

当前源链：Hot.editPoolHot对DEPOSIT/DEGRADE/CROSS用增量 `cache[c]-=amount`；Hot.audit允许列缓存及误差在1e−9内，dry waterTop与S+P也按该EPS核；M2Sink dry写与WaterSandField.updateSandTop:7358用sand+susp+pool.particleMassAt生成包络。renderer则要求严格0≤bottom≤mix≤top≤water≤capacity。**物理允许的微负派生残差进入严格渲染域**是本次已证实的跨模块合同不一致。

但原包 `poolReplayComplete=false`，没有完整pMass/ID/state/free-list及从上一次合法值到首错的事务写入历史。因此不能断言57列实际已无live粒子、哪一次LAND/CROSS独占产生该残差，也不能把末次写waterTop的函数直接定为首个算错writer。captureTimeMs未测、完整包source映射未闭合等边界仍保留；不因此重复索要已拿到的20项材料原bits。

## 3. 触发、锁存、CURRENT及包身份

现行EmptySandDemo在非null回执且`!currentFrameSucceeded`时emit；native Reject默认current=false，成功路径才设置current=true且state=CURRENT。STALE旧帧成功仍不等本candidate成功。本次包内回执code100、current=false、presentationState=STALE、requestedSwap=false、staleSwap=true，presentOldStatus/swapOldStatus均0，compatibilityMask0；说明首错曾成功重呈旧完整新管线帧，不能把STALE当新材料PASS。

`P013MaterialRejectionStore.capture`只依赖firstFailure是否为空，不要求firstSuccess存在；`exportFrozen`会返回首错对象，页面要求的是有效firstFailure token。故“无首成功所以无bundle”不是此实现的前置。此包实际上还有firstSuccess commit4/page2/scene1。首成功摘要是准备阶段成功，并非独立证明其呈现CURRENT；本批另有CURRENT日志。

SceneController在pack前设置diagnosticFrame；catch内捕获summary/point后再拷贝材料，native后按requested token附回执；getLastMaterialRejection读同一个preparation store，两个页面均可读。这里没有已证实的DEMO/双场独立store接线缺陷。现有诊断异常吞掉/导出失败只输出EXPORT_FAILED是可观测性限制，本次不扩大成必须先重写诊断的阻断任务。

报告声明s151 signed SHA `3c8abfae71bbff53ca5e1ae98137b187e2d11f96af4d121a0423597ff0748f6c`；按s151文件名未定位本地HAP，设备安装/构建逐文件映射未独立复算。0cc0439与当前6个相关源码的LF归一化比较及SHA见分析JSON，且0cc0439已包含T178/T185源码；“源码无差异/CompileArkTS跳过”不能证明已安装HAP身份。**本次24分片和回执revision是设备实际有材质V1诊断的正证据**，不能反过来因包缺项声称设备没装T178。V2物理首错未触发不单独证明所有V2分支已设备执行。

## 4. 双场控制臂与采集纪律纠正

C0 PID36531：present0×520、冷−1×1，无本次物理首错；仅一次GEOMETRY_CHANGED初始化stop。C3有6笔dry.receipt ACCEPTED，实际准入合计4.45114754486084，commit继续；本窗口无first_physics_failure_v2/page-physics-failed，不改变T181旧首码NOT_CAPTURED结论，不重新跑无触发宽矩阵。

C1 PID38777只有131条物理帧，到09:28:20.741止；20.744已CLOSED。C2 PID39412只有133帧，到09:29:14.017止；14.029已CLOSED，首pan25.338晚11.309秒，488条pan均随后。不能把C2当“真实注入后持续运行、因此材质已修”的控制；只是已采到静置活动窗未出现目标故障。此问题沿T116/T204已有提前finish采集工程纠正，不新增宽矩阵或平行整体归因。

console的C0/C1“归属OK”也与报告不符：Cold-Start将Log-Step/Run-Cmd输出和最后PID一并写success stream，`$c0pid/$c1pid`收到数组，console实际DRIFT。start/end行的标量PID仍分别相同，可人工按raw锁定四臂；不能将这个脚本变量污染误判为OS换PID，也不能称自动归属断言通过。click/swipe有裸调用，审计文件大小不代表每条操作退出码完整。

## 5. 后续路由与实施边界

### A. effector/采集工具工程：现存材料勘误，不使用真机，可立即执行

所有权为T180/T187报告/提取工具/归档；按本稿从C3-full重组24片，核token/part/total/hash，保留原raw与source包身份缺项；修“rejection0”“T178未接DEMO”“C2真实注入控制”“PID断言OK”。被动诊断按rows.event读，采集按PID/run/window而非全历史First1，PowerShell helper只返回一个typed PID，日志走独立通道。不要造旧包、补写原bits、重跑已有数据。

### B. designer：干场派生缓存—publication—renderer域合同，设计问题

本次已具源域失败原值，不再以缺20项bits阻塞设计。与T197整体复杂度方案协调，明确colParticleMass的意义、权威live pool归约顺序/失败边界、dry waterTop包络的合法域与publication消费域。评估在固定阶段以非负live质量确定性重建派生cache/包络的方案及成本；若要改缓存归约次序、物理审计域或既有事务操作图，必须版本化并说明源账/预算/rollback/原F64等价或真实变化，不能以工程判断擅自改算法。

禁止renderer加EPS、water=max(water,top)、负cache钳零、放宽H/drift、修改质量/床或跳过当前材料验证冒修复。renderer保持严格拒绝，失败FAULT或真实STALE完整旧帧。若方案要从snapshot重建完整池，应在owner提交/派生边界处理并计入整事务，不能每帧UI额外scan/copy后隐藏物理错误或把逐事件O(K)工作引回热点。

### C. developer：方案后工程落地；必要观测是精确hunk

不使用真机。数值实施先等B方案，再由architect修对应工程文档；不能因本报告已有负数就直接改Hot。若设计证明需要最后writer/完整pool因果，最小观测hunk限于首个目标失败的同token冻结：13池buffer+live/free/nextId、cache前后/操作kind/amount/row/commit身份，来源标publication/authority/derived；复用冷预算，只读既有失败输入，不再调用snapshot/updateSandTop改变首错值。字段未采/null及字节上限明确，不全量逐帧dump。仅补证实现可独立关闭，不以真机为开发关闭前置。

### D. 独立tester与后续同DEMO窄设备

独立tester不使用真机。先核本原包的hex/SHA/predicate和原值不变，再按设计的合法域与确定顺序给反例：空列残差、partial/full LAND、跨列出入、mixed和dry分支、正/负零/subnormal、失败回滚及publication一致性；不能只复测store getter代替完整生产链。若需动归约，覆盖source/ID/cache/账本/事件与完整事务成本，不将新语义差异藏在容差里。

设备卡必须真机标签、串行队列，实质前置为实际修复与独立无阻断。只取同EmptySandDemo一个静置控制和T95/T180实际准入负载，确认run/包/源/新诊断版本、成功source与持续物理帧、当前材料严格合法及CURRENT/FAULT/STALE准确区分；沿新XComponent+shader含背景前景，禁止Canvas上屏/失败回退。修复前已有首错不必无目的重现；未再次触发不能签产品普适通过，T200观感须等实际修复。

所有后续prompt：共享树不回退他人改动；不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本人状态有限例外），不改其它行/队列或滴答同名清单；无设备卡明确不使用真机，设备卡标真机/串行；失败回原卡看评论、成功结束待验收；manager按交付登记不加验收门。

## 6. 交付状态

本轮“零导出缺口”的首次核证已完成，直接拒绝谓词与设计路由已明确；属architect成功交付，产品故障仍未修复，精确首次产生cache残差的事务历史仍未知。本人行结束待验收，原卡回执明确勘误/证据与后续；不关闭他卡、不更新manager清单。没有产品PASS、没有新设备执行或测试重跑。
