# T116：T156 到件核证与完整事务复杂度分流 V3

2026-10-28，architect；原卡 `6adf3c1ce4b017ab3404b7c5`。本轮消费 T156 `952da4a` 正式报告、两份原 appfreeze、按PID过滤的原始hilog、脚本/console、step3-store，及基线 `0cc0439`。不使用真机、不重跑测试、不改业务代码/判据。证据路径、SHA、行号、时序和统计在 `docs/analysis-T116-T156-arrival-v3.json`。本稿接续 V1/V2，优先修订其尚未到件的判断，不覆盖旧raw或代写effector报告。

## 1. 本轮裁定

**设备长同步任务复发成立；首次归因与设计分流已交付，T194 可据此开展整体设计。实际失败事务的调用乘数/原始输入仍缺，T116按关键缺证规则保留“看评论”。** 这些缺项是后续计量/回放所需，不是T194开工或manager登记本轮交付的附加门。

1. 两个故障PID均有精确索引、seed/lift链与audit采样；R2另有collectBirthHeaders采样。新索引被真实执行有证，但“某个热点已完全消除/迁移”“单一排序死循环”“已实测数百birth”均不能由采样栈推出。
2. 逐birth满池重建是源码事实；应写 **M×单次find**，而非把B固定成N。T156“聚合O(N²logN)比旧O(N²)更糟”混用聚合与单次分母，不成立；某些早命中负载旧扫描更便宜仍是可检验假设，不由渐近上界判断本次耗时。
3. 收尾身份问题比报告所述更明确：**R3有独立run，并未复用step3 run；R3本身也在pan之前已CLOSED。** 报告所引258行/50.8MB属于step3旧PID；R3本PID真实CLOSED为274/0。两者都不证明新注入后的稳定/完整双流。
4. 分流主线为**设计问题→既有T194完整复杂度重设计→T195独立全链规格→architect工程化**；不直接派“再优化一个排序/把merge挪循环外”等局部修复。采集身份/脚本缺陷归工程；旧报告勘误归effector。没有证据判物理注入功能本身失效。
5. T57仍等待必要真实修复及独立稳定性。T175 OFF_ALL零分配门禁、T128单pair差分、T191严格O2隔离开发均按原范围成立，不替代完整事务/设备稳定性。

## 2. 设备证据逐项修正

### 2.1 PID、真实进度与栈

| 臂 | PID / run | 本PID原始证据 |
|---|---|---|
| step3 / R0 | 21230 / `p011-run-1793148867083-3` | 129帧，08:54:27.293–33.428；33.431 CLOSE_REQUESTED、33.438 FENCE CLOSED、33.439 CLOSED 258/0 |
| R1 | 22265 / `p011-run-1793148908460-3` | APP_INPUT_BLOCK，原文件08:55:23；两条WS_FRAME physical=1744/13522ms；sequence/substep最终4；采样含index→merge、audit→sink→seed |
| R2 | 23181 / `p011-run-1793148975272-3` | APP_INPUT_BLOCK，08:56:30；physical=1725/13553ms；sequence/substep最终4；采样含audit APPLY_TAIL、index→merge、sortActive→collectBirthHeaders→seed |
| R3 | 24636 / `p011-run-1793149047925-3` | 137帧，08:57:28.022–34.591；34.595 CLOSE_REQUESTED，34.610 FENCE/CLOSED **274/0**；首pan38.299、末pan46.088 |

两个freeze文件头部PID与启动后记录22265/23181一致；console最终PID为空，符合进程已不在的观察，不能称“全程存活”。故障被发现前后仍出现少量帧/commit与收尾，不等8秒内响应合格。physicalTimeMs是整帧Date.now墙钟区间，既非某次find独占CPU时间，也不分解成GC或单一阶段占比。

R1原ETS栈：compareGroup/compare/sift/sort/findBody/find→mergeOldestHot:953→editPoolHot:978→preparePatchHot:1053→M2Sink:230→seedBody:213→liftByShake:7083→applyMotionChannels。另有TS audit→SINK_ROW采样。

