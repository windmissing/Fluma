# T985 · G13 SOURCE 单角色接入与 G9 GPU 计量

2026-11-25 · architect · 当前 Intel UHD 630 `8086:3e98`、驱动 `31.0.101.2140`。用 T977 实际 JSONL、冷档和 T983 独立复核制定接入。G13 先独立证实 SOURCE 的 16 槽，再带同一角色策略证实 SOURCE→QUANTUM。G9 保留等价候选，单组 ABBA 性能未定，业务准时 0/4。 [T977 正式报告](../test/reports/t977/report.md) · [T983 G13 复核](../test/reports/t983/g13.json) · [T983 G9 复核](../test/reports/t983/g9.json) · [本卡复算原件](../test/artifacts/t985/evidence.json) · [复算脚本](../test/artifacts/t985/replay_evidence.py)

## 1. G13 资格证据与接入边界

同一 T975 PE `c27e3742` 的 5 个角色各有优化和 no-opt 两次实际资格。每次都可用 `pid/processStartTick/ownerTid/role/moduleSHA` 把 module 与 pipeline 调用、提交和等待拼到同一进程。`PC_G13_STEADY_OP66_V1` 在角色自己的 fence 后保存两次探针 16 原字、mismatch `[0,0]`、`acceptedRoot=null`。正式报告里「未见 OP66」与十份原日志有出入。优化臂约 80.614–106.266 s，no-opt 臂 **1.809–3.490 ms**，均为 `vkCreateComputePipelines ENTER→EXIT` 的 10 MHz QPC 差。报告的 0.4–0.5 s 是整进程观察时间。`OP55_58_INSTALL/OP56_59_AUDIT/OP_48/OP48_REMAINDER` 的 shader module 创建成功，随后各有 pipeline ENTER，180 s 观察窗结束前未采到 EXIT。当前能判实际调用超窗。具体驱动阶段和旧组最终返回均缺证。 [实际资格 JSONL](../test/reports/t977/runs/qual) · [T975 同 PE 工件](developer-T975-v1.md)

**T986 的最小接入**在 Win32 `Boot::RunT833Candidate` 冻结命名的 SOURCE-only 策略。三个显式臂共用当前 T963 guard/copy/compare，保持正常业务入口原策略和正常权限。首版从同一生成计划取 16 槽，以已有 `REST_BOOT_RAMP_SOURCE` 真实 SOURCE 输入独立准备、提交、读回。跳过原 eager `prepare(false,k_quantum)` 和 QUANTUM 提交，防止其它闭包先拦住这次证据。显式 fixture、私有 QUANTUM 或预览工作走各自已有入口，参数层拒绝与首版 SOURCE-only 混用。真实方向与输入 canonicalize 沿原函数。 [Win32 角色入口](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc:192) · [生成计划](../entry/src/main/cpp/gpu_resident/g13flow/contract_generated.h:4784)

| SOURCE 逻辑序号 | 现有共同角色或基线 | 快臂实际模块和绑定 |
| --- | --- | --- |
| 0/1/3 | `OP_37`，op50/51/53 | 共同角色，各自原 flags |
| 2/12 | `OP52_62_GUARD` | 共同角色，各自原 flags |
| 4/7 | `OP54_57_COPY` | 共同角色，各自原 flags |
| 5/6 | `OP55_58_INSTALL` / `OP56_59_AUDIT` | `OP55` / `OP56`，ACTUAL，原 set 0，BASE 0→Candidate 1 |
| 8/9 | `OP55_58_INSTALL` / `OP56_59_AUDIT` | `OP58` / `OP59`，REPLAY，原 set 1，BASE 0→Replay 2 |
| 10 | `OP60_COMPARE` | 共同角色，Candidate 与 Replay 比较 |
| 11/15 | `OP_61`，op61/63 | 共同角色，各自原 flags |
| 13/14 | `OP_00_9_11`，op9/10 | 共同角色。独立 seal 选项应在所有臂一致，改 16 槽的一派发合同则另开户验 |

