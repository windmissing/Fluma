# T425 CPU 增量审计等价性证明 v1

2026-11-09，designer。原卡 `6af162f7e4b07013b1a688ce`，CPU 参考 P4，GPU P5 主线优先。路线 `cpu-closed-prefix-audit-v1`，状态 `DESIGN_READY_FOR_ARCHITECT`。本稿消费 T420 阶段 C、T423 A/B 实现、T424 基线与当前源码。交付证明条件、可行结构、复杂度和可执行数学反例。不使用真机，不改产品实现。T423 A/B 可继续交付。

执行约定为完成本卡及角色检查。清单仅更新本人状态，不修改其他行列或滴答同名清单。后续画面由 XComponent+shader 绘制，失败沿 FAULT/完整旧帧 STALE 合同。

## 1. 裁定

**采用成功证明命中、存疑回原切点全验的分域方案。** 在已全验通过的基线、私有 owner、全部实际写入可追踪、所有检查依赖闭合时，可以减少公共结构、exact70、strict 和重复字节比较的扫描。实际读写仍按原逻辑切点发生，proof miss 回到该切点的原 verifier。新证明自身发现异常时只标记待核，原失败的 code、cut、ordinal 和冻结原件由原校验裁定。

**原序 RN64 fold 采用原序后缀重放。** 复用不变前缀，从最早受影响的 physical slot 或列继续按同一左结合式执行。通常可减少重放长度，最坏仍是完整 fold。整型 exact70 树可以维护精确库存，不能把它的最终舍入值直接充当 Hot 原序 fold 的答案。全局场 sum 也有自己的列顺序和表达式。

可先实施两个独立层级。C1 只在 predicate 全依赖未变时跳重复读取，变更域仍原验。C2 在明确闭包的干场 profile 中更新独立 shadow reference，触槽/触桶验证后证明同一成功结果。C2 需要比 T423 的单行 undo 更完整的写入与读取登记。未知外部写者、别名泄露、恢复输入、未完成 mutation 和未覆盖的 native/callback 边界都走原验证。现有代码没有这些新证明的完整实施证据。

## 2. 当前合同逐项拆开

| 原切点 | 已核当前顺序和算术 | C1/C2 的义务 |
|---|---|---|
| `DerivedCoordinatorV2.verifyPublicBoundary` | 先已有 status，随后原 serial+boundary 快路，再 binding → structure → audit.build → 按地址 acc limb 比较 → count/cache bits → strict。成功后更新 lastVerifiedSerial/lastBoundary/verifyCertificate | 保留前两项优先序。在原将进入某个 verifier 时才尝试该域证明。任一存疑域执行原序验，不越过更早失败 |
| `verifyStructure` | header、free stack 活跃前缀、physical-slot 活跃结构与 finite y/vx/vy、live count、4-pass radix 后重复 ID | 全局唯一 ID、free 集合/槽 state 的对应、header 和单槽谓词均有独立证明。free 栈次序及未用尾字节另属原字节域 |
| `DerivedViewV2.build` | 清 acc/count，按 physical slot capture/member，再 liveCount，再 bucket 0..255 roundAt。结果与 canonical acc/count/cache 对拍 | 建立独立池成员 reference。tuple 的 state/phase/列/mass 原位型与边界受控，canonical 派生域写入也必须追踪 |
| `DryDomainV2.flushBeforeRead/verifyStrict` | flush 只重算 dirty 列并写 surface/waterTop，strict 全128列读固定左结合式及非负/有限/占据关系 | 追踪实际字段、cache、capacity、dirty 和派生写入。调用读边界前完成原有 flush 副作用 |
| `auditFullBody` | 从 patchScratch 按 physical slot 对相别/列做 RN64 累加，再按列做 before/after 总量 fold，预算检查和末残差比较 | 与 exact70 分域。保留同一操作图、初始 +0、每一 prefix 的结果及原容差 |
| `DerivedCommitV2.prepareInactive` | fill → public verify → matchesAuthority → metadata/source overlay → mint seal；随后 commitPrepared 检查 preparedContent | 新校验不能移到 fill 之前。复制回执和版本要覆盖真正被比较的全部字段/原字节，source overlay 在原位置做 |

