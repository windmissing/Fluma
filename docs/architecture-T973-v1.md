# T973 · 目标 9 续根、目标 13 编译与 Demo 采证首次归因

2026-11-25 · architect。原件为 T968 `67c2877b`，产品环境为 Intel UHD 630 `8086:3e98`、驱动 31.0.101.2140。三项均已离线首归因。G16 Completion v2 五臂的小域结果沿 T968 登记，连续产品场景仍用各自验收条件。 [T968 报告](../test/reports/t968/report.md) · [独立复算原件](../test/artifacts/t973/evidence.json)

## 1. 目标 9 与 G15 · 续根实际热段及最早拒绝

| 同原 fence GPU，ms | A2-off-r1 | B2-on-r1 | B-audit-r3 | B-ops-r1 | G15 pos-on |
| --- | ---: | ---: | ---: | ---: | ---: |
| 整根，15 个粗点首末 | 1,136.681 | 1,091.594 | 1,190.131 | 1,066.492 | 1,525.944 |
| CLEAR | 31.648 | 30.500 | 35.535 | 25.264 | 23.827 |
| DISCOVERY J1 | 353.044 | 348.025 | 422.799 | 326.716 | 457.619 |
| AUTHORITATIVE | 333.847 | 317.293 | 320.649 | 313.229 | 500.094 |
| REPLAY | 353.103 | 336.713 | 350.933 | 341.181 | 471.365 |
| COMPARE/PACK/SEAL | 55.757 | 49.400 | 50.592 | 50.146 | 62.558 |
| host grant→submit-return | 1.747 | 1.942 | 1.989 | 1.906 | 1.750 |

五次 Q 的业务首停均为 `GRANT_DEADLINE`，原 fence 返回 `VK_TIMEOUT`，`tailRead=false`，同 nonce/fence/gen 的迟到收尾确认后才读 GPU 点。粗点以 36 位回卷和 83.333 ns 周期重算。A2/B2 是唯一可见完整等价对，GPU 差约 **45.088 ms**，只是该对诊断差。审计和操作细点位于同粗跨度内。B-audit-r1/r2 只有 SOURCE 超期，实际 `422.799 ms` 属 **B-audit-r3**。B-audit 29 点、B-ops 81 点不能用所有 span 求整根；表只取 15 粗点，也不可把两种细档的值合到一个根。 [T968 原日志](../test/reports/t968/runs/goal9) · [T961 前一轮成本及审计口径](architecture-T961-v1.md)

