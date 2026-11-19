# T679 · 目标15—19候选表示与GPU平台工程约束

2026-11-19；architect；P3。消费T673 `algorithm-G15-G19-roadmap-v1.md`、完整C01–C24案例、既有平台原件/SDK声明及当前G9/G11/G13资源合同。本卡为 **RESEARCH_CONSTRAINT_INVENTORY_COMPLETE / NO_ROUTE_FROZEN / NOT_IMPLEMENTATION_READY**：不使用真机、不联网重做选型、不编译/运行产品或测试，不实现新CPUstepper，不冻结MPM或双层F。

十九目标顺序13→14→12→15→16→17→18→19不变，10暂停、CPU停投、旧湿卡继续暂缓。盘点可独立完成，**不是1–14的新增开发/测试门**，不授权低优先产品实施。机器表 `architecture-T679-constraints-v1.json`，完整来源/24案例/符号字节复核 `analysis-T679-evidence-v1.json`；现G9/G11/G13合同一字节不改。

## 1. 哪类证据能说到哪一层

| 等级 | 本次实际输入 | 可据此成立 | 不能外推 |
|---|---|---|---|
| D-HIST 旧设备实证 | T131 JSON B318/EGL1.4/compute原型；T232 Maleoon920 pbuffer | 各自当次limit查询及整数/跨dispatch/尾1024B小探针结果 | 新XComponent窗口、压力/本构/接触核、全量分配/实时或3D资格 |
| D-M2 当次窗口查询 | T591c完整raw serial1/epoch2/window387041997984/context387048859216/owner4360，Maleoon935F/B320 | 该context rawGL0的限制数值，见下表 | 当前未知G13失败数值、其它驱动/context、新route/FP精度或全流程PASS |
| S-SDK 静态声明 | 本机26.0.0 `GLES3/gl31.h`原件SHA | 编译头存在SSBO/compute/barrier相关token/函数声明 | 这些token宏的十六进制值不是设备上限；头文件不授浮点原子/int64/FP64能力 |
| C-PROFILE 既有合同 | T647、DM v2、G11 v2、G13 layout/assets | 各自已定bindings/count/mask/字节布局/资源与退役边界 | 编译/设计表即设备可分配或新物理已可信；旧预算天然适配新表示 |
| A-RESEARCH 假设算术 | T673 stride、点/节点/接触/排序/层数例 | 在明确假设下精确字节/元素/工作量公式 | 完整显存/RSS、数值分辨率充分、手机实时或选型结论 |
| U-UNKNOWN | 新本构/接触/水耦合/源容量、shader与热后成本等 | 必须登记未知及其最小输入/测量归属 | 不凭空写PASS或给一个看似精确的总性能数 |

T131全局bindings16，T232 global32/compute16/vertex0/fragment16，T591c global32/compute16/fragment16：保各自驱动/context，不能挑最大值合成一张“所有环境保证表”。T232浮点原子/int64扩展项0、double负编译、timer扩展项1也仅该pbuffer域；不能靠SDK声明猜新设备行为。

## 2. E01–E04：GPU资源、程序和确定传输

T591c同context数值：max SSBO bindings32、compute stage blocks16、fragment blocks16、单block1073741824B、offsetAlignment256B、shared32768B、工作组各轴count65535/size1024、总invocations1024。**这些都是查询值，不是可分配总内存或新kernel支持证书。** 当前G9/G11/G13坚持4个同时compute binding/64 lane是各自工程选择，新候选若要更多须独立设计/核真实stage与combined上限，不修改现合同或默认认16/32都可随意占满。

E01每个pass必须列真实buffer对象、base/range/size/readonly/write、每stage active blocks、最大的实际绑定range与对象容量；拆多个逻辑数组进一个SSBO只减binding数，不减字节。256B对齐作用于range base，不把每个U32/particle填256B；所有Nc/Np/q/C/stride/offset/GLsizeiptr/索引乘加及padding checked，空/越界在写前具名失败。大数组分块要列跨块索引/halo/归约与更多pass，不是自动解决方案。

E02程序规模没有本项目已证的“多少KB必过”阈值。现M2约85KB SOURCE曾开放LINK，RX145268B曾设备拒重复局部符号，G13错误29原始能力值不齐；不能把这些不同错误拟合成shader字节上限或驱动通病。可复用单一canonical源码→生成header→ELF内原字节/hash→实加载BuildId/实际program→缓存key与反射原件链、fatal生成和独立输出目录；不能复用旧shader缓存资格。多program拆分须额外计编译/切换、uniform/反射、跨program同步和代码身份，不保证更快。

