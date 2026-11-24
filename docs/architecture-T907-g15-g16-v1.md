# T907 · G15 扩角干沙与 G16 跨竖直小域工程合同 v1

2026-11-24 · architect · Windows PC Vulkan · 离线设计。G15 从现有 G9 DM2 扩域，首轮沿 T906 干床、±45° 和静止承托限制。G16 独立使用全占位圆盘验证支撑、离壁、首撞和重堆，初态冷建档。两者都驻留 GPU、只由同根 shader 呈现，禁止 Canvas 回退。干沙不等待目标 13/14/12。 [T906 正式算法及阈值](algorithm-T906-planar-avalanche-v1.md) · [平台 E01–E15](architecture-G15-G19-research-constraints-v1.md)

## 1. G15 · 复用现有 G9 权威状态

### 1.1 版本、姿态与定点

- 建立独立的 G15 DM2 派生 profile，由同一 manifest 生成 W 配置、Expected、SPIR-V、角色、角表、冷档字典和 Scene ABI。父 DM2 的 ±30° scope 继续用于回归。当前 PC host 的 angle 校验和查表、DM transport/许可、progress clock、quota reader 及 shader gravity gate 都有 `±7680`，新 profile 逐一升级到 `±11520`（1/256°）。上游的 85°/s 与角域先在原连续姿态校核，人工或夹具超域具名拒绝。T843 的原五次段及冻结初态、输入 SHA 进入 Expected。按同一 accepted 时间轴对每个 J 的微步取中点姿态，用端点姿态登记连续输入和计算角变功。SOURCE 不消费时间，失败根保持原时间。切段点按同一数学时刻核连续性；时间取样位置作为 profile 字段冻结，所有角色共用。以 1/7680 s 的整数索引表示上述时刻，包含 J32 的半微步。
- 新生成表为 `sin/cos` 有符号 Q20 LE 对，每个角一行，从 −11520 到 +11520 共 23041 行。索引 `angleQ8+11520`。用高精度生成并核 RNE 及奇偶对称，零角精确 `(0,1048576)`，端点 `(±741455,741455)`。配置和冷档保表 SHA 与取样字典；GPU 从已冻结表读取，同一微步内 `(gx,gn)` 一套。保旧 `uAcceptedAngle/uAngleSin/uAngleCos` 对父 profile 的语义，新 profile 用独立时间输入和 table range，在每个角色的相同既有阶段先锁存实际角字和投影再推进。GPU 评估冻结的分段输入，独立 reader 以多精度复算 sample/quantization、范围、速率和临界点。上电静止重力在 x/n 方向按既有 G9 `gravity×Q20/2²⁰` RNE 换算。
- 沿 T554/T647 保 `S/F/P` 整数 Q20 库存、每列水平动量 S64 low/high、实际来源段和原 free-pool 世代；Q16 摩擦、速率与库存换算各用原分母。列宽实际为 `0.10/128 m`，模型高度换算沿父 manifest，斜率分子和列距先转同一空间量纲。以统一的正分母 `dx` 比较 `d=gx×dx−gn×ΔB`、`an=gn×dx+gx×ΔB`，`5|ΔB|≤3dx`、`4an≥g×dx`。对正/反方向按 Q16 的 `(±d)<<16` 与 `µ_s×an` 比较，等号静止，先判支撑和范围再除。两端理论最小承托约 0.28284g，实际量化也逐面检查。S128/U128 乘加、带符号 RNE、非负 floor 和检查后收窄顺序沿父合同。以正分母计算带符号 `S_eff` 工作量，允许有效床有负参考量，高程值禁止回写 S 或先 clamp。每一输入、动员后、中点、结束的支持与 CFL 检查实际值，域错、容量错、数值错、可细分 CFL 分别留原原因。

### 1.2 实际面流、来源与最终提交