public acc 比较检查 `256*70=17,920` 个 U16。全部 payload、失败域和读取处以当前符号及输入 hash 绑定。T423 的 local undo 主要覆盖稀疏行和 DerivedViewV2.finishActual 的触列，公共全验仍存在。T424 有 R 记账和匹配负载缺口，不能据其低成本样本证明本方案高接触收益。

## 3. 等价命题与必要前置

设原逻辑检查为有序序列 \(V_1,V_2,\ldots\)，每个切点状态为 \(X_i\)。可观察结果包含分相/物理/事件/receipt、原始 Number 位型、源和成功提交前缀、第一失败的 code/cut/ordinal、原注册域的首错冻结字节和发布 token。新实现仅改变校验和复制工作的证明方式，物理算术操作图、写入顺序和已定义错误判线沿原合同。

要求 P0–P8 同时成立。

| 条件 | 可实施证明要求 |
|---|---|
| P0 基线 | 从同一切点跑完整旧 verifier。仅在其成功且 shadow 逐字段/位型一致后建立证书。旧输入在原逻辑允许但不满足新结构假设时，只能保留原验证资格 |
| P1 所有权 | 私有 ArrayBuffer、固定 owner 和 view、无共享内存/可写外借/未登记 DataView、callback 不可重入且副作用闭合。facade 的私有字段和冷拷贝是已有基础，内部 writer 清单仍须完整核证 |
| P2 实际写集 | 字段级 hook 覆盖直接下标、set/fill、header、free-list、生成号、ID、cache/limb、原子回滚及别名地址范围。before/after 取真正字节，不能只信请求量或事件量 |
| P3 依赖图 | 每条谓词的完整读集和类型/长度/offset/shape、capacity、policy、params 绑定。版本变更沿反向依赖失效，无法枚举的读取按全域 |
| P4 切点 | 保留原检查次数与先后，仅支持成功证明命中。proof 更新不得抛出抢先的业务错误或改已定义失败码。发生 miss 或反例时，在原切点原序运行旧检查 |
| P5 分支 | key 绑定 owner/root/view/branch/proofEpoch、slot generation、ID、birth/source 及 mutation 区间。子提交保留父 undo，回滚恢复业务字节并使旧控制证书失效 |
| P6 数值环境 | F64 RN-even、渐进 subnormal 和位型读写按原支持环境。每个等价环境独立建基线，不跨后端直接复用 Number 结果 |
| P7 证据读者 | 保留原 first-fault buffer/scalar 的逻辑视图和检查副作用。读者在中间 cut 读取的数组和 scratch 需立即可见或可确定物化，不得用末态重建失败前缀 |
| P8 容量 | 固定资源/日志/有序索引容量、版本计数不绕回。根写前选完整路径或有界私有路径。证明元数据不足的处理必须可恢复到原语义 |

**归纳证明。** 基线给出所有原谓词在 \(X_0\) 的值及位型相同。每次受控写入只改变已登记读集，未失效的谓词沿用原值。已失效域以真实前后字节更新 reference，并在本次原切点证明原谓词成立。若不能证明，旧 verifier 在原 \(X_i\) 上执行，得到同一最早失败。成功时按原操作顺序写入元数据和输出，建立下一步归纳假设。父回滚恢复旧原字节及与之匹配的 reference，控制 epoch 增长阻断 ABA。由切点归纳得到整个执行前缀等价。

这一定理依赖 P1/P2，不能从 `contentSerial`、hash 或 readonly 字段声明单独推得。TypedArray 变量 readonly 仍允许元素被另一个持有者修改。有限散列存在碰撞，散列只作身份辅助。

### 原切点的执行规则

第一版使用**整切点全成功证明**。在任何原 verifier 工作区/成功副作用写入之前，对整个待替代 cut 的依赖形成纯证明结果。任一阶段未能证明成功，就从同一原状态进入一次完整旧 verifier。这样不会因前面已执行 audit.build 又从头重跑而多增加 revision，或在冻结之前重复修改排序 scratch。确需逐阶段复用时，工程另给精确 resume 点和副作用记录。

