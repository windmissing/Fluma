# T183：GPU 高 merge 原始口径修订与下一步工程

日期：2026-10-27。角色：architect；原卡 `6ae0b709e4b0f11bdf1e2210`。消费 T173 `1ac0b58`、T171 `961d837`、T172 正式报告及 T169 工程合同。本轮不使用真机、不重跑测试、不实现业务代码。仅解析已存 JSONL、包和编译产物；逐项复算值见 `docs/analysis-T183-existing-evidence-v1.json`。本稿是 T169 的后续修订；原报告/raw 保留不覆写。

## 1. 裁定与路由

1. 原报告六个路线中位数与 **33 次含预热**口径吻合，不能命名稳态；p95 也不符合 T169 nearest-rank。按显式 warmup=false 重算如下。结论仍为：这台设备、这两个 synthetic 高 merge fixture 下，GPU 两档均未胜 LEGACY_SCAN；512 档胜 T123。不是普适 GPU 无收益，不授权生产迁移。
2. `ed26af35…` 确为 `test/reports/t165/source-manifest.json` 的旧身份。T173 报告/archive 的此字段是登记错误；当前 V3 manifest 可复算为 `fac20656…`，设备 native 回显和签包 SO 均含该值，不能因旧登记直接判实际运行旧 V2。
3. 找到可实施的**首项工程实验**：实际四个 native 编译单元全部 `-O0 -g`，而 prepare 内 hash/pack 是较大的已测子区间。先做隔离 target 的严格 `-O2` 对照，仍保留 SHA/冻结/精确范围/消费校验。尚无优化后收益，不承诺能弥补完整事务差额。
4. 四类注错中的拒绝/同进程 reset 接力有原始设备证据；owner 耗尽后**新 OS 进程恢复为 NOT_RUN**。正式目录实际只归档五份 JSONL；另外五份 fault 文件仍在 `.codex/local-test`。这是报告/归档及证据范围问题，交 tester 精确补齐，不判产品 FAIL、不要求重跑已有采样。
5. 接力分为离线归档勘误、开发严格优化构建、独立验证、串行设备收益复测；见 §7。manager 按执行交付登记/派工，不加验收门。GPU 目标6继续最高调度优先，目标7不借此插队。

## 2. 99 次采样的正确统计口径

两个 perf 文件各为 COMMAND + 99 ATTEMPT + 99 ARCHIVE_TIMING + READY/CLEANUP/COMPLETE。各 9 次预热（每路3）与90次稳态（30个完整三路 block）；六种排列各5次，两个 fixture 使用相同排列序列。全部99次状态 TRANSACTION_RETURNED，无 error；warmup 与稳态完成率均100%，失败率0。只说明这些存档运行，不推外部故障率。

p50：偶数样本中间两值平均；p95：升序 `ceil(.95*n)` 第值，稳态 n=30 对应第29值；max 照留，不删异常值。单位 ms，分母是完整物理 attempt。

| fixture / route | 稳态 n | p50 | p95 | min–max |
|---|---:|---:|---:|---:|
| WET_192_64 / GPU | 30 | 1750.5 | 1792 | 1316–1807 |
| WET_192_64 / T123 | 30 | 1727.5 | 1773 | 1202–1784 |
| WET_192_64 / LEGACY_SCAN | 30 | 1428.5 | 1456 | 991–1473 |
| WET_512_128 / GPU | 30 | 4852.5 | 4888 | 4823–4900 |
| WET_512_128 / T123 | 30 | 5918.5 | 5952 | 5572–6005 |
| WET_512_128 / LEGACY_SCAN | 30 | 3959.5 | 3984 | 3930–3984 |

逐 block 以 GPU 减参考路线，不能拿两路中位数之差冒配对中位数：

| fixture / 参考 | 差值 p50 | p95 | min–max | GPU更慢/更快/相等 |
|---|---:|---:|---:|---:|
| 192 / T123 | +21.5 | +113 | −38…+114 | 24/6/0 |
| 192 / LEGACY_SCAN | +324 | +368 | +282…+383 | 30/0/0 |
| 512 / T123 | −1068 | −998 | −1122…−716 | 0/30/0 |
| 512 / LEGACY_SCAN | +898 | +944 | +855…+955 | 30/0/0 |

min–max 是本批观察范围，不是置信区间。一个 Worker/context 内的 blocks 存在顺序/温度/频率相关，不能把1920/3840请求当独立收益实验扩大样本量或宣统计显著。192档稳态最早块较快仍保留，不事后重新定义 warmup。六种交错顺序有证，不等于排除了漂移。

