# T238：GPU 驻留沙水 M1 工程协议 V1

2026-10-28，architect；原卡 `6ae1fbafe4b0c1a6bd64bfba`，目标6/P5。消费 T237 `a069962` 算法、T233 `f41de57` 逐 word 工程和 T231 平台边界。本稿与 `analysis-T238-gpu-water-M1-engineering-manifest-v1.json` 冻结新空 run 的 **`GRS_Q20_WATER_M1_V1`**；状态 `ENGINEERING_READY / CAPABILITY_REQUIRED / PRODUCT_NOT_IMPLEMENTED`。不使用真机、不改业务代码、生产默认路由或在途 T236 实施。所有背景/前景/沙/水只走新 XComponent+shader；故障为 FAULT 或显式 STALE 的完整上一有效新管线帧，禁止 Canvas 上屏/回退。

## 1. 新版本、接口和支持边界

M1 是 M0 的独立扩展，不覆盖 M0 wire、reader、shader 和已存证据。新 run feature bitmap `0b0011`：单 H 沙、真实在途水包 J、落水入 W、head2 邻流、AIR/WET 沙、床抬水面、同 token 水体与水包。M2 pair/merge、M3 双沙/U/渗流等配置位必须在任何库存写前拒绝。M1 初态空且 `bh=0`，非零基岩/分层初态为 `UNSUPPORTED_INITIAL_STATE`；旧 F64、M0 或 T226 旧 run/snapshot 不导入。新时间积分是 T16 隐式 drag/梯形位置，故连干沙轨迹也不保证与 M0 或旧 F64 逐位相同。质量守恒、非负、容量、严格 Hq605394、预算、ID/来源、确定顺序、root 原子性不因新 policy 降门。

M1 public API 只通过 resident opaque handle 扩展：`configureM1(profile,angleTable,optics)` 冷原子建全资源和新 run；`submitSource(command32)` 仅入 FIFO 返回 PENDING；`scheduleTick(rawWindow,sensorToken)` 仅控制排程；`copyCompletion(rootToken,out256word)` 仅封存结果；`leasePublication(expectedToken)` 返回 GPU lease，不返回 pool；`freezeTrace/capture` 才能冷导出整事件/state。每入口校 run/owner/context/surface/config/geometry/numeric/layout/shader/optics epoch 和 shape。CPU 只做协议、时钟、FIFO、GL 生命周期与有界 completion 接受，绝不逐包读回、CPU 算水头/湿沙/角或每包全 sort/SHA。源成功、quantum commit、真实 CURRENT 分账。

## 2. 新布局逐 word、对齐与生命期

机器 manifest 是权威索引表：所有 `words[index]` 的字节偏移 `4*index`，little-endian U32，U64 low word 在前，signed Q20 二补数，binary64 输入保持原始双 word。`std430` 用 U32 数组，不把 C++ struct/GLSL vec3 直接 memcpy 当 wire。M1 继承 T233 state/header/completion/event 的未覆盖字段，新 revision 下保留字段写 0、读 0；M0 reader 必须拒 M1 revision。每个 range 的 `align_up(base,GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT)`、size、`GLsizeiptr` 乘加与最后有效 word 都冷 checked；设备未实测，真实 alignment 与 padding 为 `CAPABILITY_REQUIRED`，不能用 payload bytes 冒分配量。每个 buffer 对象/段记录 owner、context epoch、first/last stage、lease/fence/quarantine、rawGL；冷配置必须原子建完，失败全退休。