| 原 DM 阶段 | G15 写集和独立核点 |
|---|---|
| 冻结入口及 DM_REQUEST → DM_APPLY | 从同一旧 `B=S+F` 比较两侧有效邻面，将正失稳余量与 `an` 的比值交原深度/速率/来源上界，初轮 shear 系数零。来源 prefix、真实 q 和 p 不变由原 DM_NATURAL 账核对。等阈静态、超坡和超角分别留首面；墙面无外侧通量。 |
| PREDICTOR_FACE → MIDPOINT_BUILD → CORRECTOR_FACE | Predictor 仅 WORK，保 `pEntry` 和半步 `pMid` 的原舍入。预测与修正各以同一瞬时的 `gnF²/2` 及等效床做水静力重构和侧压力修正，取消这个 profile 同阶段原有的显式 `gx F Δt`，重力只计一次。真实有符号面通量 q、动量、两侧压力及支撑标志只从 corrector 消费，来源不足按实际可用物料缩减。静态成对屏蔽仅归零质量流，反力与压力仍入账。动摩擦用中点有效法向及实际使用的冲量。墙反力、零通量、归零前后和符号反转逐项核对。 |
| 通量应用 / 摩擦 / 捕获 / 停积 | 一笔有符号真实 q 由邻列等量分账并同步搬来源。P 保实际撞击/接触/合法离床语义，归零事件只消费实际余时一次。静置稳定由物理时间累加，沿父 `1/3840 s` rest tick，0.10 s 为 384 tick。每次合格微步加 `32/J`，破坏资格清零。仅在 p=0、有效支撑、实际相邻 q=0、无输入/撞击/动员且来源可表示时 F→S。无需把任何旧 bed 或 F/P 画面作为当步成功。 |
| audit → replay → typed Commit | 按 T906 实际 B 势能及前后重力投影、墙反力和碰撞、动摩擦、连续姿态角变功重算能量，禁止把 `S_eff` 值当实际势能高度。诊断 tie/raw pose/面通量入同根原字。保原发现、权威、拒绝前缀、accepted replay 和最终审计，actual/replay 同 BASE 与 pose 输入，保原位 16/17 与扩展资格。任一失败禁切库、禁增 commit/accepted time 或发 CURRENT。 |

原 `DM_REQUEST/APPLY`、面流和来源所有权是复用基线。新增角时序、等效床和摩擦按上述绑定实施，原先已证明的小角度等价范围另做回归。 [父调度与字典](architecture-T554-g9-dm-dispatch-v2.json) · [父 DM 工程](architecture-T554-g9-dynamic-mobilization-v2.md) · [T647 接受与 rest tick](architecture-T647-v1.md)

同一 Commit 锁冻结 `run/nonce/rootKind/base/nextCommit/sourceCommit/acceptedQuanta/poseInterval/config/layout/seed + stateBank/faceWitness/publication slot/generation`。SOURCE accepted 不长物理时间，重复无新增，QUANTUM 只加一根。新增面流可见性在 PACK 从最终经过审核的 corrector q 派生，实时 marker 消费实际有符号 q 与来源；当 WORK 会在下根改写时，把呈现所需面通量复制到各自发布槽。shader 同时消费同根 S/F/P、面流和姿态，既有 G11 组合 writer 可沿 T839 的联合 proof 接入。首轮也可由派生 profile 的独立 shader 直接画同根 S/F/P 及面通量标记，分别验物理和标记读取。用一枚业务 fence 与实际 live permit，保 DM2 的 250 ms 和 33,333,333 ns。SceneLease 绑定 frame、publication、窗口/几何/swapchain/image 身份；计算、画面和冷读分别持 pin，真实完成 fence 后释放，WSI 另按实际 image retire 计。已提交缺 fence 时隔离该代，超期或错误保上一有效新管线帧标 STALE 或具名故障。 [联合呈现和退役](architecture-T839-v1.md) · [原 PC 部署边界](architecture-T902-v1.md)

### 1.3 资源与 G15 放行

干域 N128、K192、每列床/流来源各 64。原计划最坏 12 微步 pass、2404 根派发，五 state、四 arena、四同时 compute SSBO。新增仅从已有 WORK 或 PACK 可寻址区间取；需要新 dispatch、event/path 行或额外可读场时重算并保首个不满足证明的上界，不挤旧缓冲。