R2原ETS栈包含auditParticleHotBody:1145→APPLY_TAIL:1210→seed:213；同文件TS明确有P011MergeExactIndex→mergeOldestHot→editPoolHot，后续ETS还见sortActive:687→collect:721→collectBirthHeaders:136→seedBody:198→light seed:7089。因此T156“R2无merge帧”的概括应勘误。不同采样位置保留，不能把多个sample堆栈按调用次数求和。

本批日志没有本PID POOL_PROGRESS/WS_EVENT pool_progress。页面默认诊断false，开启依赖“池诊断切换”；执行脚本没有明确启用并握手。源码诊断存在、T154/T155/T175离线通过，不等设备已启用。**R1/R2实际Nmax/live、B、M、audit/index/copy次数、λ/候选次数及冻结原F64均为NOT_CAPTURED，分析JSON用null，不填0。** step3 ledger的Nmax512不能替代失败时N/L，也不再使用旧报告N192估值。最后粒子总质量0.053067…并不能反推出birth或live个数。

### 2.2 R3新run与旧文件混入

按PID24636过滤，R3 STORE行已指向独立run `1793149047925-3`，generation1；真实FENCE requestId=ui:1、sent/written=274/0、lastSequence274、lastSourceEpoch0。CLOSED先于首pan **3.689秒**。本PID共468条pan_update，其中400条accepted=true，均晚于收尾；收尾后没有新物理帧。

`SandCanvas.frame()` 在captureFinished时直接排下一frame callback并return；`stageGestureSandAndFeed()` 也先检查captureFinished，之后才调用field.injectGestureSand。pan的accepted只表示坐标门通过，早于dt/事务入口，不等成功准入。默认pool trace关闭还会隐藏inject_skip日志。因此这批 gesture_sand=0 可以由提前收尾解释，不能据此宣“触摸产品三次都未注入/独立注入bug已定”。真实未收尾路径若仍失败，须以自己的入口/回执取证另行裁定。

脚本R3-pre/post dump含旧PID历史，`Select-String 'P011_OBS_STORE ' | Select-Object -First 1` 和 CLOSED的First1均未过滤PID/run/time；它选中step3旧run，不是页面复用。console“CLOSED258/0”在08:58:23打印旧事件，不能当新触发时间。脚本每臂启动后盲点(240,620)，本PID随后均出现ui:1收尾；需改为可识别控件/状态断言，不能仅靠把最后一次finish click排在swipe之后就声称业务顺序已修。哪次屏幕命中该控件还需实际UI/输入关联，不由坐标本身定论。

step3-store精确复读：50818215B、258行、sequence1..258连续，全部context.commitStatus=committed、audit.conservation=true/errorCode=null；sourceEpoch0、externalAdmissionCount0、particle live0、particleEvents0，Nmax512/cols128。它是真实step3健康窗口资料，不是R3或带新源负载资料。仅采这一文件不能签双流manifest COMPLETE。

console记录R3两次recv均0B，脚本直接 `hdc file recv ... | Out-Null` 且未保留退出码/stderr；cmd-audit只覆盖Run-Cmd包装过的操作，**不含这两次拉取和裸click/swipe的完整命令回执**。报告“241MB审计含全部命令”需收窄。后来step3文件成功存在，不等于证明脚本R3拉取成功；旧文件原码恢复与本run收尾验证分列。admissions 0B不能直接解释成产品无准入，也不能凭此跳过全部manifest：按既有schema分别判空流是否合法、sent/written与行数/最后key是否一致，缺流/缺fence/跨run就显式INCOMPLETE，不改完整性判据。

### 2.3 源码/包身份界限

本轮从git对象读取0cc0439的Hot/M2Field/M2Sink/ExactIndex/WaterSandField/SandCanvas/HotBudget/TiltPlanningPass八个文件；当前源码除LF/CRLF外逐字相同，SHA分列记录。原栈行号与此基线对应。T156声明s151 signed SHA `3c8abfae71bbff53ca5e1ae98137b187e2d11f96af4d121a0423597ff0748f6c`；本地按s151文件名未找到签包，未做重编/安装/真机读回，不把声明升为独立签包核验。该缺项不抹掉原栈确实执行新索引的证据，也不阻止源码设计分流；后续打包必须保留source/flags/unsigned/signed/实际安装退出码映射。

## 3. 同分母复杂度修订

