# T11 设备取证回执：endpoint 合规 run 与同账本 peak 双报（v1）

日期：2026-10-25 16:00–16:15。执行角色：effector（真机）。构建：**s141**（`entry-default-s141-signed.hap`，sha256 前缀 `2ea9b107`，源码树指纹 `45d28876`，HEAD `5c522d8` 含 T20/T36/T39/T40/T41/T24）。设备：VYG-AL00（62T0225B10010020）。

## 0. 结论摘要

| 目标 | 结论 | 关键依据 |
|---|---|---|
| endpoint 合规 run | **BLOCKED（结构性，非本次采集失败）** | 三项装配/构建缺口见 §3：`setEndpointSampling` 无页面装配（metricCommitKey 0/N）、store worker abc 未入 HAP、摇晃无 want 停续通道 |
| 同账本 peak 双报 | **BLOCKED** | peakSampler 同上未激活；peak 唯一设备出口（store）不可用 |
| 协议 §0 状态印证 | `endpointProtocolEffective=false` **获实证支撑** | 缺的不只是"一次 run"，而是设备采集链的身份契约（phase/clock/stamp）在页面装配层不可达 |
| 可交付实证 | 见 §4（六项） | 72 笔注入全准入、epoch==count、E 计入注入、守恒 1e-11、静置末窗 M/R=0.099% PASS、帧健康 |

## 1. 采集执行（三臂，tools/t11_endpoint_peak_run.ps1）

| 臂 | want | 时长 | 样本量 | 场景身份 |
|---|---|---|---|---|
| T11-A | `rendermode 4, wssnapshot true, shake 1.0, testmode true` | 65s | metric 262 / shake_lift 2456 / frame 1228 | **持续摇晃态**（shakeInput 恒 1.0 无衰减） |
| T11-B | 同上 + shake 0.6 + 两次全宽 swipe 注入 | 45s | metric 188 / gesture_sand 72 / shake_lift 1701 / frame 851 | 摇晃态 + 外源注入 |
| T11-C | `rendermode 4, wssnapshot true, shake 0, testmode true` | 25s | metric 153 / frame 1445 | **静置对照** |

**采集链路事实（新踩坑，已修脚本并登记）**：`aa start` 不带 `--ps rendermode 4` 时 `parseLiquidCAWant` 默认 `renderMode=1`（细沙 CA），WaterSandField 不步进、无 WS_METRIC（初版脚本因此零帧；补 rendermode 4 后恢复 4 样本/s）。首轮 store pull 失败定位为 worker abc 缺失（§3-B）。

## 2. 逐臂判读（tools/analyze_t11_endpoint_peak.py，analysis.txt）

| 项 | T11-A（摇晃） | T11-B（摇晃+注入） | T11-C（静置） |
|---|---|---|---|
| 末 1s 窗 M/R（协议 §1.3，阈值 5%） | 0.987941 **FAIL**（摇晃态） | 0.785008 **FAIL**（摇晃态） | **0.000990 PASS** |
| 全程窗峰 M/R | 0.992101 | 0.991547 | 0.983946 |
| 守恒 actualMass−E | 2.18e-11 PASS | 2.00e-11 PASS | 1.36e-11 PASS |
| particleEvidenceKind | STRUCTURAL_ZERO_OFF | STRUCTURAL_ZERO_OFF | STRUCTURAL_ZERO_OFF |
| metricCommitKey 非空 | 0/262 | 0/188 | 0/153 |
| 帧健康（H01：>1000ms 冻结） | max 83ms，0 冻结 | max 76ms，0 冻结 | max 68ms，0 冻结 |

判定口径：摇晃臂 FAIL 属**场景身份披露**（协议 TC-WS-08 要求"摇晃后静置 8s"，本run shakeInput 恒 1.0），不计产品缺陷；静置臂 T11-C 为协议意义上的合规末窗（M/R=0.099%）。

## 3. endpoint 合规 run 的结构性阻塞（三项，按协议生效链排序）

### A. `setEndpointSampling(clock, phase)` 页面侧无装配（P1）
physics 层 API 存在（WaterSandField.ets:2271，Opt-in、run 首子步前调用才有效），但全树无页面/入口调用 → `endpointSampler=null`、`peakSampler=null`、`endpointStamp=null`、`endpointPhase=null`。后果：`metricCommitKey` 恒 null（协议 §3 身份契约：runId/sequence/frameId/substepId/committedOrdinal 全部不可得）、phase 身份不可得、peak 不采样。**最小修复**：SandCanvas 在 reset 后、首子步前调用（需 P011EndpointClock 与 phase identity 构造，参照 tester 执行器 `test/p011_endpoint_protocol_v1_execution.py` 的构造方式）。

