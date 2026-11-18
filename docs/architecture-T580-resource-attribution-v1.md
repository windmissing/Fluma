# T580：目标12事件行/arena资源公式缺口首次归因

2026-11-18，architect，不使用真机，不执行T578脚本/产品/GPU测试。输入T578正式报告/源码、T567正式工程/机器表/生成器、T554/父R1和G12设计。当前首次归因及必要工程修订完成；原T578 PARTIAL保留，不由本卡改成tester PASS。机器公式/四档项账见 `architecture-T580-resource-formulas-v1.json`，源审计见 `analysis-T580-resource-attribution-v1.json`。

## 1. 裁定与证据精度

**没有发现四档eventRows/arena数值错误或需要增加设计预算。** G12设计已有 `Eparent+32*(2K+8N+4Kw+32)+64`，父DM设计/生成器已有 `Eparent=32*(6N+2K)+(4N+K+16)`；T567工程机器表原本列全四档eventRows。仅G12设计的reference_profiles节是两档，不能把它当工程机器表只有两档。

工程确有**公式可审计性/基线标注缺口**：T567没有把父域、增量、SOURCE64与WORK含DM基线集中成可直接引用的机器分解。现已补本稿/公式JSON，并在T567工程MD、机器表及生成器显式引用；所有原数值/字节offset/profile ABI/调度不变。

T578 `recompute`实际从自己硬编码MANIFEST读eventRows（两档为None），再算arena和所有下游项；这些值不能宣称完全由独立公式重建。None时arena/resident/ROOT/FULL等都是None，检查comprehension还用`if r[k]`跳过，不能据此说四档下游独立全过。原PARTIAL是需要保留的结果，后续修复的是既有独立复算通道，不是让manager重跑或调测试阈值。

K512/Kw64正确反算为 `(44584960−163840×64)/256=133200`。报告133590.625若保pathRows163840，必须使用arena **44684960**，比正式值多100000B；源码和表均给44584960，且该源码未保存一段产生小数的反推过程。因此只能判“原报告算术或未记录操作数不一致”，不能进一步冒称已证哪次抄错/哪套基线，更不能把它归为GL对齐。独立证明必须从下列行域正向构造，反算只作交叉核对。

## 2. EVENT64逐项预算

N128、最大micro数32、EVENT64=256B。父R1每micro的4N列动作分别为自然动员/力/摩擦/沉积，N−1内部面+1 step seal合N，P运动/捕获各K，合5N+2K；DM替换自然动员并新增N条私有预测事件，合**6N+2K**。根级保守包络**4N+K+16**沿父：4N触摸列复合信用、K源/包根级信用、16根控制全局信用。它们是上界分配，不是要求所有stage无动作也发假物质行；SOURCE≤K+16已被父根包络覆盖。湿writer替换旧动作时保守信用不缩，不允许同一实际操作经两个别名重复emit。

| G12每micro增量 | 行上界 | 计费规则 |
| --- | ---: | --- |
| P_KICK_A / P_KICK_B | K+K | 各跨两pass的复合pending行，只reserve一次；最终seal不再分配一份 |
| WET_PROBE / FS_PRESSURE | N+N | 只读probe不冒实际冲量；压力配对用实际j |
| FS_DRAG_A / FS_DRAG_B | N+N | F/S端点按复合阶段字典，不按每pair无限展开 |
| WATER_FACE / WATER_ORIGIN | N+N | 内部水面≤N−1；两个墙反冲计后面的共享global信用，不额外加129面 |
| WET_CAPACITY_ENERGY / HYDRO_SUMMARY | N+N | 后者一行带七次HYDRO epoch，不是7N行 |
| J_FLIGHT / J_CONTACT / J_ARRIVE / J_RETIRE | Kw+Kw+Kw+Kw | 每种每slot每micro至多一行；J路径另算PATH16 |
| 阶段/审核/墙等共享global | 32 | 整个micro合计上界，不能32行之外再另加墙或每stage重复开窗 |
| 合计 | **2K+8N+4Kw+32** | 不含父域，来源map不按每origin新放大EVENT64 |