统一变量：C列数，N池容量，L当前live，B实际进入birth循环的次数，M选对find调用次数（含失败），A audit次数，R某外源事务实际λ尝试数，V实际preview候选数，S帧内子步数。另记J=seed调用，Q=plan request rows，H=motion rows，P=全池/场复制次数，I=按ID线性找槽次数，T=票据数，E=事件及lineage字数，Z=归档字节数。**B_requested、B_entered、B_committed必须分开；M不等已提交merge数。** 失败与丢弃preview的已消耗工作也计入。

### 3.1 每次find与birth循环

现`P011MergeExactIndex.findBody`每次从当前scratch重建：扫描N、两个L元素heapsort、重复ID检查、清理/构建K叶树（N≤K<2N）、至多L次A选择/二分/rangeMin。合法输入下单次上界 **O(N+L log(L+1))**，scratch内存O(N)；centroid/ID/free-list改变后没有跨mutation旧证书可直接复用。重复键仍有ID/slot次序，sift根严格下移，不能因“采到sift”判死循环。

`editPoolHot:968–999`逐phase计算 `floor(mass/q) + (mass-floor(mass/q)*q>0 ? 1 : 0)`，q=massPackage，按原F64操作；B不是天然等于N。每个进入的birth在freeCount=0时至多调用一次find；成功merge释放一个槽又被本birth用掉，满池时后续birth可能继续find，失败立即返回。该支路 **M≤B_entered**；初始空槽F且没有中途其它释放的成功单段，M=max(0,B−F)。这是条件推导，不是本次实测。直接显式MERGE事件另计mergePair，不混入find计数。

所以这部分上界是 **O(B + Σ[m=1..M](N+Lm log(Lm+1)))**。只有另行规定满池L=Θ(N)、B=Θ(N)的负载族，才可约成O(N²logN)。B可能由mass/q、多列/phase、重试、journal余量共同约束，不能拿N512或“数百birth”替代真实乘数。

对照T123前 `469ee37^` 的mergeOldestHot：每个候选A重扫N找最小剩余ID，再扫N找B，至多L个A，单次O(NL)⊆O(N²)；它同样处于birth满池循环中。相同M分母下旧项为O(MN²)，新项O(M(N+LlogL))；B=Θ(N)时旧上界可达O(N³)、新为O(N²logN)。**这不证明所有负载实际更快**：旧扫描首个A即命中时可能只O(N)，新版本仍先排序，有常数和输入敏感差别。要判本次新旧速度/总算量，必须同冻结输入、同语义计数和实测，不能用两个不同层级上界作优劣结论。

### 3.2 完整调用乘数基线

| 上层/条件 | 实际调用与乘数，源级成本 |
|---|---|
| 页面/外源入口 | pan坐标门→dt→构造C长数组→stage/captureFinished门→injectGestureSand；成功源账提交独立于后续step，当前还存在直接入口与每子步≤1队列消费，不能只预算帧内队列 |
| 湿外源λ | injectGestureSand的8次for，R≤8；每次容量/η/audit与各列H/L正质量seed，成功不提前退出列循环的失败工作也计，最多每次2C个seed；save/restore与事件回退单列。**lift自身没有这个8次λ循环，不给每条shake链机械乘8** |
| 每帧 | 正常step dt裁到1/30，S∈{1,2}；每子步先消费队列、备份、planner、authoritative、rollback/commit/观测。用户输入在帧外的同步直接注入另计 |
| planner | REQUESTED/CLAMPED/OLD最多3个实际preview候选；V为实际执行数，不是3个都必跑；每候选restore/collect、可能particle apply/finish、shadow channels/分配，最后restore；accepted后还有authoritative和consistency镜像，不能把preview当零成本或假设每候选都执行lift births |
| collect阶段的lift | `collectingTotalMotionRequests=true`时逐列记录riseRequest并continue，不调用seed；preview的shadow `allocatePreviewPlans`不直接调用liftByShake。只有走到有写入的apply lift才按正heavy/light各seed，≤2C；各分支独立计J，不乱乘V |
| 单次seed | beginStep冻结/备份→collectBirthHeaders（一次完整collect+sortActive，复制planB身份header，随后覆盖birth request）→单row apply→finish→事件导出；不是“一次轻量插入” |
| 普通particle collect | beginCollectBody做A/B两次完整collect+sortActive再比较；后续apply/finish与每row sink，不能与seed的一次collect混为同计数 |
| row/patch | field/budget/guard→preparePatch（取pool、scratch复制、birth/merge或ID找槽）→publish→SINK_ROW audit→budget close→事件/lineage；失败可能fallback再prepare、rollback，保留原码/票据与已耗工作 |
| Hot audit | 每次含pool读/扫描/free-list/ID sort/列账/预算audit，O(N+Llog(L+1)+C)；成功的单row seed通常SINK_ROW+APPLY_TAIL+FINISH共3次，失败可提前退，不强写3J；普通Q行成功apply通常≤Q+2次 |
| 其它非merge项 | motion每row `particleSlotHot`线扫N；非birth事件也用线扫找slot，I次是O(IN)，H=Θ(L)时仅motion查槽就可O(NL)。HotBudget.reserve去重及slotHot查token线扫T，Q行票据链可有O(Q²)上界；这些是源码可达成本，不称本次栈已测热点 |
| 复制/归档 | 全池13buffer和列场复制每次O(N+C)，export快照/事件序列化、page/ledger/Worker消息、writer/fsync按实际E/Z计；固定容量/零分配不等零复制或常数时长 |
| GPU隔离 | 每次只读pair请求仍含freeze/精确prepare/sort/range/SHA/bridge/upload/dispatch/fence/消费；CPU merge/ledger仍权威。GPU不自动减少M/J/A，不把fence等待当GPU timer；未启生产GPU，不把隔离微测套入本次APP_INPUT_BLOCK |

