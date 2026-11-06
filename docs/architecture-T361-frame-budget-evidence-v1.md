# T361 · 帧率无收益首次归因与预算补取工程 v1

**2026-11-06 续裁见 §6。** T363 已追回预算原件。设备慢窗主导段定位到 CPU 粒子 `hotApply`，§1 的待归因状态及 §3–4 的取档条件分支以本轮证据更新。

2026-11-05；architect；不使用真机。读 T352 原报告、三份 phase 日志和 layout0，T351 delivery/签包台账、FrameBudgetV2、页面和 DiagnosticColdFiles。本文未重跑设备、未实现产品。CPU 主线优先；T353/T359/T360 功能冻结与本性能案独立。

## 1. 裁定

T352 的低速现象保留为 **PERFORMANCE_FAIL**，所测包空场约12fps、后段约1.3fps，33.3ms目标未达。现有数据只能支持吞吐低、未见明显终点改善，不能支持“主导阶段必在native/pack”或严格统计意义的零收益。报告“严格语义PASS”仅收窄为已记录帧CURRENT/request==actual、零所记拒绝；它不证明所有物理位型/错误路径及沉降通过。

首次分流为 **采样口径及证据取用工程缺口**。阶段预算尚未取回，诊断ON/OFF在实际页面不可切换；取档按钮没有hilog本属设计，不是预算不可测。先按§3追回已有私有文件；如无文件，按§4补齐可取证工程后一次有界补采。不能在无新证据/工具下原包盲重跑，也不能删audit、改dt或猜主导阶段。**目前未完成项是设备主导阶段排序及诊断开销实测**，非等待设备本身；本人行看评论，交manager落实tester补取/必要developer出口修订，再回architect分流。

## 2. 分母、包与当前资产

已实核两个本地HAP：T352所测 `T351-isolated-final-signed.hap` SHA `18399de86843f21780a9cf113ea086e656fca85f084bc4c7e1a47b96f68c976c`；T351最终delivery隔离包 SHA `70aec640eef4e0347748c7114d5a50b668c484b3e8a1d1ad38c2ed3b26c0edd4`。二者不同，不能把T352对前包的FAIL冒称后包测过；最终delivery补了子步身份，后续导出必须按JSON自带schema解析。`t351-project`为可重建工作目录，不是旧包源逐字归档证明，旧包与具体源码哈希关系须以其生成台账/构建产物补齐，不能靠目录名保证。

`P013SceneController`按callback参数timeNs约每秒才记一条scene日志。相邻记录可算 `(日志wall差)/(frame差)` 的**区间平均帧周期**；其中含日志传输/输出调度误差。它不是每个callback原始间隔分布。T352的p50/p95/max应改名“所选scene区间平均周期的分位数”，并给筛选边界；逐callback分位数只能由budget row原始interval或等价原始callback收据计算。真实fps可按同run稳定窗口frame差/wall差估计，不能用日志条数/秒。

本轮只离线解析保存资产，未执行产品测试：

输入SHA256：phase0_empty=`febbea89746d42ee7f1b318bd4b6733e1917b0cb30e2c15df6a4fddc2f7ff113`；phase2_touch=`02a1e2d971ea6deb09842eeeba4daba0f7dd9500c87fdcc3229e18ba260b203b`；phase3_budget=`bf992554a08d183d3057285a0979c7ded23feb7946a6781731309b8bc225914e`。解析只取ASCII的`WS_EVENT p013_scene` JSON和前置时分秒毫秒；文件其它系统行有非UTF8字节，解码replacement不参与数值计算。P95采用有序区间均值索引floor(.95*(n-1))，不是逐帧p95。

| 文件/同run `p011-run-1793882179550-3` | 首末scene | wall差 | frame差 / commit差 | 可支持的口径 |
|---|---|---:|---:|---|
| phase0_empty | 20:36:19.787 frame1/commit4 →20:37:37.864 frame941/commit1884 | 78.077s | 940 / 1880 | 12.039fps；78个区间平均周期中位83.50ms、P95约83.67ms、max约84.83ms（日志ms精度） |
| phase2_touch整个保存文件 | 同一首点 →20:39:49.411 frame1402/commit2815 | 209.624s | 1401 / 2811 | **包含空场前缀**，不能全文件直接命名“触摸段”；全窗6.683fps不作为触摸稳态 |
| phase3_budget整个保存文件 | 同一首点 →20:41:10.773 frame1508/commit3027 | 290.986s | 1507 / 3023 | 又含前两段前缀，不是新增235帧或独立run；须去重 |

