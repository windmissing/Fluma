# T937 · PC G15 管线资格与 OFF 私有重放复采（effector）

2026-11-24。两项独立。开发集显 PC（Intel UHD 630 `8086:3e98`，driver 31.0.101.2140/1656924），GPU 驻留、shader 渲染，无 Canvas 回退。T935 完成后同机执行（卡面"集成版优先占同一 PC"）。证据根 `test/reports/t937/runs/` 与 `G15_OP23_RUN/`。

## ① G15 opcode 23 单角色资格 ABBA（T933 工件）

PE `t933-g15-release/pc_sand_dm2_host.exe`（9aea6572），`t933_g15_qualify.py --run` 两 ABBA 组：

| 组 | A（原模块） | B（去 OpSourceExtension） |
|---|---|---|
| source-extension | **挂死（1 臂、4 臂，forced watchdog 杀）** | **G15_OP23_QUALIFIED（2 臂、3 臂，pipelineVkResult=0）** |
| DM2/G15 前缀交错 | 4/4 全过（1-A/4-A 与 2-B/3-B 均 QUALIFIED） | — |

- **归因结论**：opcode 23 模块内的 52B `OpSourceExtension` 记录是 igc64 管线编译挂死的触发器——**去该记录（其余逐字相同）后编译通过**。这解释并收敛了 T927 G15 宿主挂死（原判"机器态相关"，本次定位到模块内容级触发器）。
- 前缀组 4/4 过：DM2/G15 0–22 前缀不构成因素（ABBA 无分化）。
- 收据：每臂 `OP_23.receipt.json`（exeSHA/argv/exitCode/forced/lastPipeline 含 moduleSHA、VkResult、ownerTid）；挂臂记录 lastPipeline 停在 module 加载后无 pipelineVkResult（编译未返回）。
- **后续路径**：正反沙丘正例须 developer 定夺去记录模块的正式打包（SHA 门/包身份）后由 effector 复采——资格门已从"不知道为什么挂"变为"已知触发器、去之即过"。

## ② OFF 实采样私有 GPU 重放（T927 off-1 action 7 参考）

参考 `.codex/local-test/t933-off-reference.bin`（922ddef7，T927 off-1 日志封存：action 7、Δ15,026,700ns、接纳 126,053 Q20）。PE 用 T933 集成包 + t830 全量档模块：

| 臂 | 结果 | 关键字段 |
|---|---|---|
| off-1-private（OFF） | `M1_PRIVATE_OFF_COMPLETE` | **acceptedRoot=null**、SOURCE/QUANTUM fence VkResult=0、admitted=126,053（==参考）、127 派发 |
| off-2-private（OFF） | `M1_PRIVATE_OFF_COMPLETE` | 同上（重放两次一致） |
| off-observe-private2（ON） | `M1_PRIVATE_OFF_COMPLETE` + `M1_QUANTUM_COLD_CAPTURE` | 冷观察归档 `observe-16713673733571.m1root` |

- **原字**：每臂 10 件（source-base/command/completion、quantum-base/candidate/completion/events/independent/publication、reference.bin 副本）——交 T496 核证。
- **真实授权/fence**：当前运行 QPC、真实 fence 等待 vk=0、私有身份 GPU 初始化空 BASE 后全字回读校验——沿 developer 合同。
- 备注：ON 臂首跑 `M1_PRIVATE_REAL_PULSE` 拒（primeClock 脉冲竞争，进程在脉冲到达前自检失败）；第二次跑（脉冲 settle 后）过。原字无窗口发布（acceptedRoot=null，私有范围），**不算正式四臂**——按卡面口径。
- 首跑负证据：私有输出目录已存在时宿主 `M1_PRIVATE_REFERENCE_LAYOUT` 拒（create_directory 语义），首组三臂因此重跑——已按"OUT 须不存在"约定。

## 结论

| 子项 | 结果 |
|---|---|
| G15 opcode23 ABBA | **完成**：OpSourceExtension 触发器定位（A 挂/B 过 ABBA 分化 + 前缀无关）；正例待 developer 定打包后复采 |
| OFF 私有重放 | **完成**：两 OFF 臂+一 ON 观察臂全 `M1_PRIVATE_OFF_COMPLETE`，admitted 126,053 与参考一致，原字 10 件×3 交 T496；acceptedRoot=null 依合同 |

GPU 驻留、shader 渲染全程。