E03粒子/网格传输、清零、排序、接触、压力/应力迭代、audit/replay/scene pack分别列dispatch与全局依赖。一个workgroup内barrier不等跨workgroup排序/接触已同步；确定归约需实际全局阶段/相应GL memory barrier。不能每点/merge插fence、CPU逐步读回做调度；单尾证书/提交锁可复用思想，新route的完整证据内容/字节/期限仍待正式设计。现4binding/1尾1024B等不自动套给3D路线，更不能靠删审核硬套旧数。

E04无序float atomic累加不默认逐字确定/完整trial-replay相同。候选可比较固定点宽累加、(node,id)排序定序归约、有界bin gather；各自须范围/舍入/顺序/拥挤度和最坏工作量。标准int/uint操作路径与可选float扩展分开验证；无float atomic时不能悄悄CPU求和。新状态tensor/Fp/C/应力历史、随机/源计数、接触历史均纳入candidate/拒绝/回放，而不是只比最终质量或全场hash。

## 3. 候选表示的工程差异（不裁定选型）

| 候选 | SSBO/block/工作区的逻辑需求 | dispatch/接触/回放与迁移风险 | 仍缺的正式输入 |
|---|---|---|---|
| A 固定列S/F/P有限扩角 | 可复用当前base/候选/work/out分域；来源cohort、DM工作/事件/宽算术需重算 | ±45°角表/gn除法/作用域/CFL与动员；原P容量不足全床脱离，不能扩角即90/180 | 15支持材料/湿域、角/速率、预算、参考；现±30°不热改 |
| B 多方向/局部多图＋自由体积 | 同时旧/新chart、overlap owner map、分裂/合并队列、free-body/source/history | 图切换所有权、空隙/连通、重映射动量/应力/势能及反向；双表示共存峰值不可漏 | 最大图/交叠/碎片数、拓扑、转换准则与完整trial/replay |
| C 全体积材料点—网格（MPM类） | 点位置/速度/质量/体积/来源/变形/塑性/仿射历史；grid质量/动量/力/应力、贡献/排序/边界work | qNp传输与确定汇聚、本构迭代、网格穿越、接触和可能全局求解；不是旧P改名 | 2D/3D stride、核支撑、点/格分辨率、本构/历史/空实/接触、精度与容量 |
| D 全体积Eulerian应力/体积分数 | 格心/面心量、来源/历史tensor、占位/界面、算子与迭代层级 | 空实/应力/来源对流、数值混合、连通/零空间与压力；水MAC不能只换摩擦参数 | 张量闭包/对流、自由面/壁、混合容量与稳定离散 |
| E 全DEM | 粒子姿态/角速度、bin/key/index、canonical pair、切向接触历史/碰壁账 | 搜索最坏O(Nd²)，有s/Bmax才可界；刚性子步、pair归约和跨步历史迁移/删除 | 真实/代表粒径质量、Nd/Ccap/Zcap/Bmax、接触刚度/耗散/旋转、dt |
| F 连续体＋局部DEM | 两套状态同时驻留＋分权mask/映射/待转换/历史制样 | 重叠区不双计质量，转换功/应力/织构/来源，频繁转换全部replay；省粒数不保证净收益 | 何时切换、唯一质量owner、转换可逆程度与最坏共存域 |
| 18 两/多层F或直接体积分层 | 逐层质量/动量/来源/mode/rest与交换/event/audit/scene；已有体积量可只做无质量诊断 | 内力等反、层间携质量动量/功、空层/层序、P转换；不能再叠一份有质量F | 同观测2/4层/体积比较，速度剖面/床剪切/载荷参考；双层未冻结 |
| 19 有限厚度3D沙水 | 点/格/面三维、z速度/应力/来源分布、6壁、真实水占位/压力/黏性、3D scene | 厚度与粒径/层数收敛、前后壁反力及相间功、狭缝连通/求解器；2D谱界不可直接搬 | b/d/h/Nz、单位、6壁/润湿/出平面动作、3D水/沙离散与性能域 |

E05接触的key至少绑定实际粒子/slot generation、pair或wall身份、接触状态寿命；重用index不能继承旧摩擦历史。接触/邻居/源列表溢出具名拒整candidate，不能截邻居/删低权源凑绿。GPU宽算术、排序和bin工具可复用模式，但不是未设计新kernel的正确性证明。

## 4. E06–E09：完整状态、来源、几何和水线

T673要求完整trial/replay。`A`为所有尝试/审核/重放累计micro，不能只按成功J计；示例两scope试1/2/4为14 micro，试1…32为126，仅算术，不冻结新selector。每micro账为 `qNp + IσNp + sort + IqNg + contactWork + sourceWork` 等实际操作；数值迭代上界/失败原因、拒绝prefix证据、source去重与接受时钟均独立。不要给三份bank名就以为包含全部旧接触/几何/水状态。