首个冷 route 的 physical（GPU/T123/LEGACY_SCAN）分别为192档927/1249/1026、512档3214/4040/2723；GPU另有冷 setup27/24、field构造14/17，均不含在 physical。冷样本不是稳态样本。性能 requests 原报告2112/4224含预热；稳态应为 **1920/3840**，每 attempt64/128次真实 merge，三路共同 audit134、pages66、λ2、rollback0、overflow=false。

## 3. 身份链与原始状态范围

### 3.1 source、包、SO 三层分开

以下 SHA256 完整值均收入分析 JSON；本表保留完整关键值供检索：

| 层级 | 实际值 |
|---|---|
| V3 sourceManifestSha256 | `fac20656756675b6fb1b7a2ec12a4d0ddb3721777a3e414ff6e186d863195006` |
| unsigned HAP / 当前 buildManifest 参数 | `0092232bfbfda83565f2318a050b7b9e10b053573c940b392746b77e620b1191` |
| signed HAP | `ebd7cdfe269c72469314a231bb5fff59c7a9fd5608fbeb8b0e2365100bfdd4c2` |
| obj 未剥离 SO，1593216 B | `0860560faf076141d8af691c5b5e62d0eeb22d2e392bff55cf38758d72404663` |
| signed/unsigned 包内 SO，360208 B | `4a13fd52cf12afb257a1eb07d3499d5175a5d5c57e97051444c650185806486d` |

V3 source 重算式是 SHA256(UTF8(JSON.stringify({revision:'T171_SOURCE_V3', v2Source:嵌套V2.sourceManifestSha256, inputs, edits, fixtures})))，顺序按生成器；不是 source-manifest 文件自身 SHA。设备 JSON config/nativeSourceBuildSha256、generated BuildIdentity/CMake 宏一致指向 fac20656。包内三 shader 与四 fixture 字节 SHA 分别与原 manifest 一致，signed/unsigned 中这八项内容逐字节相同。

两个 SO 总 SHA 不同不能直接当错包：ELF `.text/.rodata/.data/.dynsym/.note.gnu.build-id` 的长度及 SHA 均相同，符合剥离调试段后的同一链接产物，不能把 obj SHA 填成已安装包内 SO SHA。此处是当前本地产物关联证据；不新增“重装且读回设备内SO”的事实。下次归档需分别列 obj/packaged SHA 和剥离/打包步骤，保留 install/launch 退出码及设备 process/run 关联。

### 3.2 poolSha 不等于全部物理状态

正确性原始 `after.poolRawBytes` 是13段 hex，按固定数组顺序各自 hex解码后**字节拼接**做 SHA256，三路分别一致：

- WET_192_64：`0ee7cf0ba5664c2625a37965e0ea47f4bd5778fd268c522fc5dd592784919a6e`。
- WET_512_128：`13f4cf38b4694027d5cb7c4185255c315c8e41ff821ccad3d651359f5fd128c3`。

它覆盖 mass/x/y/vx/vy、phase/source/state、id/sourceId/freeSlots、colMassH/L；不是 JSON文本SHA，不包含所有标量、场/源账/预算、commit或事务事件。短16字符仅关联摘要，不能替代64字符SHA或精确比较。

对本批 correctness 三路另外逐字段比对 after：只标准化两个准许身份位置 `source.lastRecord.runId` 与 `snapshot.externalSourceState.admissions.lastRecord.runId`，其余字段（含完整snapshot、预算raw、source/drift、total/local、commit）全部相同。common 比较同时保留数字与原F64字节，只清 pairStride32 槽4/5（mutation身份）、12（selector wall）、30/31（workspace owner/run）及 pageStride12 槽0/1（owner/run）；对应 rawHex 同槽也清零。余下 pair/event/refund/page原F64、lineage、计数全部相同。直接对整份 after/common 字符串 SHA 比较会因这些身份/时钟不同假报失败，禁止删整块 source/snapshot 以求通过。

性能臂99次 normalized after均相同，三路common计数与FNV摘要一致；fullDump=false 的 pair/event/page/refund raw 缺失，所以**性能臂逐pair原F64未重新证明**。FNV32明确仅关联，不能当身份SHA/碰撞免疫等价证据。正确性臂与 T172 独立参考才提供各自范围的补充，不把 stand-in 当设备执行。

## 4. 计时嵌套与成本判断

以 `T171Native.ets` / `pair_binding_v1.cpp` / `pair_prepare_v1.cpp` 的实际边界为准：

