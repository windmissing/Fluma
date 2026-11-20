# T720 RX 与 G13 首帧 shader 编译分流

## 原件与定界

| 证据 | RX T594-v6 | G13 T654-v4 |
| --- | --- | --- |
| 原签包 | T719-rx，SHA256 `e6a34091…` | T654-g13c，SHA256 `d92ee0fb…` |
| 取自包内的 compute GLSL | 146301 B，2147 行，SHA256 `2cb9a848…` | 259025 B，4312 行，SHA256 `7563c614…` |
| ELF BuildId | `0f591129d680fb2bb398e731913b354c0197786a`，原日志明确进程映射值相同 | `1af61bbf379c3bd6620117df0fa49669981936ad`，旧现场未读映射值 |
| 最后阶段 | PID 23170，GL TID 23607，ticket/run/context=1/1/1。入队至 `SHADER_COMPILE_ENTER` 110.027344 ms | boot `342780307497971`、attempt 172。归档摘要为 `PROGRAM_BUILD` / `SHADER_COMPILE:ENTER`，handledTicket=0 |
| 实际采样与终态 | 布局见编译入口及 UI 存活，报告停点超过 240s。相同 PID 仍在 12:48:40.178 的进程列表中；原件无编译返回及 GL 线程栈 | 报告等待超过 90s 后离开应用，重入为 UNCONFIGURED。现存原件为回执、诊断摘要与布局，缺绑定 PID 的退出记录、四页同 sampleSeq、owner 线程栈和编译返回 |
| 可判定 | 队列已消费，GL owner 已进入源码编译；编译是观测窗口内的停点 | 旧诊断采到编译入口；进程是否退出及原因待系统级记录。编译是否返回同样未观测 |

两臂均无 Create、READY 或首帧成功原件。G13 目前可确认同类型的 GL 调用入点，比较设备端实际加载身份与耗时还需分臂取证。签包内的 RX/G13 shader 字节直接从对应 ELF 提取并与各自来源记录核对。完整证据哈希及最初的原始行见编译回执。

## 修复

- **RX 缓存入口。** 独立 RX 的 GL 工厂已能按环境、实际源码、layout 和 linkConfig 校验程序二进制，页面此前没有为该路由提供 filesDir。现在真实研究入口传应用 filesDir，GL owner 编译前从现有 T475 store 预载，沿原来的反射、warmup 与 audit 执行。旧双参数 NAPI 调用仍可用。预载失败走原源码路径，计入首帧总耗时。缓存文件与回执仍由已有 store worker 处理。
- **G13 UI 同步等待。** 源码证明 g13Stop、BeginSession 等同步调用可在队列中排到执行中的 `glCompileShader` 后，等待条件没有超时。现在同队列已有配置调用时，停止仍即时撤销权限并异步排入 owner 清理，其他同步请求明确返回 `CONFIGURE_IN_PROGRESS`。已进队的同步调用优先于可延后的配置，保证不会被后来的编译插队。GPU 资源继续由同一 owner 安全退役，运行中的 driver 调用自然返回后才清理。G13 已有 T475 缓存入口，预载失败只影响缓存。此修复覆盖可确认的 UI 等待路径，旧退出报告的具体原因仍需设备原件。
- **同票证据。** 两路记录实际映射 BuildId、sourceSHA、ticket/scope 与首个有效 swap，G13 在进程外可留存的 hilog 中记录编译/链接调用、状态和二进制加载阶段，仍保留 32768 B 诊断布局。RX 另记预载和 ConfigureResearch 返回。保留原有 SSBO/反射、同步、数值门、GPU owner 与 XComponent 显示链。

## 首次总成本和拆分落地条件

单程序冷缓存的端到端成本以 `t(首有效 swap)-t(配置提交)` 为主，同时独立记录算力资格、Build 返回、原生 Create 总耗时、compute READY，以及 swap。G13 显式激活需单独记录时刻，避免把人工等待混入编译成本。成本链依次含队列/EGL、cache preload/lookup、同源编译链接或合格 binary load、反射、该次导出、资源、种子初始化与数值验收、首个可呈现帧的生成与图形 program、成功 swap。驱动延迟编译可能转移到反射、初始化 dispatch 或首次 draw，分段时长须与总时长同录。暖启动需首轮通过并实际出现 PERSISTED；未知格式、坏条目或驱动拒载计一次同源码回落。已有缓存上限 32 MiB/条、128 MiB payload/32 条分别计算，连同每个角色实际 binary 大小和暂态提取开销比较驱逐与部分命中。缓存无法为首次冷启动提供现成 binary。

| 路线 | 有意义的候选依赖切分 | 首次 Create/READY/有效 Swap 成本边界 |
| --- | --- | --- |
| RX | 先验证只包含 ROOT_INIT、资格入口及其审计闭包的 Boot shader，和保持资源所有权的 Root/Source；若 Root 仍主导冷时延，再独立比较 COMMON/SEAL 与 FLIGHT、DM、表层流动等 opcode 依赖簇 | Boot 能收敛 Create 中 warmup 的编译图。首个通过验收的 root/发布仍要等实际访问的 Root/Source 与图形 role；仅把后者编译挪出 Create 无法证明首帧收益 |
| G13 | 先验证 op 64/65/66 的独立 Boot 闭包，再以源事务、根推进以及 PACK/AUDIT 为界独立找关键路径。只在数据指向根推进时，再拆 advection、viscosity 和 pressure/projection | Compute READY 仍需 Boot 初始化、FP32 双组与 fence readback。首个有效 swap 还需真实 root 提交、pack、场景反射与显示，READY 变快可能只是把后序编译后移 |

本卡保留原 shader 字节。拆分只有按真实传递调用闭包裁剪才值得测，替换 `uOp` 分支标签不足以证明 driver 编译图变小。正式变体须为每个程序生成并核验使用的 uniform/SSBO、来源/角色/环境缓存键和 opcode→program 全覆盖，维持相同资源布局与数据字节含义、原 dispatch 顺序、memory barrier/fence、生命周期和回放/数值门。每个角色记单独的 compile/link/load/reflect、切换开销与缓存 bytes。同时比三种端到端配对：单程序冷、单程序合格暖、拆分冷/部分命中。只有实际首次有效 swap 有收益且既有门全保留，才推进拆分。避免在第二上下文随意并行编译，当前两路已有 GL owner；跨上下文共享、保活与取消需独立证明。

## T713 / T654 最短取证

1. 分别签装本卡对应未签包，原件保存包 SHA 和打包 ELF BuildId。冷起只配置一次，分别录 RX `G9RxConfigure` 或 G13 `G13Init` 连续 hilog，匹配 PID、boot/ticket、run/context、shader SHA 与当前映射 BuildId。准备一次短的 UI 存活/响应采样。G13 保存同一 sampleSeq 的诊断四页。
2. 原 240s（RX）或 90s（G13）窗口截止仍只有 ENTER 时，保存进程 PID 与 UI/GL owner TID 的同刻状态，能取得时保存 owner 栈和原先后阶段。若应用离开前台，保留系统进程退出/重新启动和 fault/freeze/OOM 原件，及进程是否仍存活。按已观测阶段分流；未返回记为未观测，缺原件的退出原因保持未知。结束一臂后再运行另一臂，避免同机争用。
3. 若返回则续录 Create、READY、有效 Swap 和原始错误。G13 READY 后按一次激活并记录时刻。冷轮成功落盘后才保留相同签包装态重启取暖轮，附对应 cache key 的 T475 JSONL 与 binary SHA；若冷轮未返回，暖轮如实记未到达。整组仅以当前票的原始证据判定。
