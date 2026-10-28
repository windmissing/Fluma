# T209：明水 B 完整子步开发交付

2026-10-28，developer；原卡 `6ae15c23e4b0ab67701971aa`。规格为 T203 `f90077b` 的 `architecture-T203-clear-water-B-v1.md`、其 engineering manifest，以及 T196 `CW_B_STEP_DESIGN_V1`。不使用真机。目标7；没有生产渲染、旧物理或 GPU 路线迁移。

## 实现

在 `physics/clearwater/` 新增 BTypes、ExactMass、Events、Guard、Hydraulics、Particles、Step 七个模块，集成 A State/Source/Engine 指定 hunk；Types 仅扩展预建 StepResult。保留 A 原 status 0..22、row20/56/24、key、负零 payload 比较、queue64/dedup4096 不驱逐、初始基准及两个 kind 成功序号。B 未安装时正 dt 仍为 UNSUPPORTED_FEATURE；dt0 不消费队列/预算/事件/成功身份。

- **ExactMass**：70×U16、scale 2^-1074，二进制位编码、带溢出预检的加法、无负值减法、比较；两个70-limb scratch 和8B冷 bit view。维护私有 P 精确和及固定 Bcap，不把其舍入值当 canonical P。最终和 replay 都按递增 ID 单遍 RN fold，同时写 P 与 colParticleMassH，保留 A 原审计。
- **来源与五 bank**：canonical/sourceBefore/stepBefore/work/replay 独立 backing。cold attach 从同 owner 原 source event24 构造固定4096×16B沙 ID 区间，逐 live ID 二分解析 originSourceCommit；water 不入区间。后续沙成功出生在原 source shadow 中填写 origin/wait，与源事件及库存一起发布；full LAND 清 slot metadata 并 O(1) 摘链/退槽，partial 保留 ID/origin。reset 清五 bank metadata、来源、sealed 代、预算、计数与时钟测量状态。
- **水与床**：N−1 条边各一次、奇偶首序随成功 stepSerial 切换。水用五上界、真实 RN 差分及精确容量审核，不扣固体预算；水后冻结 etaMedium。床在粒子之后，用 top-first mh/bh stock 图，一次影子 patch 和最多3边 actual guard，没有额外旧通道、q 二分或 λ 重试。
- **预算与 guard**：300dt total、28dt local；整包 crossing 两端各扣一次 local、total 一次。LAND/BED 按预留→裁剪→一次退款的 RN 顺序记录真实前后账，非直接把净 q 当作等价 RN 算式；replay 复算同一顺序。单列/邻列 guard 调用既有 `p011GuardAdmitNumericInto`，实际 CanonicalS 再核；全步最后核入口冻结 non-worsening bound。保留 H=tan30、drift>=0.001、EPS 原作用域及真实水/沙绝对差1e-6，没有调阈值。
- **粒子**：递增 ID 活动链一轮；空气/湿速度按给定 RN 图，候选直线弦分段，床/墙/列边/水面优先。目标床相等时优先阻挡跨列；右受阻 nextDown(c+1)、左受阻 c，接受左跨 nextDown(c)，没有几何 epsilon。重复零时间同平面拒 STEP_TIME_NO_PROGRESS。侧等待独立于 BED_WAIT，不隐形分裂粒子或误 LAND。每粒主循环最多 e−1 行，末尾现有 fold 留1行做床后 CONTACT 收尾。
- **事件与原子提交**：独立 event40/lineage3/summary16 双 arena；逐写前预留，unused 槽正零、flags 保留位和 kind 分义核验。replay 按全局 ordinal，检查旧位型、ID/origin、actual 库存、容量、guard、预算及退槽；不把 summary start/count 当连续事件 slice。最后比较库存、pool/free/链/cache、origin/wait/header 与预算，预检所有固定视图后，无外部回调地同步 copy 与切 sealed。三类 whole audit 仅入口/work-final/replay-final，成功3次，不逐 event rebuild/copy/audit。

源成功后 step 失败只丢当前 candidate；第二子步失败保留第一成功子步及旧 sealed arena。Out 给 coarse status、独立 reason、stage、首次 ordinal、attempt、lastAcceptedCommit、sourceCommitAtEntry 和输入角；成功 sealed header 另绑定 run generation、owner、config、input/geometry/clock window、angle/dt、source/physics/step/arena commit。stage 1入口、2预算/精确核、3水、4轨迹、5床、6fold/末审、7重放、8提交。

提交区间固定 `.set` 前已核 backing、长度和 metadata 绑定，期间无 yield 或用户回调；这些同步固定视图复制不再有 detached/越界条件。计时 callback 在 protected copy/切 sealed 之后才再次调用，clock 自身错误仅使时间无效，不撤销已成功物理。

## 读取、计数与未实现边界