| nativeTimings 槽 | 范围与包含关系 |
|---|---|
| 0 | native入口 snapshot/identity 校验；不含ETS构建context |
| 1–5 | prepare validate、freeze、sort、range、hash/pack子区间 |
| 6 | 整个 prepare，包含1–5及边界间开销；不与其相加 |
| 7–10 | executor upload、dispatch/barriers、fence等待、map子区间 |
| 11 | executor request，包含7–10及其它executor工作；不是完整native调用 |
| 12 | verifyAndConsume；oracle ON时含独立旧扫描，性能OFF仍有权威消费校验 |
| 13 | reply()对象及常驻诊断字段组装 |
| 14 | return包装，包含13及额外prepare/slot/build/可选dump字段；非JSON归档总成本 |
| 15 | ETS bridge墙钟，包含native验证/prepare/request/consume/return；起点在context对象与身份字符串构造之后 |
| 16–31 | lo/hi身份及role/ordinal；绝非耗时 |

physicalAttempt 含所有 public seed/step、原CPU merge/λ/guard/账本/audit、热栈共同证据与NAPI来回；不含每attempt冷field构建、before导出、GPUconfigure、physical返回后的after/common编码与写盘。ARCHIVE_TIMING 是上一ATTEMPT行的 encoding / write+fsync，不能放到每request nativeReturnEncode名下。commandThroughCleanup又含队列/fixture/setup/归档/cleanup；UI收到报告仍为 NOT_YET_MEASURED。native steady_clock 与 ETS OHOS ACTIVE ms 分域，不直接相减。

稳态GPU请求每项均有限值，无 null；单位ms，以下仅各分项自身分布，**不同列不相加**：

| 分项 p50 / p95 | 192，n1920 | 512，n3840 |
|---|---:|---:|
| hash/pack (5) | 2.699453 / 2.757344 | 4.368594 / 4.452344 |
| prepare总 (6) | 3.120469 / 3.289532 | 5.372968 / 5.627657 |
| fence CPU等待 (9) | 1.3560155 / 1.646094 | 1.408203 / 1.634532 |
| executor request (11) | 1.585156 / 1.897031 | 1.652266 / 1.890469 |
| return encode (14) | 0.109062 / 0.130782 | 0.088437 / 0.109219 |
| ETS bridge (15) | 5 / 6 | 7 / 8 |

先逐attempt把其请求的同一槽求和，再对30个attempt取p50：bridge307/925.5，prepare187.630860/684.000935，hashPack159.706796/554.791488，request103.943823/215.071564。这些仍是嵌套分项，不叠加。全部稳态 bridge 实测累计9126/27801ms，占GPU physical累计51393/145740的17.8%/19.1%；prepare累计5542.518277/20572.386378中hashPack累计4722.812954/16685.358454，占85.2%/81.1%。这是定位优先级的依据，不是“仅此瓶颈”或“减去这些成本就能胜CPU”的反事实证明。bridge整数ms量化、未包context构造，不能把bridge减native各项命名纯NAPI成本。

micro原raw为162请求=18warm+144稳态，6格各24稳态、42次configure/reset。详见分析JSON；此为不同Nmax/族/K/h/context下的条件fence等待，非GPUtimer（NOT_MEASURED）、非完整事务收益，三点也不宣近线性。

## 5. 四类注错与日志的真实证据范围

五份现存 fault JSONL 的路径/bytes/SHA均列于分析JSON（formalArchive=false）。本轮只读，不替 tester 把未归档事实改成“已正式归档”。

| 类别 | 设备已见 | 不能扩大成的结论 |
|---|---|---|
| QUARANTINE_GL | 真正 TEST_ILLEGAL_UPLOAD_TARGET，rawGlError=1280且firstFailure同码、quarantined=true；第二请求BINDING_UNAVAILABLE；FAULT_RESET live0/resourcesRetired；新owner2/epoch2恢复64次merge | 没有真实fence TIMEOUT注入，不称超时恢复覆盖 |
| STALE_REQUEST_HI | owner lo1保持、hi0→1，请求入口 REQUEST_CONTEXT_OR_INPUT/INPUT_INVALID、consume NOT_RUN、dispatch0；reset后恢复 | 不是GPU完成后的迟到消费拒绝；未单列第二次原生直调不可用探针 |
| STALE_CONSUME_HI | 实际executor OK后独立current owner hi0→1，consume STALE/OWNER_CONSUME_STALE；第二请求不可用；reset新owner/epoch恢复 | 不混为wire破损PROTOCOL_ERROR，不宣覆盖所有身份字段 |
| OWNER_EXHAUSTION | 专用allocator PID46269，原CAS分配UINT64_MAX−1（hi4294967295/lo4294967294），64请求成功；同PID reset后configure OWNER_GENERATION_EXHAUSTED、owner0/live0，无READY | 文件至此COMPLETE；NEW_OS_PROCESS_REQUIRED_NOT_ENV_RESET只是标记。新PID恢复NOT_RUN，不能称整项恢复PASS |
| SHADER_SHA_MISMATCH | 私有build shader898B（原897），actual `bd3a7e3d…`与原expected不等；compile/link NOT_RUN、live0；改用original目录后3shader匹配并恢复64merge | enum execStatus=GL_ERROR但rawGL/EGL为0，不能宣真实GL拒编；只是read/hash早退 |

