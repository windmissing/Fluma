# T746 · RX BOOT 资格与非零 SOURCE 实施前置

2026-11-20 · G20 · P5 · architect · 离线交付。

**RX 的 BOOT GPU 原尾冷暖各通过一次，12-pass 非零 SOURCE 的输入和着色器角色已离线锁定。按 T736，正式实施放行还缺一次 RX BOOT 的同进程启动 tick。** 优先找回既有进程原件。无原件时，effector 复用 T738 签包，在一轮 Configure 中同时保存 PID/startTick 和原 BOOT 链。补齐这一项即可按本族证据放行。DM、G13、暖缓存精确命中和完整 QUANTUM 各沿本身的合同推进。 [T736 实施触发](architecture-T736-v1.md#实施触发) · [T746 原件联核](architecture-T746-evidence-v1.json)

## RX 实际包和冷暖原件

签包 SHA `73027d13…` 与 T738 验签回执一致。签前和签后 ARM64 `libgrs_g9rx.so` 字节相同，BuildId `f23b8bd61d2cfb3969eaf78e9e4ea988cbef5047` 对齐两轮实际加载日志。ELF 中有一份与 T734 生成物逐字节相同的 22,318 B BOOT 源，SHA `8e0a89e6…`、`roleSetSHA=6237d612…` 与实际编译/加载日志一致。 [T738 签包回执](../test/reports/t257/T738-rx.json) · [T734 构建原件](../test/reports/t734/compile.json)

| 原始日志范围 | 同进程作用域 | Configure 入队到 BOOT 成功 | BOOT program 构建 |
| --- | --- | ---: | --- |
| 冷，837–869 行 | PID 43774，owner TID 44158，ticket 1，run/owner/context 1/1/1 | 637.261 ms | 463.020 ms，含实际编译 176.141 ms、链接 281.435 ms |
| 暖，5716–5759 行 | PID 45505，owner TID 45618，ticket 1，run/owner/context 1/1/1 | 186.281 ms | 15.363 ms，原日志没有应用侧源编译或链接事件 |

两轮均见 `ROOT_INIT → WARM → fence 完成 → MAP → UNMAP → PREPARE → BOOT_AUDIT_OK`。fence 原返回 37146（`GL_ALREADY_SIGNALED`），GL 0、EGL 12288。`WORK work[7].begin=1166976` 实际 map 8 B，unmap 成功。原字为 `[0,6559303]`，seal 十六进制 `0x641647`，两个 little-endian 字编码为 `0000000047166400`。以原 UNMAP 字为准，修正部分摘要中的十进制转述。同 PID/TID、单次 ENQUEUED 到终态、原 run/context 及 BuildId 能绑定每条 BOOT 链。系统另有旧 PID 43774 的退出记录（1668 行），随后由不同 PID 45505 新建 ticket。现有原件无这两条链的 startTick。 [T738 v3 原日志](../test/reports/t491/t738/v3/h-warm.txt#L837) · [实际 8 B 审核](../entry/src/main/cpp/gpu_resident/g9_boot.h#L182)

暖轮的成功 Build 和已安装的源编译观察器支持应用走 `BINARY_LOAD` 分支的推断。驱动两行 `Shader compile fails` 原警告留存，其后实际 Build、反射、fence 和 BOOT 均成功。较早的 T475 原件有 `97efd53b…` 持久化和 payload SHA `6c916488…`，离线复算 37,965 B 有效载荷相符。v3 暖轮缺自己的 load 时点、key 与实际加载 binary SHA，当前只登记 BOOT 暖时长和代码路径推断。正式冷暖统计须在同次收齐实际 cache route/key/format/binarySHA 和 init 审核。 [暖轮原日志](../test/reports/t491/t738/v3/h-warm.txt#L5716) · [较早 T475 回执](../test/reports/t491/t738/rx-cache/97ef.jsonl) · [真实 Build 分支](../entry/src/main/cpp/gpu_resident/program_cache/program_factory.h#L262)

补 startTick 时，在下一次授权设备窗口中，于本次 PID 存活期间保存 `/proc/<pid>/stat` 原行及第 22 字段，连同包 SHA、实际映射 BuildId 和同票 BOOT 日志归档。外部读取受权限限制时，复用 `/proc/self/stat` 的现有解析，把结果带入 RX 同次启动/BOOT 诊断，同时保留读取失败原错误。单轮足以补 SOURCE 前置，已有冷暖内容可直接引用。 [现有读取实现](../entry/src/main/cpp/gpu_resident/g13flow/boot_qualification.h#L31)

## RX SOURCE 最小实施臂

1. **明确变体和实际程序。** `BOOT + SOURCE` 保留同一次合格 BOOT 的 GPU 资源，完成 SOURCE build/reflect 才允许原 Configure `APPLIED`。当前 T734 的 `BOOT_AUDIT_OK` 为终态。使用实际 T720 已签包父源重生 SOURCE，离线核得 **41,767 B**、SHA `ee66f5f2…`，原 pass 表 SHA `290ed82f…`。glslang 实际反射为 40 个 uniform 和 SSBO 0–3，1 组 × 64 lanes。实现按实际组装字节执行 49,152 B 上限，分别登记 BOOT/SOURCE program、真实 GLES 反射及同一 `roleSetSHA`，该变体重新记录缓存身份。保原算法、布局、同步及 oracle，沿 T731 共用可移植的输入和 pass 合同。 [T736 角色合同](architecture-T736-v1.md#角色反射和绑定) · [T731 后端工程](architecture-T731-v1.md)
2. **原入口冻结真实正输入。** K192、GNR1 profile `827477575`、INPUT `1481788999/rev3`、T641 S40、初始中性角、无前序根和触摸。用实际上传 591,872 B seed SHA 和 Config64 SHA `f7c133d2…`，模板 SHA 只供对照。原页面命令 `sourceId=2,mass=0.2,x=64,y=200,vx=0,vy=0`，首次有效 intent 由页面生成。原 NAPI 冻结五个 LE binary64 和 words74–76 `[2,1,63701]`，其余字段使用实际 session/epoch/receipt、nonce、几何与时间。本例 `0.2` 位型 `0x3fc999999999999a`，父规则给出 **209,715 Q20**。 [原页面入口](../entry/src/main/ets/pages/GpuResidentG9Rx.ets#L122) · [T641 配置和模板](architecture-T641-seed-config-v1.json)
3. **通过正常时钟取零配额 SOURCE。** 实际 scene/geometry 合格后 `SetActive` 并刷新实际 session，在首个合格 pulse 前提交原命令。首 pulse 建 baseline，按原 owner 流程在 quota 入账前检查 `pending=0`、fresh、队首和中性角，只放一次 kind=1、ordinal=0。`ClaimLocked` 前保同锁检查，`Transport::Begin` 核同 kind。若 quota 先到，保留真实原件，按原 Stop/retirement 重开 scope。沿用 33,333,333 ns freshness、250 ms 根期限、每 Pump 10 pass 和原 8 ms 检查点。 [RX 时钟](../entry/src/main/cpp/gpu_resident/g9rx/progress_clock_v3.cpp#L52) · [T736 admission](architecture-T736-v1.md#source-only-admission-和-dm-退役顺序)
4. **按原证据原子提交。** 使用 T736 原顺序 op0–11，op1/8/9 用原 B/C 覆盖，op0 前一次 state copy，op9 后两次 arena copy。核 12 次 dispatch barrier 和另计 3 次 copy barrier。真实 fence 结束后，同 permit 从 K192 `WORK work[8].begin=1171072` map 1024 B。completion `[39..41]=12/12/12`、`[42]=0xffff`、`[62]=209715`、`[80..87]` 为实际 SOURCE SHA，附同根 program/pass/roleSet 收据、原 cold 区段和 `ROOT_BEGIN → SOURCE_BIRTH → ROOT_END`。原 oracle 预期 slot0/packet1/generation1/origin2，x=67,108,864 Q20、y2=419,430,400 Q21，质量 5,368,709,120→5,368,918,835 Q20，outcome1、commit+1、quanta不增、pathCount=0。reader 按版本核实际 role 与 shader。复用 T736 的重复、q0、错身份、撤权及迟到负控，按原事务规则验证队列和 bank。 [原传输](../entry/src/main/cpp/gpu_resident/g9rx/transport.cpp#L402) · [原 pass 表与机器预期](architecture-T736-source-contract-v1.json)

developer 按上面的接续合同开发并编译，tester 准备原 oracle、reader 和签包，effector 执行真实设备验证。若同 SOURCE commit 可呈现，记录实际 XComponent Swap 并注明物理时间为零。完整 QUANTUM、持续 30 FPS 和 G20 首次有效 Swap 仍按既有条件测。GPU 驻留优先、CPU 停投，唯一上屏为 XComponent+shader，禁止 Canvas 回退。 [T740 QUANTUM](architecture-T740-v1.md) · [T741 G20 计分](architecture-T741-v1.md#g20-完成度口径)

离线复核已通过 `python tools/architecture/analyze_t746.py .codex/local-test/g13-glslang/bin/glslang.exe`。本卡未执行真机 SOURCE。G20 仍约 **33%（4/12）**，RX 暖轮实证仍在 BOOT 阶段。
