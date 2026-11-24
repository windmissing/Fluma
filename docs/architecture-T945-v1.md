# T945 · 目标9续根与目标13纯流场的集显工程合同

2026-11-24 · architect · Windows Vulkan。两项在 Intel UHD 630（8086:3e98，31.0.101.2140）独立推进。目标9先取得可信 DM2 QUANTUM 再开 FULL。目标13先解除 OP_48 资格阻断，再独立缩短真实 F02 根。复用 T944 的计量和退役接缝，保各自物理、布局及 typed 接受。 [T944](architecture-T944-v1.md)

## 1. 目标9 · 同进程 SOURCE → QUANTUM → FULL

### 已有证据与未测阶段

- T923/T924 六包资格通过，四份 DM2 SOURCE 各自 verified，按真实含一个 origin 的 seed 做受限归一后六对全等；T922 的原 diff 仍只覆盖空 origin seed。R1 同批 SOURCE 超期。先用 DM2/K192/S40 与 T924 split-v2 新宿主稳定复用 14 派发 SOURCE，保 T921 同 roleSet SHA。T924 提前准备实测 DM2 SOURCE 从 39–65 ms 录制降到 0.3–2.05 ms、3/3 接受，收益用于稳定起点。T927 后续引用了这些原件。 [T923 核证](../test/reports/t923/report.md) · [T924 六包和准备](developer-T924-v1.md)
- T921 split-v2 同进程 steps1-r2。SOURCE nonce1/plan1/fence gen2，grant→stop **232.570 ms**，14/14/14、accepted，commit 1/A0，3 条事件。随后 QUANTUM nonce2/fresh plan2/fence gen4，grant→submit 返回 **3.7892 ms**，还余 **246.2108 ms**，业务 VkResult 2、tailRead=false。计时只在 Root::Source 开，SOURCE 31 点、Q 缺原阶段时标。首个 Q 主要消耗在实际提交之后的区间，GPU 各阶段份额待补。 [同进程 stdout](../test/reports/t923/runs/t924-runs/dm2-t913-split-v2-at-t921-steps1-r2.jsonl) · [计时开关](../pc/vulkan/sand/owner.cpp)
- 晚冷档为同句柄后续诊断 gen5。19 件 SHA、INPUT128/GPU_INPUT128 均复核。Q 的 CURRENT_BASE==BASE==上一 SOURCE ACTUAL。Q 的 private ACTUAL/REPLAY 仅头字 commitLo、acceptedQuantaLo、lastNonceLo 不同；晚尾见 selected J=1、2404/2404/2404、0x3ffff、260 EVENT/1 PATH、私有提议 commit 2/A1。晚尾与比较保诊断身份，gen4 的业务拒绝和已接受 CURRENT 仍按原账。 [迟到原封套](../test/reports/t923/runs/t924-runs/raw-dm2-t913-split-v2-at-t921-steps1-r2/diagnostic/1663211005500200-2/envelope.json)

### 工程增量与实测矩阵

