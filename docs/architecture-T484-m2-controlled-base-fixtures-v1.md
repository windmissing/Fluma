# T484 · M2受控BASE与数值、来源聚合补证工程方案

2026-11-12，architect，P5，不使用真机。消费T479/T470/T446和实际host、GLSL及冻结原字节。本卡只交工程方案和首次分流，不改线上接口、算法或tester报告。原件哈希、112件目录清点及既有oracle推导的示例见 [证据索引](analysis-T484-T478-T480-evidence-v1.json)。

## 1. 纠正原报告解释

T479卡称105件原件，当前目录 `.codex/local-test/t479-c` 实存112个bin：24组bank/pub/work-control/work-origins共96件，另16件command/completion。没有完整逐场景manifest，不能据文件数推覆盖或把多出的文件算新增用例。以下核对只认文件名、字节与hash。

| 命题 | 实际实现和原件 | 裁定 |
|---|---|---|
| emit=.125Q只生一粒 | 131072=122333+8739；n1-mid的SoA slot0/1分别为122333、8739 | 总请求拆为两次birth，原T470-B1只描述首粒不完整；不是剩余请求被丢弃 |
| 输入y不是出生y，shader自行散开 | `birthSelected`明确outRow[1]=x、outRow[2]=y；冻结执行source SHA为eb479a7b…，n1-mid/n1-after/n2-after所有live slot y均为0 | 此解释错误。command binary64经过原Q20 floor后直接赋出生y；测试把SoA当AoS扫描会误读 |
| 同列两次SOURCE无merge说明超出radius | n1-after仅4/192槽在用，`decide`先找最低空槽，满池才STEP_PAIR；两次接受SOURCE的origin分别1/2 | 没运行pair选择，不能证明radius、tie或RNE |
| origin key含列号，同列多SOURCE才可聚合 | origin取接受SOURCE序号newSource，不由列坐标组成；同列的n1两次SOURCE也是origin1/2 | group的column与origin是独立维度。不得修改origin语义让同列SOURCE合并 |
| N1c检查serial delta | numeric_audit.py判据尾部有`or True` | 该条无条件通过，报告PASS不能作为serial断言证据。原件2→4可单独复算delta=2，但需修真实判据 |
| N3是radius+1反例 | x=288.5已超x合法域[0,128)，返回input reject65538 | 仅是输入域负控，不是合法pair半径负控；半径作用于y差 |
| N4独立聚合 | 原判据在整个org区滑动搜索131072/0相邻字，没有按origin索引或BASE槽重算 | 原始前两条确是131072，但原判据不能证明分组及无串位，需要按2*(origin−1)精确检查 |
| N6数目证明重选最小pair | 仅检查merge=2/birth=2/serialDelta=4，未导出/重算每行选择 | 可保留数量子集，尚不能证明逐次最小pair、snapshot和RNE |

另有来源错配：host的N3实际调用seq=3，手写归档command沿用N1的seq=2。后续必须归档真正上传的同一份32U32字节，不能另拼一个“等价命令”。这些是测试设施/判据问题，未证明产品算法缺陷，交tester修订。

## 2. 工程决定：现有host设施足够，直接交tester

`SourceFixture::edit`已有 `glBufferSubData` 和barrier，`read/snapshot`可读实际accepted bank/WORK。无需新增线上接口、产品backdoor或另派业务developer。tester在测试编译域扩展当前fixture即可；以下接口只位于新 `test/<新卡>/`，不进HAP/NAPI/native产品库。

建议测试接口：`FixtureSeed SeedViaSource(profile, commandBytes)`、`FixtureReceipt InstallBasePatch(seed, span<PatchWord>)`、`RootEvidence RunSource(commandBytes, expectedHash)`。PatchWord含bufferRole/wordOffset/oldValue/newValue；Receipt含seed来源、profile、shader/layout/host hash、原BASE/WORK有效账前缀hash、patch清单与安装后hash。root、bank、sourceEvent、dedup/terminal、generation与RootGate的header镜像从真实已接受种子保留，不能在下一root的pass4/8内临时改candidate来绕入口审核。

