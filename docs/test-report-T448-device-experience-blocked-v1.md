# T448 · GPU 显式沙水入口真机体验报告（真机段 BLOCKED：GPU 路径无有效内容帧，交 architect）

2026-11-09，effector，真机（62T0225B10010020），设备串行（T410/T408 窗口外执行）。T438 双签包已真机安装并经普通菜单进入显式入口页，**但 GPU M1 路径在真机 VSync/queue 环境下反复重建失败，四个 run 全部 STATE_LOST，画布无任何粒子/水面内容**——体验主目标（连续掉沙/注水/侧扩散/水质感）无法行使。

## 已真机完成部分

| 项 | 结果 |
|---|---|
| 签包核验 | regular `87692250...732b`、experiment `10cc8e83...95f` 本地逐位一致，均安装成功 |
| 普通菜单入口 | Index 显示「进入 GPU 沙水体验」按钮，点击进入 `pages/GpuSandWaterExperience`（router 转换完成有日志） |
| capability 显示 | regular：`T438_REGULAR · K192/Kw128 · GRS_GPU_SCENE_Q20_M1_V2`、进度 `V1`；experiment：`T438_EXPERIMENT_V2 · ...`——与 T438 README/双 profile 能力契约一致 |
| 显式新空 run | 「新空场」按钮响应，run 0:1→0:2→0:3→0:4 递增（regular）；experiment 同样可点 |
| XComponent | `grs-m1-experience` SURFACE 创建成功（RSSurfaceNode type:6、onLoad/OnSurfaceCreated/native OnSurfaceCreated 日志齐全），bounds [0,688][1280,2458] |
| 输入控件 | 沙/水选择、手动倾角开关、V1/V2 切换按钮在位；touch 注入（swipe/click on canvas）执行 |

## BLOCKED：GPU 路径无有效内容（不冒通过）

**现象**：每次新空 run 后状态行恒 `STATE_LOST / run 0:N`（regular 4 个 run + experiment 2 次尝试全部一致），准入/拒量/在途/落水全 0，画布纯黑无任何渲染内容（截图 s2–s6）；触摸期间曾短暂转 `PAUSED_CLOCK CLOCK_GAP / run 0:3`（脉冲在走）但从不 READY、从不出现任何粒子。

**日志证据**（hilog 原件窗口，进程 32486/37312）：
- GPU 队列线程反复循环：`async worker started` → `~QueueInternal start` → `wait handler Termiated done` → `Device init` → 重建——每次新 run 触发一次（regular 时间点 20:38:09/20:39:05/20:39:40/20:41:50；experiment 20:47:38/20:48:34）
- `EglWrapperHookLayer init Failed!`（C01400 OpenGLWrapper，worker 线程 33066）
- `base munmap ... process destroy 4g fail`（GPU queue 销毁时反复出现）
- grsm1 模块自身零输出（无 WS_EVENT、无 faultlog/jscrash）——native Create/loop 失败静默，STATE_LOST 由页面 status 快照呈现

**定性**：真机 GPU 环境（VSync queue + 系统 EGL hook）下 M1 transport 的 GPU loop 无法建立稳定 context/worker，每次 run 初始化即失败回 STATE_LOST。这与离线 ANGLE SwiftShader host（T280/T434 全绿）和 T421 预约链路（同包真机 RESERVED→ARMED→FROZEN 正常）形成对照——差异集中在 XComponent SURFACE 绑定的 GL context/queue 生命周期，属 developer 域（创建路径/线程亲和/EGL 配置），非体验执行问题。

## 未行使项（设备可做范围已尽）

- 空容器/切沙水/触摸落下/停源后水到达/积水侧扩散/水面观感——画布无内容，无法观察
- 录像：无有效内容可录（s1–s6 截图存档替代）
- 系统可见性、主观体验、帧率证据——依赖有效渲染帧，均 NOT_MEASURED
- 缺真人的摇晃单列——未到

## 证据

- `test/reports/t448/`：11 份布局（l0–l10）、6 张截图（s1–s6，含 STATE_LOST 空画布与 CLOCK_GAP 瞬态）、hilog 关键行已转录本报告
- 复现：冷启→菜单进入→新空场→观察状态行，双包双进程 6+ 次全部一致

## 所需前置（交 architect 分流，接 developer）

1. developer 排查真机 GPU loop 创建失败：`EglWrapperHookLayer init Failed` 上下文、queue 线程亲和、XComponent SURFACE 的 EGL surface/context 配置、85MB buffer 分配在真机 GPU 的可行性（T433 §4.2 App isolation ~82–86MB）；
2. 或裁定真机 GPU 首选回退路径（如 SwiftShader/软件 GL 分支）与能力披露方式；
3. 修复后本卡续做全部体验项。grsm1 静默失败建议加诊断输出（developer 域）。
