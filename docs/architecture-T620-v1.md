# T620：CREATE_WAIT 握手与 GL 原件归因

2026-11-19；architect；不使用真机。结论：测试设施握手存在缺口；原报告对sink和driver的排除不成立。交tester修T496原设施并有界续测，不先派产品修复。未复跑三臂或构建。

## 原件给出的事实

消费 test-report-T496-create-only-v5.md、T518/T547/T561合同，test/t496/external_source_diff.cpp、t561_seams.h、gl_sink.inc、gl_forward_sink.cpp 和三臂 `.codex/local-test/t496/*t496v4c-{c0,c1,c2}.ndjson`。stage文件三个均0B，CREATE_WAIT iter是协调线程等待计数，g_hostNowNs固定1000000，不是owner心跳或资源commit。

| arm | GL ENTER/EXIT | 最后一条未配对callId | functionId |
|---|---:|---:|---:|
| C0 | 5959/5958 | 5958 | 411 |
| C1 | 6264/6263 | 6263 | 411 |
| C2 | 6548/6547 | 6547 | 411 |

按runId/arm/attempt/pid/**callId**连接，不能用不存在的callSeq。源表411明确为glDispatchCompute，前一对为eglGetCurrentContext；此前同类dispatch大量返回。因此不是“sink没有注入、无悬空调用”。末尾开调用也可能被watchdog截断，缺kill时刻/调用时长/owner栈/program-role，不能直接证明永久driver死锁。缓存命中只说明相应资源命中，不能排除随后compute/native等待，不能从cache日志推断资源commit。

## 两条独立因果链

CREATE_ONLY用captureCommittedEpoch==epoch判断成功；真实app该字段在资源创建后可写入，但Allowed/READY仍可为false。Resources完成转WAIT_NEW_SESSION，再由RecoverySessionLocked校验serial/epoch/run/page并绑定实际session，PagePulse使Fresh满足，TryReady才能Allowed。CREATE_ONLY分支没有调用RecoverySessionLocked/PagePulseLocked/DispatchActions；提前progress.BeginSession不等于恢复代握手。hostNow固定也不支持真实freshness/timeout判定。

另一个测试阻挡是D1OwnerHook在PumpBegin等待g_permit或g_scriptDone；CREATE_ONLY仅结束时释放该条件，若owner走到此处就被夹具停住。这是**潜在后继阻挡**，当前末尾glDispatchCompute不证明已经走到PumpBegin。资源创建卡点和恢复后等待不能互相替代归因。

## tester 可执行续作（总墙钟不超过900s）

1. 在新唯一attempt先固定源SHA→编译命令/ELF SHA→实际运行模块及sink identity；启动记录schema、sink路径/open结果、首条已知API ENTER/EXIT，按callId自检。原件保留，修报告附勘误，不重写旧数据。
2. CREATE_ONLY使用真实单调clock，或有明确profile的脚本时钟；后者不宣产品墙钟性能。用独立只读快照分别记录RESOURCE_COMMITTED、WAIT_NEW_SESSION、RECOVERY_BOUND、FIRST_PULSE_ACCEPTED、READY/Allowed和ownerProgressSequence。owner序号由实际边界推进；协调线程iter另名coordinatorWaitIteration。
3. 仅资源commit后读取真实recovery serial/epoch/run/page，调用现成恢复session入口取得返回session，然后真实首pulse/UI fresh条件并DispatchActions；禁止私改Allowed或fresh bool。CREATE_ONLY的D1 hook改非阻塞observer，或协调器对每checkpoint显式ack且独立watchdog；不保留只在退出释放的D1屏障。
4. 先只做一个修正臂，给阶段独立预算，例如启动/资源/握手各≤240s、清理≤60s，整体硬上界900s含join/drain；阶段未推进即采证停止，不用延长等待换结论。只有证明已到相应资源阶段且对比问题仍存在，才按实际缺证选择另一个臂，不默认重复900s×3。
5. 发生未配对411，持续外部采集最后owner进度、当前程序/source SHA/dispatch role和原有GL ENTER，记录watchdog发出/接收/退出时刻、线程栈/等待对象（平台允许的现成只读工具）。sink不插glGetError、额外GL查询、glFinish或改变调度。无栈权限具体登记，不从缺失推导driver已排除。

现有只读seams及hook已足够先改设施；若实际打包版本无所需状态/程序role且不能由现有调用上下文定位，精确列缺失字段及阻挡调用原件，回architect按最小固定容量seam交developer，不直接泛派产品故障。新测仍不承诺READY、创建成功或性能PASS。

续作约束：tester维护夹具、构建签名部署和采证，developer只在真实产品接口缺口时按工程稿开发编译。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外）或滴答同名清单。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。具体阻断须原卡报告未完成、原错/复现/证据/尝试、不能继续原因、前置和接手角色。真机执行另标真机并串行；本次architect不使用真机。GPU驻留优先、CPU停投、XComponent+shader唯一上屏，失败FAULT/显式STALE，禁Canvas/软件GL产品回退；目标10暂停，水线13→14→12，不推进旧湿M1。
