# T657：修正创建臂进程退出与 dispatch 证据首次归因

2026-11-19，architect，不使用真机。结论 **PROCESS_EXIT_CAUSE_UNPROVEN / TESTER_COMPLETE_T646_FACILITY**；未证dispatch内硬崩或driver缺陷，不泛派产品日志。正式表`architecture-T657-contract-v1.json`、证据`analysis-T657-evidence-v1.json`与共用只读脚本已交；没有运行产品/测试或延长等待。

## 原件的精确边界

t646cA是run=t496-t646cA/arm=create_C0/pid62736/createAttemptId=t646cA。533行含**267 ENTER、266 EXIT，仅一个未配对**，完整键为callId266/functionId411。报告的“267个未配对”将ENTER总数当未配对数；411是API编号，旧v4c的callId5958等不能据同编号叫同一程序、角色或调用点。

最后有参原件：UseProgram=4；binding0..3依次buffer1/11/12/13；call263同program后的glDispatchCompute已返回；call265 glGetProgramiv(program4,pname34625)有EXIT；call266 Dispatch ENTER只记x=1，**y/z未记**。这些是传参影子片段，不是query核准后的GL状态、有效对象大小/代、uniform值或正确物理输入。wrapper在ENTER持久化后才设置IN_API，返回后设置RETURNED再写EXIT；缺同attempt ActivePhase不能排除日志写入/退出前缀、异步异常或外部终止。

第一到最后GL记录约138ms，是host steady_clock原生ns区间，不是UTC epoch或完整进程生命周期/调用持续时长。日志CREATE_WAIT的wallNs=1000000却标steady_real，CREATE_ONLY hostNow仍脚本；D1已修不能推广为C0已修。stages文件0B，out-cA 0B不能证明实际stderr已独立采到；没有可核child wait退出码/异常地址、parent/child启动原件、watchdog发出/退出账或系统dump。tasklist事后空和未见WER窗口不区分异常、正常退出、外部kill/父任务清理等。

本卡额外只读检查现有host证据（`analysis-T657-host-inspection-v1.json`）：CrashDumps目录不存在，WER ReportArchive/Queue无t496匹配；Application事件1000/1001按t496.exe精确筛到的仅为11月14日RADAR_PRE_LEAK_64/record48339旧记录，不属t646cA。本次未启动任何产品或更改系统配置；“未找到当前异常原件”不等“没有异常”或能反推外部kill。

现有C0缓存收据保SOURCE/INTERMEDIATE_QUANTUM/FINAL_QUANTUM的实际shaderSHA与单调时间，FINAL与尾部时序相邻，值得保留；但无pid/attempt/program名称绑定，不能直接将program4认作已证完整FINAL program身份。source/handle代/真实模块指纹须由既有参数/返回值shadow补齐，不能用相邻时间替唯一关联。

因此撤回“挂起转dispatch硬崩”“同一调用点不同结局”和“已超tester域”的充分性判断。保本轮strict C0入口、sink有记录、最后开放调用及进程后来不在的窄事实；D1/D2/32cycle未到非空SOURCE正控，仍NOT_MEASURED。host ANGLE诊断不等真机GPU产品FAIL。

## 原 T496 tester 必须完成的范围

T646原六步中的GL shadow、实际C0 clock、阶段快照、同attempt退出证据是本次必要范围，不能以scope外删去后再泛派产品日志。修CREATE_ONLY自己的a.hostNow/Emit时域、资源发布真实guard与实际恢复绑定/首pulse返回码；复制D1改动标题不代C0代码落实。原runner900s与450000×2ms循环也须纳入**整体≤420s**（资源≤240、握手≤120、诊断/清理≤60、全部共享总额），不扩大窗口、不先跑三臂。

启动前冻结fixture/单源树/工具/PE SHA、argv/cwd/PATH和实际载入ANGLE/EGL/GLES/driver模块路径+SHA/BuildId或版本；保parent与child PID及创建时间、进程handle，不能后续按PID数字误认复用进程。只操作该child，Start-Process使用WindowStyle Hidden；runner必须真正wait child并保存正常退出码的signed/unsigned/hex，不能仅tasklist空。watchdog保存deadline、kill-request、目标pid/start匹配、kill原API结果、观察到exit与真实code，独立记录是否由collector主动终止。

若真实异常发生，使用现成每child debugger/异常采集工具保存exception code/address/faulting thread、模块与栈的**有界minidump**；没有工具/权限保原始错误，不写全局WER注册表或改所有进程策略。不启动本卡新产品测试；tester后续仅一个已声明修正臂，保持trace≤32MiB，诊断dump单份≤64MiB另列host证据信用；超额保原错/INCOMPLETE，不无限full-heap dump。调试器暂停/栈采样是诊断扰动，不能混报性能PASS。进程已死只读其退出/dump原件，不能拼新进程续栈。

按T646扩固定容量参数/返回值shadow：program/shader创建真实返回、对象代与delete、ShaderSource实际length拼接字节SHA、Attach/Link/Use与实际role、XYZ、绑定/分配大小/代、dispatch相关既有uniform值，原件携ownerTid/context/attempt。只观察原API已见参数/返回，不加glGet*/glFinish/错误消费/额外flush；表满或缺字段具名不完整，不默认state0或成功。对日志ENTER_EMIT/IN_API/RETURNED/EXIT_EMIT和sink锁/IO也取同identity外部样本；实际异常线程可能与last dispatch owner不同，不能预先归driver。

raw stdout/stderr、launch/wait/kill、阶段/ActiveCall/线程等待、最后API及program/source/绑定在同attempt归档并SHA；未知字段保UNKNOWN。已发生开放调用前，可在不改GL调度的现有BootProbe/Submit程序context取大阶段；无新增泛产品日志必要。只有精确证据表明某已进入的产品内部等待无法由现有seam/参数/栈识别，列具体入口与缺字段再architect定义最小产品只读stage，开发只开发编译。

共同执行约束见`architecture-T655-T658-boundaries-v1.md`。本卡首次归因与精确续办已完成，真实退出原因仍待原件，不假产品PASS或driver缺陷。