前三故障 attempt 共同计数 mergeCommitted0、rollback1；不可要求整份 before==after 来证明回滚，因为该attempt第一笔seed已经合法提交，失败发生在第二笔。要证明失败事务所有字段原样还原，应与**相同成功前缀、尚未开始失败事务**的独立状态/账本/事件比较；现文件含before/after与分页事件，但尚无该counterfactual逐字段正式比较结论。交tester先从原raw分页/现有前缀资料核；确实无法重建的字段列精确缺项，再补专用取证，不把rollback计数冒全状态证明。

正常READY/恢复3个program compile/link=OK，日志状态EMPTY，长度/返回字节0、truncated=false、采集GL错误0，source path/bytes/mtime/expected/actual完整。SHA早退第0项compile/link/log=NOT_RUN；其余项source未读、stat NOT_CAPTURED、missingReason=STAGE_NOT_REACHED。不能把这些未到达项填EMPTY。源码有4096预算、返回字节/NUL与CAPTURE_FAILED处理，T172有离线源码/替身证据；本批无真实拒编/link失败、截断或CAPTURE_FAILED设备样本，不把实现存在冒分支实跑。

## 6. 最小工程修改：严格优化构建，先不改算法

实际 `.cxx/default/default/debug/arm64-v8a/compile_commands.json` 四个pair_{binding,prepare,executor,owner}_v1.cpp均 `-O0 -g -fno-limit-debug-info -fno-fast-math -ffp-contract=off`，CMakeCache=Debug。包内SO与当前obj代码/只读数据/动态符号/build-id相同；因此有充分本地线索优先做编译配置实验，仍需下轮归档实际编译命令绑定新包，而非只改报告标签。

**H1，首轮只做这一项性能变量**：在隔离生成工程对 `p011_gpu_pair_napi` target增加显式测试variant `STRICT_O0` / `STRICT_O2`；STRICT_O2使最终有效选项为 `-O2 -fno-fast-math -ffp-contract=off`。不要把全工程切Release来混入ArkTS/宏/断言差异，不启fast-math、FMA contraction、Ofast、LTO或NDEBUG变化；原整数SHA轮函数、canonical LE V2、原F64谓词和消费代码不改。原O0仍可生成。保持shader/wire/fixture/步骤/ABI/注错编译宏一致，不关闭SHA、audit、oracle正确性臂或定容证据。generator白名单登记/逆向diff，生产entry不改。

构建variant是独立身份维度：记录工具链版本/target/全部实际compile与link flags、宏、obj/包内SO/build-id、unsigned/signedHAP/manifest SHA；configId和run输出明确variant，不能沿用fac20656作为优化构建的唯一身份证。variant配置/生成器编辑纳入新source manifest，buildManifest参数仍绑定实际unsigned产物，签包再独立登记。旧raw按旧身份保留，不能覆盖O0基线或把新计时追贴旧run。

**H2，仅追加测量**：拆 hashPack 内 snapshotDigest/payloadSha/canonicalIdentity编码+identitySHA/header组装的独立区间，在原32槽外新增版本化命名字段，不挤占身份槽、不改既有含义。单列context构造、native整体、reply常驻日志拷贝等范围。用同一scope内时间差，测量本身有开销，O0/O2均使用同一仪表版本另留原始基线；不能拿新增仪表臂直接归因所有变化。H1独立对照可先做，H2不设为必须等齐的开发前置。

**H3，待H1/H2再裁定**：缓存configure期不变UTF8/config身份片段或SHA块处理优化、减少常驻日志重复reply属性；需列明对象生命周期/字节等价与冷日志仍可追溯。不先改紧凑ABI或跨请求缓存池摘要。每mutation仍全freeze、原校验、sort/range与13buffer SHA重新构建，消耗时再验当前视图/身份；不能因mutation号相同就跳过内容SHA/pointer校验。任何范围算法、数据结构/选对顺序、异步多请求、batch融合、跨mutation复用、GPU权威merge改动须先交designer，未在本稿授权。