SOURCE新增**64行仅在arena根包络外加一次**，不是SOURCE16个pass各加64，也不是每个micro再加64。一次源命令只选沙/水一种、DIRECT_H/FLIGHT_J一种，不发生普通面输运；新增物理水源≤1、受影响列容量/能量≤1、受影响列hydro摘要≤1，以及16个SOURCE pass各至多一条聚合global，合≤19，余45信用不制造事件。SOURCE全列几何检查写derived cache/聚合首错，不逐列再发128条HYDRO行。accepted replay比较同一源流，不将第二执行计为再追加一套有效源事件。

因此一份arena的总E为：

```text
Eparent = 32*(6N+2K) + (4N+K+16)
Eextra  = 32*(2K+8N+4Kw+32) + 64
E       = 32*(14N+4K+4Kw+32) + 4N+K+80
N=128: E = 58960 + 129K + 128Kw
```

32是**每个试验/scope的最大micro数**。发现+拒绝前缀+ACTUAL/replay的190 micro是执行工作量，不能再乘进单份arena；六发现scope的保留成本在六个独立arena计。当前trial/selected/公开槽四arena与可选六trial各有相同容量上界，valid cursor按实际路径，未执行试验标SKIPPED，不能读旧残留冒原件。

## 3. 四档正向构造与裸arena对齐

| K/Kw | 父micro行 | 父根行 | 父E | G12每micro增量 | 32×增量 | SOURCE增量 | 最终E | PATH行 | arena B |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 192/64 | 1152 | 720 | 37584 | 1696 | 54272 | 64 | 91920 | 81920 | 28774400 |
| 192/128 | 1152 | 720 | 37584 | 1952 | 62464 | 64 | 100112 | 114688 | 32968704 |
| 512/64 | 1792 | 1040 | 58384 | 2336 | 74752 | 64 | 133200 | 163840 | 44584960 |
| 512/128 | 1792 | 1040 | 58384 | 2592 | 82944 | 64 | 141392 | 196608 | 48779264 |

PATH=32×8K+32×16Kw=256K+512Kw，sand/P与水J不混stride；EVENT64 256B、PATH16 64B。arena的eventBegin0、pathBegin256E，**两段之间padding0**，A=256E+64PATH；N128可直接化为 `A=15093760+49408K+65536Kw`。所有四档event端点为256B倍数、PATH整64B行，裸arena也恰为4096B倍数，不能通过“对齐余量”解释非整数E。

