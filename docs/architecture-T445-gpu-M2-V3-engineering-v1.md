# T445 · GPU M2 V3 工程闭包与资源布局

2026-11-09，architect，P5。不使用真机。消费 T440 §10、设计 manifest、T233/T238 字段及 M1 实际核。冻结 `T445_M2_ENGINEERING_WIRE_V3`，字段、mask、pass 和四档地址见 [机器可读工程 manifest](analysis-T445-gpu-M2-wire-v3.json)。本稿为独立模块的实现合同，实际设备能力和时延仍待验证。

## 模块与入口

新增 `gpu_resident/m2/` 的 layout 生成、source/quantum shader、owner/transport、Run 和 scene reader。首版用独立构建目标与显式 M2 冷启动选项，只有新空 run 可用。由单一集成 owner 将 M2 variant 接到已有 GL owner、短控制接受和页面能力分派。可共享只读整数原语及时间/控制协议，复制或显式实例化冻结的 M1 shader 基础。T434 的四个 M1 文件、签包、前缀路由和默认 M1 reader 保持原版本。构建接线放在独立生成目录或 M2 目标，公共 CMake、app bridge 和页面接线串行落地。

features=7、wire/layout/algorithm/source/scene=3、policyId=1，严格核完整布局/组合 shader/角表 SHA。其余 numeric/time/angle/medium/motion/water/optics 取设计 manifest 原值。配置只接受四档 K192/512 × Kw64/128、单 H、固定 package/lightMax122333、mergedMax244666、Bmax=K 和 radiusQ65536。半径 0/CQ 仅给离线诊断输入。旧 snapshot/旧 reader/未知 policy 返回不支持，禁止当作任意 merge。全部显示由 XComponent+shader，错误报告 FAULT 或完整旧帧标 STALE。

## 逐字段与互斥读写

所有 SSBO word 为 little-endian U32，U64 低 word 在前，SoA 以 capacity 为 stride。manifest 给出 header/completion/publication、沙/水/列、source32/terminal32/dedup64、来源账和两种事件的完整索引。保留位为零。token 和冷档的 schema 版本独立于物理 numeric revision。

| 对象 | V3 写入及消费门 |
|---|---|
| state header256 | 132–144 为 policy/package/lightMax/radius/Bmax、最近 SOURCE merge/birth/stop/stride/行数、mutationSerial U64 和逻辑 arena。145–255 为零。QUANTUM 逐字保持最近 SOURCE 摘要。冷空 run 的 sourceRecordWords=128，其余摘要计数/arena/serial 为0 |
| completion256 | 160–172 投影上述字段，173–255 为零。SOURCE 的旧 event64 范围51–54全零，独立 SOURCE128 描述符为168–172所列语义，QUANTUM 仍消费原 event64 范围。接受须核 SOURCE stage97=8191，QUANTUM=255，audit99=1023，hi=0及原 replay/GL 门 |
| command32/terminal32/dedup64 | command.word1=3、原 binary64 三对字原样。terminal保留原布局，word1按 V3 stop/reason，word31 bit2为 M2、bit0/1为沙水耗尽。已存 terminal 的 replayFlag=0，dedup重放只改响应为1，不覆写账本 |
| sourceEvent16 来源账 | 前14字原布局，14/15 是 mergeCount/stopCode。原 deliveryMode 从同源终态读取，或对水由 birthCount=0 的 DIRECT、1 的 FLIGHT 交叉核。源账仍为64B每条，只按接受 header[34] 的有效前缀读 |
| SOURCE128 | 每条512B，magic、rowSeal、used-mask按 T440；kind15/16/17/18=MERGE/BIRTH/DIRECT/END。行1-based ordinal U64、存储 row0 起，四个16word槽快照，96–127保留0。MERGE的q是 oldB.mass，P及实际准入不变；BIRTH/DIRECT 写准入差；END只投影本 root 总准入 |
| QUANTUM64 | 仍256B，word1=3/rootKind=2，继承14种已核 mask，QUANTUM只允许3–14。1/2 为原 SOURCE mask 的字典保留，不能接受为 V3 QUANTUM 行 |
| publication | sand16/water20/column16/header256 布局不扩，header版本/特性升3/7。GPU pack 和 audit 从同一候选核每字段、有效密集前缀、尾零、float列约束和全部身份。以组合 shader/layout SHA 间接绑定 policy，reserved 不私占 |