可供设计/独立验证使用的**可加工作项**基线（按实际执行集合求和，不是重叠计时相加）：

```
Wparticle ≤ Σfind O(N+L log(L+1))
          + Σcollect O(N+L log(L+1)+Q+H)
          + Σaudit O(N+L log(L+1)+C)
          + O(Pother*(N+C) + I*N + ΣticketOps Tcurrent + B + Qtotal + E)
Wpublic   = Wparticle + WfieldChannels + Wguard/eta + Wrollback + Wpublication
Wframe    = Σ实际外源调用/λ尝试 Wpublic + Σ子步/实际候选与authoritative Wstage
            + Wobservation(Z) + WnewPipelineRendering
```

Pother只计前述find/collect/audit内部未已计的复制；rollback/field副本也按同原则去重。计时区间另按inclusive/exclusive scope标识，不能拿Σaudit计时+其内部copy/sort计时二次求和。源事务8次λ已展开进调用集合时，不再外乘8；J/M已为全帧累计时也不再乘2C/V/S。

此式暴露完整链的粒子/票据乘数和待设计接口，**不是已证明所有field通道的紧渐近界**。T194必须继续展开WfieldChannels（含relax/guard、η求解、shadow镜像、逐列particleMassAt、其它启用通道）、观测/发布的实际循环及容量，给容量内最坏界/设备目标，不能把未展开项视作O(1)。T156无这些实际次数，故本轮不写完整设备W的定量数值或唯一耗时首因。

现M2冷容量Q2048、event4096、lineage8192、motion=N；index树K<2N；work固定pool副本内存O(N+C+Q+E+lineage)，加preview/观测/渲染/GPU staging各自预算。事件容量会拒绝超限事务，不等B天然≤N或固定缓冲足以保证UI预算。失败清掉临时事件仍要计算清前工作量。

## 4. 设计与工程交接（先整体方案，不派局部业务补丁）

### T194 designer：本轮首次分流到件，可开工

消费本稿、分析JSON及既有T123/T128/T130/T137/T183；明确上表哪些是源码事实、条件界、设备实测、未知。负责 `docs/algorithm-T194-whole-transaction-complexity-v1.md`，给public source事务/正常子步/整帧（含帧外直注入）完整有向调用和每层执行乘数、最坏时间/内存/排队界。

以整体结构处理重复rebuild/collect/audit/copy/ID查找/票据查找，而非只把排序换一次。比较增量索引、同revision不可变阶段/证书、完整事务native owner和GPU；这些都是待designer证明方案。候选pair合并改变质心/可配范围与后续ID选择，先在循环外预选多对或“一次merge腾多槽”不能直接等价；证书须绑定pool/field/budget/请求与mutation，不能越过write-set失效；不默认允许改变原F64加法顺序。

