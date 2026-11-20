# T696 · GPU 两程序与持久化四臂工程合同

2026-11-20，architect，消费 T689 正式候选。先比较 **完整传输三程序与两程序 × compute 持久化关与开**。每路线单独配对，M1与M2各自保父版，设备收益待测。独立 QuantumCandidate 始终构建并使用自有程序。T682 的中间程序 compile+LINK 用时约15.001s，只用于确定优化优先级。

| 构建臂 | 完整 Create 的 compute 角色 | cache |
|---|---|---|
| A0 | SOURCE(0), INTERMEDIATE_Q(1), FINAL_Q(2) | 原 GPU 源码，保持 T663 cursor 行为 |
| B0 | SOURCE(0), FINAL_Q(2)，1=NOT_USED_BY_VARIANT | 原 GPU 源码 |
| A1 | SOURCE(0), INTERMEDIATE_Q(1), FINAL_Q(2) | T475 factory 逐角色冷/暖 |
| B1 | SOURCE(0), FINAL_Q(2)，1=NOT_USED_BY_VARIANT | T475 factory 逐角色冷/暖 |

计算角色的持久化开关与选择结构彼此独立，均在生成时确定，不新增运行期可变权限。推荐本卡版本的窄 CMake 枚举 `GRS_FULL_CREATE_VARIANT=THREE|TWO` 和 `GRS_FULL_COMPUTE_CACHE=OFF|ON`，生成 header 与 CMake/manifest 同一权威，在同一路线所有相关 TU 核一致，非法值构建失败。默认保持各继承产物既有行为，生产切默认另按实测裁决。四臂和独立Q的构建契约不因临时对象缺席重新编号。

## 生成与实现归属

- M1 以当轮冻结的实际产品源为基线，`app_bridge.cpp::AppTransport → m1::FullM1Transport`，保真实 CMake 和 XComponent。M2 使用 `tools/m2/prepare_cursor_build.cjs → test/t468/generate.cjs → T452 实际 App/m2route`，固定 T663 原生输入/生成回执。每路线四臂使用同一父源/增强。当代M1的 ledgerPrefix/计账不可回退成 T663 M2 版本。生成前核 git commit+源SHA与逐对象编译/回执闭包，编程接点在实际 `m1` 或输出 `m2route` 的三入口。以受版本控制的真源和显式 late overlay 实现，禁止手改生成缓存或整套套 T475 历史恢复 overlay。
- `m1/{quantum_candidate.h,.cpp,full_transport.cpp,source_transport.cpp}` 和 M2 对应真实生成位置。把 QuantumCandidate::Create 拆为内部私有资源计划、资源分配、角色 program 构建/反射步骤，沿 C++ friend FullM1Transport 的既有权限。建议接口 `PlanCreate(k,kw,owner) → CreatePlan`、`InitBaseResources(plan) → TransportCode`、`BuildAndReflect... ` 局部职责，不向页面或通用外部导出半完成对象。Plan只校 profile/owner、布局/容量与原 capability，不分配 GPU、不读入替代物理参数。A原顺序严格为 plan→临时build→分配→临时反射→建表→退临时→最终build，独立 Create 同此第一段且不删除自身可执行。B为 plan→原分配/角表→建同一表→build最终 program→原扩容/最终反射及 Prepare。独立Q即使在 B也保自身 shader、原 API/Frozen/Capture/Release；若它使用共享工厂，策略必须在四臂固定且单独标记，不能拿其编译算完整传输第三程序。
- SOURCE build/load 后现有 reflect/prepare/资源门保持。最终Q向同一当前 program 重新反射全部真实使用的 uniform/type、必要项/允许优化项和四SSBO绑定。保 pipeline/layout/math/angle/role SHA和真实源字符串，A/B中 SOURCE/FINAL GLSL 对应字节必须相等。原 program/site 编号、各路由实际 passes、barrier/dispatch/fence/lease/命令布局/提交和接受权威不变；T341 warmup 仍真实运行，T536组拆分和其额外政策不混入这轮。

## 资源与失败边

四档逐一核旧公式、分配大小、偏移/对齐与 first failure。候选bank保 `words=16*K+16*Kw+2048+256`，基础workspace保 angles 和 sourceEvents，原 `expanded=workWords+words+256`，T663额外+8192 words 当作路由事实。Q eventCapacity为 `4*(48*K+16*Kw+254)+K`，每条256B。

| K/Kw | Q candidate bytes | 原最终 workspace M1 / T663 M2 B | 单 Q event arena B |
|---|---:|---:|---:|
|192/64|25600|547408 / 580176|10795008|
|192/128|29696|552272 / 585040|11843584|
|512/64|46080|570192 / 602960|26605568|
|512/128|50176|575056 / 607824|27654144|

以上是组件数，不是全设备峰值。保旧分配阶段、扩容后角表重传、两个Q eventArena、2×1024B结果、isolation仅有资格时等长scratch，SOURCE自己的双bank/event/pub/result另帐。业务buffer数保持等价。逻辑事件与冷span剩余额度、孤立 staging、旧新代重叠、场景FBO/纹理和 GL program/driver/缓存文件峰值分别记录，未知不填0。移除中间程序不等于减少全部 GPU payload。整块首次按最终尺寸优化另立变量。

