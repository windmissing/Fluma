# T961 · 目标 9、G15 与目标 13 的超期归因及工程增量

2026-11-24 · architect · Intel UHD 630 `8086:3e98` / `31.0.101.2140`。原 250 ms typed 门、真实 fence/代、GPU 驻留和 shader 渲染继续适用。先做可逐字对照的增量，各线按实际证据判定。 [T954](../test/reports/t954/report.md) · [T959](../test/reports/t959/report.md) · [原字、时标和试编索引](architecture-T961-evidence-v1.json)

## 1. 目标 9 与 G15 · 活跃阶段占九成

T954 control-r3 和 replay-indirect-r1 各有同一进程先接受的 SOURCE，SOURCE ACTUAL 逐字等于下一根 QUANTUM 的 BASE/CURRENT。Q 均在原业务 fence gen 4 返回 2，`tailRead=false`。15 个点来自同一个业务 nonce/plan/fence/gen/pool-reset，原根晚退役确认后读取。晚尾真实选中 J1、GPU WORK `control[16]=2404`，回执 `outcome=1, reason=0`。J 由 GPU 物理与证书决定。两份 EVENT/PATH/FIXTURE 全字节相同，独立复算了仍在树中的回执、范围与 SHA。两臂的 full AUTHORITY/SELECTED arena 只留下相同的摘要文件，原字节未随 T954 版本保存。J2/J4/J0 需要真夹具，已有 J1 结果不扩展到这些档位。 [control 原日志](../test/reports/t954/runs/goal9/control-r3.jsonl) · [间接臂原日志](../test/reports/t954/runs/goal9/replay-indirect-r1.jsonl)

| 同 fence GPU，ms | 目标 9 control | 目标 9 indirect | G15 四臂实际范围 |
| --- | ---: | ---: | ---: |
| CLEAR | 36.563 | 39.378 | 33.317–71.507 |
| DISCOVERY J1 | 365.804 | 371.106 | 455.641–584.694 |
| AUTHORITATIVE | 329.558 | 320.438 | 423.540–425.854 |
| REPLAY | 361.358 | 349.398 | 449.174–450.308 |
| 其余 DISCOVERY J2–J32 + SELECTED + REJECTED_PREFIX | 9.350 | 9.324 | 约 9.3 |
| COMPARE/PACK/SEAL | 55.994 | 54.761 | 58.574–59.429 |
| **整根** | **1158.645** | **1144.435** | **1431.338–1598.078** |
| grant→submit 返回，host | 1.903 | 18.557 | 44.545–197.978 |

间接臂依然编码 2404 逻辑槽，J1 的 REPLAY 372 个工作组归零，新增 1 个间接参数屏障。单对样本 GPU 总量差约 14.210 ms，REPLAY 差 11.960 ms，host 录制/提交也有变动，缺配对重复以估净收益。三段活跃工作合计占 90.96–91.20%，G15 四臂为 91.17–92.91%。T954/T959 全原件的最后 DM48 每列均为闭式分支、合计 256 个 Eval、**0 次二分迭代**。样本热段不能归因为隐式求解的 32 轮二分。阶段点覆盖 init、实际物理及证书，细到单操作仍待补。 [原计时边界](../pc/vulkan/sand/owner.cpp:39) · [DM_REQUEST 及 DM48](../entry/src/main/cpp/gpu_resident/g9dm/pipeline.comp:1251)

G15 修订版四个实际执行方向臂都回出晚尾 1/0、selected J1、业务 Vk=2。负/正各有全量证据，重复负/正两份只保存 Completion。干前沿修复需独立维持共享零冲量、湿边压力、真实来源与能量、正反搬移和最终呈现。预算优化共享执行器及等价证明，G15 保原 ±45°、角色包及独立证据。 [G15 测试结果](../test/reports/t959/runs/g15/g15-result.json) · [干前沿数学](algorithm-G15-T952-dry-front-flux-v1.md)

### 1.1 首个可实施增量 · 逐盘建列库存

