# T622：M2 创建终态与跨 attempt 证据

> 2026-11-19 T591 v2后继归因见`architecture-T651-v1.md`及contract表：独立probe无真实M2 create，先tester将collector嵌入真正M2页并修bootSerial/终态/完整原件/64次总查询预算；空Store未触发本稿产品fallback，不派物理返修或默认新增诊断线程。

2026-11-19；architect；不使用真机。结论：旧进程终态不可恢复；当前已有独立mutex的只读Create查询接口，先交tester修证据闭环，不重造产品持久化系统。保留旧DECLARATION_MISSING，未测产品PASS。

消费T588、test-report-T591-external-receipt-v1.md及test/reports/t491/t591a/external.json。其HAP/包内libgrsm1.so摘要是安装收据；process明确写新M0 pid57432，timeline也为libgrsm0.so，185行47页且GREW_DURING_EXPORT/INCOMPLETE。notes的“install+load receipt”不能支持实际M2加载。linkedOriginal只列包/ELF SHA，没有原native/manifest文件SHA；不能将报告文字当已存在的连接字段。原T461进程死亡，新epoch4/run2/context3不能拼旧LINK后继。

## 已有产品接口的能力边界

实际 `.codex/local-test/t562-m2-fix/.../m2/create_diagnostics.h`：Store三attempt固定环，Begin分配serial并记epoch/window/tid/context/identity；Recorder Enter设calling和current，Exit记录实际返回，Finish设code/end/sealed。Scope析构只恢复thread_local active，**不会自动Finish**。因此只有观察到真实Finish才能称terminal，析构、UI跳页、查询超时均不能代替。

app_bridge.cpp 的m2CreateDiagnostics(具体serial)直接Global.Copy→Json，只持Store自身mutex，不取App mutex、不执行GL；m2CreateLog同理，开放GL调用期间原则上可以查询。Copy(0)只适合发现最新serial，一旦取得必须固定非零serial，不能在下一attempt替换后继续拼接。三槽被覆写时明确STALE/NO_CREATE_ATTEMPT，不当终态。查询不触发create、不改GL error、不帮owner完成。

## 新 attempt 持续外部采证方案（tester）

用明确M2页面/route开始一次新尝试；记录设备build、HAP/包内目标ELF hash/BuildId、pid+进程启动时间/boot身份、实际映射模块路径/BuildId及其与包内字节关系，不能只用pid或安装SHA。若平台读不到映射，明确LOAD_MAPPING_MISSING，不伪load证明。

在打开创建前启动外部collector：每≤1s读取一次实际m2CreateDiagnostics；发现serial即锁住epoch/window/context及进程身份。开始/阶段变化/开放调用/终态均逐条落盘，记录采样时间、原完整JSON、collector序号和SHA。同步取当前对象的m2CreateLog，不等创建成功后才采第一份。默认单次≤60s/64次查询；每份UTF8≤16KiB，外部目录最多1MiB（含索引，达到上限停止并标CAPACITY_REACHED，不覆盖旧attempt），可减少样本以留索引空间。此是测试证据预算，不占产品cold payload/broker。

已Finish：封最终terminal/code和所有实际阶段/原始GL返回；仍calling：保最后ENTER+尚未EXIT，记OPEN_CALL_AT_OBSERVATION_END；进程退出：PROCESS_LOST，最后开放调用不伪失败返回；未发现serial：CREATE_NOT_OBSERVED。链接原T461只作historicalReference，列原文件路径+整文件SHA并声明differentAttempt=true，绝不融合序列。

若UI同步调用堵塞导致无法发起只读查询，先用实际现有异步worker/外部导出入口，不能从本地C++无App锁推断JS事件循环可重入。若证明无独立可调用通道，再按此最小产品seam交developer：外部可读的三槽快照，或独立非GL诊断线程，仅复制Store，固定总≤64KiB、文件/页每≤16KiB、atomic临时写rename、失败留原件/DIAG_IO_FAILED；身份含进程boot+attempt+epoch/context，关闭/重启不把旧槽当新终态。先提供“调用通道确被堵塞”的原件，不默认增加产品落盘/新的开发关闭门。

产品真缺Finish路径则developer补显式每一实际退出分支Finish与首因，不在析构中把取消/死锁自动SUCCESS；当前未见终态原件不足以宣称代码漏Finish。tester先按接口执行，缺证准确回architect。

续作prompt：不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外）或滴答同名清单。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。具体阻断列未完成、原错/复现/证据/尝试、不能自行继续原因、前置和接手角色。开发只开发编译且不使用真机；tester负责现成包签名部署/执行，真机串行。GPU驻留优先、CPU停投、新XComponent+shader唯一上屏；失败FAULT/显式STALE，禁Canvas/软件GL产品回退。目标10暂停，水线13→14→12，旧湿M1暂停。