工作量预算超限的原子拒绝/背压/调度必须保留完整源事务成功边界、请求身份与回执、顺序/ID/守恒/预算/失败原码/rollback/事件，不能少做物理、半提交跨帧、跳audit或降低输入伪收益。语义确需升版时列差异交既有裁决流程，不默改。搬独立物理owner能改善UI阻塞，但总算量另算；GPU仍最高调度优先，严格O2为对照不替结构方案。设备毫秒目标由真实校准支持，未知常数明确待测，不能host秒数外推。

### T195 tester：等T194，独立全链规格

所有权独立规格与预期oracle，不实现业务；将上表变成各scope的进入/完成/失败计数恒等式，覆盖满池可配/无配、首A早命中/末A才命中、相同键与质心跨范围、空槽F、B独立于N、C多列多phase、8次λ失败、V0..3和S1..2、motion ID查找/票据链、guard/audit/复制/观测/GPU全项。选择B,N,C,Q独立变化的负载，不能只取B=N画一条曲线或重用旧287pair冒全链证明。

给每公共事务/帧aggregate上界、内存/队列界及热点迁移反例；独立比较完整F64状态/ID/源账/预算/guard/每事件lineage/失败/回滚与发布序列。未实现方案预期RED登记NOT_IMPLEMENTED；不能把预试RED当新产品缺陷。原码错误/计量缺失先回architect，不自己删判据。设备收益另立真机任务，不在此卡跑真机。

### 采集工程与最小缺证（不作为T194等待项）

1. **effector离线勘误，可立即做、不使用真机**：按本稿PID/run/时间/行号修T156报告对R2、R3、258文件、完整cmd-audit的表述；原raw不改。核已有晚续拉取记录是否有274本run ledger/admissions/fence及真实传输退出码，存在则只补归档；不存在标NOT_CAPTURED，不重跑宽矩阵或补写旧时间。
2. **观测工程随整体方案工程化，不使用真机**：复用T154有界ring/span/首次超长原值，不再做重复热点优化。需在第一物理step前明确启用并可验证配置/日志握手，不能等UI卡住后才点击诊断；新增真实I/票据扫描/全层P/E/Z计数若当前未覆盖，由T194/T195定义字典后再实现。R1/R2无诊断raw不能事后补数；首次threshold是部分运行快照而非replay起点，完整回放另须事务入口原池/场/预算/输入身份。
3. **后续effector精确设备取证**：真机标签，设备串行；仅在相应方案/仪表独立验证齐后执行一条目标复发与一个实际运行的对照，沿真实前置排队。冷启锁PID并读取本PID本窗口STORE run/generation/source/variant，未CLOSED状态下确认帧仍增；先真实request/成功sourceEpoch或明确拒绝回执，再唯一显式finish并对本run末key；无真实准入就报告INPUT_NOT_EXERCISED，不签新注入稳定。获取首长窗口N/L/B_entered/M/A/collect/index/copy/IDscan/ticket/候选/λ/耗时scope及bounded原输入；计数未导出为null。裸click/swipe/拉文件全部经记录退出码/stdout/stderr的统一执行器；禁止First1全历史选路径。只合法拉本run文件，生成真实fence并核manifest，不绕权限。

全部下游prompt须含：不使用真机（设备卡则真机标签与串行队列）；共享树不回退他人修改；不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人状态有限例外；不得改滴答同名清单。所有渲染含前/背景仅新XComponent+shader，无Canvas上屏或失败回退；故障FAULT或显式STALE上一有效新管线帧。失败/缺关键证据看评论并回原卡，成功结束待验收；manager登记与裁决不新增验收门。

## 5. 本轮交付与保留

本稿给出了设备失败首次核证、同分母成本修正、可执行整体设计与独立验证交接；不会因真实N/B等缺测重复空转归因，也不再另派逐点热点修复。实际负载数值、唯一耗时占比、精确整事务回放及注入后稳定/完整双流仍未证明，原卡据实“看评论”。T194可接力，T195仍以T194方案为真实前置；本执行会话不改他卡状态/队列、不关滴答卡或同步manager清单。未用真机、未重跑任何测试，未给产品PASS。
