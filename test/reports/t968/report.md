# T968 · 目标 9/13 候选、集成 Demo 与 G16 集显实测（effector）

2026-11-25 · 开发集显 PC（Intel UHD 630 `8086:3e98`，driver 31.0.101.2140/1656924）。GPU 驻留、shader 渲染，无 Canvas 回退。工件全核：T963 四 PE（`8a73a08c`/`c0498082`/`fa3c4112`/`39bc9c18`）、T967 Demo（manifest `c403c82b` 5/5+启动器 7 模式）、T967 G16（manifest `23630e48` 7/7）、T969 独立 checker。

## ① 目标 9 库存单扫 + G15 对照

**单扫 ABBA**（t963-dm2-control，同 T954 seed 同窗，`--inventory-single-scan` 开关）：
- A2-off vs B2-on（均 SOURCE 接受 + Q vk=2 晚尾诊断封存）等价判定：`selectedJ=1`、首错 (0,0)、`WORK control[16]=2404` **两臂一致**；`EVENT/PATH/FIXTURE` 逐字相同；其余件差异词全部落在身份/时钟/seal 区（与 T954 同构分布）——**单扫模块（opcode 13/32/33/42）业务等价**。
- `inventorySingleScan` 标记在 envelope 与时标行均正确区分。
- `T963_INVENTORY_BINDING` 4 行逐 opcode 模块 SHA live 报告。
- audit 细档：**29 GPU 点**（15 粗点+14 逻辑槽点）+ 逐段命名 spans（Q1 DISCOVERY 422.8ms、SELECTED 1.41ms 等——持续成本实录）；operations 细档同采。
- 同 fence：Q root nonce/fence/gen == 时标行（29/29 available vk=0）。
- A1/B1 臂 Q vk=1（submit 级拒、dispatches=0）如实登记；ABBA 等价对用 A2/B2 晚尾臂。

**G15**（t963-g15 正负 × 单扫开关）：4/4 全 `vk=2`（iGPU 窗，T959 模式）、首错 (0,0)、invScan 标记正确区分。

## ② 目标 13 分闭包 + 819 标量 SHA + F02

**资格（5/9 过）**：
- 过：OP52_62_GUARD / OP54_57_COPY / OP60_COMPARE（五分 SOURCE 闭包三角色 pipeline EXIT vk=0）+ OP48_VISCOSITY / OP49_PRESSURE（监督脚本 CHILD_EXIT 0）。
- 挂：**OP55_58_INSTALL（20,709B）与 OP56_59_AUDIT（27,438B）**在 `vkCreateComputePipelines ENTER` 挂死（91s+121s 双证）——大 GLSL igc64 编译挂死家族；OP_48/REMAINDER 旧挂持续。

**819 标量 SHA ABBA**（F02 + seal-stages ± scalar）：
- A 臂：PREPARE/SHA(`9f24f251`)/PUBLISH ×4 EXIT vk=0；
- B 臂：PREPARE/PUBLISH 共享 + **OP11_SHA_SCALAR_RING（`b34d442c`）全链 live**（shaderModule+pipeline+submit+wait 全 vk=0）——819 仅换 SHA 模块的替换语义实测生效。
- 两臂 GRANT_DEADLINE 迟到根 + `T955_ORIGINAL_RETIRE` vk=0。

**F02-nonzero（J1）/F02-zero**：均迟到根拒（iGPU F02 模式）+ ORIGINAL_RETIRE vk=0 + 冷档 1.79MB 封存；WORK provisional MAC/RHS 提取受阻于归档段布局（Completion+820 slots+7 buffer 段需 BootLayout 映射）——T964 离线 oracle 已覆盖数值（RMS 6.61e-11、残差 1.79e-6、四类负控）。**目标 13 判定：部分**。

## ③ T967 集成 Demo——环境受阻（如实登记）

合成输入（`--m1-demo-synthetic`）要求 `GetForegroundWindow()==hwnd`（DriveSyntheticDemo 前台门）。无人值守会话中前台锁定不可越过——三种前台获取法（SetForegroundWindow、AttachThreadInput、Alt 模拟）全部被会话拒绝；python 直驱（893 行）、启动器 60s、后台 30s（3,916 行）均无合成输入行。

- **已有权威证据**：T966 冻结 r4 日志（118 MOVE+两次持按、4,356 Q、全日志 SHA 冻结）+ T969 离线判据（4s 窗 12 根/52 BIRTH/6,158,872 Q20；60s 后 0 新 SOURCE 等全链复算）——合成路径已被覆盖。
- **缺口**：真实静按/同粒轨迹/回正/出生 ROI 同根证据需要**交互会话**（用户在窗口前）或 developer 提供免前台输入选项。
- 启动器 5 件 manifest 校验 live（verify 段每次跑都核）。

## ④ G16 Completion v2 五臂——全过

| 臂 | exit | 证据 |
|---|---|---|
| FREE | 3（业务拒 reason 11/9） | certified-tail 2 行：**branch 1 自由漂移**（consumed=R=923,228,058、覆盖 52 壁+78 盘对、自由净空下界）→ branch 3 后续第三撞 |
| SUPPORT | 3 | 同构（自由漂移+第三撞） |
| **THIRD** | **0** | **T969 checker 过**：disk 13、**wall 3**、impact **3.99997µs**、Q24 括号 (8318720,8318977)、attempts 1、prior contacts 双值 |
| **THIRD_MIRROR** | **0** | **checker 过**：wall 2（镜像壁）同时长——**T965 的 fail-unresolved 被 Completion v2 修复（实测确认）** |
| SUPPORT_LOCAL | 0 | support-v1 96B 局部承托证书封存 |

- 全臂 `acceptedRoot=null`、热 fence gen1 与冷 gen2 分开、Completion 版本 2 标记 live。
- checker 对 FREE/SUPPORT_LOCAL 报 G16_THIRD_IMPACT_FIXTURE 缺行（该 checker 面向 THIRD 档案——非缺陷，语义不适用）。

## 结论表

| 子项 | 判定 |
|---|---|
| ① 目标 9 单扫 | **过**：单扫业务等价（control16/selectedJ/EVENT/PATH 同）、绑定/细档/同 fence 全采 |
| ① G15 对照 | 过（iGPU 窗拒，invScan 标记正确） |
| ② 目标 13 | **部分**：资格 5/9（两新闭包大模块挂）、标量环 ABBA live、F02 迟到根归档+布局受阻 |
| ③ Demo | **受阻（环境）**：前台门不可越；T966/T969 冻结证据为权威；需交互会话或免前台输入选项 |
| ④ G16 | **过 5/5**：THIRD/MIRROR 修复确认+checker 双证、FREE 自由漂移证书、SUPPORT_LOCAL 证书 |

证据根 `test/reports/t968/runs/`（goal9 A2/B2+细档、g15 两臂、goal13 五臂+量子归档、demo 三次尝试日志、g16 五臂全件+checker 结果）。
