# T496 / T657 接续离线报告

2026-11-19，tester，不使用真机。host ANGLE 是本卡诊断环境，不是产品软件渲染回退，不代表设备、物理或性能 PASS。产品源码保持冻结 T492 单源，不修改 Allowed、fresh、250ms、SOURCE 或捕获门。

## 已交付设施

`test/t496/build.cmd` 保留非空 FullM1Transport 双 TU 布局一致性门，所有产品头/cpp 使用 t492-m1-delivery。增加 NOMINMAX 修复 Windows 宏污染。严格具名模式 C0/C1/C2/D1_OFF/D1_ON/D2，无参数和未知模式非零拒绝。`run_bounded.py` 每次新 attempt、新进程、新 sink；真实 Popen handle wait、parent/child PID/startTime、PE SHA、argv/cwd/PATH、实际载入 ANGLE/EGL/GLES/D3D 模块 SHA，单臂总420s（360s请求结束，60s清理）。缺 sink/active 5s立即停，不默认900s或三臂串跑。

GL 固定表 shadow 仅观察原参数/返回：实际对象创建/删除及代、ShaderSource length 拼接 SHA、Attach/Link/Use、同进程 program binary 与源 SHA 映射、role、XYZ、SSBO绑定及分配字节、uniform。补动态 eglGetProcAddress 的实际调用入口，避免函数指针绕过 wrapper。表满明确 complete=false；observedOnly 不冒真实GL状态核准；不增加 GL 查询、同步、错误消费、flush。ActivePhase 文件映射采 ENTER_EMIT/IN_API/RETURNED/EXIT_EMIT，含 call/function/ownerTid/steady ns；不是UTC。

每 child 异常处理保存真实 stderr、异常码/地址/tid 与单份 MiniDumpNormal（上限64MiB），无全局 WER 修改。`read_dump.py` 读取原 dump 的异常、模块与原始线程栈/context SHA；不猜符号栈。原 trace 上限32MiB；原始 stdout/stderr、父级 wait/kill、ActivePhase、bootProbe、最后API均随 attempt 归档。CDB 未安装如实 NOT_AVAILABLE；不从进程消失或API编号411推driver崩溃。

C0 接真实 steady clock，实际资源 guard 成功才标 RESOURCE_COMMITTED；调用现有 RecoveryRequest/RecoverySession/首 pulse/TryReady/Allowed 链。D1 采用 T494 §5.1允许的显式固定 native/UI 时间，真实 wall/transport clock 另列。独立 mutex/cv 锁外交接，不塞源到 Wake；SOURCE 原32word及SHA入日志，初始绑定 pulse 后业务 seq 连续。修复脚本重复时间 pulse、相对 sampleNs 和排空递归锁；排空与同 stamp Pump/Poll 有界，不增加pulse。

D2 独立控制线程在实际 wrapper 的 BEFORE_ORIGINAL_GLFLUSH 且 captureRootBusy 真实为1时调用生产 Reserve/Read；20ms 交接上限，标签不冒 driver 内部等待。保完整 request 原JSON及终态/安全点/身份，不能用无SOURCE解释没有真实cut；只有真实FROZEN才成功。

## 原始过程与有限结论

- t657r1/r2：首次资源240s门超时，child正常退出1，总306.641/303.594s；不能称dispatch硬崩。r2已捕到实际FINAL_QUANTUM源/绑定/XYZ，退出原件与旧API编号不混用。
- t657r3：补实际RecoveryRequest后，真实资源约40.077s提交，RecoverySession=2，FIRST_PULSE_ACCEPTED，Allowed=true；正常退出0，总40.375s。证明本次C0握手，不证明D1/D2或产品指标。
- t657d1off：无owner checkpoint，5s guard；补已有Wake观察交接，不注入业务源。t657d1off2真实 SOURCE ENQUEUED_NOT_ACK，未提交；异常 stderr=0xe06d7363/tid2256，dump96696B、17线程，地址属于本dump的KERNELBASE.dll；wait code=0xc0000409。发现测试排空递归mutex后修复。此证据不支持driver崩溃归因。
- t657d2a：无真实cut，NOT_MEASURED；首个真实pulse间隔34.627ms超过33.333ms后撤销，与D1 SOURCE独立。
- t657fixedoff/finaloff：显式脚本时钟后抓到 PULSE_CLOCK_REVERSED，修动作表两处重复pulse时间，未改产品时钟门。
- t657monooff：误在link完成前启动旧PE，link报LNK1104；按PID/start核对终止自有旧child。operator-abort.json标明无产品结论。之后严格先成功编译再启动，不当成功复跑证据。
- t657monotonic：真实SOURCE入队但 accepted/source/physics 均0，phase/permission=GPU_PROGRESS_TIMEOUT，正常退出8，总44.312s。
- t657d2monotonic：真实忙切点captureRootBusy=1/Allowed=1，生产请求RESERVED/WAIT_ROOT，随后CANCELLED/IDENTITY_CHANGED、WAIT_REVOKED_DRAIN、安全点0；正常退出9，总46.235s。保完整request原JSON，未伪报FROZEN。