E07来源需要定义每粒/点纯origin还是混合份额、格混合的容量/压缩/输运、转换后责任归属。旧每cohort64源不是新route已证够用；只保originID不能替任意混合比例。若**假设**每cell64项×16B并保存3bank，128²为50331648B，64²×8为100663296B，这只是未选定布局的容量例，不能再无条件加到已含同一来源存储的stride上。

E08几何从固定列底到任意支撑/自由体积，必须显式实际固体占位/位置/应力/质心、空隙/孔洞、碰撞边界及新旧几何区间。旧单列总质量不能重建分离体积或未知应力/剖面；需要物理seed或标记的重建假设，不能隐式从均值恢复全部历史。90°/180°不通过abs/epsilon/夹角绕现模型支持域。目标17连续未wrap相位、Ω/α、固定参考点角动量/壁功、ID/来源/事件长寿命另列，不能每圈reset隐藏漂移。

E09水13→14→12真实前置不因盘点完成放行。含水候选需区间几何/扫掠体积、face开口/法向/面积、边界速度、总压力规范恢复与黏性载荷、相间冲量/功和真实固液体积；G13规则满盒MAC、原pressure谱界/预条件/alias门不能直接用于任意切割单元/3D薄隙。压力不消除闭盒过填，glyph不代表排水体积。干15研究数学上不必假装依赖所有湿功能，但实施资源优先序不变，正式含水方案仍消费真实可信接口。

## 5. E10–E12：scene、cold和峰值按域分账

scene须直接消费同一accepted物理/几何身份。旧列顶Scene不能呈自由沙体；目标19需表面/体积可见性、前后遮挡及3D接触参考，视觉拉伸不算真实z运动。借用GPU pack→VBO/合法shader消费模式及完整帧lease，不能由旧pbuffer vertexBlocks0推所有窗口无SSBO，也不能默认任何窗口顶点SSBO可用。新增surface extraction/粒子实例/排序/OIT/体积采样都记字节、pass、barrier和fragment成本；XComponent+shader覆盖背景前景，失败FAULT或明确STALE，禁止Canvas/软件GL产品回退。

GPU resident、GPU FULL/witness、native cold/输入页/owner helper、FBO和driver各分列，按**同时活跃/保留区间**求峰值：

`peakGPU=max_t(state+work+sort+contact+geometry+trace+scene+coldGPU+oldNewOverlap+FBO+driverGPU)`；native/RSS另有cold副本、页、JSON/worker、helper/栈/JIT等。两边量纲同为B也不能混称GPU显存，更不能把一个block limit当总量。

当前K512 DM合同resident98022528B、最大单对象23334912B、GPU FULL140009472B、两native FULL槽376119296B；G11 DM/V8192 resident104117376B、最大单对象23334912B、两native FULL388308992B，witness GPU/native各4722688B。G11已知合计641920128B**含GPU和native**，并排除driver/FBO/JS/对齐等，非实测RSS；T660又单列512B native bookkeeping及实际proof/helper。旧profile明确不许未退休就任意并存，不用其总payload给新路线冒已验证可分配。

现broker host process credit524375552B、GPU FULL credit366564432B有各自用途；后者不是系统GPU总容量，前者不是进程RSS上限。可复用双slot/单builder/单reader、真实free后退信用、fence/ContextRetired生命周期；新route需要新预算/责任/实际sizeof，不能悄悄复用旧票据限值或以析构/逻辑Trim退活内存。冷档需全部新状态、tensor/接触历史/几何/水/源及目录版本与SHA；一张hash不替完整原件，TTL到也须真实reader/GL退休后才可复用。

profile迁移有两种研究选择：先撤权退休释放旧对象再冷建新（停顿与冷证据另测），或正式预算允许有界旧新重叠。二者均须保原件/失败原子性，不能只给新状态净增量；旧S/F均值没有的应力/层序只能标重建假设。新layout/event/seed/config/scene/cold/cache identity独立，不重写现G9/G11/G13为新能力。

## 6. 假设载荷的精确复核与单block陷阱

沿T673：3bank；2D每点96B/节点64B、每cell4点、支撑9；3D每点192B/节点96B、每cell8点、支撑27；节点=(Nx+1)…，不是cell数；排序仅候选16B/贡献×双缓冲。全部是**state+排序的部分下界**，不含本节其它工作/trace/contact/water/scene/driver，非冻结ABI或性能预测。

| 假设grid | 点数 / 节点数 | 单bank state B | 单sort buffer B | 3bank+双sort B |
|---|---:|---:|---:|---:|
| 2D64² | 16384 / 4225 | 1843264 | 2359296 | 10248384 |
| 2D128² | 65536 / 16641 | 7356480 | 9437184 | 40943808 |
| 3D64×64×8 | 262144 / 38025 | 53982048 | 113246208 | 388438560 |
| 3D64³ | 2097152 / 274625 | 429017184 | 905969664 | 3098990880 |