```text
atOriginalPublicCut(boundary):
    execute original status and original same-serial/same-boundary fast path
    eligibility = proofOwner.canCoverWholeCut(boundary, actualViews, actualRanges)
    if eligibility is unknown:
        return originalFullVerifierOnce(boundary)
    witness = proofOwner.checkPendingChangesWithoutWritingOriginalVerifierState()
    if witness does not prove every original predicate succeeds:
        return originalFullVerifierOnce(boundary)
    materialize or account original successful verifier side effects at this cut
    perform original lastVerifiedSerial/lastBoundary/certificate updates once
    record new logical-check / actual-proof work receipt
    return original OK
```

旧完整检查成功但新结构假设仍未齐时，保留原成功结果，proof维持UNCERTIFIED。proof失效不会主动更正原数组。尤其 verifyPublicBoundary 本身没有 flush 调用，不能为使 strict 通过而在这里添加 flush；只有原调用点已经要求的 flush 才可执行。fill 仍在该 cut 之前实际发生，seal 仍在该 cut 和原 authority 证明之后。

## 4. exact70 的可行增量结构

### 4.1 编码、宽度与 RN-even

当前编码以 \(2^{-1074}\) 为一整数单位。原 F64 bits 的 exponent 为 E、fraction 为 F。

```text
E=0          -> units = F
1<=E<2047    -> units = (2^52+F) << (E-1)
zero         -> units = 0
```

原 Exact70.encodeMassBits 接受 ±0 的零值，但 live capture 单独要求正 mass。活质量域为 0<m<=256。所有正库存按 phase*128+floor(x) 入桶，同一桶的 U16 little-endian 长度70，整数域 [0,2^1120)。最多512个活槽，每槽<=256，全部质量<=2^17，因此全体库存的整数最多为2^1091，所需 bit_length=1092，距1120位仍有余量。该界只对已经证明的有效槽成立。非法 limb、错误 count 和未验输入不能借该界免检。

每桶保存独立 `expectedAcc[70]`、expectedCount 和 expectedCacheBits，来源为已全验的原槽集合。每个实际 mutation 先按原 member 顺序处理 oldA/oldB 的移除，再处理 newA/newB 的加入。上次没有成员变化的 y/vy 或同列 x 变化只影响结构谓词和原字节域。mass、phase、floor(x)、活跃状态变化使旧新桶 dirty，ID/state 的 generation 依现有 finishActual 精确规则推进，不能假设只有出生才变 generation。

对每个被触桶，用原编码域从真实 tuple 生成 expected 增量，对独立 reference 做 checked subtract/add，再用原 RN-even 规则生成 expectedCacheBits。核 canonical 所有实际受写 limb/count/cache，触桶上界按整70字比较给出。更细的 word 证明仅在 P2 已覆盖所有该桶写地址时可选。未写桶沿基线等式。至此证明：

\[
A_b=\sum_{s\in b}\operatorname{units}(m_s),\quad
n_b=|\{s\in b\}|,\quad
\operatorname{bits}(cache_b)=\operatorname{RNE64}(A_b2^{-1074}).
\]

**prefix 与错误不能合并。** checked subtract 先比高位确定无负，再借位；checked add 先预演最终 carry 再写。旧实现也保留这些局部错误切点。只有完成同一逻辑切点全部允许操作后才可合并脏桶 round 工作，任何本来在中间发生的 tuple/limb 检查仍需作等价判定。负中间库存后再补回、溢出后再减回均须保留首错。异常 proof 只作该切点存疑，业务错误仍走对应原检查。

round 从最高非零位确定53位 significand，使用 guard/sticky/保留位奇偶。正好 half 且 retained 偶数不进位，retained 奇数进位，significand 溢出时向 exponent 进位。全零产生 +0。subnormal 按整数低52位精确输出。最小正 subnormal 不得 flush 成0。cache 对拍按原32位双词，`-0 === +0` 无法满足这一比较。

### 4.2 reference 与主派生值各自受检

expectedAcc 与 canonical.acc 独立存放并独立登记。不能用 canonical.acc 自己减旧加新，然后把自身相等当 proof。reference 更新使用实际 tuple 的前后字节，校验登记的 slot/generation/ID 与 before 所匹配。修改 cache/limb 而池 tuple 未变，也必须使比较域 dirty。非法 reference 或缺失 before 时禁用命中，原 cut 全验。校验结果只读 canonical，不能先用 reference 把坏 cache 修好再做原验证。