同一 `g9AuditMassGeometry` 在活动微步及最终审计等热段重复执行。原代码每一列逐一遍历所有 K 个盘寻找归属，库存统计固定 **O(NK)**（128×192=24,576 次槽读）。首个 Release 实验臂只替换这段库存归并。lane 0 在 128 个 uint 临时桶中单扫 K：空槽跳过，合法列累加包质量，非法列由原后续逐槽审计按原优先序拒绝。每列用原 `g9AddMass` 上界 268435456，溢出采用上界+1 哨兵，不提前改 control，也不提前终止其他列的统计。原 S/F、map/origin 账及列审计执行到同一个比较点才查对应桶。其余质量、唯一 ID、盘位、空槽、动量、来源和状态字段照旧检查，保原第一输出错误与失败前缀。只改 QUANTUM 实验臂，候选桶用一个 workgroup 的 512 B 临时区，由现有审计 lane 0 独占，零额外产品 SSBO/状态/派发，(O(K+N)) 仅指该项。归并算子用 16 份冻结 GPU state 原字和 256 组构造样本逐列对照，含越界列和溢出。实际根收益由成对 GPU 验证。 [原嵌套扫描与逐槽判据](../entry/src/main/cpp/gpu_resident/g9dm/pipeline.comp:1849)

Developer 在 control/replay-indirect 保留原 reference，上变体只改该扫描。测 `MICRO_AUDIT_SEAL`、`FINAL_MASS_ORIGIN_MOMENTUM`、PACK 中 root audit，并给 DISCOVERY J1、AUTH、REPLAY 各保边界。第二个取证窗口分别拆 INIT_COPY/arena 清、OP21 DM_REQUEST、23/24 FLIGHT/FOLD、25–31 通量及来源。每臂同 PE/物理输入/预热轮按 ABBA，以相同记录项比较。额外点只作诊断臂，业务窗口照旧。Tester 复用 T953，同版核 2404 逻辑、真实 fence、全范围原字、正长度的 J2/J4，和实际 J0 全拒、重复 ID、坏列/坏槽、来源满与撤权。无足够物理覆盖的数据单列。把 full AUTHORITY/SELECTED 的实际原件留在下一对对照中，使摘要可独立复算。 [T953 顺序门](../test/reports/t953/handoff.md)

### 1.2 预算其余阶段

以目标9间接臂的提交时刻为例，250 ms 原窗还约 231.443 ms，扣该样本非三热段约 103.493 ms，三段合计只有约 **127.95 ms** 的上界，较现 1040.94 ms 约需 8.1 倍总降幅。它是敏感性估算，真实接受还需 fence 与验收校验时间。G15 pos2 即使三段归零，现 host 197.978 ms 加其余 GPU 约 101.479 ms 已超过期限。并行准备 grant 前可固定的录制、管线与 descriptor，只在 grant 后冻结合法输入、nonce、plan、deadline 及实际 latch；任何重用保上一资源真实退役和原一枚业务 fence，禁把旧许可/点位转给新根。按真实录制子段确认是否复用 T924 的退休准备方式或 permit 无关 command buffer。

下一步按实采热点逐项推进。若反复每 scope 全 arena 清占大头，先证明新代可见范围、写入尾与拒绝前缀，再做有界清零。若活跃算术占大头，在 DM48 或面流选一个操作，按原 q/q+1、来源及舍入拆列并行。先测窄精确除法和多列读取的对照，相同整数结果及首错才进入实际。保独立 discovery、authority、REPLAY，各自生产、选择证书、J0 和完整拒绝前缀。长期用当前 PC 的实际 J/CFL 和有效物理增量验证；上屏仍需实际 accepted root。 [T945 预算及封存合同](architecture-T945-v1.md) · [原规则](architecture-T554-g9-dynamic-mobilization-v2.md)

## 2. 目标 13 · 管线资格、时延和输入真实性分别落点

### 2.1 建管线问题按 SOURCE 角色进一步拆

T954 v2 PE `d9e6f89c…` 四角色资格入口有效。黏性、压力实建且 OP66 过。旧组 `f01959cc…` 和余闭包 `64101778…` 同在真实 `vkCreateComputePipelines ENTER` 后约 60.010/60.032 s 仍未返回，保监督收据和调用栈。F02/F03 已提交的根只引用压力/黏性新闭包，Remainder 拆分修 SOURCE 资格，F02/F03 时延走 §2.3。 [余闭包收据](../test/reports/t954/runs/qualify-g13/OP48_REMAINDER.receipt.json) · [旧组收据](../test/reports/t954/runs/qualify-g13/OP_48.receipt.json)