1. **先交 Q 时间线。** Developer 用 T924 配套包、同一原 FIXTURE128.bin（3580ccb1…）及 roleSet f51229f4…，交错跑 SOURCE-only 与 SOURCE+一根 Q（--steps 1）。前移 SOURCE reset、实际 SOURCE rate 和冻结参数固定；首组关 WSI/FULL，raw envelope 保留。新增 Q-only 分段查询，至少标 upload/clear、六组 DISCOVERY、SELECTED、AUTHORITATIVE、REJECTED_PREFIX、REPLAY、COMPARE/PACK/SEAL、tail copy，及实际 active J/micro、首错和空槽数量。保 query reset serial、业务 fence 对象/代、真实 grant/activeRemaining、device period/available。晚 Q 维持原 250 ms 拒绝，实际退役后才收同 fence 查询并另作冷拷贝。从原样本区别没有 grant、未提交、业务超时、坏 tail 与 native acceptance 失败。按同包开/关点位独立计量。禁止用重复 SOURCE 替代从同一 accepted CURRENT 续 Q。
2. **首个可实施优化取预排空体。** 2404 逻辑槽＝六 J discovery 804 + rejected-prefix 804 + AUTH 392 + REPLAY 392 + ROOT 12，12 类微步操作各预排 190 次。先从 Q 原点证明哪些组仍执行实际工作。首个 profile 只改一个已证无副作用的 post-selection 空体或尾部空 micro，用 GPU 权威 latch 调度轻量槽或间接派发。保原逻辑顺序、初始化、完整拒绝前缀、真实选中 J、原首错/掩码、独立 replay 和 seal。真实 dispatch/barrier、逻辑 opcode 和 Expected profile 明确绑定；旧执行器作为同输入对照。保持 Q submit 和原期限，非法 latch、错代、缺前缀与触发真正 J2/J4 的负控仍拒。不得强制 J1。 [冻结调度表](architecture-T554-g9-dm-dispatch-v2.json)
3. **再处理 active 段。** 128 列、K192、实际 A 活槽、D 路径步、L 来源前缀和 E 事件。若拆时标显示 active 显著，先按一项拆并行字段：动员宽算术、FLIGHT_PATH/FOLD、来源链或独立重放。保持 q/q+1 证书、top-first/Hamilton、真实 midpoint/corrector、F→S 和 P→F。目标成本随实际 J·(N+A+D+L+E) 与确有依赖的有序写项增减；动员活跃 Eval/宽除须实报。先用原范围子组并行与逐字等价宽除；跨列和来源信用、事件顺序继续精确。T944 可借计量/归约原语，M1 的库存/事件不能充当本族证明。

**Tester/effector。** 先冻结同一 SOURCE/BASE，以原 250 ms 取得 fresh Q、native accepted token 及 commit1/A0→2/A1。再走无 SOURCE 的连续接受、DM20/DM22、DM29/DM31，真实查 S→F→P 与 P→F→S，复用 T670/T838/T845。核 actual/selected/replay、有效 PATH/EVENT、各源守恒、物理 q、能量、撤权和首次真实退役。连续 Q 正例后另采 ROOT 与同根 shader；再开 FULL，执行前预约六个 discovery arena、实际 prefix 与 lease，单槽 ROOT 26,750,976 B / FULL 103,354,368 B，实报总成本与实际 GPU。首个正例先保真实同代 state 和 publication，不能靠事后读取拼成 FULL。 [三态和 ROOT/FULL 合同](architecture-T845-v1.md)

## 2. 目标13 · 启动资格与真实求解

### 第一件交付 · OP_48 物理拆分

T916/T921/T923 默认和 noopt 的四臂均在 60 s 仍 CALL_UNRETURNED，45 s 栈与高 CPU 定位到实际 igc64 长调用；T890 曾同 SHA 在 11.1103 ms 返回，T901 曾用约 1226.7 s 返回。复用证据，停止已无分化的 noopt/前缀重复试验。T937 的 G15 元数据结论属于其实际模块。G13 下一自变量是实际入口闭包。 [T921 原栈](developer-T921-v1.md) · [T915 原件](architecture-T915-v1.md) · [T923 noopt](../test/reports/t923/report.md)

从 T916 实际 Vulkan role_OP_48 用 build_roles.py 的 split_group 试编三份。保私有 FP32 OP66、四 SSBO+layout UBO、24 B push、64 lanes 和 FP32 控制。Vulkan 1.1 SPIR-V 校验、反射和旧模块逐字复现均通过。

| 角色 | 业务 opcode | 完整 GLSL / SPIR-V B | 模块 SHA 前缀 |
|---|---|---:|---|
| 原 OP_48 | 48/49/52/54–60/62 | 49,073 / 261,304 | f01959cc |
| 黏性 | 48 | 16,276 / 71,676 | 812db31f |
| 压力 | 49 | 24,613 / 125,372 | ca603a59 |
| 其余 | 52/54–60/62 | 34,283 / 171,208 | 64101778 |