phase2末段frame1394→1402在20:39:43.265→49.411，8帧/6.146s≈1.302fps，低速窄证可复核。空场940帧即使每帧dt都等于cap，也最多31.333模拟秒；报告“38.6模拟秒/91墙钟”与此保存窗口不一致。12fps×91s×1/30约36.4s，亦不能直接变38.6s。不能擅选另一个窗口补凑；tester须登记原计算脚本、精确首末点和原dt/Q。

触摸期部分相邻记录的commit增量大于2×frame增量（如20:37:56.909→58.004等），符合外源发布也增commit；“commit/frame恒2”不能概括含注入的全窗。即使纯STEP每帧两个子步，也仅是Q的线索，不能从commit数×1/60无条件重建elapsed：必须核每次实际substepDt、成功/休眠/异常和外源A。182条CURRENT在文件中不是182个展示帧，不能把节流行当帧总数。日志并列保留，不修改原件。

## 3. 先补取已有文件，禁止重建旧档

真实页面 `aboutToAppear`直接开启`model.budget.enabled=true`并接renderer；按钮调用 `budget.exportCold()`→`new DiagnosticColdFiles(context.filesDir).save('v2-budget-'+Date.now()+'.json',text)`。save循环write、fsync、close后计算待写UTF8字节SHA256，返回{name,bytes,sha256}；页面只短暂显示name，下一正常tick会覆盖status。**没有自动manifest文件，也没有把路径/bytes/hash持久化或输出hilog**。原实现计算的是输入字节hash，不是重新read-back哈希；传输后应独立哈希验证，不能假称现存manifest已保存。layout0仅证明“导出帧预算”按钮存在（bounds[444,547][836,687]），不是点击/成功写档证据；当前资产未有点击后layout/返回文件名/冷文件。

tester最小补取任务：先保留当前安装包身份、应用数据和原run环境，**不卸载、不清数据、不再点导出覆盖旧运行身份**；通过现有已授权的应用私有文件调试/沙箱导出能力只读列出`context.filesDir`下`v2-budget-*.json`，记录实际目录、权限、文件名/size/mtime及设备原错误。`context.filesDir`是运行时路径，不能把猜测的`/data/storage/el2/base/files`当shell必然可读路径。已验证沙箱映射/权限后再用设备工具file recv导出；若工具支持run-as或IDE应用文件浏览，先核实际支持，不能在本稿伪称设备已验证某命令。权限失败保留原错误，不反复猜shell路径。

收到原文件后本地独立算SHA256、UTF8字节数、JSON schema/capacity/rows/runs/tokens/overwritten/invalid/open与覆盖首末callback；构造旁置manifest绑定设备、HAP SHA、实际来源路径、采集/取回时间、file SHA、覆盖窗口、trace enabled，不回填为旧包内既存manifest。环只保最近128帧，约12fps时10.7s、约1.3fps时98.5s；是估算覆盖时长，须由row interval实核。多导出文件先按run/epoch/callback serial去重，不重复计样。incomplete由覆写或invalid等多原因触发，覆写说明全run缺失但不自动否定合法最近窗；非法栈/倒钟/缺parent子步另标不能归因。

若文件存在且含有效阶段，可以直接回architect分析，不必重新采集。若未存档或没有私有目录访问能力，tester报告`NO_BUDGET_FILE`或原始权限错误，附已尝试方式；不凭mtime造回先前scene窗、不把现在新导出命名旧run完整证据。沉降读数另复用T359切点接口，不恢复T347每120帧全池热导出。

## 4. 必要的出口工程修订（给developer）

若§3无法取档，最小工程卡仅拥有 `P051DerivedV2Demo`诊断控件、`DiagnosticColdFiles`冷出口及FrameBudget冷metadata适配；与T359 page hunk串行交接，不回退物理或预算计量。新增显式诊断开/关和状态读取（run/epoch、ON/OFF、sampleCount），**只在帧边界且budget无open时切换**；使用待切标志，下一callback入口应用。不能帧中关enabled导致栈未闭合；保留独立callback/frame serial单调递增，导出明确mode/segment与切换点，OFF段不伪造阶段行。OFF callback间隔可由同一个最小单调标量记录器或独立设备追踪获取，明确其自身成本；否则不能比较ON/OFF逐帧分布。