### B. store worker abc 未打入 HAP（P1，构建缺陷）
`P011_OBS_STORE` 落出 filePath 但 worker 加载失败：`GetEntry failed for ets/physics/P011NaturalObservationWorker.abc`（hilog 16:13:16）。根因：worker 源文件在 `entry/src/main/ets/physics/`，hvigor 默认只为 DevEco 约定的 `ets/workers/` 目录生成独立 abc；该文件（d5a4e4f 引入，10-19）从未有过成功设备落盘记录。**最小修复**：移至 `entry/src/main/ets/workers/` 并改 `ThreadWorker` 路径（或 hvigorfile 配置 worker 编译规则）。

### C. 摇晃无 want 停续通道（P2，场景构造限制）
`liquidStartShake` 仅在 aboutToAppear 读取一次；运行中仅面板桥接 `setWaterSandShakeInput` 可改；AppStorage 更新不经 onNewWant 传导（无 @StorageLink）。自动化无法构造"摇晃→停→静置 8s"。**最小修复**：applyWant/onNewWant 通道支持 shake 中途更新（或 schedule player 增加 shake 段）。

## 4. 可交付实证（本次 run 的真实产出）

1. **外源注入准入（T11-B）**：72 笔 `gesture_sand` **全部准入**（rejectedReason=None），`sourceEpoch` 0→72，**epoch==准入笔数==72**（T40 裁定的"合法相等"口径获设备实证）；admittedH/L 累计 16.292845/16.292845 与账本 `sourceInjectedHeavy/Light` **逐位一致**（1e-6 内）。
2. **R_WS 分母构成**：E 从 5120 → 5152.585690，增量 32.585690 == IH+IL（协议 §1.2 `E=VH0+VL0+IH+IL` 落地实证）。
3. **守恒**：三臂 `actualMass−E` ≤ 2.2e-11（协议 1e-9 闸门 PASS）。
4. **静置末窗**：T11-C 末 1s 窗 M/R=0.000990（0.099%）≤ 5% PASS——协议阈值判定的设备侧正向样例。
5. **帧健康**：三臂 3524 帧，max 83ms，>1s 间隙 0，真崩溃/ANR 0。
6. **P=0 披露**：`particleEvidenceKind=STRUCTURAL_ZERO_OFF`——粒子路线未激活（T31 阻塞连带），分子 M=U；协议 M1 的 P_H/P_L 项在本 run 结构性为 0，**不得**据此宣称粒子项已验证。

## 5. 完整性资格（T40 §4 判据）

- `setExternalAdmissionEventSink` 页面侧无装配 → 无 STREAMED 原始记录；
- store 文件因 §3-B 不可得；
- **资格状态：SUMMARY_ONLY**（WS_METRIC 汇总 + WS_EVENT 事件流；无准入事务流，externalAdmissionCount 不可独立重算）；
- 附加披露：**WS_SNAPSHOT 通道受 hilog 单条 ~4096B 上限截断**（263 行仅 1 行完整，全量账本 ~50KB 摇晃态全截断）——协议 §3 若需逐点全量独立核对，须改落盘通道（store）或分片序列化，不能依赖 hilog 长行。

## 6. 接力建议（交 manager 派工）

1. §3-A/B 两项 P1 修复（developer，可同一卡：endpoint 采样装配 + worker 目录迁移）——修复后 T11 可重跑合规 run（含 peak 双报）。
2. §3-C P2（场景停摇通道）随 T31 装配缺口一并处理。
3. 协议 §3 身份契约的**设备可达性**应作为 endpoint 生效前置写入 decision 稿（当前 §0 "缺一次合规 run"的表述不完整：缺的是采集链装配）。

## 7. 限制声明

- 本回执不宣布 endpoint/peak 协议生效；FAIL 判定限于摇晃态场景身份披露；STATUS 资格为 SUMMARY_ONLY。
- 未改产品代码/判据/协议；采集脚本与分析器已入库（tools/t11_endpoint_peak_run.ps1、tools/analyze_t11_endpoint_peak.py）。
- 产物：test/artifacts/t11-endpoint-peak-run-20261025/（14 文件：3 dump + 6 截图 + console + analysis）。
