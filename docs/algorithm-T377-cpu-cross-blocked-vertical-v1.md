# T377 CPU 跨列受阻后的纵向推进

2026-11-06，designer。原卡 `6aed720be4b07013b16e75c9`，P3。路线 `cpu-cross-blocked-vertical-v1`，`DESIGN_READY_FOR_ARCHITECT`。适用对象为显式启用新运动修订的 CPU 原始 F64、派生 V2 干场热事务。依据 T353 §7、T369 两份冷档与当前 Hot/M2 实现。GPU compute 主线优先，本稿作为 CPU 参考合同。不使用真机，产品实现与验证另卡。

## 证据和入口

T353 把首处分歧定位在权威 `collect → apply`。T369 两个实际保留的采样窗中，ID 86/87/88 的预测纵向有变化，零获准 T4 之后的池、bank 与 slot 仍为旧位型。两个端点间约 21.1667 成功物理秒。环缓冲覆盖了中间记录，结论限于实际保留的两个窗。T350 的旧窗缺同 run 原档，归因范围沿 T353。冷档中的 BUDGET 分类尚不足以区分 local 和 total。

现行 `P011ParticleHot.collectParticlePlanIntoBody` 先计算干场半隐式步。冻结源床命中优先归 T3，剩余粒子才分为同列 motion 或跨列 T4。T4 的 `predictedY` 已按**目标列**床高抬升，直接拿它回写源列会制造虚假爬升。`P011ParticleM2Sink.applyInto` 先查目标容量，随后整包预算及退款。`editPoolHot` 的零 grant 分支当前只将 x 夹在源侧面内，遗漏 y/vy。T4 成功、已有同列 motion、正常 source、湿场、GPU 及历史 route 保持各自身份。

建议冷建 run 固定 `P011_CPU_DRY_CROSS_VERTICAL_V1`，绑定后端、派生 policy、介质、参数、计划与证据版本。旧 run/旧快照继续由旧行为读取，新 revision 下只允许明确匹配的冷建或同 revision 恢复。禁止将此行为差异标成纯性能等价。所有渲染仍使用 XComponent+shader，完整 swap 才报 CURRENT，故障按完整旧帧 STALE 或 FAULT 合同处理。

## 积分与源列约束

候选半隐式式子沿现有 dry F64 操作顺序。\(dt\) 是已接受子步时长，\(\theta\) 是该子步实际物理角。数值单位沿现行列坐标、材料高度与模拟秒。

```text
vxFree = vx0 * exp(-0.01*dt) + 9.8*sin(theta)*dt
vyFree = vy0 - 9.8*cos(theta)*dt
yFree  = min(C, y0 + vyFree*dt)
xFree  = existing_wall_and_adjacent_column_clamp(x0 + vxFree*dt, c, N)
```

只计算一个全子步候选。T4 行增加只读 `blockedYFree`，来源为目标床修正前的 yFree，原 `predictedY` 继续用于成功 T4。复用原 `predictedVy` 作为 vyFree。仅 T4 行有此候选，其他行预填规范 +0。字段进入定容 A/B plan、copy/stage、request identity、ticket 重复识别、projection、冻结首错与回执版本，拒绝缺字段、非有限值或篡改。负的 yFree 是触床候选的合法中间值，提交态仍须在 [0,C]。

冻结源床 S0 优先的 T3 分类照旧。对于已经排成 T4 的行，若最终 grant=m 且原校验全部通过，按旧成功路径把整包送到相邻目标。若最终 grant=0，且来源是有效目标的 CAPACITY 或已关闭票据的 BUDGET，执行源侧约束。任何非法输入、错误缓存、审计失败、异常票据、非相邻目标与未知零原因均走原错误及回滚。

```text
c = floor(x0)
xWall = target > c ? c+1-1e-9 : c+1e-9
assert floor(xWall) == c
Srow = (bh[c]+bl[c])+mh[c]+ml[c]    # 取与既有实际床面同运算序
hit = (min(y0,yFree) <= Srow)
if hit:
    state=2, x=xWall, y=min(C,Srow+contactEps), vx=+0, vy=+0
else:
    state=1, x=xWall, y=yFree, vx=+0, vy=vyFree
```

