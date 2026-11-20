# T684 · T496 双臂完整动作与有界证据合同

2026-11-20，architect。不使用真机。**T496 的首个已证阻断在 host 夹具的业务调度和记录预算。** V4 OFF 在120s只送46/52动作，原件有80次接受观察与78份实际completion。32MiB trace逐行用到33,554,177B，最后只有调用51327的ENTER。退出0、排空账为零和已接受前缀都不能补齐余下动作或两个completion。保留 C0/C1/C2 启动守护、短 D2 的真实 RESERVED→ARMED→FROZEN 窄证。完整双臂和原32cycle仍未测。

## 证据成本与调度

对 fd0b8401 V4原trace计数得到102733条。411 dispatch占10,426,020B，412 barrier占5,396,436B，15/16/17的EGL当前对象读取共11,907,997B，416 fence poll占3,300,854B。重复式 JSON envelope、dispatch shadow、轮询标签大量占用。各调用的顺序、参数、返回、tid、native时刻都要保存。按当前已捕前缀做字节预算，32B定长行与无损字典文本（completion转回1024B真字节）总8,565,137B，未计容器开销。该估算只覆盖现有前缀。

现 `external_source_diff.cpp` 每阶段可在App锁中直接拼串/写，`gl_sink.inc` 每条落盘都fflush并重复转义 shadow。业务循环用同 stamp 反复等待全部 idle，甚至会挡住必须在 owner 忙切点送达的动作。脚本两臂仍固定 **52=48 pulse+2 real SOURCE+2 capture controls**，以原 `BuildScript` 的 actionId/declared stamp/seq/32-U32 payload/真实操作结果为权威。唯一实验变量是原两个 capture control 的 OFF no-op / ON 真控制。SOURCE保留外线程生产和忙态正控，使用T494原已允许的真实 checkpoint 锁外交接。后续调度以 owner 发布的 checkpoint 序号触发，匹配各动作真实指定切点；只对需要等待真实资源/active quota/Fence的后续组暂缓。每次等待都记录 actionCursor、pending、activeRemaining、mailbox、inflight和 wall 时间。各组并行职责不变，协调器不持scheduler锁再持App锁，不让I/O“完成”变成业务许可。same-stamp Pump/Poll 正常运行，真实业务进展优先；不补 pulse、改 stamp、刷 deadline 或跳动作。D1 wall≤120s、drain≤30s、资源≤240s、外层420s（360s请求结束、60s清理）保持；取交集，不能把分段上限相加后超外层。任何门到时保首个未送动作与终态，而非退出后认排空。

保 actionId 7/29 的两源、19 的 RESERVE 和43 的 END 同原一组 pulse 的顺序，首组前等真实绑定/Allowed。动作与 checkpoint 绑定表固化在 tester 清单，两臂匹配同一实际 owner 业务状态。先等到可作真实忙态送达的 checkpoint，再由外部生产线程递交该组 SOURCE，保入队、owner 消费与原结果，避免对这些组执行默认全 idle gate。其它 pulse 只等冻结的业务里程碑，避免照旧把输入一口气灌满额度。任一目标忙切点未命中，记录该组、已到 cut 与完整前缀并结束该见证；不得重打下一组 pulse 为它制造 cut。

## 冻结 T496 tester 记录格式

在现有冻结 T492 host 夹具修 sink/runner，实际产品与 GL 原函数顺序不动。每臂新进程/新 sink、同一个PE及模块/source manifest、同初始输入/缓存条件。每 arm 32MiB 硬信用，预算初始分区：12MiB API记录与字典、16MiB root state/event/completion、4MiB schema/动作/业务/索引/首错及封口。分区先保这三项必要证据，实际达限报告各区计数和剩余字节，既往原始 stdout/stderr/minidump 按原独立预算归档。

