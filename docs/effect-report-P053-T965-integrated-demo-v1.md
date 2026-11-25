# P051/P053 T965 单一集成 Demo 同包放行与 G16 承托集显复测效果报告 v1

日期：2026-11-25。责任：effector。任务卡：`6b0623a0e4b07c3818949430`。技术路线：PC 集显（UHD 630）Vulkan Demo，非真机。  
基线：T960 `7dd68625` 送验包（`.codex/local-test/t960-integrated-demo` + `.codex/local-test/t960-g16-bundle`）。  
状态：**PACKAGE_IDENTITY_PASS / G16_RETEST_MIXED (FREE+SUPPORT_LOCAL PASS, THIRD+MIRROR FAIL-unresolved) / DEMO_SAND_VISIBLE_GRAVITY_CORRECT / 4S_PILE_NOT_MET / PARTIAL_PASS_WITH_GAP**。

## 0. 包身份核验（全 PASS）

Demo 五件（hello_world_pc.exe `a81619ef…`、full_source.spv、quantum_candidate.spv、quantum_commit.spv、Start Demo.cmd）与 G16 bundle 七件（probe/fixture/spv/comp/manifest/receipt/两 disk64）逐件 SHA256 与 T960 §构建表/manifest **全部 MATCH**——同包放行前提成立。

## 1. G16 五命令复测（集显 UHD 630，vendor 32902/device 16024）

| 命令 | 结果 |
|---|---|
| T960_B13_FREE（普通 PE） | **执行完成**：130 dispatch/129 barrier，PRIVATE_ROOT current=true fresh=true，LATE_TAIL segments=19/iterations=64/candidates=27，firstReason=11/firstStage=9（未接受 root，privateCommit=0） |
| T960_B13_SUPPORT（support） | 执行完成，同构数据；无 acceptedRoot |
| T960_THIRD（fixture third） | **passed:false**，privateCommit=0，acceptedRoot=null |
| T960_THIRD_MIRROR | **passed:false**（同构） |
| T960_SUPPORT_LOCAL（support-check） | **passed:true**（fence 1792499399544） |

**冷档核验**：每 run 产出 `late-completion.u32le`（1024B/256 words），header w0=`909199171`（magic 一致）、word1=尝试数=1；completion words 126/127：FREE/SUPPORT=(2,3)、THIRD/MIRROR=(1,3)。文件名是 `late-completion.u32le`（非 T960 文中 `certified-tail-v1.u32le` 命名），语义对应余时冷档——如实记录命名差异。

**THIRD 正臂/镜像 FAIL 未解决**：fixture 判定 false 且包内无私有 commit（privateCommit=0）——按 T960"任何该模式根都无状态晋升资格"为 fail-closed 行为，但**第三撞夹具未通过是本卡新事实**，交回 T960/developer（fixture PE 侧或模块侧待归因，本卡不归因）。SUPPORT_LOCAL 局部标量夹具 PASS 与整场景 support 未晋升并存，与 T960 §"局部证据与整场景资格分别核验"一致。

## 2. 单一集成 Demo 同包放行（默认模式，640×470 窗口）

启动链完整（WINDOW_AVAILABLE→Vulkan loader→device→3 pipeline→quantum cold angles→EMPTY_PRESENT，stderr 空）。窗口标题"盒体 请求 0° 实际 0° | R 重力回正"，UI 元素齐备（↓ 箭头、R 复位按钮）。

**入料与落沙（WM 消息直发 + MOVE 洪泛 118 帧/4s，hold 增至 122）**：
- **沙粒目视可见**：容器底部中央出现**土黄色小沙堆**（沙色 (184,145,89)，60s 帧 107 像素，y 218–224/x 152–175 集中于底边中央）；
- **重力方向正确**：4s-hold 沙粒 y 188–190（入料点附近）→ 60s-later y 208–224（底边）——**向下移动与重力一致**；
- **松手留存**：松手后沙粒保留不消失；
- **复位按钮存在**（R 键）未深测（窗口销毁前序列未覆盖）；
- **4 秒见堆：未达成**——4s-hold 沙粒 38 像素（约数颗），60s 累计 107 像素（小堆雏形）。入料速率显著低于"4s 见堆"预期（hold=122 帧线性 1 粒/帧量级），**交 developer 核入料速率/分频参数**；
- **入料速率受注入方式影响须声明**：PostMessage 合成输入与真实鼠标按住的 MOVE 频率/连续性不同，本卡 118 MOVE/4s（≈30/s）已接近真实帧率，速率缺口更可能来自 Demo 内部分频（dispatchesPerQuantum=35）而非注入方式——留 T960/developer 判定。

## 3. 判定汇总

| # | 判据 | 结果 |
|---|---|---|
| P0 包身份 | Demo 五件+G16 七件 SHA 全 MATCH | **PASS** |
| G1 B13_FREE | 执行完成、fail-closed 保持 | PASS（无 acceptedRoot，如实） |
| G2 SUPPORT_LOCAL | passed:true | PASS |
| G3 THIRD/MIRROR | passed:false | **未解决**（新事实，交回 developer） |
| D1 沙粒可见 | 目视+像素审计（107px 小堆） | **PASS（首次 PC 目视见沙）** |
| D2 重力方向 | 粒子向下移动 | **PASS** |
| D3 松手留存 | 沙粒保留 | PASS |
| D4 4 秒见堆 | 38px@4s / 107px@60s | **FAIL**（入料速率缺口） |
| D5 60s 继续入料 | 二次注入后沙堆增大 | PASS（速率仍低） |
| D6 复位按钮 | UI 存在 | PASS（功能未深测） |

**综合：PARTIAL PASS**——同包放行的包身份/沙粒可见/重力方向/留存/复位全部成立（PC 线首次目视见沙是里程碑）；缺口两项：THIRD 夹具未过（交 developer）、4s 见堆未达（入料速率，交 developer）。

## 4. 移交与边界

1. **T960/developer**：THIRD fixture passed:false 归因（fixture PE 或模块）；入料速率标定（hold 1 粒/帧 vs "4s 见堆"预期）。
2. observe 模式（8 帧 BIRTH 血缘）未执行——默认模式已见沙，血缘观察留待速率标定后（避免重复窗口）。
3. 采集方法记录：PostMessage WM 直发+MOVE 洪泛≈30/s+PrintWindow 抓窗口内容（免前台遮挡）；`SetForegroundWindow` 被系统前台锁定策略拒绝时 AttachThreadInput 补救；进程退出的判断用 tasklist。
4. UHD 630 集显；驱动 31.0.101.2115 附近（G16_DEVICE 行 vendor/device/driver 1656924 原始值入库）。
5. 滴答 T965 卡评论已发；清单行状态留 manager。