冷导出固定格式manifest包含HAP/source manifest标识（由构建注入）、文件名、实际filesDir路径、bytes、SHA、run/epoch/最近serial范围、coverage、enabled、overwritten/invalid/clock精度、导出请求/完成wall时间。只记录必要路径/诊断数据。通过现有应用可访问文件导出通道提供原JSON和manifest；优先复用已验证私有文件工具，无权限时新增显式用户文件导出/分享冷操作。不能把私有path字符串显示当作文件已可取回。UI保存状态独立于运行状态，不被下个tick吞掉；忙时禁重复保存，失败保留具体阶段/原错误，不只`BUDGET_EXPORT_FAILED`。文件采用唯一nonce和临时写→fsync→成功提交，manifest最后写；导出异步完成时标原run，reset不能换标。为公共DiagnosticColdFiles加新API时保留旧save签名和其它调用者语义。

最近128帧既有计量先复用；记录成功子步真实dt/elapsed需要读取本次WaterSandField结果及T359合法切点，不在导出时重新模拟。导出本身是冷操作，exportCold同步快照之后的fsync/JSON不混入steady窗口，下一callback rawInterval可能含导出停顿，必须标`COLD_EXPORT_ADJACENT`排除并单列。一次有界空场/注入/UP各窗，按128帧实际覆盖能力及时导出；不要求性能失败包重跑全套物理。开发仅编译/非机检查开关边界、导出失败/短写/重入/reset身份、hash/bytes与取用通道；独立tester再执行设备补取，非开发关闭门。

## 5. 收到有效预算后的唯一分流规则

JSON字段以文件自身schema为准；当前T351 V1 frame row[1]是rawCallbackIntervalNs、[2]是请求physicsDt、[7/8/9]是成功Q/A及递交R、[19]是callback内wallMs、[24:44]互斥stage总和、[64:84]调用数，run/token有平行数组，子步ring容量256。Date.now整数ms不可称ns阶段精度；原callback参数是number ns，两者独立。核互斥和与wall闭合、无invalid，再按frame同窗统计，不能求各段p95之和当整帧p95。callback interval包含前一callback执行及调度，不能机械用同row interval减wall解释等待；连续serial时将interval_i与wall_(i-1)配对，剩余仍含调度/外部UI工作/时间域差，需验证时钟及anchor，不称GPU空闲。

| 证据主导 | architect后续路由 |
|---|---|
| publicVerify/exact70/structure/hotApply合计主导 | 按N/live/R/Q/attempt拆累计工作；工程消除可证明重复，若严格证明下界冲突则明确不变量转designer，不能盲跳audit |
| pack/sceneSetup主导 | 给实际pack函数/次数/资产配置变化，派渲染准备工程；不以PC替身耗时替代设备 |
| nativeDraw API wall主导 | 仅定位NAPI→native返回这段（含锁、拷贝、GL提交/swap/驱动等待），需再细分这些段，不能宣GPU shader唯一原因；异步GPU时间未测 |
| callback wall小但interval大 | 核时间基准、回调调度、UI/日志/导出/后台及显示节拍；给有限外部追踪接点，不改physics dt追帧 |
| enabled/invalid/coverage不合格 | 返回精确字段与文件缺口，不新猜产品瓶颈 |

按参考33.3ms、真实逐callback/互斥段p50/p95及window吞吐报告ON/OFF/负载，列native GPU时长NOT_MEASURED。T352 FAIL仍保留；本稿不是修复收益证明。所有渲染前背景只走XComponent+shader，失败FAULT或完整旧token显式STALE，不能为性能删严格域、物理事务、守恒或endpoint合同。

## 6. 2026-11-06 续裁：CPU hotApply 主导，转粒子事务工程

消费 T363 正式报告、原卡回执及 `test/reports/t360/v2-budget-t352-era.json`。原文件58488 B，SHA256实核 `f320e6a6bfd4f7f22d14a3ab572075cdc39b00f830f4ee0d7eae569c537f0ee3`。T363 由 `v2-budget-1793882460817.json` 的生成时点、沙箱来源和同 run/commit/frameToken 绑定至 T352 所测 `18399de8…` 包，最终交付 `70aec640…` 是另一个包。本文件自身没有嵌入 HAP SHA。以下归因绑定 T352 所测包的这段存档，最终交付包的实测仍待独立取证。

