# T651：M2 创建与只读采集并存通道首次归因

2026-11-19，architect，不使用真机。结论 **TESTER_FIXTURE_REPAIR_FIRST / PRODUCT_FALLBACK_NOT_TRIGGERED**。原T591 v2设施未激活M2 create，不能据此认定真实创建期间通道被堵；目前不派物理返修或产品持久化线程。正式续办表`architecture-T651-contract-v1.json`、源码/保存原件证据`analysis-T651-evidence-v1.json`及共用有限复核脚本已交；不运行产品或设备。

## 1. 64 个空样本不能触发 fallback

probe-log实际S1..S64均NO_CREATE_ATTEMPT/STALE_CREATE_SERIAL，前52与newRun后12一致；未锁serial。T591Probe没有XComponent、真实M2页面激活和owner创建链；newRun只Invalidate并wake，不能创建缺窗口的GPU对象。它只证该probe页Store为空，不证M2 Create的开放调用、只读通道阻塞、漏Finish或缺独立线程。

实际M2页是该构建树`GpuResidentM1.ets`：libgrsm1、真实SURFACE XComponent与show/foreground/setActive/beginPageSession/帧回调；实际owner在已得GL context后围绕gpu->Create设置create468::Scope并显式createScope.Finish(code)。m2CreateDiagnostics/Log只Global.Copy后序列化，Store自己mutex；不取App mutex、不向owner排GL任务。创建在owner线程，不要求collector另开一个前台页面；把同一tester定时collector嵌入真正M2页面即可保留这些生命周期，当前未试过。

probe另有明确设施缺陷：native字段是`"bootSerial":"U64"`，当前regex却找未加引号的数字`serial`，即使有attempt也不能正确锁住；转换Number会丢U64。`sealed`/nativeEndNs才是终态语义，当前code0/endNs子串规则不符。120/80及1400字符slice只能做屏幕摘要，不能冒完整JSON/原日志。每tick两次API×64等128调用，也不是T622的64次查询预算。这些均由tester维护，不退developer。

## 2. 原 T591 tester 可执行合同

不改产品native源码。新增**测试模块**`ets/physics/T591CreateCollector.ets`及有界文件writer/index，并在同构建树真实GpuResidentM1页装配；保该页native库、真实M2 route/宏、XComponent callbacks、show/hide/active/page session及物理参数。不能仅将路由换回独立probe或在另一VM复制App对象。与产品ELF逐字节核相同，测试ABC/页改动单独登记；签包、安装/执行与脚本均tester负责。

collector在允许该真实XComponent创建**之前**先arm并同步保存初始读取与collector启动收据，然后按原页面流程自然创建；不能等READY后才开timer。可用测试页的已arm状态控制首次XComponent挂载，不能修改native创建语义或塞假window。app.native实例可能已存在，须记启动前最新serial，不把旧槽当此次新create；新配置/已存在窗口的真实reset仍由原入口触发，不能靠重开collector制造成功。

每≤1s读取m2CreateDiagnostics("0")仅用于发现**新**attempt；发现后以JSON解析完整十进制字符串bootSerial，锁boot/进程启动、pid、模块路径/BuildId、epoch/window/eglContext/candidateRun/ownerTid。后续只读该非零serial，STALE/进程退出明确结束，不重新跳0追新槽。原T461仅historicalReference+原文件SHA/differentAttempt，不接死进程后继。

总窗口≤60s、采样tick≤64、**所有native查询合计≤64**（含diagnostics和各object log），1s周期不为了补丢样追赶；无attempt时不读无意义log(0)。按program字段实际reported/copied/offset/变化，在编译/链接日志变动或终态时读取object0..2，预留调用预算，预算不足准确标未收齐。只保存实际程序/sourceSHA/原GL返回，collector不发新GL查询、不清GL error、不修改物理或唤醒create。

每份**物理记录**UTF8≤16KiB，目录总≤1MiB含索引，先留32KiB给索引/终态状态。保存每次完整native原字符串及raw log并SHA，不能slice后声称完整。若固定原JSON超过16KiB，把**同一次已返回字符串**按≤8192 raw字节分块base64，逐块index/整份SHA与总长度，禁止重新调用native去拼一个快照；达到目录信用前停止，标CAPACITY_REACHED而不覆盖。UI只保短摘要，不能不断追加原文到@State日志当外部持久化。

T462原raw log保在Store，不复制第二套history。文件在UI锁外异步有界串行写，busy不积Promise队列；App/Store锁都不能跨IO。保collector序号、真实采样起止时间、native完整JSON及调用ENTER/EXIT，不将UI Date.now当native create时钟。归档是不可逆追加的每采样原件，`.part`临时文件写完再atomic rename发布单样本，失败留原错/partial，不把rename失败冒成功。manifest最后写，所有raw/目录SHA分名、无自hash。

真实Finish须`sealed===true`、nativeEndNs>0、实际code均有原件；code0才是创建返回成功，也仍不等业务物理PASS。仍inCall记OPEN_CALL_AT_OBSERVATION_END；scope析构/超时/页面hide不造Finish；进程死记PROCESS_LOST，serial被覆写记STALE_CREATE_SERIAL。当前源在gpu->Create返回后有显式Finish，尚无证据证明某实际退出漏Finish。

## 3. 何时才考虑 T622 产品 fallback

本轮已有同页独立Store读取路径，不认“只能一个前台页面”为实质前置。若真实M2 attempt已开始，且collector的client ENTER/EXIT、同identity owner stage/inCall与外部栈表明UI主线程确被某同步native调用堵住、现有同页/合法异步通道均不能读取，tester保精确调用与原错回architect。不能从timer不动直接推Store被锁，也不能从C++独立mutex推JS必定可重入。

只有该新证据成立才启用T622的非GL线程/外读三槽固定快照、≤64KiB/每页16KiB、锁序与atomic发布工程；当前不将其作为开发或T591开工前置，不授权泛化落盘系统。若需新产品接口，必须先正式定义所选字段/截断、真实sizeof（不能复制三份可能超过64KiB的完整Attempt）、源/模块身份、失败原码和Finish切点，再developer；不把测试夹具交开发。现有32/64KiB fallback只是条件方案，不是本卡未完成项。

本卡已完成首次归因及精确原卡续办。T591 create终态、实际开放调用等待、M2编译/资源/恢复/物理指标仍未测；64个空样本不是产品PASS。执行要么完成当前范围/本角色必要检查，要么遇确实无法继续的问题并完整回执才停，不能做一点就停；局部受阻先做其余。失败列未完/复现原错/证据/已试/不能继续原因/前置与角色，原卡评论并本人行看评论，成功结束待验收。不允许修改D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外），不改其它列/他行/队列或滴答同名清单。GPU驻留、CPU停投、仅XComponent+shader前背景，FAULT或明确STALE，禁Canvas/软件GL回退；目标10/旧湿M1暂缓，水线13→14→12。开发仅编译交付，tester/effector签名部署执行、真机串行；不加manager或开发设备门。
