# T733 · RX / DM Shader 拆分实施准备

2026-11-20 · architect · 目标20 · 离线架构交付。

## 裁定

先交付 RX、DM 各自真实 Create 的 **BOOT-only** 诊断臂，跑完原初始化与 GPU 回读即可独立衡量首阶段。第二步接入 **BOOT + SOURCE**，以原 12-pass 的合法非零 SOURCE 根核对数值和身份。第三步才启用完整九角色与 2,404-pass QUANTUM，同原臂比较完整根与有效首帧。三步共用同一源生成入口、角色身份和 owner 契约，逐步增加可执行的原路径。单体臂保留供同机 A/B 显式选择；本轮范围为目标20。

| 签包原件 | 实际内嵌源码 | 包内 ELF BuildId |
| --- | ---: | --- |
| RX T720-rx，签包 SHA256 `3c6af26b…` | 146,301 B，`2cb9a848…` | `27c55879e0bf46db946611ef87f4551b53e4c683` |
| DM T718-dm，签包 SHA256 `cb636553…` | 146,316 B，`c10b15ee…` | `9954e008b4d8b08907f30c44849f2cd2564ddf4d` |

从两份签包的实际 ARM64 ELF 抽取源码，并同其生成原件逐字节核对。DM 独立 `g9_dm_differential` 的 BuildId 为 `cb94c860b110e7dcdd2b8041ad9772858b9d8a2e`，采证时独立核对映射及实际调用库。T594 v7 见 RX 编译进入无配对返回，v8 的 DM 多进程运行只提供活动和退出观察，现有数据不足以裁定驱动卡死原因。 [闭包与原件索引](architecture-T733-closure-v1.json) · [T594 v7/v8](test-report-T594-v3.md) · [T718 独立原件](../test/reports/t718/compile.json)

## 生成边界

RX 与 DM 分别从各自生成器最终送给驱动的 `pipeline.comp` 切分。在 `tools/g9rx/generate_assets.py` 与 `tools/g9dm/generate_assets.py` 内，先完成各自 SSBO 和限定词变换，再以原完整源码及 `schema_generated.h` 生角色资产和 dispatch 表。CMake 把选择的资产嵌入实际包及对应离线入口。每个实际变体、每个单元对最终编译输入按 UTF-8 精确字节过 49,152 B 门。记录生成器版本、源码、布局及包内字节的 SHA256。直接 SPIR-V 和 Vulkan 源码的计量按 Architect 规范分别处理。

生成器仅按原 opcode 特化 `g9Run`。保留实际依赖的辅助函数、原顺序的声明、传递常量依赖，以及 SSBO、类型、uniform 和所有运行时条件。声明组被引用时完整保留。marker opcode 仍执行它原有的通用检查及宿主 copy/binding。未映射 opcode 在派发前拒绝。复用原 `(op, role, j, micro)` 调度，保持各 ordinal 与工作组不变。离线脚本属于证据原型，接入时对解析遇到的新语法拒绝生成，并核对 CFG、资源合同及动态差分。

| 程序角色 | opcode | RX B | DM B |
| --- | --- | ---: | ---: |
| BOOT | 12，`0x7fffff00` | 22,318 | 22,353 |
| SOURCE | 0–11 | 41,767 | 41,802 |
| ROOT_SETUP | 13–18 | 33,544 | 33,579 |
| TOUCH | 19–20 | 43,797 | 43,833 |
| DM_TRANSFER | 21–22 | 40,188 | 40,224 |
| FLIGHT_FOLD | 23–24 | 38,221 | 38,256 |
| FACE_FLOW | 25–30 | 47,206 | 47,238 |
| CAPTURE_AUDIT | 31–32 | 37,588 | 37,624 |
| ROOT_FINAL | 33–43 | 36,117 | 36,152 |