| 增量账 | 逻辑上限 |
|---|---:|
| WORK 中只读角表，表 184328 B 加布局余量 | 184576 B |
| WORK 姿态与根、J/事件和审计工作区，其中 pose 记录上限 63 个试探取样项 + 冻结端点 | 32768 B |
| WORK 面 129×64 B、列 128×64 B、对齐及墙账 | 20480 B |
| 两个 publication 槽各 8192 B（含 129 个面记录），当槽持 lease 时保持不可写 | 16384 B |
| 余量供实现测量对齐及剩余合同 | 7936 B |
| **新增上限 / 与父逻辑 resident 合计** | **262144 / 55601792 B** |

原父 resident 55339648 B。以上为 packed 规划，实际 device bind 的 minStorageBufferOffsetAlignment、VkMemoryRequirements、最大范围、FBO、driver、FULL/ROOT、临时冷复制及旧新代并存分别核收据和底层字节。设备实测不满足时用确定窗口、合法退役或拆分工作区重新证明；冷迁移只在 profile checkout 和代退役完成后进行。保现完整 shader 编译输入每单元/变体 49152 B 上限，先验证产生的各模块。`2,404` 为逻辑父调度上限，正式 profile 需提供逐 pass 绑定/写集、最坏 row/path、宽算术取值范围及 GPU 实测方可确认复用该计数。先交 ±30° 父回归、15-A 子模型和 ±45° 真根，再跑 15-B/C 实际砂丘、支持边界、早停及退役负控。T902/T904 的 DM2 实际根/超期证据用于公共 writer 和预算对照，G15 仍需实际接入、可接受根及定量场和画面证明。

## 2. G16 · 小域技术验证接口与放行

13/52 盘可执行种子、真实壁姿态与多盘读取接口沿 [T928 工程修订](architecture-T928-v1.md)。旧旋转壁 GPU 原件的 TOI 门和小域阶段状态以该修订的独立复算为准。

1. **初态与姿态。** 单盘解析接触先行，随后实名 13/52 盘初态。冷建档固定 `id/generation/origin/m/r/I/material/pose/position/velocity/spin`，保 SHA、SI 单位、面积率、来源质量、质心、初态无超界重叠。接入连续姿态 sampler `{t,theta_cont,phase,omega,alpha,center,inputId}`，消费 T843 正反向五次段和快慢、近 90° 保持对照。接触点输出墙 ID、真实法向、世界坐标和 `vWall`，世界重力恒向下。固定 `1/120 s` 名义根，细分及根时间由 GPU 资格决定。对 G15 共享时钟/档案协议即可，G16 启动不要求 G15 产品通过，也不需要水线或从 S/F/P 重造力链。
2. **候选及范围。** 以段起始姿态作参考坐标，把盘运动和连续旋转墙的扫掠界换到该坐标系。桶宽/高不小于最大直径。2.5 mm 试验桶在盒内为 40×80，边界按已证明的扫掠域纳入实际桶及墙候选，越域、bin overflow、邻边/总边超限时拒绝或细分。用固定 ID 前缀建立 unique `(idLow,genLow,idHigh,genHigh)` 或 `(id,gen,wallId)`，两端邻接按 key 排序；列出实际扫描桶和候选次数 Q。先以独立小域逐对参考核无漏边、无重复、扫掠夹根及 TOI；产品 GPU 走分桶。每盘邻边上限 16、总边 8192、N≤1024、桶≤3200，所有边一次计数，含壁候选的预算与上界写入验收清单。半径 1.25 mm 的等粒盘在 1024 容量下面积率约 25.13%，35% 至少要约 1427 盘；如需在原槽内以大粒径验证更高面积率，先冻结独立粒径/种子/保真判据。13/52 的小域证据不能代替 16-C。
3. **冲量与步骤。** 真实第一向内夹根触发事件；按同容差分组接触并推进余时，普通跨 90° 无状态强制切换。每轮边从共同速度快照更新压紧、切向冲量，盘按固定邻接顺序归集到下一快照。保 `gap,lambdaN,lambdaT,vN,vT,vTarget,TOI bracket,wall velocity,wall work,contact residual` 原字。没有不合格负冲量或越过 T906 残差门的通过路径。起始档位为 J≤256、≤2 段/微步、≤16 投影轮；标注最大位移 `0.1 r_min`、墙扫掠范围、真实容量/耗时。缩步不足、焊死/掉接触或迭代上限真实报错并保原 accepted，内存分配与超期失败独立。FP 运算、归约顺序及禁止未约定 contraction 从模块反射和小域复算核对。
4. **独立输出。** 在完成收据另分记录 `candidate range/TOI check/contacts+solver residuals/state+scene audit/actual bank generation/time/deadline`，出档绑定 GPU 物理审计、选中事件窗口、私有失败前缀和实际图形结果。独立核 `ΔP, ΔL, Δ(K+U)` 的源/反力/墙功/耗散。接触每盘实际源，按要求导出飞行、贴壁、支撑移交、首碰及重堆；视觉直接用世界盘位置、来源和同代姿态的实例，PACK 回填同一 lease，保 buffer、draw/graphics fence、真正的 image/present 及退役。当前验证使用版本化 250 ms 诊断臂和同根 deadline。借用产品 owner 时原期限和有效 pulse 照旧；私有算子或慢速档保实际耗时并仅提供算子证据。