固定桶触位图、紧凑触桶队列、slot before tuple 和 reference 可复用现有 audit workspace 的同等容量，但工程必须证明其独立性及生命周期。每实际成功 mutation 的维护 O(70*t_m)，原 public cut 合并至 O(70*t_b)，其中 t_m<=4 为旧新两槽涉及桶数，t_b<=256 为自上次 cut 的触桶并集。参考更新不免除每原 tuple cut 的异常判断。

## 5. 结构和严格域如何减少公共扫描

### 5.1 free、ID 与单槽结构

从合法基线维护 aliveCount、freeCount、freePosition[slot] 和活动 ID→slot 的确定有序树。free 栈原顺序作为独立业务字节保存，位置映射只回答集合完整性。每个 slot 状态或 freeSlots 活跃前缀写入，按真实前后值更新相反映射并校验唯一对应；header 改动重验原 header 谓词。slot 的 x/y/mass/vx/vy/phase/source/sourceId/id 都有自己的 read-set，移动 y 不能只增“质量版本”。

ID 变更 erase/insert 两端，O(logN)，重复先标存疑，等原 structure 或 Hot ID 检查轮到时执行旧序全验。旧 physical-slot 查找和按 ID 选对属于其它操作，不能被本树的排序验证暗改。活动/空槽对应与 ID 集合检查成功，可证明四趟 radix 后不会有重复。若原诊断读取排序数组，需按原 radix 的最终内容物化，复杂度按实际读取计。freeCount 之外的 freeSlots 尾部不影响 structure，却仍在 matchesAuthority 和冻结字节域中。

每次退役、再出生、AIR/state2变化、checkpoint/clone、view copy、rollback 都按原 generation 行为登记。证书 key 至少含 `ownerGeneration/view/branch/proofEpoch/slot/generation/id`。新增 proofEpoch 为独立控制域，回滚也单调增，不写入物理 source/commit 或改变原 ID 分配。只恢复 slot 原 generation 而复用旧证书存在 ABA，必须同时使旧 branch/cut 失效。

### 5.2 dry 列依赖

逐列读集为 bh/bl/mh/ml/suspH/suspL、cacheH/L、capacity，以及被比较的 surface/waterTop 原字节。任何一种字段变化均使该列 dirty，capacity 或 policy 变化使全列 dirty。同相跨列使两列失效，mass/phase转移使旧新桶及其列失效。场写者修改 bh 必须传播至 strict，即使 pool 无任何变化。

保持当前表达式的逐操作顺序：

```text
mix = bh + bl
top = (mix + mh) + ml
P = cacheH + cacheL
envelope = ((top + suspH) + suspL) + P
```

flush 的 dirty 清除和写 surface/waterTop 是实际副作用。verifyStrict 使用原 Number 比较规则，而 matchesAuthority 使用字节比较规则。不能把两者统一改成 bitwise，从而让原本 strict 允许的 surface -0 变成新业务失败。反过来，cache -0 不能借 strict 的 Number 相等绕过原 cache bits 对拍。新增证明资格可以更窄，资格未满足时回原检查，其原成功或失败均保留。

若列全部依赖自已验基线未变，则该列 strict 必然同样成功。触列按原表达式重算，任何存疑在原 PUBLIC_STRICT_DOMAIN 阶段调用完整 verifyStrict。不能在 acc mismatch 之前返回 strict 错误。邻边坡度 guard 的读集包含 c-1/c/c+1 和实际倾角，另属其原 cut，不能用只依赖 c 的 dry strict 证书代替。

## 6. 原序 RN64 fold 的证明边界

Hot `auditMassH/L` 从 +0 开始，按 physical slot 递增对每桶左折叠。它与 exact70 最终舍入值不同。1、2^-53、2^-53 均在当前单粒质量范围，依序 RN64 结果为1，先合两个小项再加1为1+2^-52。以旧总量减旧项再加新项也无法恢复已经丢失的低位。容差同样不能提供位型等价或所有边界输入上的判定等价。

