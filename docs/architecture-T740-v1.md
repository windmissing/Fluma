# T740 · RX / DM 完整 QUANTUM 角色接续

2026-11-20 · 目标20 · P5 · architect · 离线交付。

完整臂采用 T733 **九程序角色主案**。SOURCE 仍用 T736 的原 12-pass 路径。QUANTUM 使用其中八个程序，按原 2,404-pass 顺序完成整个事务。实施须先有本族 BOOT 与非零 SOURCE 证据。RX 还须在同一完整臂修正 owner Pump 调度，避免固定 2 ms 等待耗尽根期限。DM 的独立入口和真实 XComponent 呈现分别取证。 [T733 主案](architecture-T733-v1.md) · [T736 SOURCE](architecture-T736-v1.md) · [T740 机器合同](architecture-T740-quantum-contract-v1.json)

## 启动条件

- **RX BOOT 现有证据**。T738 原始 G9Boot 日志同 PID 1256/TID 1942/run 1/owner 1/context 1，关联 RX Configure ticket 1 和实机映射 BuildId f23b8bd6…。FINISH_EXIT 返回 37146，随后从 WORK byte 1,166,976 成功 map/unmap，实际两字为 **[0, 6559303] = [0, 0x641647]**，GL 0、EGL 12288，随后 PREPARE_EXIT/BOOT_AUDIT_OK。签包内 DSO 与 T734 字节身份吻合。8 B 尾部已经在原日志，缓存 bin 内常量偏移不用于该判定。当前仍缺 PID start 原件和新进程暖命中。先补真实 start 关联，保暖对作为独立量测。 [T738 RX 原日志](../test/reports/t491/t738/h738rx2.txt#L2188) · [RX 签包回执](../test/reports/t257/T738-rx.json)
- **DM BOOT 现有证据**。T738 的实际 pbuffer driver 返回 1638/BOOT_AUDIT_OK，rootsAuthorized=false。现存 JSONL 缺同域 stderr/G9Boot 的 fence 和真实两字尾，缺本次进程映射。T734 实物中 a64de80e… 属 DSO，637,128 B 的独立 driver ELF BuildId 是 869acf4d…。由同次原 stderr、PID/start、实机 executable/DSO 映射及 8 B 尾补齐；包内或本地 ELF 身份单列。T734 BOOT 源 RX 22,318 B、DM 22,353 B，冻结单体对照分别 146,301/146,316 B。既有结果证明拆分 BOOT 返回，完整臂及历史静默退出的具体原因仍需自己的证据。 [T738 DM 原回执](../test/reports/t491/t738/dm20-boot.jsonl) · [T734 构建原件](../test/reports/t734/compile.json)
- **本族 SOURCE 前置**。沿 T736 取实际 split BOOT 完成后 8 B 原尾、同 scope 资格及成功的非零 K192 12-pass SOURCE。原输入、completion、typed commit、cold 证据同一 token，admitted=209,715 Q20、commit+1、quanta 不变。DM 须走真实 Controller 的零配额提交预检。两族独立满足才进入对应完整臂实施。当前无该 SOURCE 实测，本卡仅离线准备。后续 developer 交实现和编译；tester 管独立 oracle/fixture；effector 负责签装、设备证据和效果。 [T736 实施触发](architecture-T736-v1.md#实施触发)

## 程序、顺序与资源

生成器以真实最终 `pipeline.comp` 与原 `schema_generated.h` 产角色表、绑定表和实际字节 SHA。RX/DM 分别生成，静态原件中原 QUANTUM 的 `(opcode, scopeRole, Jslot, micro)` 逐项相同且匹配 T554 工程表。保 ROOT 12、DISCOVERY 804、AUTHORITATIVE 392、REJECTED_PREFIX_CHECK 804、REPLAY 392。GPU 继续按原 J、首次选优和 guard 执行或跳过槽内物理，native 仍派发全部原槽，marker 也保其通用游标、屏障及宿主操作。所有根沿各自状态、WORK/arena、原 4 SSBO 和 64-lane 组。ROOT_INIT ordinal 0 使用 BOOT，末 ordinal 2403 由 ROOT_FINAL seal。 [T554 固定表](architecture-T554-g9-dm-dispatch-v2.json) · [算法依赖](architecture-T554-g9-dynamic-mobilization-v2.md)

| 程序角色 | opcode | QUANTUM 槽数 | RX / DM 静态 B |
| --- | --- | ---: | ---: |
| BOOT | 12 与 warmup 保留 opcode | 1 | 22,318 / 22,353 |
| SOURCE | 0–11 | 0 | 41,767 / 41,802 |
| ROOT_SETUP | 13–18 | 45 | 33,544 / 33,579 |
| TOUCH | 19–20 | 28 | 43,797 / 43,833 |
| DM_TRANSFER | 21–22 | 380 | 40,188 / 40,224 |
| FLIGHT_FOLD | 23–24 | 380 | 38,221 / 38,256 |
| FACE_FLOW | 25–30 | 1,140 | 47,206 / 47,238 |
| CAPTURE_AUDIT | 31–32 | 380 | 37,588 / 37,624 |
| ROOT_FINAL | 33–43 | 50 | 36,117 / 36,152 |

全角色在真实 Configure owner 上构建、反射、落实资源资格，根 claim 后无 compile、link、cache 等待。各 program 单独存 uniform location，按真实名/类型/元素数和 std430 block 合同验证。每根首用按同一冻结 Permit/Config 设置 Common，后续写原 `uOp/uRole/uJslot/uMicro/uPass`。程序角色和原 `uRole` 是两个独立域。以本 owner/context 的实际 bind 回执追踪 program，首次及异角色时才调用 glUseProgram。未知 opcode、映射或对象不符在 dispatch 前失败。每个生成阶段逐个对完整编译输入 49,152 B 过门。单体只保冻结比较 arm。T475 按实际角色源和原 ABI/后端/驱动配置复用。完整 roleSet 与 per-role cache key 分别记录。跨臂复用只发生在实际 key/ABI 匹配并完成新 context 反射与适用准备时。模块或 key 变化后按实际 LOAD 或源码路径记账。Vulkan 后续对照可复用 opcode/数值/资源逻辑；按后端分别映射 descriptor、同步和缓存。 [T475 持久化](developer-T475-program-cache.md) · [Vulkan 对照](architecture-T731-v1.md)

### 绑定与同步

用冻结对象解析下表，BASE=slot1、SELECTED=slot3、REPLAY=slot4、WORK=slot5、ACTIVE_EVENTS=slot6、SELECTED_EVENTS=slot7。CURRENT 与 CANDIDATE 从实际 bank/permit 取得，AUTHORITY_EVENTS 为 8+permit.arena。默认 B/C/W/E 是 BASE/CANDIDATE/WORK/ACTIVE_EVENTS。

| 原位置 | B / C / W / E | 原宿主 copy |
| --- | --- | --- |
| BASE_FREEZE（1） | CURRENT / BASE / WORK / ACTIVE_EVENTS | 原 GPU 全量冻结。无额外 host copy |
| scopeRole 3、4 | BASE / REPLAY / WORK / ACTIVE_EVENTS | 保原拒绝前缀与接受重放 |
| FINAL_ENERGY_CURSORS，role 2/4（1200/2398） | SELECTED_EVENTS / ACTIVE_EVENTS / WORK / BASE | 仅 role2，在该 pass 后 ACTIVE_EVENTS→AUTHORITY_EVENTS |
| SCOPE_SEAL_AND_CHOICE，role4（2399） | CANDIDATE / REPLAY / WORK / ACTIVE_EVENTS | 原校验 |
| DISCOVERY_COMMIT_SELECTION（808） | 默认 | 该 pass 前 CANDIDATE→SELECTED、ACTIVE_EVENTS→SELECTED_EVENTS |
| COMPARE_SELECTED_AUTHORITATIVE（2400） | SELECTED / CANDIDATE / WORK / ACTIVE_EVENTS | 原比较 |
| COMPARE_REPLAY_STREAM / PACK / ROOT_SEAL（2401–2403） | 默认 | 原 stream 比较、审核与最终 seal |

FULL 另保六个 discovery scope 在 ordinal 23/55/111/215/415/807 的全 arena copy。ROOT 模式每根主机发起 3 次 GPU buffer copy，K192/K512 分别共 26,126,336 / 47,343,616 B，FULL 为 9 次。保持这些时点与原 barrier，并区分 2,404 个真实 dispatch barrier 和 copy barrier。每次 dispatch 或 buffer copy 后沿原 `SHADER_STORAGE | BUFFER_UPDATE`。GPU 物理仍读写原五 state、四 arena 和同 WORK，再沿原授权检查推进 ordinal。copy 从当前 root 的实际 logical object 解析，禁止跨 root 复用旧 GLuint 租约。 [RX transport](../entry/src/main/cpp/gpu_resident/g9rx/transport.cpp#L490) · [DM transport](../entry/src/main/cpp/gpu_resident/g9dm/transport.cpp#L491)

## 250 ms 与成本量测

- 原版每 pass 安装 4 个 indexed binding，共 **9,616 次**。按原表计算的本根相邻逻辑位置变化 B/C/W/E 是 **8/10/0/4**，若中途无失效，4 个初始绑定加 22 次位置变化即可。首个正确实现可沿现有 Bind；需要优化时使用 owner-local shadow，以真实对象/范围/代号为 key，每次成功绑定后才更新。新 root、其他 scene/冷操作改变目标、context/window 换代或错误使 shadow 失效。设备报告实际 bind 数与失效原因。每 pass 仍按表得到完整四槽，SSBO 资格/原 barrier 保留。
- 主案 **802 次**相邻异角色切换。190 次各发生在 DM_TRANSFER→FLIGHT_FOLD、FLIGHT_FOLD→FACE_FLOW、FACE_FLOW→CAPTURE_AUDIT，另 176 次 CAPTURE_AUDIT→DM_TRANSFER。八角色备选调 23–27/28–32，切换 **612 次**，DM 单元最大 48,950 B、仅余 202 B。两组的调度和布局摘要一致。先对主案实测，只有 owner 等待、copy/readback 和原 GPU 负载已分清，证据显示程序切换是根预算主要开销时，才启用八角色同父成本对照。重分组不得越过字节上限或修改原 pass。报告实际 glUseProgram，分别列初始 bind、中途变化及退役。
- 原 Pump 保最多 10 次 dispatch、8 ms 进批检查、现有 Flush 和单根最终 fence。**2,404 次至少要 241 批**。目前 RX App::Loop 的 wait_for(2 ms) 谓词不包含正在提交的 root。若每批间保留该等待，仅 240 个间隔就约 480 ms，已超过 250 ms，尚未算任何 GPU 或驱动开销。它是静态识别的 owner 调度阻断。完整臂增加 owner 私有阶段 `SUBMITTING / WAIT_FENCE / IDLE`，提交期间每批返回后，在检查窗口/Stop/撤权与公平调度后立即进入下一批，不走 idle 定时等待。等待 fence 和空闲阶段沿有限轮询和原期限，避免在 fence busy-spin。每批之间处理控制事件，所有原 permit、33,333,333 ns pulse freshness 和 250 ms 绝对截止保留。DM 真 Controller owner 同样核 cadence，不能用紧轮询 pbuffer 的数字代 App 调度。 [RX 当前 2 ms 谓词](../entry/src/main/cpp/gpu_resident/g9rx/bridge.cpp#L299) · [Pump 边界](../entry/src/main/cpp/gpu_resident/g9rx/transport.cpp#L584)
- 指标从同一个进程上的 Configure→实际 role compile/link/reflect/prepare→READY→SOURCE→首 QUANTUM→首有效 QUANTUM Swap。原根单独拆 root deadline 前的 queue/owner wait、每批 CPU Submit、program/bind、3/9 copy、Flush、GPU fence 等待、1024 B map/unmap、typed commit、scene/Swap。保 raw ENTER/EXIT、tail/first GL/EGL、每轮实际分母及超时归因。常规计数聚合，详细日志采样和 cold capture 在独立采证轮，避免把额外采证开销当产品路线成本。单体、九角色和有触发才开的八角色比较使用同输入、同布局、同类缓存、新 PID/start。持续场景另报 30 FPS、P50/P95 帧时、真实新代画面比例，以及 120 Hz 物理 quota 的 generated/accepted/debt 和超期。初始 pose 稳定只代表首根测例，不能据此推断稳态实时。 [T727 量测边界](architecture-T727-v1.md)

## 提交、首帧与退役

1. **根与证明。** 在本完整变体复用 T736 的实际 baseline 与 SOURCE 入队顺序，先取得本次非零 SOURCE 的真实提交，再以其 CURRENT 为首 QUANTUM 的 BASE。正常 pulse 产生 quota 后原 Claim 取得 kind=2/ordinal=1，使用同族 K192、angleIndex 0、无新触摸，携已接受、仍在床面上方的非零 packet。首根校验同 token packet 的非零物理推进、运动及能量来源与既有事件、状态 oracle。另用含动员的已有用例对照 T556 的 DM event 15 oracle 和 T652 的 bit16/17 负控。一次成功物理量子消耗一次 quota，commit/quanta 各+1。零结果或 q0 的 SOURCE 不能满足该量子正控。独立比较 split 与原单体或同族已可用 oracle 的守恒、来源、动量、所选 J、拒绝前缀和接受重放，按字段比较物理字，动态身份在各 arm 独立绑定。
2. **原子 gate。** claim 时冻结 Permit 原身份、实际 roleSetSHA、实际 ROOT_FINAL 源码 SHA/GLuint，按原 512 B Config 与 512 B INPUT 上传。封样 SHA 对应 ROOT_FINAL，角色集合 SHA 单列到私有 Expected/observation。根内固定 program 表、对象代，每个实际 ordinal 的成功调用、屏障和 cursor 由 owner 绑定。最终 fence 实际变为已信号，继续核同 owner/current permit，从 WORK work[8].begin 读 1024 B，成功 unmap 后才交原 typed gate。QUANTUM 审核掩码必须为 0x3ffff，真实派发/屏障 2404/2404，GPU 的状态、事件、封印和原 guard 全过才晋升候选并发布代号。RX 的私有 gate、DM T647 的实际 program/roleSet 与 Expected 各按 T733/T736 继续。typed BLOCKED 走原配额 blocked 记账和回滚，accepted 不增、不消费命令、不发新有效帧。FAULT、撤权、迟到、坏原件或错 role 不晋升。 [T647 合同](architecture-T647-v1.md) · [T652 独立反例](test-report-T652-v1.md) · [T556 Oracle](test-report-T556-dm2-oracle-v1.md)
3. **同 token Swap。** 在 Commit 后登记不可变 publish 证据，含 kind、nonce、plan、commit、quanta、generation、bank 代、scope/geometry、deadline、shader 与 roleSetSHA。当前 scene token 字符串含 run/context/surface/generation/extent，按同 owner 的 SceneView 与提交原件做明确连接，保公开 token 格式。首量子验收臂在该 generation 呈现终态前暂缓下一次 root claim，按原 33,333,333 ns 出帧节奏及 deadline 出第一帧，避免覆盖证据。原 XComponent owner 使用 CURRENT 的 SSBO/instance attribute、原 barrier、offscreen 合成与同窗口真实 eglSwapBuffers。比较交换前后代与 token、surface/window/GL/EGL 原返回和 scene fence。SOURCE 的较早首次 Swap 可同时记录，QUANTUM Swap 单独取证。失败保标明 STALE 的上一有效帧或具名故障。DM 可复用现有 `Scene::Draw` 与 bridge 设计。完整臂的呈现阶段须在已注册的真实 XComponent owner 接 DM Controller 与同族 Scene，核实际窗口映射。独立 DM pbuffer 的结果只计完整计算根。 [RX SceneView](../entry/src/main/cpp/gpu_resident/g9rx/scene.cpp#L188) · [DM Scene](../entry/src/main/cpp/gpu_resident/g9dm/scene.cpp#L73) · [DM 构建域](../tools/g9dm/README.md)
4. **退役。** GPU root 完成或取消后按原 fence 清退根 flight，独立保 scene lease/fence 及 ROOT/FULL cold pin。带提交或部分提交的拒绝不能把 bank、program 或 event arena 提前复用。冷读保实际 BASE/CURRENT、SELECTED/AUTHORITY arena、真实 scope 和原 lead/capture 证据，期间不抢 owner，保持原 credit。Stop/换窗口/重配置先撤权限再 drain，旧 context 不发当前 context 的销毁命令。原 owner 安全终点按 program 先 Forget 后 delete，旧 context 走失效路径。取消、超期、回滚、角色/绑定或 token 错配都留实际失败阶段作为负控。GPU 驻留优先、CPU 停投，唯一上屏路径是 XComponent+shader。

离线复核命令为 `python tools/architecture/analyze_t740.py`。本卡未实施完整臂、未使用真机。