- **固定开销有证，主要预算仍在有效微步。** 单扫 B2 三个活跃段约 1,002.03 ms（91.79%），其余约 89.56 ms。若保持该非活跃成本，250 ms 扣当次 host 提交约 1.94 ms 后，只余约 **158.50 ms** 给三活跃段和读回校验。J2–32 的 DISCOVERY 单项约 0.18–1.99 ms，SELECTED 1.41 ms。审计细档 OP32 正式微步三段 22.84–24.28 ms，OP33 对应三段 46.71–50.52 ms，OP42 24.88 ms。操作细档的 DISCOVERY/AUTHORITATIVE/REPLAY 同一微步分别为 OP25 **97.06/88.71/91.95 ms**、OP27 **61.74/60.08/61.31 ms**、OP21 **53.15/43.86/45.30 ms**。OP16 3.37–3.98 ms，OP23 3.75–4.43 ms，OP24 0.13–0.15 ms。审计仍实质占时，首个继续优化按实采最大 OP25/27。 [原计划和细点](../pc/vulkan/sand/owner.cpp:832) · [面通量实现](../entry/src/main/cpp/gpu_resident/g9dm/pipeline.comp:1492)
- **Developer 接最窄面算术对照。** OP25/27 共用 `g9FaceFlux`，写面结果前原通量估值、压强、床修正和重力使用多次通用 `g9SDiv`，其无符号实现固定 128 位 restoring loop。先在 25/27 现有物理入口给 `g9FaceFlux` 与后续 `dmPredictor`/`dmPredictorEvents` 独立时点或窄离线函数剖面，优先单变体只替换 `g9FaceFlux` 中**正数 2 的除法**。在符号幅值表示取 `q=mag>>1,r=mag&1`，沿原 `g9FromMagnitude`、平局奇偶和原失败顺序；第二步有证再考虑 `D=120·J·2^20` 的整除高部和回拼原余数、一般窄除数或逐面并行。面、列、最终遮蔽的原三段依赖保留。GPU 设施需要 wider limbs 时回原算术，保零流携压强、镜像壁、q/jp 及双侧同一面账。Tester 先用 T964/T953 冻结 J1 和能杀死正负半值、舍入、合法零库存/干湿边、坏范围/首差的负控，保持 2404 逻辑和三个独立 scope。再各自量同 PE/同种子对照。单项优化不能独立关闭原 250 ms 门。 [原整数除法](../entry/src/main/cpp/gpu_resident/g9dm/pipeline.comp:945) · [原 q/jp 与列依赖](../entry/src/main/cpp/gpu_resident/g9dm/pipeline.comp:1504)
- **G15 四臂首差按 rootKind 纠正。** `neg-off`、`neg-on`、`pos-off` 都只有 **SOURCE**，12 派发、submit=0、原业务 vk=2、`TAIL_NOT_READY`，grant→首停各 **263.477 / 252.952 / 256.267 ms**，没有 Q 被提交。`pos-on` SOURCE 于 232.287 ms 业务接受，之后实际 Q 才发生 2404 派发、vk=2；粗点 1,525.944 ms。`inventorySingleScan` 标记与旧修后正向 Q 的诊断可保。三份未提交 Q 的零打印首错没有物理含义，也无正负 × 开关的实际 Q 等价结论。SOURCE 未及时是三份首错，现有无 SOURCE 操作细点，分不出冷队列/真实计算/主机等待。**Tester** 可先纠正 verifier 的先后资格及只收到 SOURCE 负控。**Effector** 用现有同 PE、同输入的合法新 grant 分别抓有资格 SOURCE→Q；保真实同 fence 和 tailRead，给 SOURCE 分段与早期冷/预备对照，每臂首个真实阻断按原记录。用 T959 已有晚尾作物理参考，不能补新臂资格。 [四臂原根](../test/reports/t968/runs/g15) · [T964 干湿原字](test-report-T964-v1.md#g9-单扫)

## 2. 目标 13 · 角色资格与私有投影分别推进

### 编译角色

两滚 T968 的 `OP55_58_INSTALL`、`OP56_59_AUDIT` 在实际 `vkCreateShaderModule` 返回成功后停于真实 `vkCreateComputePipelines ENTER`。原轮报告最长等候约 91 s 和 121 s。旧 `OP_48`、`OP48_REMAINDER` 的监督收据在同一调用边界未返回；黏性、压力及新 GUARD/COPY/COMPARE 实际通过。能证实的是 **指定模块的驱动管线调用未返回**。两个新挂起单元暂无同次编译器栈证明具体优化器过程。大小只用描述工件。新版 34,626 B GLSL / 225,072 B SPIR-V 的标量 SHA 已现场建管线和执行，说明样本不能按源码大小给出必挂门限。 [资格原日志及旧监督收据](../test/reports/t968/runs/qual-g13)

从原 `role_OP_48.comp` 用同 `split_group` 重切，实际 Vulkan 1.1 SPIR-V 校验，五绑定、64 lanes、原浮点执行模式，双组逐字复现交付包。单 OP55/58 各 **20,129/91,496 B**（GLSL/SPIR-V），原组 20,709/94,676 B；单 OP56/59 各 **26,615/127,316–127,332 B**，原 27,438/131,536 B；调用指令数仅 `246→241`、`372→363`。四个 singleton 均保原 `drive()` 的宽浮点依赖。AUDIT 另需 `fieldAudit`、`wallLedger(false)` 和 scope 封印。 `--g13-split-source-roles` 在 `OP_48` 角色解析先于 `--g13-split-op48`；原旧回退只作监督对照。 [现有绑定](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc:207) · [试编和原 SHA](../test/artifacts/t973/evidence.json)

**Developer 可立即做两级独立候选。** 先打包 OP55/58/56/59 singleton 的确切 selector＋OP66，沿真实直接资格入口逐个给 manifest、cache/实际 SHA 和 ENTER/EXIT。并行准备更窄的阶段诊断。INSTALL 利用原 `drive → syncAll → sourceInstall` 边界，先做 `drive` 与 `sourceInstall` 两段。AUDIT 先利用原 `fieldAudit→drive→audit(1/2)→syncAll` 之后的确定顺序，接 `wallLedger(false)→audit(15)→scope seal`。原 `fieldAudit` 尾与 lane0 `drive` 之间无显式全工作组尾 barrier。若拆这两段，双故障竞争的第一 `atomicCompSwap(c[514],...)` 可改变。因此最细 field/drive 拆分初版只作资格诊断。随后若需要常规启用，以版本化的明确同时故障优先合同及负控决定，并保原单故障坐标、真实曾失败分支和不得发布条件。实现保持相同 logical 索引、只一次 `c[522]/c[523]` 计数及状态重置，子阶段在实际 scope/descriptor 之间加资源可见性 barrier，保各自热提交、审计 bit、compare、SOURCE 完成回执及状态发布唯一性。把物理派发与 logical 槽分别计量。**Tester** 可立即落 singleton 绑定反射、错 op/错 scope、分段只计一次和 first-error 验证，四个角色的现场资格是 effector 实测前置。正常 SOURCE 线通过须再跑真实 INPUT、16 逻辑槽和后续 Q，资格入口的 OP66 通过只登记到角色。 [SOURCE 原序与各 scope 标记](../entry/src/main/cpp/gpu_resident/g13flow/pipeline.comp:119)

### F02 非零晚根原字可复用，业务发布仍待证

- **解包已完成。** `PC_G13_T833_RAW_V2` receipt 在 `t833QuantumArchive`，`fieldOffset=3376` 是 Completion 1024B 加 14 个 168B 实际 latch 快照。820 是逻辑槽，实走 852 派发。原档七段全 SHA、文件 SHA、Completion 256 字、CANDIDATE digest `w96..103` 与七个绑定分别核。WORK 起点 **597808 B**。`layoutWords[16+6]=49920` 转字节后取 8320 个 FP32 MAC，`layoutWords[16+12]=115936` 取 4096 个 **signed 64-bit Q32.32** 细层 RHS，raw 绝对区间 `[1061552,1094320)`。实际 `storeQ` 每格写低/高两字。T964 检查文件输入用 16B，适配器应先按原 8B 解码和独立摘要保存，再逐值符号扩展到 checker ABI。另按 `work(13)` 取细层 signed 残差。 [归档 writer](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc:919) · [WORK 地址与原字布局](../entry/src/main/cpp/gpu_resident/g13flow/mac_access.glsl:4) · [已复算实际正负臂](../test/artifacts/t973/evidence.json)
- **非零 J1 数值证据。** BASE 的 8064 非零面与 WORK `work(6)` 逐字相同。4096 个 RHS 逐值等于独立制造参考，RMS **2.467153385**；ACTUAL/CANDIDATE 与 REPLAY 的最终速度和压力原字段逐字相同，其他状态身份/提交字分列。4096 个非零压力，压力 RMS 误差 **1.445738e−8**，真实 GPU 残差 RMS 和独立重算都为 **2.165072e−6**，散度 RMS **2.402141e−8**，候选速度 RMS **7.672412e−10**。两独立 CTRL scope 的 J1 micro0 真 OP39 及 16×16-word pressure cycle 记录均封印，cycle **0–7** 各有 68 个 active sweeps，第 7 个首收敛，8–15 原证书 no-op；零臂两侧第 0 个收敛，其余 no-op，真实原 RHS 和输出场为零。T964 实际检测数值门在实采非零候选通过，本轮只有一次有效非零 J1 晚结果。最终 shared WORK 是整根退役后读到的单份残存场，不能代 ACTUAL、REPLAY 各自 pressure 前的独立 MAC/RHS fence 快照。 [T964 原 checker](../test/t964/projection.py:105) · [原 scope trace 布局](../entry/src/main/cpp/gpu_resident/g13flow/trace.glsl:1)
- **业务资格和后续时延。** 非零根 `17293773988017`、原 fence gen1，业务 `GRANT_DEADLINE/nativeError=9` 后原 fence 真退役成功，再做独立 cold copy。原字 completion `(7,0,0)` 和数值证据保留为晚私有诊断，`acceptedRoot=null`、`privateProjectComplete=false`。零臂同类。原同 fence GPU，非零/零 **2581.574/1740.288 ms**，ACTUAL **1107.170/575.109 ms**，REPLAY **958.145/620.519 ms**。已有 seal-A/B 实际同类零场原点，819 SHA 单段 **554.955→45.724 ms**，整根 **1939.964→1298.878 ms**。A/B 只确认现场单对差，净整根收益受两 scope 时间变化，需配对重复。非零臂送验本身仍是旧 SHA，各 scope 比零臂多七个 active cycle，即使省去全部现有 seal 仍需显著降耗。Developer 优先保持标量 SHA 等价证书，再按 T961 对真实非零臂点测两个 scope 的各 cycle/OP47 独立 SHA，优化有界除法与压力细网格同字路径。Tester 现可独立复算冻结原件、适配 64→128 和双 scope trace、零场/错 offset/错代与过期负控。后续直接复用同代四段阶段快照和 typed 鉴别。正式 GridView 保原期限、F03 连续和 F05 动态压力门，G14 不以本卡晚尾放行。 [T961 热点方案](architecture-T961-v1.md#23-819-的真实成本与整体时限) · [实采 F02 原档和 A/B](../test/reports/t968/runs/goal13)

## 3. Demo · 前台前置与显式合成夹具

三份正式保存日志逐行能读，分别 **767/698/722** 行，见当前窗口和 **739/670/694** 条 `M1_POINTER_READY` 的空呈现，均无 `M1_DEMO_SYNTHETIC_INPUT`、`WIN32_POINTER` 或已接受 Q 上屏。T968 提到不同尝试的实时行数，正式归档长度按上述三文件计。`DriveSyntheticDemo` 下按前必须 `pointerReady && compactShown.mapped && GetForegroundWindow()==hwnd` 连续 500 ms，不满足时重置计时并在首个 `Emit` 前返回。原三种前台获取操作失败来自现场回执，已存 program 日志缺该分支逐次 OS 结果，也无法判断每次阻断是前台、映射还是 READY 瞬态。可定位到合成 Down 之前，当前证据没有进入 SOURCE。T966 r4 是合成历史对照，T967 所请求的真输入、见堆和回正仍缺。 [原 Win32 资格边界](../pc/vulkan/main.cpp:715) · [三次冻结窗口日志](../test/reports/t968/runs)

1. **真实交互路径。** 对现有 T967 原包，effector 只用已登录的可交互桌面，用户正常激活明确实例。记同一 HWND、Session、READY、有效当前 View、真实 Down/Up、前台和 capture、实际物理键样点以及外部输入记录。独立执行中心/高位真实静按、四秒量、真实 60 秒后新 Down 实收、松手留存、接受和实画 ±20° 的同粒、回正。保 T940/T950 同 EXPORT/ROI/WSI token 口径，T971/T972 新诊断只在解释空领取时作为配对诊断包。此路可立即交具备会话的人手执行。 [T940 原体验门](architecture-T940-v1.md) · [T970 空领取合同](architecture-T970-v1.md)
2. **Developer 独立显式合成测试入口可离线实现。** 先在原模式补一次 first-failing prerequisite 与超时终态，记 `windowReady/viewMapped/readyStable/foregroundMatched`、本窗口/会话身份以及 `PostMessage` 是否实际交付。若需要非前台的显式景别对照，使用单独标记的诊断构建和启动选项，在本进程 UI owner 的已有 timer 直接将有界测试序列送入本窗口的合成输入入口，身份具名 `SYNTHETIC_LOCAL` 并用不可混用的 fixture hold token。有效 HWND、未关闭、实际 GPU Ready、同 generation 的已画 View 和原源权限、实际 timer、固定姿态、正常时窗仍须通过。将合成 token 校验接到共享的 view 映射和 SOURCE 路径，真实 `DOWN` 继续使用原 capture 与授权入口。合成 token 与 OS capture 分别记，原始人工入口不能拿合成标签或 token 申请真实资格。窗口重建、错 transform、真实鼠标混入、显式 stop 与失败均撤销合成输入及 pending permit，Up 单次、全链有终态。该诊断仅直接操作本进程目标窗口，禁止全局鼠标注入、改前台或改系统会话策略。前台资格缺失时，正常真实模式按原门。**Tester** 离线核入口默认关闭、无跨窗口/capture 冒充、无伪 pulse、无跨 hold 补历史额度、失效及原标签独立，并冻结合成 stationary/MOVE 和 reference 配对。实际画面仍要当前 UHD 630 正常 Vulkan 窗口和同版工件，由 effector 执行。 [T967 原合成模式](developer-T967-v1.md#demo) · [T971 针对性诊断](developer-T971-v1.md)

本卡使用现成 Windows 日志、GPU 冷档及主机 Vulkan 1.1 SPIR-V 静态校验，未打开设备或实际窗口。GPU 驻留，XComponent+shader 唯一上屏，禁止 Canvas 回退。