- 版本化 LE 行32B：seq U64、steadyNs U64、OS tid U32、functionId U16、edge U8、flags U8、detailId U32、stampId U32。完整 run/arm/PID/start/attempt 与真实模块等放头部，stamp 字典同时保脚本时间及真实 wall/transport 时钟，不覆盖旧含义。原 ENTER/RETURNED/EXIT 与父级 ActivePhase 现有映射保留。大 payload 用长度、原SHA及独立字节段；保存完整参数/返回、原错、shadow各字段，字典按 hash **并比原值**去重。原完成体1024B按binary存一次，通过精确root键引用。保所有实际调用，禁止采样后将缺行叫重复压缩。
- 用有界预分配 ring/页及写入线程在热路径外写块；生产者只登记行/原参数，无阻塞 I/O、每行格式化或fflush；崩溃可读 committed watermark、最后活动边界及已有段。页校验长度/序号/原字节SHA，文件尾保存拒写首序和未封原因。字典/ring/段耗尽或尾损坏明确 INCOMPLETE，绝不覆盖、丢调用或静默截 detail。落实前用已保存前缀离线做双向恢复比对，必须逐条还原语义与全部原参数/返回，回读格式可导出NDJSON。
- 实际accept不能仅按同名字段或行数配对。键至少含本地 scope、root kind/nonce/frame/control、source/physics commit、bank/publication/eventArena及序号。原CPU map必须由实际 GL target/offset/length/access、绑定对象+generation、真实指针返回和 unmap 成功锁身份。取 accepted 的原 `AcceptedRoot.completion` 与 map交叉核1024B；析验w[27],w[51..54]和scope。不能按 `callSeq.load` 猜编号、按数组位置对齐或以同大小的读误认 completion。主状态32项环只作定位，不作为80+个完整结果库；在真实生产的接受时点固定存证，逐项检查 ledger/token、动作、根/拒绝和尾账。

## Bank/Event 安全出口与两套见证

冻结 T492 的 `FullM1Transport::Capture` 与 `SourceCoreTransport::CaptureAccepted` 要求 frozen/owner/current、无 active/fence 且 idle，并会做 barrier/map。事实：**没有现成的 live accepted-root bank/event 读取出口**。App evidence pages只有 ledger/control/display，短D2终态页及以后 root 的最新 bank不能代替每根字节。也不能在ON业务臂对每根 Freeze、加同步读或改 owner/lease 门。

1. **主 OFF/ON 无扰动见证**用同PE、同源、同调度及同观测 sink，两臂都不新增GL readback。每根只保实际已有completion/accepted/state身份键、event边界、原protocol token与时序。完整动作、非空两类root、逐键一致、真实 drain 及捕获差异合同全部满足后，只登记该层结果。
2. **逐根原字节证明**需要增加一个只在 host 冻结夹具编译的实际 owner accepted-boundary 观察接缝。先工程接线后由 tester 运行独立 OFF/ON evidence replay，数据不能冒充主臂原件。沿 T492真实 Run/FullM1Transport（loop局部）在 accepted 实际完成、上一个 fence 已成功归还、owner还持对应context且下一根未获复用前，可用只读冷 observer。developer仅交工程接线和编译。复用既有 Capture 底层读取校验或隔离的 host helper，原 freeze 准入和业务热路线保留，不能把公共 `Capture` 改为任意调用。hook默认缺省、无 App 锁等待、无每根额外 fence/flush/pulse，不从第三线程读 GL。拒绝/撤权/未 signal 不读，不续原 root deadline。两臂用相同hook和预算，单独报告它产生的读回耗时。
3. owner helper只读同根最终接受 bank、实际 source.events 或 quantum eventArena，也要验证当前 owner/candidate bank、已接受 token、n=completion[53]≤capacity、eventStart≤n、word/byte乘法、映射长度和unmap原返回。按每根真实 event prefix [0,n) 存原字节，重复前缀可无损字典化。没有 event 时仍存0长度证明。任一读失败或超过16MiB记具体root和缺项，禁止用摘要代替需要的字节。以两 replay的原字节检查逐根 state/event/completion等价。replay要另与主臂对齐**完整**动作、accept gate、scope的显式角色代映射、order/nonce、实际completion、ledger/token和终态，保持原始身份。发生任何偏离或无法证明 replay 关联，端到端结论保持 NOT_MEASURED。离线读取有单独的干预和时延记录。

tester可先交付无损 sink、动作调度、原有completion映射与单臂失败完整留存，和 host-only 原字节接缝分别推进。新预算或时间仍不够时提交真实瓶颈时点和分区使用量回 architect，不能以分段重放补主臂丢项。32cycle使用原单独矩阵与资源/IO/pass边界，短D2和一次完整D1都不能代替。无真机分派、无driver归因、无新manager门。

要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（本人状态有限例外）。GPU驻留、CPU停投；host ANGLE只用于夹具，产品仅XComponent+shader。