| revision | word 变更与固定意义 |
| --- | --- |
| `GRS_M1_STATE_LAYOUT_V2` water16 | T233 water12 原 0–11 不变；12=`sourceId`，13–14=`birthQuantumLo/Hi`，15=`flags`。三 bank 每水槽64B；free 退役清质量/ID/来源/位置/速度/birth，保留 generation，下一 birth checked +1。 |
| state header256 V2 | T233 0–111 不变；112–113 settledTotal U64，114–115 flightTotal U64，116–117 arrivalSerial U64，118–119 quantumSerial U64，120–121 frozenMediumQuantum U64，122 motionRev，123 Tticks=65536，124 waterRev，125 opticsRev，126/127 sand/water ID exhausted，128–131 water/medium/arrival/waterEdge 行数；132–255 仍全 0。原 waterMass 表示 W+J。 |
| source32/terminal32 V2 | command 保持 32word，word1 为新 revision；kind 0 SAND_H、1 WATER。终态 0–30 沿 M0，但 waterBefore/After 包含 W+J，firstId/birthCount/nextId 按 kind 解释；word31 由 M0 reserved 升为 `terminalFlags`，bit0/1 为 sand/water ID exhausted，余位 0。dedup64=完整命令32+终态32，D4096，不驱逐。 |
| event64 V2 | 原 0–55 word 字段继承；56–63 依次 `tauBefore,tauAfter,mediumBefore,mediumAfter,etaFrom,etaTo,planeTag,eventAux`。`eventAux` 运动为 segmentTicks，其它为 0；flags 的 state/medium/wait/namespace/TURN mask 见 manifest。每 kind 精确 used-mask；不用槽必须 0，不能拿 event hash 代旧新库存。 |
| completion256 V2 | T233 0–137 继承；138–145 settled/flight 前后各 U64，146–149 arrivalSerial 前后，150–153 quantumSerial/frozenEpoch，154–159 waterRows/mediumRows/arrivals/waterEdges/deliveryMode/acceptedWaterId；160–255 仍 0。每 root 1024B，最多 G1+F4=5 尾 fence/5120B。 |
| publication water20 | **全新**20word，不沿 T233 publication water16 的可视化字段次序：0–15 精确复制 M1 water16 state 字段，16/17=`xF32Bits/yF32Bits`，18=`packedVisualTag`，19=0。退役水不进入有效绘制前缀。 |
| publication column16 | `[S,W,eta,J,P,bh,mh,R,Sf32,etaF32,Wf32,Rf32,wet,columnIndex,slopeF32,reserved0]`。最后一槽0；`wet` 仅 W>0，J 既不抬 eta 也不造光学深度。双 publication 槽各有完整 T233 pub header 身份并升 scene revision。 |

source event16、sand16、source command 原始 payload word 仍按 T233 指数读取；revision/feature 决定语义。source 终态和 state `waterIdExhausted` 防 MAX_ID 后回绕，MAX_ID 本身可分配；slot generation 再增会溢出则 root 失败。state enum FREE0/AIR1/CONTACT_WAIT2/SIDE_WAIT3，wait reason NONE0/BED1/SIDE_CAPACITY2/SIDE_HIGH_BED3/SIDE_BUDGET4/WALL5/SURFACE6，sand 的介质 bit AIR/WET 与沙种不同。按 slot generation+namespace+ID+sourceCommit 验事件来源；`sourceId` 只是外部源号，不能代成功 commit 身份。

`WORK` 采用显式逻辑段表、物理 offset 在能力查询后按 alignment 上取整；motion32*K、双 sand index2K、scan(K+Kw+N)、edge16N、audit4N、water motion32Kw、双 water ID index2Kw、双 arrival index2Kw、frozen medium4N、extra audit4N，各为 U32 word 数，冷 config128word、optics 双16word 各独立记录。水边 proposal 与床边 proposal 可在**水边完全应用并冻结所需 head/medium 后**复用 edge 段；frozen medium 保留至沙运动与 replay 所需位置，不能和 audit 同时 alias。两排序 index 可复用同一逻辑 allocator，但水 ID 与 arrival 索引在 replay 前若仍需复现，不能覆盖未消费数据。manifest 的资源模板按独立段求和，任何实际 alias 省字节必须另 revision 和独立证明，不从账上提前扣。

## 3. source、quantum 与事件回放的写集

每 source root `G≤1/tick`：完整 command32 比较 dedup，GPU 解 binary64、Q20 floor，正量化为0拒绝；water 触点 `y<S` 拒，`S≤y≤eta` 走 DIRECT_W，`y>eta` 走 FLIGHT_J、占一个最低 free water slot，v=(0,−vEmitQ)，J 当前列加 q；sand 走 M0 free scan。两类均核 `free=Cq−S−W−P−J`，source 原始请求(0,256]、I_kind、成功序号、ID/slot、append prefix、事件容量。water q 不拆多包；slot/full/ID/容量拒量真实记账。q>0 才一次增加 sourceCommit/physicsCommit/I_kind；DIRECT_W 不造虚拟水包，FLIGHT_J 当次 W 不增加。正常元数据拒绝只在全部权威库存逐 word 等 base 且 dedup 终态正确时接受；fatal 丢整个 candidate。source slab 有效 prefix 由被接受 bank 的 header 决定，未接受行不可见。water 的 birthQuantum 绑定当时已成功 quantumSerial。