SOURCE128 的 arena 取逻辑目标0/1（frame.lo & 1），实际写 scratch2。尾包回显逻辑目标，scratch2 只允许失败冷诊断，不得作为 accepted header/lease。正常接受在已有短事务内转移 scratch/目标 GPU 句柄、bank、ledger prefix、receipt 和 publication 资格。正常路径不 CPU 拷贝整段。未接受写入只在候选及旧 prefix 之外的尾部，旧有效行只读。重用 arena 前核旧冷 freeze/读者及 fence 已退休，满则背压。QUANTUM64 的双 arena 和 scratch 独立。只要 SOURCE→QUANTUM 同帧发生，仍从空的 V3 QUANTUM64 前缀起步，不把 SOURCE128 的有效行计进 event64；随后的 QUANTUM 仅延伸同帧已有64word前缀。由 rootKind、nonce、frame、arena generation 和 stride 定位归档，不以 header 的历史 sourceArenaIndex 推定读权限。

## SOURCE 写集合、身份和重放

1. BASE、已接受的来源账与 dedup 前缀从 Begin 到裁决保持不可变。先核完整身份、冷 profile、base 库存与接受 ledger，再按完整命令查 dedup。原 key+原 payload 回原终态且 delta=0。冲突和满账正常拒绝，不追加不存在的账位。新 key 有合法 terminal credit 时，正常拒绝可以追加一个 rejected dedup 行。无物理成功时列/池/预算/I/source/physics commits 逐字不变，允许规定的元数据与发布代推进。存在不可能的 prefix 关系、预留异常或 event credit 不足为 fatal。
2. SOURCE 循环每轮按 target→列容量→Bmax→ID→free/NO_PAIR 的固定顺序。free 优先，最低槽。全满才按完整 G/y/unsigned id/slot 排列选择最小 pair。先核合并所释放的槽 generation+1、至少两条事件及最终 END 信用，再运行同一 workgroup merge→birth。单写者先写 row body，再封行并推进有效 cursor。所有 lane 按统一分支经过 barrier。mutationSerial 对每个 MERGE 和 BIRTH checked+1，水包 BIRTH 也计，DIRECT/END 不计。SUCCESS/PARTIAL 真正追加一个来源事件，一次提交 I 与 source/physics 序号。正常 stop 保留且只保留已配成的 MERGE→BIRTH 前缀。fatal 后已封行仅供诊断，该 root 的全部候选一并丢弃。
3. 用已接受源序号 n 构造唯一 pending origin=n+1，值来自本次 source 提议。BASE/普通 QUANTUM 仍只准 origin 1..n。SOURCE candidate/spare 在 finalize 前另核 pending 真实命令 sourceKind/sourceId、已出生连续 ID 界、实际 admittedSoFar 和当前候选 nonce。若本 root 新生包后来合并并退役，原 birth 区间和 birthCount 保留。聚合 liveMass 时在 pending 单独加总，禁止先读未封的 sourceEvent16 尾部；finalize 写出该条后，再用同一数据核已封候选。没有成功准入则不存在可接受 pending origin。
4. 沙合并仅保同一来源组，保 a 槽、ID、generation、origin，重判介质并更新索引。b 清零只保 generation。合并前后列 P、W/J、床、I、预算和提交号相同，x/y/vx/vy 每分量按 T440 一次宽整数 RNE。出生更新槽、真实列P/J或W及派生、live/free、ID和源计数，其他 writer 保旧域。合并后的单沙槽质量可达244666，旧 M1 每 ID 原包质量上限替换为来源区间和 U64 每源活沙总量 ≤ 其实际准入。水原 birth 界保留。SOURCE preflight/final audit、spare 最终及 QUANTUM 的入口/末端/重放域统一核来源、ID、capacity/Hq、预算、U64 全相和保留域。
5. 重放从 BASE 重建 spare 和独立索引，候选热索引内容先清再复用。逐行以 spare 重新算命令准入、最小 pair/NO_PAIR、最低 free、宽 RNE、快照、库存和 prefix。MERGE 后必须紧接配对 BIRTH。END 唯一且在末尾，验证停止顺序和真实 delta。由原命令重算完整 dedup/terminal/sourceEvent16 尾部，逐字比 candidate；不能只比较 hash 或把 candidate 尾部作为 oracle。失败时重放首差保 word/expected/actual 与原阶段。