`--g13-source-only` 选 T963 组合角色 flags=0。附 `--g13-source-singletons` 时，严格在四个目标槽选择相应完整 singleton，`VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT` 即 flags=1。附 `--g13-source-singletons-opt` 时，同四个模块 flags=0，供单独归因。完整模块内保原 drive、field audit 和 workgroup 边界，每槽一派发。T975 `OP_SOURCE_DRIVE_ONLY`、20/22 派发拆段及 `OP_48/OP48_REMAINDER` 只用于各自资格或隔离诊断。本快臂 16 逻辑/16 物理、scope push 均为 0，四处 push 分别 `[55,0,0,0,0,5]`、`[56,0,0,0,0,6]`、`[58,0,0,0,0,8]`、`[59,0,0,0,0,9]`。沿原 compute shader-write→compute-read/write barrier，尾部使用原 compute→transfer→host。保原作用域权限及失败竞争顺序。 [描述符和推送](../pc/vulkan/root_plan.h:11) · [SOURCE 作用域映射](../entry/src/main/cpp/gpu_resident/g13flow/root_binding.h:21)

**Pipeline 标识。** 准备、资格、录制和回执使用同一 `role/moduleSHA/entry/stage/spec/pipelineFlags/layoutSHA/layoutGeneration/deviceGeneration` 键。同一 VkDevice 会话中，设备/驱动身份、`pipelineCacheUUID`、配置/ABI 和 role policy 应在证据中冻结。实际 create flags 与 `cache=null` 单独记。仅持有当前键下已经通过原 double OP66 的 pipeline，销毁/重建或配置、layout、实际 device 变更时重新资格。历史角色结果只作为选择依据。保 `vkCreateShaderModule/vkCreateComputePipelines` ENTER/EXIT、probe 提交与等待身份、CPU QPC 和 `VkResult`，再记录每槽实际 pipeline/flags/original push、set/buffer generation。单角色启动参数沿原资格口径。普通业务 pipeline 沿原 flags，避免现有全局 `attempt.disableOptimization` 污染其他角色。 [原资格创建及探针](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/boot.cpp:942)

**同一 SOURCE 提交与首错。** 每臂存 `run/nonces/sourceSeq/plan`、输入 origin、冻结 INPUT/CANONICAL/Config/Permit/Seed/original BASE 与各自 SHA、窗口/几何/代；冷档要能回接原 device/queue/fence/fenceGeneration、迟到终态和 staging copy fence。原 fence 真成功退役后核共同缓存可见性、同段热 Completion 和 copy 后热前缀逐字，冷档另存 BASE/Candidate/Replay/WORK/CTRL/PACK。核全部 16 槽计数、seal、COMPARE、原 hash/账本、原序号=首错、状态与两个实际状态代；尾部保原 first error/status 与首个不同文件/word、GPU raw、expected raw。比较不同运行臂时只按旧 ABI 已登记的 run/frame/pulse/时钟/封印来源解释差异，保留原字和精确偏移。对同故障输入要求原始首错、写入权限、同期根绑定和原因一致。候选如首个失败与原不同，原样记录并退回 architect。检查 stale、篡改 module flags/SHA、ACTUAL/REPLAY 交换、错误代和错 fence，使用明确负控。日志区分 `PIPELINE_ENTER_NO_EXIT`、资格失败、首个 SOURCE 提交前拒绝、业务/诊断窗口超期、退役后原码拒绝、冷档缺证。SOURCE-only 可给 `SOURCE_VALID_DIAGNOSTIC` 或安全迟尾诊断，`acceptedRoot` 和 typed token 置空；正常业务仍用原 deadline、freshness、窗口和 promotion。 [Completion 合同](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc:54) · [原 hot/cold fence](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc:839)

**可控回退和接手。** 快臂和任一共同角色失败先冻结第一个 ENTER/原字/代，当前进程留原失败。旧组合只用显式另启对照，等到既定观察窗或首个终态。组合还超窗时可用同模块优化 singleton 单测缩小问题；新旧结果同样写实。四目标槽动作可逆，无隐式角色替换、同次跨进程重拼、Canvas 路径或 CPU 产品求解。Developer T986 可先出诊断包，tester 独立核角色表、真实输入合同和负控，effector 在 UHD 630 上取实际 SOURCE 同次原字。只有业务 SOURCE 与后续 QUANTUM 实际顺序、各自合法时间和同代正确性成立后，再选是否在正式候选入口启用；QUANTUM 的原优化和资格各自另记。

## 2. G9 ABBA GPU 时间裁决