最后一行若把3bank state全装一个绑定block，1287051552B已大于本次M2查到的1GiB；两sort合一个block1811939328B也超。分别一bank/一sort虽各低于1GiB，仍不证明能同时分配/满足4binding或吞吐；分块/稀疏/gather也需精度和最坏拥挤度证明。本表不是“3D必需3GB”，更不是可行性或无解裁决。

同样，示例8格内域+每侧1格halo、完整节点临存：2D10²×64=6400B，3D10³×96=96000B，后者超过本次shared32768B。只减少线程数不使相同临存数据变小；需要正式数据分块/字段分期/全局存储与增加同步的账。示例不冻结tile/kernel/接触宽度。

3D MAC面数例64²×8为103424、64³为798720，u/v/w/p、压力/黏性迭代和几何层级另计，不藏在材料点state里。2/4层F旧T673裸增量663040/1989120B只是`5(D−1)N(12+16L)`（N128/L64）中质量动量/来源部分，缺mode/rest、交换/work/events/audit/scene，不由“小于1MB”裁定双层优先。

## 7. E13–E15：输入、性能与24案例映射

新路线须声明姿态/速度域与时钟来源、连续未wrapθ/Ω/α/接受区间、暂停/反向/重复/溢出，准静态重力变化与真实转动分域；不得由native接收时刻或UI callback推真实手速。C02/C13–16直接需要这些身份/输入原件，不以数值角表存在冒资格。

用户显示30fps即可；性能须分新接受帧率（不算STALE重复）、acceptedSimTime/wallTime、root p95/p99/最坏、全部trial/replay/迭代、GPU活跃/CPU提交/尾等待、排序与接触峰值、热后持续和并发渲染。显示30fps不证明仿真实时，也不要求120fps渲染。新profile的deadline/误差/子步政策由正式设计给出，不自动沿用/放宽现250ms/GAP门或借“手机GPU”给大概率实时承诺。

24案例逐项保留原T673 criterion/reference/NOT_RUN，机器表映射工程约束：C01事务/来源；C02坐标/输入；C03容量/性能；C04水几何反冲；C05–07角域/崩塌/数值；C08拓扑/迁移；C09–12自由体积/碰壁/支撑/全床容量；C13–16全周/停止反向/寿命/轴向；C17–20剖面/反流/交换/候选比较；C21–24薄隙水/真实z运动/间隙壁/三维耦合。缺合格参考的材料效果为REFERENCE_REQUIRED，不把自一致案例变成材料标定PASS。本轮只复算元素/字节/映射，不运行物理推进或新增CPUoracle。

## 8. 交正式15设计的最小输入与接力

| 输入组 | 最小内容 | 提供方/未齐时登记 |
|---|---|---|
| 支持域 | 45°动作/速率/停反、干湿/材质、二维面内或出平面、盒尺度与单位、初态/边界 | designer按用户目标显式定域；未定DOMAIN_REQUIRED，不替用户冻结 |
| 同观测比较 | T673 A有限扩域与C体积等同参考/分辨率/误差、保留到16/17的能力与必须换的部分 | designer；MPM仅候选，不因本盘点采用 |
| 物理/历史 | 全算子/本构/接触/作用功/支撑/状态字段及来源、trial/replay、拒绝/稳定性/迁移 | designer；不能只交profile/几段伪代码 |
| 定容与算术 | N/Np/Ng/q、L/源、Nd/Ccap/Zcap/bin、dt/迭代/tries、数值范围/舍入、事件/证据上限 | designer给算法上界，architect转offset/binding/checked/预算 |
| 可信参考与设备范围 | 当前G9真实物理证据/仍NOT_RUN项、必要水接口、目标设备/驱动支持范围及原始probe资产 | tester整理现证据；effector未来真机按正式设计量测，不占本卡设备 |
| 资源/性能与冷迁移 | GPU/native/FBO/driver分别预算，旧新并存/退役，真实接受30fps与实时要求/热后范围 | architect在正式方案后落合同；未知COST_NOT_MEASURED，不补PASS |

正式15方案齐才进入对应工程/开发；16–19按其真实机制/证据接力，目标18不是9/12/15–17的倒挂前置，二维PASS不放行19。高优先13/14/12继续按当前清单前置，**不等待本盘点再次审批或后续15研究关闭**。

执行prompt：不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态有限例外），不更新滴答同名清单。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。designer定算法、architect工程约束/首次归因、developer开发编译；tester离线用例/oracle复用/夹具/host/构建签名分析，effector全部真机与效果采证串行。当前不编产品/跑测试/用真机，不引入CPU求解或低优先实施；19目标顺序和XComponent+shader唯一渲染/FAULT或明确STALE边界保持。