## Pass 与资源

| Pass | binding0 | binding1 | binding2 | binding3 | 写集合 |
|---|---|---|---|---|---|
| 0 | BASE R | CAND RW | sourceWORK RW | SOURCE128 RW | 复制 state，清本 root 控制/有效窗口，保历史 ledger/dedup |
| 1–3 | BASE R | CAND R | sourceWORK RW | SOURCE128 保留 | 独立 preflight、索引初始化与 one-WG 排序。只记录预检结果与索引 |
| 4–5 | BASE R | CAND RW | sourceWORK RW | SOURCE128 RW | 一组持久 kernel、按有效行封签、终态和来源尾部、candidate 全域审计 |
| 6–8 | BASE R | CAND R | sourceWORK RW | SOURCE128 R | spare 初始化、独立索引与 replay，重算预期尾部 |
| 9 | BASE R | CAND R | sourceWORK RW | SOURCE128 R | 全域逐 word、确定性首差与审核结果 |
| 10–11 | BASE R | CAND R | sourceWORK RW | publication RW/R | pack 后独立 audit，同 root/代际，未接受不可 Acquire |
| 12 | BASE R | CAND R | sourceWORK R | completion W | 最终低位 stage bits 和审核结果，seal 最后一写 |

按 manifest 的 13 个 pass 提交，group-size 64。0/2/6 可按不相交域多组覆盖。3/4/7/8 恰一个 group。9–12 首版只派一组，用单确定写者处理最终归约、pack/audit 与 seal，避免依赖跨组同步。8 审计期重复使用同一个 4096×U64 scratch，不能与未完成的原子累计并存。1/5 已用一组。写后按消费者用相应 SSBO、buffer-update、vertex 等 barrier。每 SOURCE 或 QUANTUM 只一个尾 fence 和成功 map1024B。保留 V2 安全点、非续期 root deadline、真实33,333,333ns GAP、120Hz整数配额及发布 lease/release。GL 错或晚到尾只退休，不接受或复用未退休的槽。

SOURCE WORK 的旧 command/terminal/status/dedup/spare/ledger 地址照基线；尾加 2×pad 索引、64word 控制、8192word 来源和64word冷配置。控制区前32word为 root 运行状态和首错元数据，后32word为独立 replay terminal。逐字生成事件期望值时就地比较，避免另造无界事件副本。**QUANTUM WORK 为独立 GL 对象**，其 spare 后 replayFrozen 保持原位，再独立追加8192word 来源累计区。这样 binding2 始终单一 WORK，避开跨 WORK 别名。T440 模板只列 SOURCE 扩展，本稿为 QUANTUM 增加32768B。manifest 中范围为半开 word 区间，实际传 GL 以 checked 字节转换，不把逻辑段当未对齐的 range base。

| K/Kw | SOURCE128 单 arena B | sourceWORK B | quantumWORK B | 常驻 payload B | 每计划最大 dispatch |
|---|---:|---:|---:|---:|---:|
| 192/64 | 197120 | 1372032 | 580176 | 35210960 | 465 |
| 192/128 | 197120 | 1376128 | 585040 | 38388176 | 521 |
| 512/64 | 524800 | 1394560 | 602960 | 84019152 | 501 |
| 512/128 | 524800 | 1398656 | 607824 | 87196368 | 557 |