quantum 从 current GPU copy 入 candidate，先入口 audit；接受角/Hq/固体预算后，两色 head2 水边各 propose→prefix event→apply，随后冻结 etaMedium/bed2/head2/wetMask；GPU sandId、waterId 排序。water 逐 ID 空气运动只改 J 所属列/包，触面 CONTACT_WAIT 尚不加 W；sand 逐 ID 在 AIR/WET 真实分段，床接触 LAND 使 P−q/S+q、W 不变、eta 抬 q；床三色守 M0 guard 与固定 total/local grant，receiver free 始终含 W/J；末 sand contact/fold。water 再按 `(column,y,id)` 升序排序并一次 ARRIVE，J−m/W+m，水总量和 R 不变，低包抬面后可使高包同轮到达。新 W 与 LAND 抬面只在**下一 quantum**进入 head2/介质冻结，不在本 quantum 偷做二次水流。末全列 P/J/eta/R、phase U64、角 guard/预算/来源/事件上界审计，统一事件 replay 到 spare，逐 word 比 candidate，然后 pack pub/completion。M1 入口和末尾活水包 `vy≤0`，封存末活水包 `y>eta`，否则 fatal；不能把包永久 WAIT 藏掉不变量。

head2 与 bed2 用 signed 宽整数：`2*(S+W)−(2c+1)*u` / `2*S−(2c+1)*u`，水边五界 `q=min(floor(kappaW*delta2/240),floor(delta2/4),Wdonor,max(0,floor((head2Donor−max(bed2Donor,bed2Receiver))/2)),Cq−Sreceiver−Wreceiver−Preceiver−Jreceiver)`。入口非法库存先 FAIL，不能靠 max0 抹掉负库存。相等 head 不随机选向；两色起点随成功 quantum 奇偶轮换，同色无冲突、edge 升序分配事件 ordinal。水不扣沙 total/local budget，源 I_water 不因边流/ARRIVE 增加。冻结 medium 用真实 W，J 不当面水。

T16 `T=65536` time ticks/quantum、`D=120*T=7864320`。AIR/水包 `v1=RNE(D*v0/(D+kAir*r))+RNE(a*r/D)`，WET `v1=RNE((D*v0+kWet*r*vT)/(D+kWet*r))`，位置 `p1=p0+RNE((v0+v1)*r/(2D))`；每分量 signed 宽乘、除和溢出核，不靠 float/CPU exp。若预测速度换号，先按最早零点≤16轮整数二分切 TURN，再形成无反向弦。几何 `λ` 用有理交叉乘，tie 床→墙→列→水面→TURN，事件耗时 `ceil(rSeg*λNum/λDen)`，正时为1..rSeg，lambda0 仅可真实拓扑变化；重复同 plane/无进展 `TIME_NO_PROGRESS` fatal，不能塞 epsilon。CONTACT_WAIT/墙/高床/预算/容量约束以同一事件的 `tauBefore→T` 消费剩余 ticks，记 `eventAux=rSeg` 和 λ，不把 WAIT 全当自由飞行；下一 quantum 重新评估。沙事件上限48/slot/quantum，水16/slot/quantum，含末行；超过 cap 整 root 回滚，不丢行或余时。source/quantum 成功前缀不因后续 root 失败撤销。

event64 一个 root 一个 global ordinal 流，flags bit18–19 的 namespace SAND0/WATER1/FIELD2/SOURCE3；kind 1–14、按 kind used-word mask/stock A/B 映射在 manifest。field 行 slot/id/source=0，不能撞真实 ID0；所有未用 word 精确0。原 `sourceCommitLo/Hi` 是成功来源，不等 `sourceId`。parallel water/bed edge 先 prefix 确定唯一 ordinal，resolver 单 writer，重放严格相同顺序、old/new stock/位置/速度/介质/τ/λ/来源/guard/容量/预算逐项核；冻结面要从重放到水边末的 state 派生，不能读后来变动的 candidate。replay 是产品自检，独立 WM oracle 另实现。失败尝试占 frame event credit 并封存首错；双 frame 保留上帧与本帧，冻结占满背压，常规不下载大 event，完整长期 trace 走显式 credit/IO 模式。

| root / 阶段 | 仅 candidate 可写的域 | 接受或失败时的边界 |
| --- | --- | --- |
| SOURCE SAND_H/DIRECT_W/FLIGHT_J | 对应 sand/water slot、列 bh/mh/P/W/J 与派生、独立 sand/water ID/generation、I_kind/成功序号/commit、source dedup/event 有效 prefix、event cursor、pub/completion | GPU 审计/重放后尾 fence，native 接受才使 prefix 与账可见；fatal 留 base；元数据拒绝须全库存逐 word 同 base。 |
| QUANTUM head→motion→bed→arrival | 两 pool 全状态/列场与 frozen medium、角/固体预算、arrivalSerial/quantumSerial/physicsCommit、事件 cursor、pub/completion | 任何 WATER_EDGE、LAND、ARRIVE、TURN/WAIT 或 replay 错都丢整 quantum candidate；前面已接受 source/quantum 不撤销。 |
| 诊断与场景 | spare replay bank、未 leased publication、status/completion、冷 capture | spare 不作 authority；candidate pub 接受前不可显示；capture IO 故障只标不完整，不撤销已接受物理。 |