可行的原序结构为每桶按 physical slot 建有序成员索引，每成员记录**经过该槽后的原 RN64 prefix 位型**，另存桶首根。membership 或 mass 在 slot s 变化时，旧新桶分别记录最早受影响 slot 和最末 dirty slot。用最早 dirty slot 的不变前驱 prefix 作为起点，在新成员序中重放其后的全部项。初项仍从原 +0 开始。在同一 physical slot 处，若新 running bits 等于已存旧 prefix，且后续成员和质量字节已经证明全部未变，可提前停止。只有“当前 prefix 相等”而后面还有 dirty 项时不能停止。

证明按后一项归纳成立，因为输入 running bits 和下一项 bits 相同，单次 RN64 加法及其后各项相同。每桶最坏需要重放全部活成员。一次变化发生于首槽且改变 running bits 时，后续每一项可能决定何时进入新的舍入区间，单一子树 sum 不足以得到相同结果。这里的下界是**采用原序 prefix 表达的 replay 最坏 Ω(L_bucket)**。本稿不声称所有可能的 Number 转换函数索引都不可能更快。

before/after 全局守恒量则按列 c 的原左结合式计算，各叶先按原 `bh+mh+suspH+P_H` 等操作顺序生成。把改变列之前的 prefix 复用，余列重放，最坏 Ω(C)。before 的依赖还包括冻结 field 和 frozenPool、sourceExternal 与预算的原位置。任何改变初始 +0、列/相别次序、Kahan求和、exact70最终舍入、树形归约的方案须另 numeric contract，不能在本卡当等价优化。

原 full audit 的每原错误检查和第一次失败顺序仍保留。初步 C1/C2 可继续执行整个 Hot numeric fold，仅减少不变域与公共 exact70 重建。等原序后缀结构及它的 writer 闭包另行通过独立对拍后再启用更窄重放，最坏成本照实计费。

## 7. 首错、原件和事务

| 情况 | 等价处理 |
|---|---|
| 新 proof 看见 limb 异常，但原 structure 也坏 | 先走原 binding/structure，返回原先发生的失败，保留该时刻原注册字节 |
| 某 cut 中间失败，后续计划会修复 | 此 cut 按原序判定，停止执行后续计划，不把净末态当原执行 |
| 旧 cache 是 -0，pool reference 为 +0 | 在原 cache bits 比较阶段复现 CACHE_BITS_MISMATCH，严格域的数值相等不覆盖该检查 |
| 外部 callback 或 native 可写别名进入 | 调用前撤销相关闭包，回到原校验路径。若私有 buffer 已可持续外借，直到换成真正私有副本才可重新建立无扫描证明 |
| 子分支 commit 后父 rollback | 日志旧字与 shadow、ID/free 索引、dirty/prefix/证书恢复同一 mark。父标记内保存子记录，不提前丢弃 |
| source 成功，随后 STEP 第二子步失败 | root 边界沿原成功前缀。已成功 source 和第一子步仍存在，失败子步及其证书撤回 |
| counter/校验工作变化 | 原逻辑 cut 序列和业务元数据保持。新增实际工作统计单独版本化，计 logical/hit/full/fallback、limb/slot/bytes/replay；不宣称原工作量 counter 位型不变 |

`DerivedViewV2.build` 成功会更新 audit 的 ids/tuples/bits、dirty、revision；verifyStructure 也会更新 freeSeen、idA/B、radix 等工作区。C2 要逐一列明读取者。可直接维护和用常数/版本表达的成功副作用照原值推进，任何可见读者要求完整数组时按原成功构造物化。原故障冻结只认原注册域和相应原 cut，不把新证明数组冒原件。当前 canonical 与各注册 view 的 rawBuffers 都必须保持原逻辑字节。若某已注册 scratch 在任何首错会被读取，却无法通过日志和原切点信息物化，相关写者只能完整路径。

首次故障前用当前逻辑 before 物化，顺序沿当前 `prepareFailure → rawCopies/rawReaders → faultHeaders`，保留 T423 的 before 物化调用。shadow 上的假设失败不能覆盖原错误。即使所有末态相同，原始 before、字段错误优先级和被冻结的 NaN payload 不同也不算等价。

### 发布复制和扫描下界