只取原 `ROOT_GPU_TIMESTAMPS` `kind=QUANTUM`。主点 0 是原 command buffer 的 TOP_OF_PIPE，点 14 是原 tail copy、host barrier 后的 BOTTOM_OF_PIPE，粗点 0..14。其间的 12 点只给三处 scope 中 OP25/27 的实际边沿。`INPUT_READY→DISPATCH_DONE` 是所列逻辑槽的 GPU 观测跨度，不能拆读成单独一条除法指令；六处和位于整根之内。两端 tick 原字按 `Δ=((q14−q0) mod 2^36)` 重算，`83.333 ns/tick` 约 `12.000048 MHz`，与 `QPC 10 MHz` 的授时/host 等待域分开。point 及原 fence 都成功才算可读。四份 `queryResetSerial` 与原 fence generation 都为 4，核 run/nonce/plan/deadline/fence，原业务停点 `GRANT_DEADLINE`、原业务 wait 返回 `VK_TIMEOUT`、`tailRead=false`、`acceptedRoot=null`，后续交易以原 fence 的确退役为时戳读取条件。冷读新 fence 和冷部成本单独登记。 [query 编码](../pc/vulkan/sand/owner.cpp:1086) · [尾部主点](../pc/vulkan/sand/owner.cpp:1185) · [四臂原文件](../test/reports/t977/runs/face)

| 臂 | q0 → q14 原字 | 36 位差 ticks | 整根 ms | 六处 OP25/27 合计 ms |
| --- | --- | ---: | ---: | ---: |
| A1 off r1 | 4267621851 → 4283238983 | 15,617,132 | 1301.422 | 521.136 |
| B1 on r3 | 4573491377 → 4588797991 | 15,306,614 | 1275.546 | 509.552 |
| B2 on r1 | 4624053617 → 4638598134 | 14,544,517 | 1212.038 | 481.336 |
| A2 off r1 | 4675525833 → 4688747848 | 13,222,015 | 1101.830 | 447.784 |

T977 摘要中的 1071/1058/1001/906 ms 仍保原上下文，整根 GPU 测量以点 0→14 及同 fence 原件为准。B1 r1/r2 分别在 SOURCE 授权后 253.444/251.023 ms 停于 `GRANT_DEADLINE`，都没有 QUANTUM。四个成功取到诊断根的相邻对差 `B1−A1=−25.876 ms`、`B2−A2=+110.208 ms`，ABBA 均值 B−A `+42.166 ms`。A 的头尾差 `−199.592 ms`，B1 又经历两次 SOURCE 重试，单组数据不足以分离底层漂移、次序和候选效应。T983 的跨臂原字核对支持 late diagnostic 等价。当前仓库能从 A1/B1 两个冷档重读 FIXTURE/EVENT/PATH、Completion 和选定状态，另外两个臂可从原日志复算 27 点。两个现存冷档的 arena 保存登记 SHA。完整四臂物理原件比较保留 T983 当次核验口径。本卡回执以 LF 规范文本 SHA 标识 JSON/JSONL，二进制按原字 SHA。产品准时仍为 0/4。

**最短复测。** 待需要裁决面通量收益时，在同一 UHD/驱动、同 PE、同输入、scope/`inventorySingleScan`、冷档和 `--quantum-detail face` 固定的前提下，effector 取相邻 ABBA 和 BAAB 各一完整块。固定预热、进程策略、源准备顺序、每位置尝试数和外部负载。每试次保 SOURCE 停点及日志，任一位置失去 QUANTUM 将该块标缺，后续整块重做以保次序。可复用真实现有的 `--source-prepare-retired`，持续保留原 250 ms 业务门和真正原 fence，另归档迟尾。为测方向先比较每块 B−A 整根值、六处合计及每 scope 值，同时看同模式头尾与粗的 CLEAR、审计/比较/封印段是否共漂。两块已足以暴露方向翻转或次序混杂；若两块同向且目标六段的差超过相邻同模式漂移，可再采一对反转块确认。缩小到两段 FLUX/COLUMNS 时需真实 stage 绑定、首错和词等价，沿同 fence 记录两个阶段跨度以及新增栅栏成本。 [T976 精确除法和后续两阶段工件](developer-T976-v1.md) · [既有跨根计量边界](architecture-T973-v1.md)

**后续优先级。** 当前四臂即把已量六段的时长理想化减为零，整根仍有 **654.046–780.286 ms**（其它段固定的原数据算术说明）。可保留精确除法作为等价对照。若目标是推进 250 ms，优先定位有效微步其余大段和审计，同 T973 已证 OP21、OP32/33 与最后审计拆解共享一个冻结输入。只有对应 stage 真实绑定和独立迟尾首差可观测，才开始对应的缩减候选。整个 T985 离线完成，未运行实际设备或窗口。GPU 驻留与 XComponent+shader 唯一上屏，禁止 Canvas。