执行次序：真实SOURCE造合法种子→根已完成/fence退完→读回BASE→按允许字段patch同一个accepted bank→GL_BUFFER_UPDATE_BARRIER_BIT|GL_SHADER_STORAGE_BARRIER_BIT→完整读回并核patch及不变量→冻结command和独立期望文件→原13pass SOURCE（包括preflight、candidate、spare replay、final audit和短接受）→导出completion、SOURCE128全事件、接受前后bank/pub及WORK control/origins。保持原GLSL字节，不加跳过audit的宏，不手动伪造接受或反射通过。

首版允许patch**已有live槽的x/y/vx/vy**。SoA地址为`field*K+slot`，不是`16*slot+field`；source16是另一条记录。x保持原column，0≤x<128Q；y在[0,CQ]，与场景sand/water关系相容；速度为signed32原bits且在±2^30；保留mass/id/generation/sourceId/origin/tag/wait/medium以及所有库存/账。同列变位置/速度不改变列库存或source mass，RootGate镜像无须改header。若测试需要不可eligible粒，先通过真实merge生成mass=244666的heavy槽，再patch其位置，不凭空改tag/质量伪造合法种子。

无空槽种子最简单：K192/Kw64真实SOURCE q=192*122333/Q，得到192个122333粒、同sourceId/origin、每粒generation合法，accepted sourceLedger/dedup/serial均由产品生成。下一SOURCE用精确binary64 q=1/Q，出生1质量单位；在满池上只需一次merge腾槽再birth，便于核唯一首对。种子ROOT和待测ROOT是不同nonce/commandSeq，不能误触dedup。填充粒可设y=8R+2R*slot，R=65536，既在CQ内又彼此>R；目标少量粒放低y区，避免填充粒抢pair。空间分布只用于合法测试前态，不宣称真实交互能自然生成此布局。

每个正控要求GPU preflight/audit实际成功（control preflight位、completion阶段/原reason、accepted outcome与bank切换均有证）；若首错15发生在入口，判FIXTURE_INVALID，不能降级成NO_PAIR或用新的expected追认。另做duplicate ID、坏origin映射、质量/列库存不符各一个入口负控，核原reason/阶段且接受bank不变，不能为了“合法注入”关闭原审核。

## 3. 最小数值场景与独立预期

只复用 `test/t446/m2_oracle.py` 的brute_pair/merge_fields/RNE，不重跑其旧suite、不新增CPU仿真。期望根据已冻结的实际BASE和command计算，在GPU执行前写manifest及SHA；断言不能由completion反推。示例已用现有oracle检查，索引中的SPEC_WITNESS_ONLY不是产品PASS。

| 场景 | 合法前态构造 | 应核结果 |
|---|---|---|
| 同距离tie | ID1 y=2R，ID2/3 y=2R+2，其他填充粒隔离 | 选择unsigned字典序(1,2)，不是任一相邻pair |
| nearest不是最小b | ID1 y=2R，ID2 y=3R，ID3 y=2R+1 | 虽3更近，仍选(1,2) |
| radius−1/恰好/+1 | 只有ID1/2接近，y分别2R与3R−1/3R/3R+1 | 前两例(1,2)，后一例合法NO_PAIR，零merge/birth，原成功种子不丢 |
| 非eligible夹在中间 | 真实merge种子中的heavy粒y在两个light目标之间，其余隔离 | heavy不参与也不遮蔽light候选；完整group和最小pair与oracle相同 |
| RNE非零正负tie | 等质量122333，目标vx=(0,1)/(1,2)/(−1,−2)/(−2,−3)，x/y仍合法 | 分别0/2/−2/−2，逐字核MERGE四份16word snapshot及最终SoA |
| 抵消与大乘积 | vx=(1,−1)；另vx=(2^30,2^30−1) | 精确0无负零；大乘积仍<2^48且结果2^30，不能float近似 |
| x/y非整数中点与不等质量 | 同列x相差1 Q20最低位；用真实种子中122333与8739的合法轻粒 | 每分量一次有理RNE，位置保持列/范围；不能把均值恰好整数当非零tie证据 |
| merge后重新选对 | 一次待测SOURCE要求多个birth，独立重算每个MERGE→BIRTH后当前池 | 每行pair基于变更后全池，保留完整snapshot/ordinal/serial链，不只核最终数目 |