matchesAuthority 的比较顺序从 pool header、13个 pool buffer 到8个 field buffer。若 source 与 bank 都为私有、fill 是登记的精确 copy/规定派生写，且 fill 之后到 seal 前无任何可达写入，逐范围 copy receipt 可以证明原每字节相等。receipt 绑定 source cut、destination generation、每段地址/offset/length、bulk write revision 和帧身份。源变化、目标复用、copy未完成或中间 hook 失效时执行原 byte compare，错误 detail 保留。

fill 仍须读写完整目的 payload；materialize/copy 的 Ω(payload bytes) 成本存在。减少第二遍 compare 不等于免发布 bank 构建。若采用持久页/共享不可变 bank，要另给 lease/复用及各 metadata 的合同。旧 source overlay 和在原 copy 中生成 hs/hl 等非直接拷贝字段须逐项证明，不能给整个 fill 无条件盖章。

对未闭包的任意可写内存，确定校验器若少读任意一个原必读 word，就存在两份其已读位置相同、未读 word 一份合法一份非法的状态。没有独立写入证明时无法区分。故此场景下原全量数据检查保留 Ω(N+C+256*70+原byte域) 的读取下界。hash、serial 或“已经跑过一次”不能消除此下界。相同 buffer 对象而字节被改是可执行反例。

## 8. 复杂度与固定资源模板

设 H=70，B=256，S 为自上次原 cut 后实际被写/受影响槽数，F 为 free 栈被写项，T 为触桶并集，D 为触列并集，W 为实际原字写数，R 为真正发生的原逻辑 cut。K 为 RN64 后缀重放项，V 为必须构造的完整 publication bytes。所有次数按各 root/branch/preview/失败前缀实记。

| 路径 | 证明工作界 |
|---|---|
| 初始/未知输入/不支持写者 | 原完整验证，成本至少 O(N+B*H+C+原IDcheck+原byte域) |
| C1 不变域命中 | 常数控制 key 与原读集版本命中成本；已变域继续原执行 |
| C2 sparse public | O(W + (S+F)logN + H*T + D + dependencyInvalidate)，另加真正的物化 V、原需要的 Hot fold 和原 flush |
| RN64 后缀 | O(indexUpdates*logN + K)，K最坏等于受影响序列总长 |
| proof miss | 当前记录/失效开销加原 verifier，按真实切点收费；不做错误快返 |
| failure/rollback | 逆原字日志 O(W_rolled) 加实际 frozen/materialized bytes，恢复索引或从原失败视图重建按实际收费 |

位置类更新若全 N 粒子都被写，S=N，结构谓词至少要处理这些槽。小 mass 改动也可能使一个长桶的 K 很大。B/C 小常数不消灭随实际 R 累计的成本。不能把一个 cut 的改进写成整个 callback 同比例收益。

一个**新 reference view**的保守 typed payload 模板为 expected acc/count/cache 共 38,912B，tuple before 为24N B，freePosition为4N B，slotDirty为N B，ID树键/父/左右/高度共17N B，按槽桶序树父/左右/高度为13N B，RN64 prefix为8N B，256个桶根为1,024B。合计 **39,936+67N B**。另有256桶dirty32B、128列dirty16B，合计 **39,984+67N B**，N512为74,288B，N192为52,848B。实际控制句柄、rev表、journal、分支并存、原audit workspace与原bank/读者快照另计。若复用原工作区，按实际生命周期扣除，不能同时把同一数组当独立 proof 与被比较的 canonical。绑定多个 view/preview 时乘真实同时存活数，不能只报一份。

T423 的8,136B单行 undo不自动覆盖上述 reference。工程要给可支持根的写上界、嵌套深度和undo容量。未知或稠密路径根首写前选择原路，避免半写后无日志。实际30fps收益按 CPU参考线后续匹配负载测量，T425给出算术与证明界。

## 9. 可执行反例与交接

配套 `design-T425-audit-proof-model-v1.py` 用独立 Python 整数/Fraction 与小型70 limb 算术，执行下列设计例。产品函数没有被调用。

