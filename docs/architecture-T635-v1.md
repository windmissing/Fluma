# T635：lab 取消、排空和剩余注入域归因

2026-11-19，architect，不使用真机。完成本卡归因/工程合同，未运行lab、构建或产品测试。消费T531/T560/T565原卡回执、T579实际签包、T587 v1/v2及原件。机器合同见`architecture-T635-lab-lifecycle-v1.json`，原件与有限检查见本轮audit。

## 事实与结论

两份v2记录各30行，START→END分别360,570,468ns（0.360570468s）和352,362,969ns（0.352362969s）；尾行均COMPLETED_FIXTURE_NOT_PHYSICS，未含CANCEL_ENTER/CANCEL_OBSERVED。所谓“启动4s后取消”没有同一原生时钟的调用时刻；UI仅启动时手动poll、无自动刷新，屏上RUNNING可已过期。不能证明取消发生在执行中、更不能证明工作不响应取消。v1约40分钟的叙述也不能替代原生执行耗时；保四档分页原件的窄PASS，不扩为取消/排空PASS。

**已证工程缺陷**：实际T565 lab.h和签包构建树一致，Cancel无条件改cancelled=true；Poll在完成后按该可变bool重算outcome，于是已完成结果被后到取消改CANCELLED。异步worker和独立mutex客观存在，不能称job同步堵住UI。确有未闭合的取消检查点和drain接线，属于源码工程问题，但现有两条实采不证明其运行中触发。

## 不可变终态与接口

保现有Start/Poll/Cancel/Release及FilesV2 endpoint，增记录revision=`T531_RESOURCE_LAB_RECORD_V3`。内部`CancelResult Cancel(jobId, nowNs)`返回REQUESTED/ALREADY_REQUESTED/ALREADY_TERMINAL/STALE_JOB，NAPI仍返回完整有界JSON；每次写入actual native时间与job generation。生命周期RUNNING→CANCELLING→DRAINING→FINISHED，唯一worker封存终态COMPLETED_FIXTURE/CANCELLED/CASE_FAILED/INCOMPLETE，记录terminalNs、cancelRequestedNs、cancelObservedNs和firstFailure，终态只写一次。已FINISHED取消回ALREADY_TERMINAL，原outcome/records/hash均不变；观察请求回执可另存，不回填已封records。保V1无outcome按records读、V2历史原件不重写。

取消不撤销已完成写入、不删除旧证据，也不改业务CURRENT。检查点覆盖Build填充/哈希每4096word、Pages每页、Diagnostic每有界阶段、Cache每提交/等待、reader barrier；检查点只停止新工作并启动RAII清理，不能把已排队I/O当已退。FINISHED仅在reader/builder、buffer/独立store任务、自己export句柄全部实际退出后发布。超时不得伪FINISHED；输出DRAINING+具体pending，保首错。worker不能被unsafe detach/强杀后重用其Lab/目录。

封存优先级明确：真实case/清理失败→CASE_FAILED并保firstFailure；否则在同终态锁内观察到本job取消→CANCELLED（补实际cancelObservedNs）；否则有完整END→COMPLETED_FIXTURE；缺END→INCOMPLETE。取消与封存竞态由这一锁确定，不能仅看records行数判断谁先发生。FINISHED后发起的FilesV2有独立export生命周期，不反向改变已封job；新job仍须等该export退休。

新增job拒绝条件含运行/排空/本job export活跃，回LAB_BUSY或LAB_EXPORT_BUSY，不在UI线程join未退休worker。受控线程退出后才重置job、catalog/count/导出身份；旧job命令不影响新代。终态收据由worker冻结，UI显示terminal/poll sample时间，避免静态RUNNING误判。

## 5s drain 的真实工程缺口

T565增加了`ProgramArtifactStore::DrainForTest(deadline,cancel)`，但Lab::Cache仍调用普通`store.Drain()`，未消费test helper；不能仅“helper已存在”就宣接线完成。该helper的spaceCv.wait只有predicate检查deadline，没有定时唤醒，满队列不pop/无notify会超过5s；插入fence后的future.wait_until又不观察cancel。这里是静态可证缺口，尚无本轮运行失败证据。