本合同将arena作为一个完整uint[] SSBO在binding3零偏移绑定，内部数组地址不等于另一次GL range绑定。若实现改用glBindBufferRange，实际offset必须遵守设备query的GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT，新增物理range/padding另列、不得改原逻辑行数；这里未测该值，也不假设driver/分配器零开销。Khronos依据：[整buffer绑定](https://raw.githubusercontent.com/KhronosGroup/OpenGL-Refpages/main/es3.1/glBindBufferBase.xml)、[range offset约束](https://raw.githubusercontent.com/KhronosGroup/OpenGL-Refpages/main/es3.1/glBindBufferRange.xml)。

## 4. WORK与并存副本

WORK基线必须是**T554完整DM**，不能拿R1/未含DM值直接加G12，也不能重复加DM：

```text
R1设计WORK = faces16512 + staging32768 + Hamilton1048576 + originAudit65536
           + indexes8K + input512 + summaries1536 + controls4096 + completion1024
T524工程增量 = 1536+512+9216+32768+65536+5120 =114688
DM增量      = 128*48*4 =24576
父DM WORK   = K192:1311360 / K512:1313920
G12 WORK    = 父DM +675840 +128K +256Kw
```

675840B的每段字节与既有T567完全一致：65536几何+33024水面+524288四share planes+8192控制+16384驱动+16384宽账+1536摘要+8192前缀比较+256运行配置扩展+2048保留。机器公式还完整列state原始段，不从resident倒推WORK。四档WORK仍2028160/2044544/2071680/2088064B。

设S=单state、W=完整WORK、A=单arena、R=5S+W+4A，M=65536+4096=69632：ROOT=2S+2A+M；FULL_CORE=R+M；FULL_TRIALS=R+M+6A。可选六trial的**GPU额外6A**与每个native TRIALS槽的**额外6A**是不同物理副本，不能把冷slot增量当已经覆盖GPU保存区，也不能将已有normal4A再扣一次。

两冷槽+输入22528B+页16384B的已知并存：CORE峰=R+2(R+M)+38912=**3R+178176**；TRIALS峰=R+6A+2(R+M+6A)+38912=**3R+18A+178176**。展开后TRIALS包含GPU正常4A+GPU历史6A+两native槽各10A，共30个arena等价容量、15份state、3份WORK及meta/input/page；这里“等价”仅账面分组，不是宣称全建30个GL对象。

| K/Kw | resident B | GPU额外6A B | 两FULL_TRIALS冷槽B | CORE已知峰B | TRIALS已知峰B |
| --- | ---: | ---: | ---: | ---: | ---: |
| 192/64 | 122134400 | 172646400 | 589700864 | 366581376 | 884520576 |
| 192/128 | 138968960 | 197812224 | 673701632 | 417085056 | 1010521728 |
| 512/64 | 185829760 | 267509760 | 906818304 | 557667456 | 1360196736 |
| 512/128 | 202664320 | 292675584 | 990819072 | 608171136 | 1486197888 |

六GPU增量原先已在T567 FULL_TRIALSGPUAdditionalBytes及生成器6*arena明示；本次补集中账，不将T578从总值反推该项冒独立证明。旧route pin、两slot实际capacity信用、TTL/退役仍照原合同；driver/JIT/host固定对象/FBO/JS/文件/实测对齐排除项不消失，不用上述峰冒RSS或设备可分配性。

## 5. T576影响、T578续证与完成边界

T576的水平湿域、四K/Kw档、原事件容量/偏移/资源、SOURCE16/wet6204/dry2404及250ms/GAP不变，未发现需要designer重新预算或扩大pool的反例；真实前置仍是T573完整DM父后端。本卡没有新增设计/manager验收前置。T576按本展开账实现每类emit/pending/cursor，若实际实现需要逐来源/逐HYDRO多行或不同保存窗口，保具体请求/剩余/phase证据回architect，不截尾/删反冲/静默扩预算。本卡不是实际GPU emit上界或资源分配的设备验证。

T578续证输入已齐于本公式JSON、原T567状态/WORK字典、父T515/T554算法事件域与本次四档分解。交tester：不使用真机，保v1报告/PARTIAL与源码SHA，新版本独立从primitive/公式重建E/P/A/S/W/R/冷槽/峰值，不import本卡生成脚本或把公式JSON的profiles数值当计算输入；完整断言四档，未知项不能用truthy过滤而冒通过。K512/64保存精确反算操作数/单位/结果133200，正向闭式与反算只互校，故意错stride/少SOURCE64/七HYDRO写7N/漏DM/漏GPU6A负控须能识别；源/物理trace没实现则继续NOT_RUN。设备分配/期限仍UNMEASURED，不能把静态公式结果当产品PASS。manager登记T580交付后推进原T578剩余范围，不由本人改他人状态或重复已证同域。

交developer（T576）：不使用真机，仅开发/编译/提交、不执行测试；同步消费T567对本公式合同的引用，不另开资源优化/CPU线或旧profile迁名。独立测试不是开发关闭门。设计数学不需修订，用例目标合理，manager无需改阈值；本轮分流为工程可审计性修订+tester独立性/算术证据返修。

接力prompt共同要求：不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本人行状态有限例外），不更新滴答同名清单。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下，局部受阻先做其它可做项。具体阻断原卡写未完、原错/复现、证据、尝试、不能继续原因、所需前置与接手角色，失败看评论、成功结束待验收。CPU停投/GPU驻留，全前景背景仅新XComponent+shader，禁Canvas/软件GL产品回退；FAULT或标STALE上一完整有效帧。守恒/容量/期限/GAP不放宽，不增manager验收门。

本轮脚本只做AST取证与资源整数代数，全部四档与原机器表一致；未调用T578复算脚本、不执行产品测试，不改原测试期望/报告。