源侧 x 沿当前已使用的内偏移，横向入壁速度在本次被吸收。下个子步从 +0 重新接受重力及合法跨列检查，反向倾角可将粒子拉离壁面。对于受阻行，垂向按本离散模型的直线漂移 `y0 + vyFree*t` 在源列床面判碰。若起点已被床抬至接触，触床时刻取 0。否则下降且命中时，诊断可报告 `clamp((y0-Srow)/(-vyFree),0,dt)`。时间值仅供解释，不能改变分支或由它重新积分。接触后剩余时间为静止约束，已消费于该子步。向上时沿 yFree 的原顶盖限制，床触判定覆盖起点。若量化后 y 位型不变，分列 `F64_NO_POSITION_CHANGE` 或顶盖限制，不加最小下降量。未接触的下降用已有候选，避免目标列高床通过 `predictedY` 影响源侧运动。

### 发布前的床面复核

一次 T4 受阻行可受两类床变化影响。较早的 T3 已进入 `Srow`。后续的普通床面通道可能继续改变源列床高，因此子步末还须完成以下约束。确定序为原请求顺序，使用定容的本尝试接触清单，包含本轮受阻 T4 和本轮 T3 后仍有质量的 state2。每项固定绑定 slot/id/source/phase/mass/col、父行身份及行后位型。T4 行后的命中位随记录保留。本 revision 内所有仍持有余量的 T3 共享这条收尾规则，避免给 state2 增持久来源标记。

1. 在最后床面写入和 `updateSandTop` 后、最终 audit 和 `prepareInactive/recordEndpointCommit` 前读取实际 Sfinal。原 T3 保留余量在其行内也须以 `min(C,Safter+contactEps)` 落在容器内。验证 Srow/Sfinal 有限且在 [0,C]，继承床/占据审计。
2. 已是 state2，或原受阻 T4 的 `min(y0,yFree)<=Sfinal` 时，最终锚到 `min(C,Sfinal+contactEps)`，vy=+0。受阻 T4 的 vx 保持 +0。普通 T3 的原有 vx 口径沿用。早先命中后床回落仍保留 state2 并随最终床锚定。Sfinal 抬过起点的情况记 `CONTACT_AT_TAIL`，不反推连续碰撞时刻。
3. 其余受阻 T4 保持行后 yFree/vyFree。床面尚未碰到时保持 state1；每次成功提交有 `y>=Sfinal`。收尾只写被列出的粒子位置/速度/接触态，库存和预算无新写入。用外层原子写者进入派生 V2 与首错记录。禁止 M2 已结算之后直接从渲染或发布拷贝入口改池。若目标在尾写前发生合法重用/移除，当前候选的 v1 支持面不接受该组合。校验失败回滚，并提供确切的后续写者与身份。工程须核实 dry V2 这段期间的 writer 闭包。

该时序把接触和有质量转移的 LAND 分开。触床后最早在**下一个成功子步**的正常 T3 相位尝试落床。同一个子步若已经由冻结源床归类 T3，仍按正常 T3 处理，无 T4 行。选此策略使 `T3 按 ID → T4 按 ID → 同列 motion → 后续场通道` 和冻结分母都可保持。新触床会增加至多一个成功子步的等待，受预算/guard 持续限制时仍可长时间等待。回正、停源和冻结检测以同一已提交状态区分 AIR 下降、CONTACT_WAIT 和实际 LAND。

## T3 账本与 T4 退款

原列 state2 下一次 collect 按原 ID、原相、剩余质量和源列发一笔 T3，target=-1。按常规流程参与该子步分母，单端 local[c]+total，实际床面 guard，partial reserve 及 guard 拒绝按已有票据退款。对相应 mh/ml 加 grant，同相粒子库存减 grant，V2 exact70/cache 走已存在的确定重建与公共边界检查。正余量留原槽和 ID 并锚床，零 grant 保留全量 state2。仅整包全额准入时退役一次，来源账仍是原成功外源量。任何数值退化或容量不合法均按原审计处理，不产生负库存。原 F64 预算/守恒容差不变，模型中的分数闭合仅说明数学关系。