以 T955 生成的原 `role_OP_48.comp` 做五份闭包试编。未拆余闭包原 GLSL 已逐字复现。五份保持原 opcode 与私有 OP66、64 lanes、4 SSBO+layout UBO、24 B push、FP32 原执行模式和 NoContraction。用同一 glslang/Vulkan 1.1 验证和反射。开发者按单 SOURCE opcode 绑定这些角色，资格宿主用新的 manifest 材料逐角色采实际 ENTER/EXIT、OP66、真实 SHA 和缓存开关。必要时只对仍停的 55/58 或 56/59 单独细切，在真实 SOURCE 路径再核 16 槽、同代 compare、原失效证书及可续 QUANTUM。

| 角色 / opcode | 完整 GLSL B | SPIR-V B | SHA 前缀 |
| --- | ---: | ---: | --- |
| GUARD 52/62 | 8,600 | 27,900 | f8687678 |
| COPY 54/57 | 7,186 | 18,720 | 6c6f5acc |
| INSTALL 55/58 | 20,709 | 94,676 | a65151fa |
| AUDIT 56/59 | 27,438 | 131,536 | 7b7e9807 |
| COMPARE 60 | 7,726 | 21,984 | f44390d7 |

### 2.2 原始冷档校验找出 F02 零输入

T955 PE `eb0e841e…` 的四份冷档均匹配各日志的全文件/Completion/七区段 SHA，Completion[96..103] 也与实际 CANDIDATE 全区 SHA 一致。四个原业务根都 `GRANT_DEADLINE`、`acceptedRoot=null`。F02 三个晚尾 GPU words 5..7 均为 (7,0,0)，三份实际 u/v 和 π 全零。T868 生成器明确把 F02 造为 rest 零输入；三份运行都缺非零制造投影驱动。按 T631/T866 的 64×64 目标离散压力，零 π 的 RMS 约 **0.125013**，远超原 F02 界。当前只用这批做零输入计时与路径正控。原文件保持，增加显式零臂和 `F02-nonzero` 夹具。 [T868 夹具生成](../test/t868/make_fixtures.py:136) · [T631 物理门](test-report-T631-v1.md) · [T866 投影判据](../test/t866/check.py:104)

Tester 制造非零投影的入口 MAC 速度，按独立参考 `u*=δt·G_h π_ref`，π_ref 用 T866 的离散振幅、64×64 格心相位和闭壁面不透。状态中的输入 π 保持零，固壁法向面为零，Config 的实际 state hash 和嵌入摘要重封。离线预览确认 8064 个非零 FP32 面值、最大初始散度约 0.04109446、闭域总散度为零。记录入口 u/v、同代 WORK RHS/projection trace 及输出 u/v 和 π 原字，先证求解器吃到非零输入，再按 T866/原残差核场、RMS、max、散度及误差。F02 仍是私有算子臂。F03 原晚尾 (1,0,0) 且候选速度非零，真黏性 ACTUAL 块 0 约 17.489 ms、原 trace 两侧各 16 迭代，后七块已冻结。它仍无业务接受，目标 14 GridView 继续等原 F03 连续接受矩阵和 F05 动态压力等正式可信场证据。

### 2.3 819 的真实成本与整体时限

下表使用 T955 PE 及实际业务同 fence 的 GPU 点，按 36 位回卷 ×83.333 ns 复算。现存三次 F02 有一个 A、两个 B，其中一个 B 把详点移到 REPLAY。完整同配置 A/B/B/A 仍缺第二个 A 和同点位的第二个 B。当前可比较数量级和子阶段，净收益待对照复测。 [A](../test/reports/t954/runs/goal13/t955-f02-a.jsonl) · [B](../test/reports/t954/runs/goal13/t955-f02-b.jsonl:216) · [B 重放细点](../test/reports/t954/runs/goal13/t955-f02-replay.jsonl) · [F03](../test/reports/t954/runs/goal13/t955-f03-actual.jsonl:186)

| GPU ms | F02 A seal-v1 | F02 B 三段 | F02 B 三段 + replay 细点 | F03 真黏性 |
| --- | ---: | ---: | ---: | ---: |
| 整根 | 1904.262 | 1834.029 | 1695.062 | 1756.743 |
| ACTUAL / REPLAY | 778.186 / 583.031 | 713.612 / 595.298 | 582.469 / 587.419 | 625.985 / 606.380 |
| 819 整段 / SHA 子段 | 535.553 / — | 518.993 / 518.954 | 519.363 / 519.307 | 518.475 / — |
| 两侧压力 cycle0 | 127.066 / 54.158 | 94.968 / 60.277 | 50.426 / 50.548 | 未细拆 |