### 2.1 绑定、范围和成本

所有逻辑区按字节下图，descriptor 从实际 buffer 的合法对齐 offset 绑定，内部 offset 由生成字典决定。每个阶段至多四个 compute SSBO；只读绑定同一个 buffer 的多个内部区用 offset 字，不增 binding。实现提供 device limits、反射/实际 binding、byte range 和并存账。

| 缓冲 | 逻辑布局与所有权 |
|---|---|
| 0 · 盘状态 | 196608 B。三段各 65536 B，接受只读 bank 与私有读写 ping/pong。保已接受 bank 的 pin，失败不发布。完成后封存候选，有 pin 的资源不能当下一根的 scratch；需要多一代并存则计预算或等待真实归还。 |
| 1 · 接触 | 1179648 B。三段各 393216 B，候选/工作边及两套更新或审计区，均为 8192×48 B；接触 key 和 slot/generation 随段保留。 |
| 2 · 邻接、桶及工作 | 608256 B。邻接两端 262144 B，bucket 51200 B、盘排序 32768 B、审计/临存 262144 B。先建稳定分桶/扫掠，TOI 与邻接沿有向 key 归约。 |
| 3 · Scene | 65536 B。两段各 32768 B，同代实例至多 1024×32 B。PACK 后供 shader 或合法实例缓冲读，旧发布槽待实际 graphics fence；WSI 独立退役。 |

因此缓冲 2 实际是 608256 B，总逻辑 2050048 B。物理胶接、对齐、staging、日志/FULL、图形和多代并存另计。逻辑预算从 T906 得 `256×2×(12+2×16)+32=22560`。定序先给 12 个非迭代逻辑槽分配预测、桶计数/前缀/散布、配对/首撞归约、构建/定序邻接、接触准备、写回和审计等职责，每轮边更新/盘归集 2 槽。若任何职责要拆多派发，机器表按真实 shader 调度证明所需次数和同步，诊断 ceiling 不计作已证性能。每段随证据报 `B/N/Q/C/I_c` 及实际逻辑槽、派发、屏障、峰值、退役和 GPU 时间。稳定分桶后复杂度沿 T906 的 `O(B+N+Q+I_c·C)`。

### 2.2 真实前置与接力

G16-A 开工前只要实名单盘 fixture/材料、T843 连续过程和独立孤立接触 oracle、GPU 接口字典及合法计算/呈现能力。G16-B 增加可复现 13/52 盘 seeds、扫掠/桶边容量和多触点 residual 读写，严格跑 T906 两档步长×两转速、穿透、壁冲量及重堆判据。G16-C 再升到实际代表面积率，保留对照并按成本或接触失败分流，必要时交 designer 对比备选。G15 与 G16 架构准备可并行，产品排期交 manager。developer 交可构建的 profile、真实字节和接受接口；tester 独立重算约束、宽整数或接触残差与正负对照；effector 采同根 PC 效果。开发集显优先验证功能与正确性。目标独显用户测同一版本、种子和实际场景，记录 acceptedSimTime/wallTime、新接受画面（用户要求 30 fps）、实际显示节奏、根 p95/p99、最坏和帧间隔。 [姿态原件](test-report-T843-v1.md) · [独显交接约束](architecture-G15-G19-research-constraints-v1.md)