| 行结果 | 主请求、票据与计量 | 权威状态 |
|---|---|---|
| 目标容量不足 | 原 T4 request=m，capacityClipped=m，无票据，grant=0，remainder=m | 源列约束 |
| 预算 reserve=0 | 原 T4 request=m，budgetClipped=m，合法零票据按既有流程关闭，grant=0，remainder=m | 源列约束 |
| 预算 reserve=a，0<a<m | 先按原两个 local 端和 total 一次各 reserve a，再对同票退款 a 并 close(0)。原 T4 budgetClipped=m | 源列约束 |
| 预算 reserve=m | 原 T4 local 双端、total 单扣 m，T4 全额，不调用 T3 guard | 原成功跨列 |
| 后续 T3 | 新子步新票据，local[c]/total 单端，实际净扣 g，g 由预算与床面 guard 给出 | 原列部分/全额 LAND 或等待 |

退款在同一已知的成功票据中按原 F64 操作序走 adapter 和宿主两侧，保留原收据与误差检查。浮点扣减后加回未必位对称，普通零获准行只要求既有代数闭合/审计和可重现的位型。失败回滚须恢复尝试前的实际字节，不能以反向加减代替快照或日记恢复。bottom 预算无 T3/T4 扣减。零获准 T4 仍计入原比例分母，不重新分摊额度。T3 到下个成功子步才参与新分母，零 T4 的退款票据不抵扣 T3。

源侧约束的质量/相别/ID/来源、列归属和存活计数均原样。两个端的库存缓存和占据包络不因 y/vy 变化而变化，x 的 floor 仍为 c。正 mass T3 交换时，单列 S 与 P 相反量变化，体积约束和分相总账继续由原 F64/派生 V2 审计核定。若位置更新涉及池版本或位置字节比较，须走既有 mutation/journal 与 exact70 完整性钩子。可证明未改 bucket 时允许复用现有无库存变更路径，不能凭自报“不改”跳过需要的检查。

## 事件、确定性与回滚

保留 CROSS_COLUMN 主事件和原行序、requested/granted/remainder、CAPACITY/BUDGET、disposition 6、T4 原目标、预算票据与 `released=0`。主事件的目标是尝试目标，实际列仍为 c。扩展受阻回执以带 revision 的定容 companion 记录并关联 `owner/run/stateEpoch/substep/attempt/ordinal/id`。包含原位型、yFree/vyFree、目标钳位后的预测值、实际 x/y/vx/vy/state、Srow/Sfinal、接触阶段、实际列、票据/退额、提交标记。T4 成功主事件中的 `released` 沿旧“整请求准入”语义，退役只由真实 T3 与池状态证明。接触收尾事件无质量转移与第二个 LAND，若发生 rollback 只能保留隔离的失败诊断。新版可由 T367 的预测/afterApply/预发布/bank/slot 接点扩展，旧 72 词冷档照旧解码。

相同已接受角、dt、冻结池/床、请求顺序、预算初态下，preview 和 authority 走同一算术、同一容量与退款分支。A/B 比较含 yFree 位型和 revision。已接受角的链头 projection 增加受阻候选、票据结论和行后状态的相等检查，身份不能仅以角度作为 key。链尾的投影是无预算、无库存的纯约束，以链头已对拍状态和**同一最终床字段**为输入。工程把约束加入现有最终候选/权威核验，在最终床能由 shadow 给出时对拍实际 Sfinal 位型与结果。现有链头 projection 早于后续床通道，不能冒称已比较 Sfinal。若现有 shadow 仅有计划且缺最终床字段，使用预发布独立只读复算，固定 accepted 行前像与已审核的最终床快照，对拍候选池所有修改字段。保留现有其它 preview/commit 比较。必须证明尾约束在本子步无后续消费者，且下一子步只消费完整已提交的最终态。