### 6.1 分母、身份、互斥闭合

按文件自身 `T351_EXCLUSIVE_FRAME_BUDGET_V1` 与 fields/stages/offsets 解析。无子步数组。`enabled=true, open=false, invalid=0, overwritten=1368, incomplete=true`，最近128条 callback serial1369–1496 连续，均 run `p011-run-1793882179550-3`/epoch2，commit2747–3001。首末 token 的 scene serial1368/1495 与 callback serial 相差1，关联键用完整 token，不能混用两个序号。`outcome=0` 的128行覆盖稳定慢窗，`N=512/live=101/Q=2/A=0` 全窗恒定，commit差均2。128条中121条 R=606，余534–597。R 是递交 hot apply 的请求行总数（含预览尝试），无逐行种类/获准结果。六次 hotApply、十八次 collect 和六次 hot begin 尝试每帧；`rollbacks=14` 来自 derived counter11，记录内部恢复，不能当作14次失败物理提交。

各帧 `[24:44]` 互斥和与 `wallMs` 精确闭合128/128，invalidSegments=0。按原始 callback ns 转毫秒，P50/P95/max=767.742/768.993/784.432ms，callback wall=759/763/767ms。P50 取中位数，P95取升序 `ceil(.95*n)-1`。对127个连续相邻帧比较 `interval[i]-wall[i-1]`，剩余中位8.038ms、P95 12.014ms、范围1.755–17.432ms，`sum(prevWall)/sum(interval)=98.93%`。配对仅作两时钟观测下的回调占用估计，剩余含调度、回调外工作及量化。约1.30fps原慢窗成立。T352较宽的触摸段含更快阶段，660.5ms 的节流区间统计仍采用§2修正，不能以这128帧为其整个分母，空场也不在原件覆盖内。原请求dt=1/30，成功Q=2；缺该版真实子步回执，继续分开请求dt、Q与实测elapsed。

| 独占段 | 每帧 p50 / p95 ms | 128帧累计 ms | 累计份额（分母96,896ms） |
|---|---:|---:|---:|
| **hotApply** | **634 / 637** | **80,796** | **83.38%** |
| exact70 | 37 / 38 | 4,787 | 4.94% |
| collect | 30 / 31 | 3,794 | 3.92% |
| field | 18 / 20 | 2,381 | 2.46% |
| preview 自身 | 11 / 13 | 1,376 | 1.42% |
| publicVerify 自身 + structure/exact70/strictDomain | 按同帧各列求和 | 6,313 | 6.52%（包含上表 exact70，勿重复相加） |
| authorityCompare | 8 / 9 | 1,066 | 1.10% |
| pack | 5 / 5 | 620 | 0.64% |
| nativeDraw API wall | 2 / 3 | 297 | 0.31% |

`hotApply` 单次独占最大值的每帧中位约111ms。去掉该段后同帧剩余 wall 中位仍126ms，仅表示现有账的剩余量，不能预测真实优化后的帧时间。nativeDraw 仅 NAPI/提交返回 wall，异步 GPU 耗时 `NOT_MEASURED`。包内预算全 ON，**ON/OFF 开销仍不可测**。T363 的“MEASURABLE”适用于 ON 阶段分账，不能代替成对的开销对照。原档已成功追回，本次归因不以重做导出/manifest 或重跑旧包为前置。

### 6.2 源码对应和工程归因

`P011ParticleHot.ets::applyParticlePlanInto` 进入预算 stage6 后，覆盖计划校验、逐行 `sink.applyInto` 及 motion 尾部 transfer/audit。`P011ParticleM2Sink.ets` 的 row 内有 reserve/guard、`preparePatchHot`、`publishPatchHot`、`SINK_ROW` 审计、budget/event 和 `finishPatchHot`。现行 prepare 仍读 authority→`patchBefore`→`patchScratch`，编辑后复制到 `patchAfter`，publish 再写 authority，lease checks 与实际 buffer 检查每行执行。`patchAfter` 也用于最终投影/证据与原事务 trace，简单别名化会改变写入和冻结证据。T331 已有 SINK_ROW 增量证书、K=16重验、回退及 audit ID 索引，因此不得按旧 T311 的“每行全池审计”假设制定修法。具体子段份额尚待测，证据只定位到 `hotApply` 及其工作放大。

