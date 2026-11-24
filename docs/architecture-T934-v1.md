# T934 · G16-B 多盘种子入口修订

2026-11-24 · architect · Windows PC Vulkan · 离线。**归因为 T909 实现与 T928 已冻结种子之间的入口合同缺陷。** B13/B52 活动盘的合法 flags 是 `1u`。修 GPU FREEZE 的活动槽谓词并重新绑定模块 SHA。G16-A 旋转首撞沿用 T932 阳性，G16-C 等 B 正例及代表种子。 [T928 上传合同修订](architecture-T928-v1.md#2-1352-盘和实际壁姿态) · [T932 报告](../test/reports/t932/report.md)

## 1. 原件复核

固定 ABI 为 LE `<4I9f3I`。原种子以 `id=origin=i`、`generation=material=1`、技术材料的 FP32 半径、质量和惯量写入每个槽，最后三字为 `[1,0,0]`。两份源种子和下面每组实际上传的 SHA 一致。活动槽除 flags 与旧谓词不合外，其余 FREEZE 盘字段均满足旧检查，含按 FP32 复算的惯量。

| T932 试次 | N / J | 根 nonce | GPU Completion 的 outcome / 首因 / 首阶段 / 首盘 |
|---|---|---|---|
| static 13 | 13 / 128 | `1667921041349700` | 2 / 2 / 1 / 1 |
| static 52 | 52 / 128 | `1668084856045500` | 2 / 2 / 1 / 1 |
| hold 13 | 13 / 128 | `1668162669024200` | 2 / 2 / 1 / 1 |
| fast 13 | 13 / 128 | `1668162890147500` | 2 / 2 / 1 / 1 |

对四组原始 config/Completion，seed、pose、material、layout、role SHA 槽及 run/nonce/input、bank、代、fence generation 逐字相符。Completion 的 stage mask 为 `0x801`，只进入 FREEZE 与 SEAL。候选边、私有 commit、物理步数和 PACK 盘数均为 0。两组种子的每次 `base.disk64` 逐字等于该次 `seed.disk64`，私有 candidate 的导出活动区全零。首盘在复制前即拒，退出前仍按原合同封存拒绝回执。STATIC 13 另有直接 stdout，业务 submit/retire/result 均为 0，`identityOK=true`、`privateCommit=0`、`acceptedRoot=null`。后续冷拷贝读同 nonce，复用 fence 句柄但 fence generation 从 1 变为 2。其余三次的 OS exit 由 T932 报告记录；四组 GPU 拒绝由二进制原件直接复核。原根未执行运动、候选搜索或画面发布；这批原件证明入口拒绝及该次首盘失败的发布隔离。 [STATIC 13 原 stdout](../test/reports/t932/runs/g16b/static13.stdout) · [T932 四组收据](../test/reports/t932/g16-results.json)

旧冻结模块 `0f9b9e75…ef98` 的 `freeze()` 把 word 13、14、15 都与零比较。宿主先核完整 seed SHA，再冻结配置，首根先将状态 SSBO 清零，上传实际种子，经 transfer→compute barrier 供 FREEZE 读。GPU 冷取回 BASE 与输入原字一致。因此直接修入口谓词；宿主上传、同步和 seed 门保原合同。用旧 PE 与旧模块对两档种子分别改 word13 为 0/2/3、word14 或 word15 为 1，**10/10 在调用 Vulkan 之前以 exit 2、`G16_SEED_SHA` 拒绝**，未产出根目录。 [入口与上传](../pc/vulkan/sand/g16_multi.cpp) · [FREEZE 与 SEAL](../pc/vulkan/g16_multi_v1.comp) · [种子生成器](../test/t928/fixtures.py)

## 2. Developer 最小修复

1. **明确 `Disk64::flags` 的作用域。** 小域 V1 紧凑前缀 `[0,N)` 的 word 13（byte 52）必须完整等于活动位 `1u`。bit 0 表示参与这一固定盘集合，持续接触、支撑或自由飞行都不修改该字。word 14/15 分别在 byte 56/60，必须为零。空尾 `[N,1024)` 的 16 字保持全零，不走活动盘的校验循环。这个定义适用 B13/B52 的冷上传、既有镜像，以及每个根从最近已接受状态读取的 BASE。固定盘集合的语义不外推到将来允许删盘、换代或更大容量的 C 档。
2. **只改活动前缀的比较值。** `pc/vulkan/g16_multi_v1.comp::freeze()` 中使用具名 `kDiskActiveFlag=1u`，令失败项为 `state[a+13u]!=kDiskActiveFlag`。完整值比较继续拒绝 0、2、3 等取值。word 14、15、原 id/origin/generation、材料、质量、半径、惯量及首错顺序均保持。仍逐字拷贝 16 字，运动只写 4–9 字。错盘仍是 `firstReason=2 (seed_or_material)`、`firstStage=1 (freeze)`，记录真实首盘号。保原 seed 生成、ABI JSON 和种子身份。B13 为 `d74e416cebfbc9199c85a5c25f63ed52f315873ec2b5908a7f7ce94bbe1d3168`，B52 为 `eca4c474bc8143d358e0ed5f3a67f93c7f9d0a91f9c86fbe1a405b909ffb0884`。
3. **同步发布新模块与 PE。** 用 `build_t909_angle_probes.py` 重新拼装并验证 `g16_multi_v1`，按实际编译输入记录源码 SHA/字节、SPIR-V SHA/字节、四绑定反射及校验结果。旧完整源码 32,000 B，仍执行 ≤49,152 B 的完整源码门。用新 SHA 更新 `RunG16Multi` 现有单值模块门，重编配套 PE；旧 PE/SPIR-V 作为历史配对留证，交叉拼装须报 `G16_MODULE_SHA`。配置及 Completion 的 role SHA 取实际新模块。四个 SSBO range、Disk64/Completion 布局、布局身份和材料身份不变。原始 seed SHA 校验在镜像之前，镜像后的实际上传 SHA 仍单列并绑定 GPU 回执。 [构建与反射入口](../pc/vulkan/build_t909_angle_probes.py) · [现有复采命令](developer-T909-v1.md#编译与-pc-复采)

根提升条件沿用同一实际 compute fence 与 fence generation、live window/permit、真实 250 ms、Completion 身份和原守恒审计。保持 N=13/52、J=128/256、真实每根 1/120 s、8,192 候选边、16 每盘度、16,384 邻接项及原段/迭代上限。若较后位置的盘失败，前面若干合格记录可能已复制到私有 bank。负控要比较拒绝前后 active BASE、commit、quanta、state/scene generation 和发布，私有前缀可以留作冷证据。新旧窗口或迟 fence 的收据各用真实身份，私有根仍以 `acceptedRoot=null` 报告。

## 3. 复测和放行

| 责任 | 复测及判据 |
|---|---|
| **Developer** | 新 PE 与模块编译配套交付，四个 descriptor range 与 seed 门保持。若测试需要直接进入 GPU 负例，复用私有诊断或构建隔离测试入口，改变已冻结的测试缓冲副本并单独标明 raw SHA。正式 `g16-multi` 的 allowlist 和正常发布路径保持。 |
| **Tester** | 用 T928/930 独立核两份种子和镜像，比较第一根、至少第二根换 bank 后的 flags/保留字、slot 尾部和原身份。宿主负控含改 flags/保留字、错种子及交叉模块。GPU 独立负控分别用 flags 0、2、3，保留字非零、错 generation/origin/material，并在首盘以及第 2 或最后盘各放一次错误；预期有效 SEAL 拒绝、真实首盘号且不提升。错 scope/窗口、迟 fence、超容量沿独立首因，不吞并成 seed 错，检查上一有效窗口/lease 在拒绝后无新发布。 |
| **Effector** | 用配套新包先复跑 T932 四组，再对 B13/B52 分别跑 J128/256 和 fast/slow，共八条连续臂及各档静盒。快/慢为 480/840 个真实接受根；每一对从同一份有 SHA 的实际 BASE 起步。停在首个新故障时保原 Completion、首因、原 fence、前后 bank 与实际 pose 段。通过首根仅证明入口可用。 |

物理及同窗按 T928 和 T906 判据。保离壁/侧壁阶段、前后各 0.20 s 底/顶法向冲量占比、接触残差、穿透、质量和来源、线/角动量及墙功/能量残差、末段速度与堆高/质心。沿 T930 checker 从原字复算。现工具默认只冷读最后一根或首个拒根，分段账需补充按根选择的只读冷档、或从同 BASE 确定重放的对应前缀；接触 journal 截断须补足有效行。对同窗至少抓初态和翻转后同根姿态、Scene32、实窗口/geometry、scene 范围和代、graphics fence 与实际 present，再用 fenced 像素或同窗画面核可见结果。窗口变化时单列旧代退役，不能混配姿态与盘实例。G16-C 依赖 B 的连续正例和合格代表种子。 [原物理阈值](algorithm-T906-planar-avalanche-v1.md) · [T930 独立判据](../test/t930/check.py) · [当前图形收据入口](../pc/vulkan/sand/g16_multi.cpp)

本轮重建种子并复算原始拒绝。T930 离线自检的 12 个负控通过，T932 的 A 原字复核仍通过。用同版 glslang 在临时目录重编，旧 SHA 逐字重现。仅改比较值和增加具名常量的临时试编为 32,044 B 完整源码、156,140 B SPIR-V，SHA `786bb1db9b8d3a679042bdc36d014226d68647913770d503192c297e0c10e361`，Vulkan 1.1 SPIR-V 校验和 binding 0–3 反射检查通过。正式配套 PE/模块、B 两档两速、实际同窗与 GPU 负控待上述角色完成。GPU 驻留、shader 渲染，禁止 Canvas 回退。