## 4. 四 binding、stage 和资源上界

同 T233 固定 binding0=READ_BASE、1=WRITE_CANDIDATE、2=WORK、3=EVENT_OR_RESULT，但**每 pass 重新绑定**。source dedup 查表占 binding3 时，event 写入另 pass；water/bed proposal 若只需当前值和输出计划，可分 read/write 两段；replay 的 base/candidate/spare/event 四域正好占满，结果写 completion 另 pass；pack 读 candidate、写未 leased pub、status 与 completion 四域。跨 dispatch 按 consumer 用 SSBO/vertex/image/texture/buffer update barrier，尾 fence+1024B 异步 map；workgroup barrier 不跨组。任何 pass 超四域或 range 不对齐要**显式拆 pass并修订 dispatch 上界**，不能把新增 pass 藏在原 stage 计数里。actual pass/object/range/offset/barrier/rawGL、CPU submit/fence/map 与真实 render swap 成本进入 ledger。

| pass 类 | 0 READ_BASE | 1 WRITE_CANDIDATE | 2 WORK | 3 EVENT_OR_RESULT | 核心数据依赖 |
| --- | --- | --- | --- | --- | --- |
| source admission / free scan | current bank 对应段 | candidate 对应段 | command/scan/索引分段 | dedup 有效 prefix；event flush 另 pass | 原请求 full32 比较、最低 free slot/ID；stock 只写 candidate |
| water edge 两色 | candidate 列只读段 | candidate 列写目标段 | edge proposal/prefix | event64 追加 range | propose 全读→有序 prefix→apply，跨色 SSBO barrier |
| freeze / pool motion | candidate 列/slot 只读段 | candidate slot/列目标段 | frozen medium、motion/ID index | event64 追加 range | waterId→sandId，τ/λ/medium 由单 ID writer；冻结面不读后变场 |
| bed / arrival | candidate 当前列/pool | candidate 列与退役 slot | edge 或 arrival index | event64 追加 range | 床三色后按 `(column,y,id)` 到达，退役字段与 W/J 同行 |
| audit / replay | base 分段 | candidate 分段 | spare 分段 | sealed event64 range | 各分段逐 word 比较；result 写回另 pass，不能边比较边吞首错 |
| pub / completion | candidate 已审段 | 未 leased pub 段 | status/派生/optics | completion256 slot | GPU seal 后尾 fence；host 接受前 pub 不可见 |

M1 保留 M0 的 23 个非排序 quantum stage（proposal/resolver 升版），加水边两色6、freeze medium1、水 ID init/proposal/resolver3、arrival init/resolver2、水末 fold1，共13；另 `L(K)=h(h+1)/2` 沙排序与两套 `L(Kw)` 水排序。`Dquantum=36+L(K)+2L(Kw)`，source `16+2ceil(log2 Kpad)`；K192/512×Kw64 是114/123 quantum、488/526 tick，Kw128 是128/137 quantum、**544/582 tick**。这是假设 packed WORK 下无额外拆分的**设计 stage 包络**；实际 shader/pass ledger 若超过须升版并报告真实值，不宣原上界已达成。`Eframe=4*(48K+16Kw+2(N−1))+K`，每 event64 256B，包含一帧至多G1+F4和受控失败尝试；首 fatal 停该 frame 后续 root、保留故障行，不能在同 arena 无限重试。

T237 模板经 manifest 对 T233 基线逐项复算：N128/Q16/D4096；Kw64 K192/512 payload **23,342,736/55,111,056B**，单 event block **10,795,008/26,605,568B**；推荐 Kw128 K192/512 payload **25,471,888/57,240,208B**，单 event block **11,843,584/27,654,144B**。基线 Kw64 只用于差额比较，不表示 M0 reader/layout 已变。大头是双 event arena，不能为通过设备而删日志/审计/replay。实际 GL allocation、alignment padding、program/staging、双全帧目标至少 `8*w*h`、旧 LastGood/lease 并存、driver/RSS、CPU oracle/capture、T16 算术与完整画面时延未测；profile 不足 `MEMORY_PROFILE_UNSUPPORTED`，不得静默降 Kw 或合并 offered 请求。Kw64 对约100Hz/0.9秒首落样本可能提前满池；Kw128 只是候选，不保证任意频率无拒绝。