即便移除无依赖常量，合并 13–20 仍达 50,261/50,297 B，合并 21–24 达 53,107/53,143 B。主案的 SOURCE 为单程序，完整 QUANTUM 含 802 次相邻异角色切换。预留八角色成本对照，保留 21–22，将 23–32 改分为 23–27、28–32，仍为原序列，切换 612 次，最大 RX/DM 分别为 48,918/48,950 B。后者 DM 仅余 202 B，按实际生成测量，超门即拒绝该变体。计数为完整调度内相邻切换，不含起始 bind、提前拒绝或其他任务切回。两组共 34 个候选均以 glslang 检查通过。 [离线分析脚本](../tools/architecture/analyze_t733.py) · [角色与 pass 证据](architecture-T733-closure-v1.json)

## owner、同步与结果身份

1. **BOOT** 以真实 owner/context 创建和反射 BOOT，沿原资源分配、种子、512 B Config 上传。按原顺序执行 `ROOT_INIT` 与 `0x7fffff00`，每次 `1×1×1`、局部 64 lanes，各保留原 `GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT` 和完成等待。等待之后由当前 owner 同步读 `WORK profile.work[7].begin` 的 8 B。读取 `error==0`、`seal==0x641647`，且 map/unmap、GL/EGL 都成功才记 BOOT_AUDIT_OK。记录实际两个 word 和首个 GL/EGL 原错。保留原 Prepare 检查，只有审计通过才将该 BOOT 的缓存认作后续可复用证据。诊断态暴露自己的终态。普通 READY 仍由对应启用路径全量资格决定。
2. **反射与派发** 在 root admission 前完成所需角色 build、反射及资源资格。每个角色记录本 program 的必需 uniform 名、类型、元素数和 SSBO block/binding/access，按被特化后真正可达的输入签合同。仅允许经过静态证明不可达的项被链接器移除。每个 location 与实际 program 和 owner/context 绑定。按 root 首次参与及身份变更刷新该角色的 Common 常量；每 pass 都写原 `uOp/uRole/uJslot/uMicro/uPass`，沿原 `Bind(pass)` 保留所有别名变化。尤其核对 SOURCE_AUDIT_REPLAY、SOURCE_COMPARE、COMPARE_SELECTED_AUTHORITATIVE、SCOPE_SEAL_AND_CHOICE 的 role4、FINAL_ENERGY_CURSORS 的 role2/4、SOURCE_SEAL 与 ROOT_SEAL。实际换角色才切换 program，并核对本 pass 所属角色及其 binding；SSBO 与既有 state bank 连续驻留。
3. **同步与真实读回** 保留 `Allowed()`、各 pass 和 copy 的原屏障、成功后游标递增及 Flush。每 Pump 至多 10 dispatch、沿原 8 ms 检查点和 250 ms 根期限。完整根仍在最终 dispatch 后建原 fence；`glClientWaitSync` 成功且 permit/owner 仍有效才 map `WORK profile.work[8].begin` 的 1,024 B。读取、unmap、GL/EGL 检查和原接受谓词全部成功，才能提交 bank/command 并推进当前帧。角色边界沿用既有 GPU 内存发布与消费顺序。FULL/cold 导出沿原 selected/authority 和 B/C 捕获点、空闲 fence、slot credit 与退役顺序。取消与 context 更换走原失效/退役，合法 owner 在 GPU 安全点释放并先清 ProgramFactory 跟踪。
4. **私有身份** 给 split 的每个计划生成稳定 `roleSetSHA`，按版本化 canonical 序列哈希 variant/backend、原 role 顺序与各角色源码字节数/sha、资源合同和原两根 pass 表摘要。计划声明哪些根可执行。owner 在 admission 绑定实际链接且反射合格的角色集，锁定预期 SEAL 的 program / 源码 SHA，并对每个原 ordinal 绑定实际用到的角色、owner/context 与成功派发/屏障；最终实际 SEAL 收据须匹配预期。SOURCE 使用 op 11，QUANTUM 使用 op 43。现有 `Permit.shaderSha`、Config 上传和 GPU 写入 Completion word 80–87 绑定本根真实 SEAL 源码 SHA。DM T647 的 `ProgramFacts.actualShader`、Permit 96–103 和 Expected224 96–103 同此身份，Expected224 120–127 的 `activePipeline` 对 split 独立绑定实际角色集。T647 的 observed program 必须取实际 SEAL 收据，由同 owner 验完整游标/本根角色关联。RX 保留自己的私有 CompletionGate，实现同等角色关联。新增 split manifest 与冷读校验按变体版本化，单体读法继续按其既有身份。两族的数值 word 布局、提交语义与公开接口沿原合同。通过 CPU 伪造 shader 尾部、用任一缓存 SHA 代替运行身份，或用最后任意绑定的 GLuint 代替 SEAL，均须拒绝。