只在GRS_PROGRAM_CACHE_TEST_FAULTS目标修：保持store同锁等位和插入fence；改为`spaceCv.wait_until(deadline, predicate)`。fence回调捕获一个有界共享完成状态，在store同锁更新done并notify；等位与等fence均等待(done/space/cancel/quit/deadline)，同一steady绝对deadline覆盖全过程。取消通知注册在当前lab store，cancel置位后与该store mutex协调notify，避免丢唤醒；注册对象有所有权，不保存会悬空的栈指针，锁序不持Lab mutex调用阻塞store操作。排队fence不能因超时释放仍被worker引用的状态，回调持shared ownership直到执行/退出。

Lab全部测试排空改用此helper并记录DRAINED/TIMEOUT/CANCELLED/STOPPED、queued-at-entry/fence-sequence/pop/insert/return时间。测试阻挡job也必须可被取消唤醒，随后真实排空；5s保证的是一次测试等待的有界返回，不能声称内核文件I/O被强制中断或物理资源已释放。生产Enqueue/Drain、128容量和生产信用不改。

## T587 剩余域无需泛加“强BUSY”

| 域 | 可执行构造与责任 |
|---|---|
| 运行中取消 | developer在独立lab增加预注册checkpoint barrier；tester等待BARRIER_ENTER原件再Cancel，核观察/清理/不可变terminal。不是固定sleep4s猜窗口。 |
| 同锁等位/5s | tester独立NDK harness链接同一fault-build Store，冻结一个可取消worker job、填满128队列，观察等待→pop一个→fence插入；另一例不pop到deadline；另一例等位/插入后取消。helper修复由developer，脚本/夹具由tester。 |
| FilesV2 BUSY | 同一已完成且有文件job，Begin成功后不End再Begin，真实LAB_EXPORT_BUSY；无需强制生产BUSY开关。 |
| TTL/边界/reader | 实际Begin→Index/Page→60s到期（≤1s异步采样且读不续），错id/offset/fileId/page、End后拒旧，单reader竞争均可直接调用现接口。边界时钟注入仅独立helper实例，标INJECTED_CLOCK，不冒真实60s。 |
| stat变化 | tester只改自己隔离job allowlist文件，Begin后同大小改写/截断/替换/删除，取读前后stat/hash；无产品目录任意写接口。夹具不可访问则单列设施缺口。 |

## FilesV2 同范围工程补口

实际FilePage的base64通过Raw/Append拼接而未加JSON引号；Writer::Append是原字节复制，须改String或严格固定引号，完整合法JSON不能以Good容量状态代替。T587当前两对FILE/SHA来自Poll导出，不证明FilesV2传原文件成功。

Begin部分open失败须RAII关闭本次已开fd；错exportId的End不得清除活跃别人的reader（现代码会清active且不close）；新job必须清旧catalog并在export退出后才换目录。Index先核page<ceil(files/8)，checked计算，不能让page×8 U32回绕；桥接fileId先核U64范围再缩U32。Page对已pin fd用fstat与路径identity在读前后双核，含mtime/ns级身份；同大小内容替换还需冻结的全文件SHA与collector最终SHA核一致，不能只看inode/size或chunk自hash。End/过期/异常统一释放句柄，错误完整有界回包，不刷新TTL。

固定上限沿32文件/单文件2MiB/总64MiB、catalog32×≤256B、chunk4096B、JSON16KiB、rows256×256B；新终态/取消/fence共享状态总固定增量≤1024B，实际sizeof单列metadata，不暗抬58,993,832B原payload包络，不把cache128MiB/线程栈算已包括。

## 交接

developer修原lab/Store test-only helper、桥接和页面状态/FilesV2完整范围并编译；tester维护独立harness和原T587续测，保v1窄PASS、各注入域独立结论。本卡已完成工程归因，不把尚未修复/未测变产品PASS。完整执行边界见`architecture-T633-T636-execution-boundaries-v1.md`，其内容为下游prompt必读部分。

## 2026-11-20 T688 回执覆盖修订

T587 v4保 UI fixture 正常完成/排空与Poll记录落盘窄证。两取消时钟均为0，设备上未行使独立driver的执行中/终态后取消、128等位/fence/5s及FilesV2负控。后续按 [T688](architecture-T688-v1.md) 复用T668 ARM64 driver，保host/设备边界与已登记的原卡完成。
