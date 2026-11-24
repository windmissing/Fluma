# T912 补采 · T916 修复合流验证（effector）

2026-11-24 午后。T915 归因后 developer 交付 T916（commit `d3fa6a0d`，PE `.codex/local-test/t916-ninja/hello_world_pc.exe` SHA `6dd3378f…a002`），修复 T912 卡内两个受阻项。本补采用该工件验证，仍限开发集显 PC（Intel UHD 630，driver 31.0.101.2140），GPU 驻留、shader 渲染，无 Canvas 回退。

## ① 持按跨 CLOCK_GAP 续沙（T912 缺口"sourceAnchor 断流"修复验证）

**修复机制实测**（源码核对）：`SuspendHeld/ResumeHeld/RevalidateHeld` 状态机把物理按压与 SOURCE 授权分离，`M1_HOLD_RECOVERY` 逐事件留档，恢复期清 `sourceAnchor` 且首个新会话拍只建锚。

**自然触发运行**（`runs-t916/2b-hold-retry0`，同一 Down 8s 持按 + `--m1-source-trace`）：

| 事件 | t（自 Down） | 会话 |
|---|---:|---|
| SUSPEND（CLOCK_GAP） | 3.074s | 1 |
| RETIRED（旧 fence 真退役） | 3.095s | 1 |
| NEW_BASELINE | 3.162s | 3 |
| HOLD_REVALIDATED（同 Down 重绑，新授权 epoch=17） | 3.169s | 3 |
| 首笔 gap 后 SOURCE 受理 | 3.451s | 3 |

- **同一 Down 跨 gap 续沙成立**：gap 前 25 笔 SOURCE（2.890u），gap 后 **27 笔（3.116u）**——对比 T912 同场景断流后 offer 恒 0、5.3s 零入料。
- **守恒**：52 笔呈现受理 + 1 笔在途根（130,969 Q，仅以 `M1_STALE_WAIT_REAL_PULSE` 行留档，gap 时在途、恢复后按 stale 处理）= **6,429,612 Q20 == lease sandAdmittedTotalQ**；末帧 stock==admitted==6.132u。
- **授权 epoch 换绑**：旧 epoch=1 的在途 offer 被拒（serial 390 `AUTHORITY_CHANGED`），新会话迟到旧 epoch offer 同样拒（serial 1015/1016）——无旧 offer 补发。
- `m1Recovery` 行：CLOCK_GAP、oldSession 1、balanced=true、lastAcceptedRoot 保留。
- 总入料 8s 达 **6.13u**（T912 同窗仅 1.97u）——修复同时恢复了入料吞吐。

**无 gap 对照**（`2a-sand-hold-recovery`，同窗 + 观察系列 120 帧）：40 笔 SOURCE 5.15u 全程无 gap，120 帧同根绑定照常；松手后 26.8s 窗口末仍有 15 粒在飞（入料量为 T912 三倍、呈现 ~15.6fps→物理 0.13s/s，沉降超窗——非缺陷，仅尾更长）。4 秒见堆门沿 T908 三根因结论不变，但本补采证明第一根因（断流）已消除。

## ② G13 OP_48 三组对照（T912 缺口"t909 exe OP_48 挂起"按 T915 合同复采）

按 `docs/developer-T916-v1.md` 命令执行三组 ABBA（新进程、同路径镜像、同 F02 CONFIG/STATE、关 tail/seal）：

| 组 | A/B | 结果 |
|---|---|---|
| `g13-pe`（t890 vs t909，关 snapshot） | 4/4 | **全部 `CENSORED CALL_UNRETURNED`**——A（t890）也挂 |
| `g13-snapshot`（t909，snapshot 关/开） | 4/4 | 全部 `CENSORED CALL_UNRETURNED` |
| `g13-scope`（t916 PE，单角色 OP_48 vs 全前缀，`--g13-compile-trace`） | 4/4 | 全部 `CENSORED CALL_UNRETURNED`；单角色臂 19 行内直达 OP_48 ENTER（ShaderModule 0.83ms 成功后无 EXIT） |

- **关键修正（推翻 T912 归因）**：T912 时 t890 对照 11.1103ms 通过、t909 确定性挂起；今日同机同驱动 **t890 同样挂起**——12/12 臂停在同一调用（`vkCreateComputePipelines` OP_48，moduleSHA `f01959cc…`、layoutSHA `b9390359…`、flags 0、pipelineCache null）。**挂起与 PE 身份无关，与当日机器/驱动状态相关**；T912 时段与今日之间的驱动内态变化（shader cache/温态）为候选因素，按 T915 记 `driverCacheIsolation=UNKNOWN`。
- **取证已到位**（T915 最小缺证清单）：每臂 `OP_48.receipt.json`（真实 argv/PID/TID/QPC/CPU 工时、最后 ENTER 原行）、15 个 `LoadedImages` 采样、loader/驱动 DLL SHA+文件版本（`vulkan-1.dll` 1.3.300.0、`igc64.dll` 31.0.101.2140 `4860329b…` 等）、**20s/45s 两档 minidump**（308/315KB×每臂）。
- 按 T915 合同：两组对照均未分化（全挂）→ 下一步是栈样本分析而非前缀二分；minidump 已在仓，**交 developer 分析驱动侧等待点**。

## 结论

| T912 缺口 | 修复 | 验证 |
|---|---|---|
| CLOCK_GAP 后持按断流（offer 恒 0） | T916 状态机 + anchor 清理 + epoch 重绑 | **实测修复**：同 Down 跨 gap 续沙 27 笔、守恒闭合、旧 epoch 拒绝 |
| G13 OP_48 t909 挂起 | T915 归因 + T916 对照工具 | **归因修正**：t890 今日同挂（12/12），机器态相关非 PE 特异；栈/驱动取证已采齐交 developer |

工件：`test/reports/t912/runs-t916/`（2a/2b 持按运行 + g13-pe/snapshot/scope 三组 ABBA 含 minidump/receipt）。