owner 在每阶段保一份绑定 attempt/epoch/contextGeneration/window/tid 的私有 RAII 清单，再事务式交给已核对 scope 的 FullM1Transport。原因保 capability、allocation、compile/link、reflect、prepare、warmup、retired/lost 原分支。次序内已经取得的对象逆序回收；eventArenas[0]和quantum.events 是同 GL handle 的借用/拥有关系，删除必唯一；scratch/arenas交换也要同步元数据。首次失败原值和清理失败原值分列，失败无 READY/新bank发布；有飞行/lease/退役未完成保持既有 WAIT/FAULT/STALE语义。驱动调用可能未返回，不能由控制/UI线程抢删。真实 owner 返回时再查 scope/撤权/原期限，失效阶段不得迟到发布。合法退役时 owner 先清该代 factory 记录再删除，ContextLost 时丢弃该代 CPU代账而不触发失效 context GL。只以 GLuint 或 OS线程同名无法绑定被复用对象。

## 持久化和阶段写入

A0/B0 走现有真实源码 Build/游标，完全绕开新增 cache probing、load、export、I/O，图形缓存固定同策略。A1/B1 复用 `program_cache/{program_factory.h,artifact_store.h}`、T475 真实类 EntryAbility 的 filesDir/configureProgramCache。先核本次同context环境、精确 GLSL、布局ABI、link config/format/full SHA；epoch、GLuint、包重签仅证据，不做key。命中创建本代新的 program，检查真实 binary load/GL_LINK_STATUS/原GL错误，重 reflector/uniform/资源。格式/校验/driver拒载，丢弃本候选、原 GPU GLSL一次回退；原产品 GL 错、反射拒绝、资源不足或撤权保原结果。日志/统计失败不抹产品首错。缓存关、无兼容格式、miss、拒绝、部分/全命中设置不同标签。

旧 `Prepared` 同时登记资格与同步提取 binary，旧 `Warmup`只从qualified读。改为独立的“反射/准备完成”与“准许导出”阶段，复用原工厂并保其它调用方行为。SOURCE可在自身合格且仍当前后入credit，旧已合格不因Q失败删除。最终Q在真实 T341 warmup成功与当前owner fence后才能提取并入队。A的临时Q亦仅在其原反射/分配成功后可导出，替换前 Forget/清该代；已提交的不可变 artifact只由既有I/O worker验证、原子存储和LRU，施工代不是线程借GL借口。清理所有 Delete/被复用/lost 路径的 pending/qualified，以 `contextGeneration+owner+programIdentity` 辅助现有map，未获资格不持久化。节点日志区分 EXTRACT_ENTER/EXIT、queued、PERSISTED/失败（实际目录回执）与 READY。独立核真实 GL 提取阻塞；不得偷放到热root、不把文件存在推资格。

T475 单件32MiB、payload128MiB、32件、单key历史128KiB、队列128保持。临时读回、PIN、写盘/淘汰、旧新同时持有另计。同一分测设备与驱动，禁止拿910 binary喂935F做暖证。缓存ON真实 cold、部分持久、全部持久、保数据新进程暖和新context/合法同context场景独立标记。清app缓存前仅目标专用目录；驱动隐式缓存不可控须标条件。

## 设备证据与接力

每臂交两半收据。编译侧包括生成源/CMake/options、实际角色表、profile/ABI、链接包 SHA/BuildId/ABC、真入口及原编译返回。运行侧由 tester 离线绑定、effector 串行真机，核同进程加载原件并记录真实 attempt 边沿、role、SHA、raw start/end/return/error。保实际 context/admission、各角色 compile/link/status 或 binary/status、分配、反射/准备、warmup、export 与 READY；统计总Create/首有效帧和成功率，未知和右截尾列入总样本，cache load 不记成 glLinkProgram。最初建议每臂至少5独立尝试交替配对，设备热态、源输入、四档和新进程/缓存条件明确，低样本P95为描述。实际复用 T682 现有期限和 60s/64 query 原证据，过窗以真实同scope续件及终态登记。

V01–V07/V09–V10 按 T689 原义续证，原资源/取消/窗口变代、拒载/坏key/淘汰、完整 nonempty SOURCE/Q、守恒/帧token原件均为独立检查。对照 A/B 四档相同应有 full Q dispatch 113/127/122/136、独立Q 105/119/114/128，路由 SOURCE 8/13按实际表。保原10op/8ms分批、250ms root和双GAP，首内容需真实 Swap 和对应画面，随后按有效帧30fps与帧间隔/热资源衡量，空READY不代。C依赖切分仅作后续候选（V08），保单变量。

developer完成工程接线和真实产品编译，tester维护离线收据/脚本、签包和差分，effector负责全部真机串行。无独立真机或manager关闭门。本卡不使用真机。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外）。GPU驻留、CPU停投，前景背景仅XComponent+shader，失败FAULT或明确STALE。