[RX transport](../entry/src/main/cpp/gpu_resident/g9rx/transport.cpp) · [DM transport](../entry/src/main/cpp/gpu_resident/g9dm/transport.cpp) · [T647 接受合同](architecture-T647-v1.md) · [T647 原字序](architecture-T647-acceptance-v1.json)

实施时同步核对 RX controller 的 Permit 生成与 cold 导出的 `kPipelineSHA` 使用处，以及 DM `ProgramFacts`、owner observation、split manifest 消费方和两族 `read_cold.py`。冷文件解码必须从所属计划和本根 SEAL 校验实际 scope，保留原候选、权威尾和区段校验。 [RX controller](../entry/src/main/cpp/gpu_resident/g9rx/controller.cpp) · [RX 冷读](../tools/g9rx/read_cold.py) · [DM 冷读](../tools/g9dm/read_cold.py)

## 三步验什么

| 实施臂 | 可交付进展与原路径证据 | 失败时保留 |
| --- | --- | --- |
| BOOT-only | 两族实际签包通过真实 Create 路径，两值读回、角色 build/反射、冷持久化及匹配新进程的 warm load。分别报告 build 和 BOOT_AUDIT_OK。不接收产品 root，也不发有效仿真帧。 | 真实 role、pid/start、owner/context/ticket、ENTER/EXIT 与 map/unmap、原 error/seal，保最后成功阶段及当次退出原件。 |
| BOOT + SOURCE | 同一族真实入口回放合法 K192 非零 SOURCE，按已有输入/options 绑定同一 source/seed。12-pass 整根比较 accepted/reject、authority/candidate、源命令消耗、质量/动量与审计、截断和失败不提交；校验真实 SEAL。只开放已有 SOURCE 路径。 | RX 缺合法非零输入原件时保留具体输入缺口，禁止用 DM Config 替代。DM 同时用已有独立入口校验差分，应用实际入口另验。 |
| 完整九角色 | 同一份输入、seed、Config 与 scene，依序走真实非零 SOURCE/QUANTUM。对旧程序/oracle 比已有定点、RNE/tie、边界、守恒、事件与失败事务门；按原合同 bit-exact 比数值有效域，将版本化身份字与时间戳按各自真实预期校验。检查首个完整根与同 commit/token 的有效 XComponent+shader Swap，以及设备端 30 FPS 目标。另比 250 ms 根期限和 Pump 挂账。 | 首差 ordinal/word、实际角色/绑定、错误码与最终 fence，或完整首根/呈现耗时及未达到的阶段。错 role/op/hash、漏 pass、上下文切换、时间过期、错误镜像/冷文件须 fail-closed。 |

优先复用本族签包、正式参考及已有 oracle，保留 RX / DM 分列。暖进程重新建 context 时重验真实驱动执行路径。离线用例、fixture、签名及分析交 tester，真机部署、客观取证及效果交 effector。Developer 交付实现和编译结果。 [DM 差分原件索引](developer-T718-v1.md) · [T647 反例基准](test-report-T652-v1.md)