每轮最多一个原请求或 motion 对应一个活动粒子。受阻行及保留 T3 的尾处理至多 R 条，R≤有效原请求数；定容 O(R) 额外存储、O(R) 算术和检查，禁止按粒子重复全池扫描或新增整次预算计划。具体 typed 字段/bytes 及 ring 容量由 architect 明列，含每个 preview 的留存与回滚开销。容量不足须在准备或安全落盘前拒绝，不能删事件或丢尾项以交付一半轨迹。

行内保留 Hot stage/publish/finish 与其故障切点。外层 backup 必须覆盖已完成 Hot 局部提交后的所有池/派生字节、床、宿主预算、ticket 状态、事件、尾项及 publication 序号。M2 finish 只是阶段完成，外层尾检查失败、字段审计失败、prepublication 拒绝一律恢复本次子步前状态。成功前缀按既有父事务边界保留。测试至少覆盖 reserve 后、refund 后、坐标写后、事件缓冲满、M2 finish 后、尾修正后及 publication 准备失败。失败后旧回执、过期 plan/重复 ordinal 或上一尝试尾项不能重放。所有诊断原始错误和 rollback 状态分别保留。

## 交接与检查矩阵

| 用例 | 核对点 |
|---|---|
| Z01 T369 位型锚点 | 复读两档权威预测及 apply/bank/slot。用旧 y/vy、dt 和角复算 yFree/vyFree。新策略在床为零的数值夹具应推进一次 |
| Z02 零源 local、零目标 local、零 total、比例 scale、部分 reserve、容量不足及全额 | 对每臂分别列票据、两个 local 和 total、grant/退款、实际列与原 ID。成功 T4 参考臂继续用目标床修正 |
| Z03 目标高床、源床不变 | 目标 y 钳位高于 yFree 时，零 grant 后使用 yFree；向左/向右各验 floor 和横向归零 |
| Z04 源床优先及变化 | 冻结床命中已归 T3。床在前序 T3 抬高而触床、后续通道抬高才触床、命中后床回落、顶边 eps、向上且起点已触床 |
| Z05 下一子步 T3 | 只收一次预算分母，零/部分/全额三档；核原列 bed/cache、phase/source、state2 余量、唯一退役、同 bank 与 slot |
| Z06 顺序与隔离 | 同起点 preview/权威逐位。两个粒子竞争及跨列 refund 后下一 ID 的预算可见性；多帧重复拒绝、反向释放、旧 route 和湿场差分 |
| Z07 故障矩阵 | 上述七类注错，核字节回滚、首错、事件、票据、提交时钟及完整帧 token |
| Z08 设备独立续证 | 后续另卡以同签包、同 run、实际角和成功物理秒跟踪 UP 后首拒绝→源列下降或接触→T3 全额或可解释余量，保留零跨列证据与同 token。禁止仅以非零 vy、帧计数或 live 数推断沉降 |

交 architect 工程化时需定 owner/tail hook、dry writer 闭包、备份与 failpoint、预算 receipt revision、T367 row 版本和启用隔离。产品代码、独立验证、真机及 GPU 路线分别推进。designer 的数值夹具与只读原档解析见本卡模型/证据。完成含本稿、复盘和离线设计检查，按成功交付登记。

## 本轮离线检查

两份 T369 冷档按 `attempt/substep/id` 关联。每档每 ID 八对权威预测/afterApply 与八组 bank 对得上，页面 slot 每档每 ID 四条，按实际帧末采样数检查。合计 48 对预测/afterApply 和 24 条 slot。两个端点旧 y/vy 为 31.399036625257057 / −44.27410539280644。用原物理角与 dt 复算预测 30.65877735511109 / −44.4155562087579，与本次主机解码后的 48 对预测一致。主机与产品运行时的 libm 可重复性应由工程核实。

在床恒为零、每步保持同一物理角和 dt 的反事实夹具中，从该锚点到触床为 40 个子步。真实设备的其它粒子、预算、角度和场通道会变动，该数字只作为数值夹具结果。模型另检查 3,000 个源列几何/床面样本、七种 T4 数学收支及三种 T3 收支。F64 退款有实际的非位对称算例 `(0.1+0.2)-0.2`，支持把事务回滚作为独立工程验点。结果和输入指纹见 `design-T377-cross-blocked-evidence-v1.json`。