预算中 authority 字节对拍76,800 B/帧，fill+consumer复制117,248 B/帧。Derived counter19（`copyPoolHot` 与 `copyPoolDiagnosticHot` 的成功数组 payload 计数）中位75,942,912 B/帧，按 N512 的30,208 B一次折合2514个等量 payload，包含全回调发生的复制而非 stage6 独有。counter9 中位108,977,888属于派生结构复制计数，和其它复制可能同一调用发生，不求和冒实际内存带宽。相同R不等于相同DEPOSIT/CROSSING或触列，不能仅凭606=101×6就定为接触阻塞或 T353 的冻结机理。

T351 公共边界/authority 比较改动范围相对该实测工作很小，能够解释为何只改该处难以在这一慢窗看到明显端到端收益。缺同条件旧包分段和 ON/OFF，无法估算实际节省多少毫秒，也不能外推为空场主因。首次归因裁定为 `CPU_HOT_APPLY_DOMINANT_OBSERVED_WINDOW`，T352 的性能FAIL保留。

### 6.3 接力合同

1. **developer 工程先行**。拥有 `P011ParticleHot.ets`、`P011ParticleM2Sink.ets` 和 `P011ParticleField.ets::validateHotTransfer/transferHotState` 的受控热路，以及已有 T311/T351 冷收据适配。与 T359/T364 共有接点按角色串行交接。先复用 `P011HotWorkReceipt` 和 T331 计数，在诊断变体按帧/物理 substep/attempt 划分 preview 与 authoritative。输出 request kind、尝试/重试、full/incremental/fallback 审计、检查次数、array payload bytes、actual/exact70 derived copy bytes、event/lineage、guard及各子段时间。至少拆 plan identity、prepare/lease 校验、typed array transfer/派生 copy、row audit、预算/事件及 apply tail。现有 nested inclusive 计时保留标签，只有互斥子段可相加。只按样本 attempt 开细计时，固定环冷导出，单列诊断成本，禁止每行日志/字符串/全池导出。
2. **先消可证明重复的校验与传输**。候选首件为 `validateHotTransfer` 的缓冲互异检查预绑定：按 workspace/attempt 的受控固定13缓冲组建立经完整 preflight 的白名单。每次依原错误顺序检 patch busy、shape、actual 对象/byteLength/绑定代及租约身份，等价才省重复交叉比较，未注册、detach、替换、别名、callback可变或阶段身份异常走原完整检查与原错误。不可缓存行数据合法性或成功结论。若拷贝仍主导，限定私有 staged scratch 的所有权，在被下一审计覆写前核源/目的与使用者，按实际读写闭包决定可合并的复制；`patchBefore`回滚、`patchAfter`冷投影/证据和 tuple/exact70 冻结位型必须保持。不能证明者保留完整原路。若要将每请求全池物理快照改为局部 journal，需要独立证明所有 writer、merge/allocator、严格故障优先级和回滚闭包，先交 architect 定不变量，再交 designer。T331的K、全验与fallback、每个public exact70/严格域、dt、物理通量/事件次序、守恒与事务仍作为现有合同。
3. **验证与继续分流**。非机回归选同 N=512、近L=101、A=0、Q=2、R约534–606 的高接触/跨列夹具，再含空场、低L、源入、部分/全额LAND及预览回滚。验证状态、pY/pVy、exact70、events、首错、备份恢复与 public token 同位型；同步核T311/T331/T351计数及copy工作界，不能以host耗时宣真机通过。developer完成编译/签包和本角色检查后，由独立tester用同版本包在同设备采空场、注入、停源高负载三短窗，阶段账有效且身份完整。只有需要定诊断成本时另用帧边界ON/OFF并采同分母，沿§4最小入口复用；生产通过仍需真实逐callback预算，不新增manager门。若 hotApply 已明显下降而剩余仍远超33.3ms，按新文件的主导项继续分流，保留原失败证据。

T361 本轮归因与工程合同已交付，可按“结束待验收”登记。诊断 ON/OFF、空场阶段分账及最终包性能属于明确的后续验证。T353停源功能继续独立，前景/背景仍由 XComponent+shader，失败沿 FAULT 或完整旧 token STALE。