## 缓存、总成本及实施触发

- 复用 T475 ProgramFactory/ArtifactStore，按实际角色精确源字节及角色/布局 ABI、link 配置、实际 GL/EGL 和驱动指纹绑定 artifact。命中后新 owner 仍要加载、反射和执行适用的原准备检查；相同 key 保留一次源码回退。记录缓存 preload、LOAD、REFLECT、PREPARE、EXPORT、PERSISTED/WRITE_FAILED 与拿到资格的各个时点。每角色在合法准备后按原机制交文件线程，保持每 artifact 32 MiB、整体 128 MiB/32 条及原并发/LRU 约束。角色集身份是执行计划证据，与每角色 binary 是否真正落盘分别记录。 [T475 实际合同](developer-T475-program-cache.md)
- 单体、BOOT-only、SOURCE 与完整拆分按阶段比较，禁止多份编译并发互扰。冷臂记录实际项目缓存无该 key，驱动私有缓存状态未知时标未知；暖臂在已证 PERSISTED 后以新 PID/start、同配置票据和包内实际映射身份执行。逐角色记录 COMPILER/LINK raw ENTER/EXIT、返回/日志字节、时间与 binary 体积，再以 Configure→BOOT_AUDIT→实际 READY→非零首根→有效首 Swap 的壁钟路径衡量。拆分完整臂另取 CPU 派发、实际 program/binding 次数、GPU 完成、Pumps、根期限失败、统计窗口帧时间与 30 FPS 达成率、常驻/峰值内存及 cache 写入/逐臂总量。超时记录截尾阶段与最后心跳，进程退出保留原系统证据和可证原因。对比预算预先沿 T727 划定，未达到阶段逐项记 NOT_REACHED，不能拿只跑到 BOOT 的墙钟替代首帧。
- **启动条件**：T733 的真实生成闭包、49,152 B 和原表审计已足以交 manager 排首步 developer。第二步进入之前，要求本族分拆 BOOT 在真实包内返回有效 GPU init tail，且 SOURCE 所需的真实输入与身份实现齐备。进入完整臂之前再核 SOURCE 整根与失败事务。八角色实测对照由已取得的完整臂证据触发，重点在 program switch 总耗时或等待叠加造成根/帧截止失守时。两后端共用逻辑源、角色表、布局及原数值 oracle，缓存与资源映射按后端分别实现。本阶段沿现有 GLES 实现，Vulkan 候选沿 T731 做同机能力、原子/整数语义、资源 binding、barrier/fence、缓存与真实 XComponent present 的对照；由实证再裁定后端。
- **待 T730 确认**：旧 SPLIT 的 `G13_COMPUTE` 实际仅 BOOT 23,834 B，冷 compile/link/build 分别 49.850/66.635/116.491 s。后次同 binary load 有记录，匹配新进程的冷暖、后续全部角色、READY 和首 Swap 仍待采证。T732 已实现重叠写入前的 buffer update barrier、修正字/字节读回偏移并提供原字诊断，续测确认 `INIT_ERROR_27` 所在阶段、实际原尾部与 FP32 资格。RX/DM 复核同类屏障、明确每个 offset 单位、按实际角色记日志。T730 的同机结果用于判断缓存及后端推广风险，RX/DM 现有离线证据足以继续准备。 [T727 执行边界](architecture-T727-v1.md) · [T731 后端对照](architecture-T731-v1.md) · [T732 实现](developer-T732-v1.md) · [T730 原始报告](test-report-T730-v2.md)

离线已验证签包源码身份、候选字节、glslang 与原 pass 表摘要。驱动编译耗时、binary 可复用性、算法等价、实际帧率及首 Swap 均待后续实施取证。GPU 驻留优先、CPU 停投、仅 XComponent + shader 上屏，禁止 Canvas 回退。T733 不使用真机。