## 5. 同 context 场景、设备增量与接力

首选唯一 XComponent window GL owner/context：候选 GPU pack pub 后尾 fence，native 核完整身份/epoch/revision/首错/GL/tombstone 才一次接受 `bank+revision+receipt` 并 lease 同 token pub。water20 的 x/y F32 软件 RNE 打包，column16 的 S/W/eta/R 可视化只读已审整数；F32 跨原列越界退一个可表示值，不能回写质量。scene **`GRS_GPU_SCENE_Q20_M1_V2`** 的背景、床、活动沙、水体/水面、真实在途水 glyph、前景统一读接受 pub；ARRIVE 同 token 中水包退场、W+水面增长，不能重复显示同一水量。W0透明，depth=`clamp(eta−max(y,S),0,W)`，线性 premult body 依 optics16 的光学函数，面积高光/断槽不能跨干槽搭桥；shader exp/pow 只做光学不反写物理，旧 T226 F64 parser/CPU snapshot 上传不冒驻留。真实完整 swap 才 CURRENT；pub 两槽 lease+release，满则背压/兼容旧完整 STALE；context lost 为 OWNER_STATE_LOST，M1 不从 completion 摘要恢复整 pool。跨 context 仅在 T232 实际证明共享与双向 fence 后另启；不共享的 headless 物理不计同帧目标7/8通过。

T232 现有真实 context 探针增量需核 fragment SSBO block≥1 与整数 profile fetch；Kw64/128 新最大 event 块和完整 25/57MB+render 并存、offset alignment/peak/错误码、T16 宽算术/除法/二分、M1 event fixture、compute→shader scene、lease/resize/context loss、disjoint GPU timer。若 fragment fetch 不可行，另版 GPU image/texture pack 加真实 bytes/dispatch/能力探针；不做全 pool CPU 下载回传。`CAPABILITY_REQUIRED` 不挡离线工程，真机结果未到不写设备 PASS。失败也计完整 dispatch/barrier/fence/map/quarantine/STALE/capture 成本；对照 old F64 行为、独立整数 oracle、GPU M1 同 offered/accepted 两分母，未测时延/收益为 NOT_MEASURED。

**T240 developer 接力 prompt**：不使用真机；消费 T237、T233、T238 manifest 和 T236 实际 M0 接口，新增 `gpu_resident/m1/` 或独立版本模块，实现 water16/header/completion/event V2、双 kind source、J/W/head2、T16 AIR/WET/TURN/WAIT、两套水排序/ARRIVE、全审计/replay、双 pub water20/column16/optics 与同 token scene。只开新空 run/feature0b11，M0 reader/路径和生产默认保持；不先做 CPU-only 效果、不以旧 trace 冒新 shader identity。**单个平台集成 owner** 串行负责 `p013renderer/egl_context.*`、`scene_compositor.*`、`napi_bridge.cpp`、CMake 等与 T236 共用 hunk；若 T236 未交，先独立模块/fixture，集成等接口到件。不改 `docs/后续工作清单.md`，唯自己行状态有限例外；不改滴答同名清单；共享修改不回退。全部背景/前景仅新 XComponent+shader，故障 FAULT/完整有效 STALE，禁止 Canvas 回退。developer 负责构建和非真机自测，独立 WM 与真机/观感由 tester/effector 另卡。

独立 WM01–WM12 可执行测试范围：WM01 双源/量化/满池/ID/重复；WM02 T16 落水/ARRIVE；WM03 head2 五界/左右扩散；WM04 AIR/WET/TURN/介质转换；WM05 LAND 抬面与含 J 容量；WM06 `(column,y,id)` 到达顺序；WM07 48/16 事件/τλ/WAIT 上界；WM08 namespace used-mask/翻位重放/U64 总量；WM09 root 前缀/停源/迟到/context loss/lease；WM10 同 token 水体/水包/完整 CURRENT 或 STALE；WM11 四档真实分配、dispatch、热 UI 与失败成本；WM12 M0/M1/旧 F64 隔离及 unsupported。tester 独立 Python 整数/Fraction，不复用产品 primitive 或 T237 设计模型为唯一 oracle；T239 独立规格可先行，产品未实现预测试 `NOT_IMPLEMENTED`，不作 PASS。effector 真机连续录像看先沙后水、真实落水/扩散/湿沙/落床抬面、停源 J 仍落、故障 STALE，同 token/库存证据配套；设备用机串行。测试失败先 architect 分流，manager 登记交付推进依赖，不增验收门。

本稿是离线工程协议，未实现 shader、未编译产品、未运行真机，亦未证明收益或目标7/8产品效果。