## 7. 可派工任务书（不在本会话建卡或分配新编号）

以下每卡prompt均须带：共享树不回退他人修改；不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人状态有限例外；不得改滴答同名清单；所有渲染含背景/前景仅新XComponent+shader，无Canvas上屏或失败回退，故障FAULT或显式STALE上一有效新管线帧。成功结束待验收；FAIL/BLOCKED/关键缺证看评论+原卡问题/证据/真实前置。manager不新增验收门。

### A. tester：现存归档与勘误，不使用真机，可立即并行

所有权为T173正式报告/归档补件，不改实现。按分析JSON核对五份fault原文件hash后正式归档（保留原bytes/run/PID，不能手改raw），manifest从实际十文件生成SHA/size，修source为fac20656及包内/obj SO分列；按§2重新输出warmup/稳态/nearest-rank与逐block差，旧值标历史错误。声明owner新PID恢复NOT_RUN、UIreceive未测及日志未执行分支；按§5用现有raw核失败事务成功前缀/rollback差异，缺失字段逐项列路径/为何不可复原。报告无法重建的安装/启动过程不可事后编造，可用既存日志关联。只做原始数据分析/正式存档，不重复设备采样；完成后回architect分流。

### B. developer：H1隔离strict-O2构建，不使用真机，可立即开工

所有权 `test/p013_compute` 隔离CMake/生成器、variant身份/manifest及相应开发说明；只做§6 H1、保留O0对照和V2/V3现有入口，schema追加明确版本。交付可复生成的两包、实际编译命令/SO对应与inverse diff；编译和离线自测为开发关闭边界，收益不作为关闭前置。不得改生产物理/渲染路由、不得真机。H2可独立小卡，不强绑H1。

### C. 独立tester：O0/O2等价与边界，不使用真机，等B交付

所有权独立checker/测试报告，不能镜像被测排序/hash实现。独立标准SHA256/任意分块/55、56、63、64、65字节和canonical字节参考，针对0/O2验证同SHA/wire与fullF64；四fixture三路完整状态/每pair与事件/refund/lineage保留；RN64半径边界、signed-zero/subnormal/tie、hi-word/同mutation异内容或换pointer、detached/overlap/尺寸拒绝、stale/result消费、owner耗尽/宏release不可见及overflow语义均不回退。审核实际最终有效-O2/严格FP选项、宏/断言与ABI相同，区分CPU stand-in/host doubles与设备。优化若未开发，登记NOT_IMPLEMENTED，不冒产品FAIL；失败先architect。

### D. 真机tester：严格优化收益与精确补证，等B+C，无实质前置后仅排串行设备

必须真机标签；明确设备窗口串行，不能因等机列blocked。不重新追测已交付旧采样来填历史值。新包先正确性ON/fullDump，过后性能仅oracle扫描/fullDump OFF，保留原SHA/冻结/权威消费/审计；同设备同输入192_64/512_128、每variant3warm/route+30三路block，原六排列各5；新O0/O2构建交错执行、分别保留queue/service/setup/physical/archive/cleanup及失败不剔除，报告逐block差和变体收益，热状态缺测明示。基线仪表同版，包/source/variant/flags/packaged SO/进程/run完整绑定；UI竞争另标NOT_RUN或按同新XComponent+shader管线实测，不以独立计算页冒生产UI收益。

owner仅补缺失的新OS进程恢复：记录旧PID耗尽终态→真实退出/新PID或独立OS进程启动→新owner/configure/request成功，别用同PID新env或重置专用allocator冒恢复。旧故障raw已确认部分无需再次通跑；若C或A识别真正缺少的事务前缀字段，按精确列表加受控单故障取证。真实拒编/link日志若需设备分支覆盖，另列专用测试构建，不能用SHA早退代替。完成后原卡据实报告并回architect；生产迁移仍另需既有完整收益/UI/资源合同，不能以本卡完成代替。

## 8. 本轮交付边界

核证与工程路由已交付；现有缺口不阻止B严格O2实验开工。为避免把owner恢复及正式归档缺项抹平，本任务回执明确列出未覆盖，按“关键证据不足”规则本人状态记**看评论**，manager可登记本轮architect工作完成并派A/B/C/D，无需等待manager验收或重跑本轮分析。尚无优化实现或新收益，不宣产品PASS。