Developer 在 PC 生成器及 native 角色表增加三路，完整生成新 manifest 与 PE。保 820 逻辑槽和当前 OP7/OP17 物理拆段、buffer layout、原 opcode、root ABI 和顺序。用同一资格宿主将旧闭包一次与三份候选交错，冻结设备/layout/flags/实际时间窗，留 ENTER/EXIT、CPU、实际栈/缓存、各模块真实 OP66 原字。拆分信号定义为旧调用仍停而三新角色均在窗内返回，且同包完整前缀到 COMPUTE_READY，实采一次 F02。若仅压力或黏性角色卡，保同一窄入口再拆该算子真实阶段，独立验证每一阶段；不为编译绕过修改算法。 [PC 生成和校验入口](../pc/vulkan/build_roles.py)

### 第二件交付 · F02 至可信业务场

- 固定 test/reports/t868/fixtures 的 F02 CONFIG 295c075e…、STATE e1c6b49e…，64×64 PRIVATE_PROJECT。T901 实际业务超时，晚 fence 的 whole **1770.312 ms**，ACTUAL 712.151、REPLAY 551.579、PACK_DONE→BEFORE_TAIL 499.658 ms。三个区间占约 40.2%、31.2%、28.2%。高层 pressure/viscosity 标记交叠并跨预排槽，不能用其覆盖代替 active 内核时间。14 个 OP17 位置只有 ACTUAL/REPLAY 的 J1 位置 active。旧尺度距 250 ms 约 7.1 倍，单改尾部或提前录制不足。 [T907 计时原点](architecture-T907-g13-v1.md) · [冻结配置](../test/reports/t868/fixtures/F02-config128.bin) · [冻结状态](../test/reports/t868/fixtures/F02-state.bin)
- 首轮复用原 --g13-fixture F02 CONFIG STATE --g13-diagnostics hot，补 815–819、最终 transfer、ACTUAL/REPLAY 实际 active J/micro 和压力 cycle、黏性块前后。用同底座 ABBA 独立量 T909 --g13-seal-v1，定位原 819 清拷贝及链式状态 SHA。保原摘要字序与全量；若 active 或编排空槽占主因，按 GPU 可信 latch 轻量空 trial 或已收敛块，保所有真实 J、原首错、完整重放与实际 fence。再挑实测最高的一个压力 V-cycle 或黏性 block，按规则网格 tile 并行、固定归约和 Vulkan barrier；保 Q32/RNE、S128、浮点和原收敛门。实测需要改数值算法时携原材料交 designer。
- F02 用原制造离散 π、MAC u/v、RHS 和投影/散度，按 T631 更紧的用例证书。PRIVATE_PROJECT 仍报 acceptedRoot=null。再用 F03 周期剪切先取至少 30 个真实 J1 业务根，沿 ν*=1/16、A=1/4、T=1/4 及 BE 幅值、合法步档/网格、ν 倍增验黏性。F05 用独立 forcing 的非零闭盒 MMS 及 REST/RAMP_SPIN，核物理压力、速度、黏性和墙面剪切。独立 checker 取各场的原字及原 fence/期限/范围，不使用渲染图推算压力。保 T828/T721 原数值门：散度 RMS/max 2⁻¹⁴/2⁻¹²，黏性 RMS/max 2⁻¹⁶/J、2⁻¹⁴/J，压力证书 2⁻¹⁶，投影速度证书 2⁻¹⁶/J。同时覆盖不守恒、实际 late、撤权与旧代。 [T631 数值参考](test-report-T631-v1.md) · [PC 业务门](architecture-T828-v1.md) · [独立 checker](../test/t866/check.py)

**目标14交接。** 交实际接受的可信业务流场和同代只读 GridView，绑定 fixture/ROOT/FULL、真实 root/token/commit、计算与绘制 fence。保 MAC 面与格索引、ghost/边界、真 pose/时间、原 u/v/π Q32、φ 与均值及参考压、黏度/壁速、SI 标度、实际残差/证书和图形 lease。14 在此之上验随给定沙轮廓的绕流、法向压力及剪切载荷。F02 私有根、晚尾与试编各保实际资格。

本轮新证据是同进程原字复核和 OP_48 的离线拆分试编。正式 DM2 可信续根、G13 新角色 GPU 资格与可信业务场待 developer、tester、effector 分项接力。两项保原 250 ms、原业务帧代、49,152 B 完整 shader 源码门、GPU 驻留与 shader 渲染，禁止 Canvas 回退。