不同origin、同sourceId、同column的合法前态**已经能用两次同列SOURCE得到**；隔离其余pair后应不跨origin。不可仅patchorigin字段而不修改ID所属sourceLedger区间。跨origin反例若要满池，在不同接受SOURCE中分配粒数总计K，保留每批的真实id区间和source序号。

## 4. 聚合补证无需等待注入

n1-mid的两个live槽（mass122333/8739、origin1）已经是**同origin多粒聚合**的正例，精确和131072；n1-after/n4-after的两组origin1/2各两粒各131072。直接读取每个sand槽的originLo/Hi与mass，按有效sourceLedger前缀重建字典，核WORK[sourceOrigins.begin+2*(originLo−1)]的U64值和相邻零条目。输入只有不同列不构成origin定义；同列/跨列都应按接受来源分开。

M2此版本audit要求source序号高字为0且每源准入≤CQ，合法单源liveMass≤CQ，所以高字必须0。不得为“覆盖U64高字”构造不合法高字正例；可做高字污染负控验证拒绝。n6-after origin1保留质量为192*122333，经过两个merge live槽190；origin2质量131072、槽2，合并改变槽数但不减来源质量。按原账/槽重算，不能对整个org数组扫描出现次数。

对已有raw直接修N1c为checked U64差值=本root MERGE+BIRTH，DIRECT/END不计，replay/拒绝不重复计；N1/2/4还需逐槽质量而非只看completion计数。T470已有真正fatal23–26原件及bank不变可按其真实范围复用。Q poison目前是对已成功Q的短接受Inspect负控，不能扩大为GPU执行Q发生fatal；若补该域需另明确故障切点，保原首错，不混入本次数值正控。

## 5. 直接可派tester任务书

P5，tester，不使用真机。按本稿先完成既存112件原件manifest、SoA解码、N1c/N4与报告解释勘误，再在**测试域**扩展现有edit/read/SourceFixture并执行第3节受控BASE正负控。无需产品developer前置，不能以“没有线上粒子注入接口”停卡。至少同一192/64档完成数学场景，四档资源/stride复用既存证据；新GLSL来源选T482 coherent修复后的冻结版本并记录hash，旧T479 shader只用于解释历史原件，不冒作当前产品同源。

每场景保存真实上传command.bin、seed原件hash、patch前后BASE、ledger/dedup有效前缀、独立expected及其生成先后、SOURCE128事件与四snapshot、completion、pre/post accepted/pub、WORK control/origins和bytes/SHA。入口失败先记fixture无效，保原错误及尝试，已可做的聚合/勘误先做完。结果回architect分工程/设计/用例或仍缺证，不重开已交T479、不让manager重测。

要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部受阻先完成其它可做部分。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列/滴答同名清单。CPU停投，仅复用既有oracle；所有前景背景只XComponent+shader，禁止Canvas/软件GL产品回退，失败FAULT或标STALE完整旧有效帧。成功原卡回执并本人行结束待验收；失败/关键缺证原卡写未完项、复现原错、证据、已试办法、不能自行继续原因、所需前置与接手角色，本人行看评论。

本轮完成只读来源/原件核验、合法前态工程路径、示例oracle推导及分流，不运行GPU或真机测试，不宣称未覆盖的数值场景通过。