上述超时还暴露测试动作推进未等业务里程碑的问题，因此追加固定stamp等待真实jobLive/runQueue/mailbox排空；GPU仍走原Pump/Poll，原单次脚本60s实墙钟有界；最终V4在资源≤240/业务≤120/排空≤30/总≤420s内，额外等待实际progress.pending/activeRemaining归零。最新attempt及最终结论见下节，历史错误不冒当前缺陷。

## 自检与复现

`cmd /c test\t496\build.cmd`（成功后再运行）；`cmd /c test\t496\build_shadow.cmd`；`python test/t496/check_actions.py --selftest`。shadow长度SHA/对象重用代/分配/role/uniform退休/容量反控通过，动作漏/多/错序/seq/payload/layout七项自检通过，非法CLI拒绝通过。动作表检查不能替代非空root差分。

`python test/t496/run_bounded.py D1_OFF <new-attempt>`，D1_ON、D2分别新进程；C0/C1/C2同理。最新runner启动前保存 source-closure.json，早期attempt无该启动前源快照，不回填冒原始。每 attempt 的 launch.json/result.json 是PE、模块、退出与时间原件；汇总 t657-attempts.json 只索引，不替代原始文件。

## 最新结论

t657milestoneoff/on分别正常退出0，总85.688/87.734s；两臂SOURCE=2、物理提交=49，完整52动作/seq/payload检查通过。此时terminal activePlanRemaining=4，不能冒全排空或逐root完整差分，因此V4补齐真实剩余额度/排空及原CPU map1024B completion全量观察。t657milestoned2真实忙切点RESERVED，owner safePointSerial=1后ARMED，完整END原件FROZEN/EXPLICIT_END，正常退出0，总107.25s；D2无SOURCE仍明确独立。

**当前T496看评论，首次归因交architect。** V4原始child exit=0不是整套通过：t657-v4-blocker.json联核到120s业务窗口仅46/52动作；完成80 root（2 SOURCE/78 QUANTUM），终态pending/activeRemaining及overload/revoked均0，但trace已33,554,177B，后续没有记录信用；只剩78个1024B map原件而接受见证80个。不可把缺失completion当相同。动作表checker拒不完整表，逐root checker拒完整计数不足；未启动另一同条件ON臂重复缺证。

已修失败退出：SCRIPT_COMPLETE参与子进程返回；32MiB拒写后mapped budget flag通知父级，按实际PID/start结束并保kill/wait原件；正常exit0但超额时runner也失败。budget_units验证拒写、persistent flag及后续拒写通过。t657v5d2真实RESERVED→ARMED→FROZEN，但排空轮询过快触发32MiB，父级按TRACE_EVIDENCE_BUDGET结束，明确FAIL。随后修测试排空的1ms交接间隔，保原GL调用/门/额度。短D2真实FROZEN后停止脚本，仍有界排空真实剩余额度，不添加SOURCE或补pulse。

C1/C2实测正常退出0，真实guard/恢复绑定/首pulse/Allowed通过，总41.046/43.281s；C1实际3次binary load及6次shader compile，C2实际6次binary load及0次shader compile。所有trace行有OS ownerTid。暖binary源码映射另补实际缓存绑定：Store校验wire key/长度/body SHA后，同key receipt与当前CPU已知六role源码SHA逐一匹配，再以实际GL binary参数SHA绑定program；图形双stage分别核SHA。仅证明CPU参数与artifact元数据，不冒GL状态核准；缺元数据保持UNKNOWN。

最新t657deliveryd2已正常退出0：真实忙切点→RESERVED→安全点ARMED→EXPLICIT_END FROZEN，完整分页BEGIN/FROZEN原件、非空QUANTUM及真实剩余排空保留，trace不超额。最新缓存role复核、编译和PE原件见test/reports/t496/t657-delivery-*。

请求architect先裁定完整输入范围和有界trace表示/采集策略：现有完整固定动作+全调用原件不能在本轮120s业务/32MiB合同同时闭合。不能自行延长420s/trace信用、缩表、删GL日志、放宽250ms或以前缀PASS冒整卡。GPU完整state/event逐root比较也需明确冻结owner接口的安全读法：App completion数组仅32项，Run/FullM1Transport是loop局部，EvidenceCapture页为ledger/control/display，不能替代GPU bank/event原件。产品缺口由architect分流，不默认tester改产品或转真机重跑。完整D1逐root/state/event/completion/token无扰动及32cycle仍NOT_MEASURED；不增加manager验收门。

交付PE SHA256 `889ff10c087e3d8d06aff1ed51ef2aacbc5b444ba9307517cb93bd116e29fe1c`：完整build及最后driver增量编译均退出0；短D2总47.344s、trace1,245,511B，两页含BEGIN/READY/END（state=FROZEN），checker联核父级原件SHA、PE/request/分页身份。交付C2总42.266s、六次binary load/零shader compile、六role绑定实测通过；这些是独立窄项PASS。CLI旧非零检查曾遗漏DLL PATH，已标INVALID_CLI_EVIDENCE保原值；补与runner相同PATH后，缺参数/未知模式均实际退出2，不把0xc0000135加载失败冒参数拒绝。

同一交付PE最新空缓存C0正常退出0，总43.11s；真实资源/绑定/首pulse/Allowed均通过。C0、C1、C2守护握手可用，不再沿历史无RecoveryRequest的240s空等。整卡仍按上述完整D1时间/证据信用阻断看评论。
