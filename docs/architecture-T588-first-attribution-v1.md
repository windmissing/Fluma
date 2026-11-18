# T588：M2开放Create区间与外部声明缺证

2026-11-18，architect，不使用真机。输入T461-empty-raw-v3、4778B native/719B manifest和T521/T547/T561实际域，证据 `analysis-T588-source-evidence-v1.json`。原0B raw导出兼容成功只关闭该缺陷，不推Create完成或设备物理通过。

## 原件直接结论

manifest/native/三份空raw的SHA和大小逐字核准，signed T579-m2fix HAP/包内ELF与外部delivery.json一致。native不是“完全没有Create”：boot1/epoch3/candidateRun1、真实window/ownerTid/EGL/context、limit query与SOURCE_V3创建过程已经在档。

SOURCE_V3 COMPILE从158321408945783到158325621555002，**4212609219ns≈4.213s且记录success=true**；随后CREATE_PROGRAM/ATTACH已返回，当前LINK enter158325621617971、exit0、inCall=true、inCallDurationNs41149887187。nativeEndNs0、sealed=false、firstFailure=null。code0是未封区间中的默认值，既不是Create OK，也不是已证shader/物理错误；当前success=false因尚未有exit不能冒失败结果。root NO_TRACE与Create已有API原件是两个域，不能用NO_TRACE抹去实际编译/链接。

object0 compileLog queried=true/reportedLength0支持“该编译日志为空”；linkLog queried=false，object1/2未进入，其0B文件仅导出容器占位，不支持“LINK查过且日志为空”。本轮只能定位**Create仍在LINK开放区间**，未证driver永不返回、OOM、进程死亡、shader拒绝或后续root成本。Create预热与250ms物理根期限也不能混减。

## T591现在可直接完成的输入

不需产品改动即可追加**独立外部收据**：原签HAP完整SHA/字节、包内arm64-v8a/libgrsm1.so SHA、安装包/加载库核验方法与原输出、设备/OS/build、进程PID/boot、采集单调时间区间、native文件SHA及epoch/window/context/candidateRun。delivery.json已给m2fix HAP `a332c0a1cd541e586a42984dc33bb353486c3852ec87b567b8a95606d13c44b3`、ELF `04b98996346e2706fd54e6d3da5e91fefd028d5180c408fb231473554f3c559f`，本轮只核文件，不冒独立证明现场进程确载该ELF。

m2PackageDeclaration由COLD_WANT_EXTERNAL_RECEIPT留空是既有意图；**保旧manifest DECLARATION_MISSING，不填假包声明**。新external.json以原manifest/native SHA为link，保存实际安装/加载原件与映射证书，不能只把签包路径当device加载证明。已有SDK/hdc/test签包设施的执行由tester直接做，不等developer把编译时常量写进业务。

T591继续同attempt二次只读Create诊断：核bootSerial/epoch/candidateRun/window/EGL与首件完全一致，保存采样先后区间和current enter/exit/inCallDuration/terminal。LINK若返回，保实际status/code/rawGL/link log queried/length/原bytes及后继program阶段；若仍挂起，只报开放区间的下界和采样事实，不把轮询时刻写成API exit或擅自终止再称同attempt。进程重启/窗口或context换代必须新attempt，不拼接旧LINK开始和新Create结束。

源码证据不足时缺项应精确到“没有独立可查询的同attempt terminal/stage字段”或“原API日志不可取”，再交architect修必要观测，不凭缺code派物理返修。若有平台级崩溃/阻塞栈/GL错误实证再分流driver/生成shader/资源，不能先抛给designer。

## 前置纠正

T571是V5 FULL冷谱系，不是M2 Create前置；T496离线Create设施与这次Maleoon真机LINK不是同一scope，能直接复用已交设施但不能以离线成功/失败替现场结果。T561字段/结构交付不保证当前Create完成。**当前真实前置是T591取回同次开放区间后继/外部加载收据，不是再等V5代码**。

工程合同本轮修订为：Create端口持续保Begin/current/firstFailure/End/sealed分别有语义，终态原API返回后才封；快照用短锁copy、序列化/文件锁外，不为导出调用GL或改firstCause；raw0遵循SHA256(empty)且区分未查询/查询为空。既有输入均已能表达这些事实，尚无足够证据要求新产品修改。采集JSON仍受原2MiB writer/4MiB信用，超限完整INCOMPLETE，不抬期限/GAP。

接力执行完整遵循 [共同约束](architecture-T581-T589-handoff-policy-v1.md)。本卡未运行Create/设备测试，不改原窄PASS或整体NOT_REACHED。