| 编号 | 反例/正控 | 应识别的边界 |
|---|---|---|
| E01 | 所有边界质量及随机正质量编码、add/remove、逐cut重建 | 独立 limb 与整数总和一致，round与Fraction RN一致 |
| E02 | even/odd half、sticky、进位及最小 subnormal/全零 | guard/sticky/tie-even与 +0 正确 |
| E03 | 先减至负再补回、最大limb先溢出再减回 | 净结果相等不免 prefix 错误，失败加减保留原数组 |
| E04 | [1,2^-53,2^-53] 的 serial/tree/exact | 原序 RN64 与exact最终舍入可不同 |
| E05 | 已fold总量减旧再加新 | 丢失低位无法由总量恢复 |
| E06 | 首槽质量变化，原序prefix后缀重放 | 与全fold位型一致，操作数可接近整桶 |
| E07 | cache -0/ +0、subnormal被FTZ、NaN payload undo | 数值相等不代表原字节相等，整数旧字恢复 |
| E08 | 未变pool、dirty canonical limb；同serial别名写 | 证明需覆盖 canonical 写入和原数据，不信单版本 |
| E09 | structure/acc/strict多错、失败后修复计划 | 原先后和第一cut必须保留 |
| E10 | 同slot重生、子提交父撤销、rollback后复用slot | 分支/proof epoch与generation共同防ABA |
| E11 | dirty只记pool，漏bh或capacity、漏free尾字节 | 完整反向依赖；strict和bytes域分开 |
| E12 | sparse proof与完整oracle逐cut对照 | 正常证明命中成功，unknown写转完整路径 |

architect据本稿在T420同一系列增补 `CPU_CLOSED_PREFIX_AUDIT_V1` 工程文件，先C1再明确支持域的C2，独立开关和实际工作收据；保持原数值policy及业务事件含义，新增proof/receipt版本显式绑定。必须提交每个符号的原谓词/read-set/write-set/cut/raw-reader/fallback矩阵。缺闭包的符号列出精确原因和原扫描成本，继续原路，不阻塞其它已证部分。

developer按工程稿做离线编译/自测与两路同输入对拍。tester另建独立反例实现，核每原 cut 的 status/stage/ordinal、±0/subnormal、partial/full/zero、回滚原字节、T381/T403 行为哨兵、slot/free/ID/事件、实际 copy/limb/replay 工作量。T424 的R缺账和匹配负载问题先修正后才能报告整体收益。所有失败先交architect分流；设计证明被反例推翻则回designer修订。manager按交付登记，不增加验收门。

数值语义参考 [ECMAScript Number::add](https://tc39.es/ecma262/multipage/ecmascript-data-types-and-values.html#sec-numeric-types-number-add) 和 [GetValueFromBuffer](https://tc39.es/ecma262/multipage/structured-data.html#sec-getvaluefrombuffer)。这些为Number/字节语义依据，目标ArkTS环境仍须沿原probe与独立字节反例确认。

## 10. 本角色检查结果

数学模型运行 exit0，3,000次真实成员变化的设计切点含4,726次limb加减，与按槽重建的独立整数总量及 Fraction→binary64 舍入对照；13项RNE/normal/subnormal边界通过。1,500次后缀随机重放与完整左fold位型一致，首项变化的32项强制样例须重放全部32项。另有1,500次合成稀疏证明/unknown写回完整参考，记录1,338次模型命中和162次回退。该比例由合成输入指定，不能作为产品命中率。

49组首错阶段组合中，提前用后发现的proof错误返回会有21组阶段不符，按原序fallback保持原结果。旧字日志示例验证子提交后父撤销、source及首步成功前缀保留、slot ABA控制epoch失效。反例还覆盖同serial别名、仅canonical limb被改、漏bh依赖、free顺序和重复ID、±0/NaN payload原字恢复。

对容差的补充样例使用未证明的损坏cache `0.9999999990000001`。原序值1与它的误差约9.9999986e-10，精确总和舍入值1+2^-52的误差约1.00000008e-9，跨越原1e-9判线。它展示任意输入上的判定差异，不能冒称该损坏cache已经通过V2公共全验。

上述检查使用设计小模型，未替换或运行产品审计，未执行原测试套件、真机或性能实验。等价性仍依赖P0–P8闭包。原实现的可执行反例和未来工程逐cut差分分别承担不同证据职责。