常驻为原 T433 isolation payload + 3×(2K+1)×512 + 8×pad + 33280 + 32768，仍保旧资源，无跨模块同时跑两套 owner 的常态假设。若冷两批则另有整 M1 event scratch；旧新窗口并存、双FBO至少8wh、实际 alignment、driver/JIT、CPU冷档和回收峰值另账。冷探测核实际 context 的 SSBO bindings/compute blocks≥4、fragment block≥1、workgroup invocations/size.x≥64、shared≥2048B，逐对象 GL_MAX_SHADER_STORAGE_BLOCK_SIZE 与 GLsizeiptr、range alignment、四档峰值同时分配、compile/link 和完整释放。最大单 SSBO 包含原 QUANTUM arena，不能只核512B stride 的新对象。任一能力缺失报具体 unsupported/fault，不降池、跳 audit 或拆成 CPU 每 pair 同步。单组长核受设备 watchdog、绝对期限及真实 UI pulse 约束，实际 pass/flush/fence/Swap、offered/admitted/NO_PAIR、模拟速度和新内容fps须由设备计量。

## 首错与读者

按 T440 次序裁决。身份/版本/基态在前，去重在新输入前，随后输入/几何、控制与事件 credit、正常停止门、操作、final audit、replay、pub、seal。正常停止码按设计0–9，输入拒绝 terminal.reason=0x10000|旧输入理由，END.stop=6。普通零准入用 metadata reject，完成成功或部分成功的 completion.reason=0，DEDUP_REPLAY completion.reason=0 并返回原 terminal。fatal reason23事件格式、24操作/配对/END顺序、25 mutationSerial、26逐 word 前缀差异；沿原码处理库存/容量、generation、event满和GL故障。SOURCE firstErrorStage=passId+1，行前 ordinal=0，行内为该行1-based，尾阶段=sealedRows+1。每检查族以确定性 slot/word 序给首错，后续失败只作附证。QUANTUM保持其原首错切点。completion fatal 不可触发 bank/lease/账接受。

更新 native owner 的 raw 256word 接受、header mirror、token 和 V3 scene 路由。核 root 完整身份、permit 和尾封签后短锁内接受，GL 调用仍在锁外。renderer 核 publication V3 配置、容量/slot/generation、root/scene SHA 后沿同 XComponent 直接读取 GPU pub，核实际 Swap 前后权限。accepted root token 使用独立 V3 标识并绑定 policy、stride、sourceRows、mutationSerial、scope/nonce/完整哈希。scene token 同绑定根身份，不能相互代替。冷 trace 升 V3，独立导出 SOURCE128/QUANTUM64 的 scope、stride、arena generation、root 和有效行，检查 bytes=rows×stride×4 与 hash。旧 V2 collector/checker 对未知 revision 拒判，新 V3 reader 也拒把 SOURCE128 当 QUANTUM64。历史 V2 原档继续按原 reader。

## 交付

developer 不使用真机。按机器 manifest 生成独立 C++/GLSL/reader 常量并做编译期范围断言，冻结实际 shader/布局 SHA。实现 SOURCE 完整 13 pass、V3 QUANTUM 审核、短接受、冷 trace 和 scene lease，覆盖四档真实离线 GLES、正常/部分/NO_PAIR、来源待接受、末 ID、污染/回滚、旧代与错 stride。任何超预算拆 pass 先回 architect 修订机器账。提供复跑、编译、签包验签和设备能力探针入口。T446 的独立反例按其原卡推进，真实生产差分等可复跑实现，设备验证等签包后串行。两者为独立证据，不作为开发完成前置。

执行者完成当前范围和角色检查，局部受阻先做其余，失败回原卡写具体原错、已试和前置。不得修改后续工作清单或滴答同名清单，本人行状态为唯一例外。共享树不回退他人改动，CPU 仅复用既有 oracle。

## 2026-11-10 创建诊断及独立证据修订

T461设备Create错误6首次归因与有界诊断实施以 [T462](architecture-T462-gpu-m2-create-diagnostics-v1.md) 为准；不能把该共用码直接定为组合kernel编译失败。T453/T459独立覆盖边界、断言勘误及最小补证见 [T465](architecture-T465-independent-evidence-routing-v1.md)，不更改本稿wire/算法或既有开发交付状态。