F02 B 的 819 其余 PREPARE/PUBLISH 仅 0.028/0.011 ms（另一 B 约 0.048/0.009 ms）。A→B 整段表面少约 16 ms，且真实主成本仍是完整逐块 SHA-256。F02 两侧原 trace 显示 cycle0 执行 **68** 次子阶段计数并收敛，1–15 全是带证书的空槽。已采整段覆盖跨过后续预排 J，不能用报表的压力 span 当算子耗时。B 的真实 J2/J4 四个预排区间合计约 **5.730 ms**。当前先优化 active，保空槽的逻辑 trace。

1. **先改 819 SHA 实际内核。** 保 T955 PREPARE/SHA/PUBLISH 和原字节序，按用例一次只换 SHA 模块。原 198,144 B 状态在标准链上需 3,097 blocks ×64 rounds。把动态索引 `W[16]` 换成 16 个始终静态引用的标量寄存器及固定 round 展开，只改消息调度访问，保 padding、原摘要、四储存绑定及 820 逻辑槽。已用 T955 源仿编基础角色，完整 GLSL **34,626 B**，SPIR-V **225,072 B**（`b34d442c…`），Vulkan 1.1 校验和反射过。实际 IGC 编译、寄存器、GPU 速度仍需同配置 ABBA。保温度、cache/opcodes、query scope，以及原 Canonical bytes、hash、两 scope record 和负控。GPU 上实际编译若长尾，用较小的 16-round 静态模板、四段固定索引，不改变 SHA 链。 [原动态算法](../entry/src/main/cpp/gpu_resident/g13flow/sha256.glsl:5) · [T955 接口](developer-T955-v1.md)
2. **把 ACTUAL/REPLAY 分拆到真实热操作。** 先在相同详点下对照 F02-zero 与修正的 F02-nonzero，给 OP47 `sealMicro` 内的两份 scope state SHA 各独立边沿，以及 cycle0 的细网格 Jacobi、限制/延拓和残差余量。OP47 晚原字已含两份有效 trial SHA；819 摘要的字节包含之后的状态/时间准备，不能以 OP47 摘要替代。若 OP47 确认为热，复用同一 SHA 优化各自计算，保各独立 buffer 和逐字比较。压力首先可将 Jacobi 的有界除数做精确商余专门化。保 S128 符号、RNE 和复原完整余数（除数有二次幂因子时先取高位计算商，再合并低位恢复余数）。再将最大的一个网格 sweep 拆 tile；每个 Jacobi pass 用原双缓冲和跨工作组写读依赖，固定非负平方和/最大值规约，严守边界、原字、收敛、首错和 16 cycle 上限。改变数值方案才交 designer。 [OP47 scope 摘要和 trace](../entry/src/main/cpp/gpu_resident/g13flow/trace.glsl) · [压力原实现](../entry/src/main/cpp/gpu_resident/g13flow/pressure_mg.glsl)
3. **两侧求解和封存一起进入预算。** F02 B 即使假定封存及两侧首个压力 cycle 全部省去，按当次样本其余 GPU 仍约 **1,160 ms**。拆点 815/818 各约 0.5/1 ms，保原审计。以 `250ms−真实 host 录制/提交−fence/读回/校验保留` 计算可用 GPU 窗，再由 ABBA 的新上界给两侧独立求解、SHA 和其它段分配工程预算。示例分配为 host 等 10 ms、ACTUAL/REPLAY 各 90 ms、SHA 30 ms、其余 GPU 30 ms，仅用于分支排优先。保原 250 ms、pulse freshness、两个独立 scope、typed token、F02/T631/T828 的正确性门、真实 GridView 证据。六个试编角色各核实拼完整单元 49,152 B 门，保原 Vulkan shader 路径和禁止 Canvas 回退。 [正式 PC 验收](architecture-T828-v1.md)

本轮完成归因、等价增量与 SPIR-V 离线验证。下一轮需有非零 F02 冻结夹具、可配对的时标、实际 IGC 资格与真实接受根。Developer 接单项变体，tester 接独立原字与用例，effector 用当前 UHD 630 获取同代实测。