新增 attachBInto / copyLastBEventsInto / copyBFrameCountersInto / snapshotB / restoreBInto；明确 beginBFrameInto/endBFrameInto 计量 scope，不暗中提交源或推进 dt。显式完整 frame 至多2次实际正 dt attempt；未绑定或 scope 外继续写则标 PARTIAL_SCOPE。48计数包含 enqueue 扫描、源边界与 birth、源复制字节、子步/候选、审计、活动链/fold/replay、七 kind、lineage、70limb、整池复制与禁止路径探针；merge/index/λ 为0仅指 B 自己。copyBytes/sourceCopyBytes 按实际 `.set` 的 typed-array 字节计，排除未复制的 auditMass 和 JS 标量/对象布局；分配载荷仍计入每 bank 的 auditMass。

可冷注入毫秒单调 clock；未知时间为 NaN、JSON 为 null，header.clockStatus 区分未测/采集/抛异常/倒退。wholeSubstep 包含各内部阶段，replay 包含其 final audit；不得叠加重叠时段。source enqueue/边界、cold snapshot export 在各自 scope 显式累计；未测不填0冒数据。Node clock 测试用确定性计数器验证身份/失效处理，**不是性能测量**。

snapshot V1 的 typed-array 内容和原 JSON schema/字段顺序保持：State.toJSON 显式导出 A 原字段，B metadata/binding 不混入 A JSON。snapshotB 冷复制 A 全 authority/队列/源账、canonical origin/wait/header、BConfig、一份 sealed arena/header/lineage 及四列向量/预算。patch scratch 非 authority，在 seal 时清零；sealed 预算由冻结事件的真实预留/退款图重建，因此后续失败 candidate 或后续成功 source 不会污染旧 sealed 账。导出同时区分 current source/physics 与 sealed B commit；拒绝任何输出与 owner buffer 或输出彼此 alias。

**restore 与 C 均 NOT_IMPLEMENTED**，restoreBInto 始终 UNSUPPORTED_FEATURE。没有 FrameToken、页面、切沙水输入、publication 或透明水 shader 的成功声明。无 merge/lift/U、V=1，无新 sensor 或渲染依赖。所有候选参数仍 PENDING_CALIBRATION。

## 实际资源

`test/reports/t209/allocation-192.json` 与 `allocation-512.json` 为逐数组实际 ledger，按真实 byteLength 求和，包含 origin 表及所有 scratch/header/summary，guard 四个3元素 view 复用32标量 backing，不重复计。与 T203 两主档逐项总数核对一致：

| N128/e8 | K192 | K512 |
| --- | ---: | ---: |
| event40 行数 / lineage3 容量每 bank | 1790 / 3580 | 4350 / 8700 |
| A+B 固定 payload，含保留 cold 来源表 | 4230656 B | 6312256 B |
| 一份 snapshotB 增量 payload | 3319456 B | 4284576 B |
| owner + 一份 snapshotB | 7550112 B | 10596832 B |

对象、闭包/绑定记录、字串、typed-view header、对齐、栈、GC、调用者额外保留 snapshot、旧场景与未来 C/native/driver/GL 不在数组载荷内。完整 snapshot 的复制/临时对象属于冷端成本，不能每帧隐藏调用。没有设备 RSS/帧时或整机预算 PASS，也没有减少 dedup/事件来绕预算。目标7资源冲突仍交管理/设计裁决，不能挤占 GPU 最高优先任务。

## 验证

- `node test/run_t209_clear_water_b_units.cjs`：**35/35 PASS**，B01–B14 的开发关键边界。包括水五界/隔床/溢出、空气/湿分段与出水/零时刻、相等目标床优先、整包跨列/预算量子/高床等待、CONTACT与partial/full LAND、top-first 床、IDfold adversarial 小质量、70limb 对独立 Node BigInt 位 oracle（1000随机位型及normal/subnormal/carry/underflow）、混水沙来源 attach、源成功/上一子步/sealed 后各阶段故障、arena/lineage cap、replay 旧位型损坏、五 bank 无 alias、detached/替换与输出 alias、热接口无 TypedArray/ArrayBuffer/DataView/Map/Set 构造或 slice、单/双子步 scope、K192/K512 满池计数、100步闭合场景、snapshot/reset/C/restore 边界。
- A-only 回执/序列化 snapshot 与 `0cc0439` 原四模块逐字节对照；A 段原自测 **26/26**、旧 gesture **18/18**、旧 DEMO **5/5**，均无失败。故障替换仅在 Node fixture 中，不增加生产注错入口。replay 是产品自检，不冒独立 tester oracle。
- `python test/compile_t209_clear_water_modules.py`：临时隔离 worker probe 显式 import Engine，使所有新 B 模块进入真实 ArkTS；finally 恢复 build-profile 和删除 probe，**BUILD SUCCESSFUL**。
- hvigor `default@PackageHap --no-daemon`：真实 native/ArkTS/unsigned PackageHap **BUILD SUCCESSFUL**。该任务不执行签名，signed=NOT_BUILT；没有声称解决既有签名 `00303116`。unsigned HAP 是原应用包，不表示新增 B 页面或生产 route 已接入。

日志、配置/输入源码 SHA 与 unsigned 包身份在 `test/reports/t209/`。无真机、端到端、效果或设备性能结论。独立 B 验证由后续 tester 卡执行，C/设备观感继续沿真实前置另卡。
