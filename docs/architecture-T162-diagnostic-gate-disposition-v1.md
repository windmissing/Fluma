# T162 T154 门禁分流方案（architect）

日期：2026-10-27。责任：architect。任务卡：`6ae00c6de4b017ab340e9c6a`（滴答）。
状态：**ANALYSIS_COMPLETE / GATE_EXTENSION_DESIGNED / ROUTE_ASSIGNED**。
输入唯一依据：`docs/developer-T154-pool-progress-v3.md`（T154 交付报告）、`test/reports/t154/static-gate.json`（manager 已核实）、`docs/architecture-T116-device-audit-index-arrival-v2.md` §4、`docs/developer-T149-field-column-evidence-v2.md`、`docs/developer-T135-supported-kernels-v2.md`、门禁本体 `test/probe_p011_particle_hot_graph.cjs`。纯设计零代码（不改 `entry/` 与 `test/` 任何文件；分析通过只读运行现有探针完成）。

## 0. 结论先行

**32/34 处未解析调用是「分析器模块缺口」，不是真实分配点**：它们全部调用 `P011PoolProgressDiagnostic`（poolProgress）与 `P011FieldColumnEvidenceV2` 的方法，而这两个模块**不在探针的 7 文件分析清单里**（`files` 数组 :5-6 只列 ParticleField/Prototype/RefundAdapter/Hot/HotBudget/HotPrototype/HotEvidence）。诊断 OFF 时 `poolProgress===null`/`evidence===null`，全部调用点被 `if(d!==null)`/可选链 `d?.`/`if(work.fieldColumnEvidenceV2!==null)` 短路，零执行零分配；诊断 ON 时方法体本身也零分配（§1 逐处判定）。仅 1 处（mergeIndex.find）是常开生产路径但同样属模块缺口。

**门禁接入采用方案 (b) 独立子探针**（§2），不动主门禁 ZERO_ALLOCATION 判定；baseline 2 处遗留按 §3 路由（T149 capture→T146 修复链已含谓词对齐；T123 index→门禁扩展一并覆盖）；T155 准入判据见 §4。

## 1. 诊断 OFF 控制流逐处判定（static-gate.json 34 处 ↔ 源码行号逐处对应）

判定方法：探针输出的 `unresolvedCalls[].owner/line/text` 逐条对照源码该行的守卫语法与被调模块。**34 处按被调模块分三类**：

### 类 A：poolProgress 诊断调用（32 处）——「分析器缺口：守卫短路，无分配」

被调方法 `increment/cursor/begin/endSpan/input`（P011PoolProgressDiagnostic.ets :75-107）。诊断 OFF（`work.poolProgress===null`，`configurePoolProgressDiagnostic` :2180-2186 在开关关闭时置 null）时：

| 函数 | 行号 | 守卫语法 | OFF 时控制流 | 分配判定 |
|---|---|---|---|---|
| beforeInOrder | 664 | `d?.increment(6)` | 可选链短路，求值 undefined | 零执行零分配 |
| siftDown | 672 | `if(d!==null){d.increment(7);d.cursor(...)}` | 块整体跳过 | 同 |
| sortOrderHot | 697 | `if(d!==null){d.input;d.cursor;...}` | 块跳过（含 698 begin/increment） | 同 |
| sortOrderHot | 701,704,707 | `d?.cursor/d?.endSpan` | 可选链短路 | 同 |
| editPoolHot | 969 | `work.poolProgress?.increment(19,births)` | 可选链短路 | 同 |
| auditParticleHot | 1106,1108,1109 | `if(d!==null){d.begin...}` + `d?.endSpan` | 块跳过/短路 | 同 |
| beginStepHot wrapper | 1213,1215 | `const d=work.poolProgress; if(d!==null){d.begin('BEGIN_HOT');d.increment(15);}` + finally `if(d!==null){...}` | 两块跳过 | 同 |
| collectParticlePlanInto wrapper | 1221,1223 | 同上模式 | 同 | 同 |
| finishStepHot wrapper | 1229,1231 | 同上模式 | 同 | 同 |
| copyPoolDiagnosticHot | 1236-1239 | `if(d!==null){...}` + `d?.input` ×2 + `d?.endSpan` | 块跳过/短路 | 同 |
| applyParticlePlanInto wrapper | 1245,1247 | `if(d!==null){d.begin;d.increment(17);}` + `d?.endSpan` | 同 | 同 |

（行号合计：1+2+7+1+3+3+3+3+6+3=**32 处**，与 static-gate.json「32 处开关保护诊断调用」计数一致。）

**诊断 ON 时的补充判定**（T155 会问）：这些方法体也零分配——`increment` :76 是 `counters[index]+=amount`（Float64Array 原地加）；`cursor` :100-107 是原始赋值 + `now()`（try 内调用注入时钟，无构造）；`begin` :86-92 是槽位搜索 + `row.assign(context)`（行对象预分配于构造器，:63-65）+ `this.now()`；`endSpan` :94-98 是 `delta[i]=counters[i]-before[...]` 原地写 + `checkLong`；`checkLong` :108-126 的 `new Uint8Array(buffer)` 是**视图构造**（不复制数据、仅包装现有 buffer），且只在首超长窗口触发一次（`longCaptured` 单次锁存）。`rowJson`/导出路径不在热路径。**结论：诊断 ON 分配恒为零（视图构造除外，且单次）——「诊断 ON 的分配计入独立分母」这一子探针判据（§2b）在源码层面成立。**

### 类 B：V2 证据 captureFailure（1 处）——「分析器缺口：冷路径，仅失败时执行」

`beginStepHotBody` :525（baseline 为 :523，T154 改动使其行号平移）：守卫 `if(work.fieldColumnEvidenceV2!==null)` + try/catch。诊断 OFF 时 `fieldColumnEvidenceV2===null`（V2 只在诊断启用时构造），调用点零执行。诊断 ON 时也**只在 FIELD_COLUMN 命中后执行**（:517-533 谓词失败分支内）——生产成功路径永不触达。`captureFailure` 内的 `poolHex`/`rawBits` 等数组（P011FieldColumnEvidenceV2.ets :145-160）是冷分配，与热路径无关。判定：**分析器缺口（冷诊断路径），非真实热分配**。

### 类 C：mergeIndex.find（1 处）——「分析器缺口：常开生产路径，非诊断」

`mergeOldestHot` :949（baseline :936）：`work.mergeIndex.find(p, radius)` **无诊断守卫、每次合并尝试都执行**（T123 生产逻辑：满池时确定性合并检索）。它被列入「未解析」仅因 `P011MergeExactIndex.ets` 不在分析器文件清单。`find`→`findBody`（:84-116）的内部 `diagnostic?.increment/cursor` 调用同样被可选链保护。判定：**模块缺口（生产路径，需主门禁覆盖——见 §3 路由）**。

**34 处判定汇总**：33 处诊断守卫短路（类 A+B）、1 处生产模块缺口（类 C）；**0 处「诊断 OFF 时仍有可达显式分配」**。static-gate.json 的 `disposition=STATIC_GATE_UNRESOLVED_REQUIRES_ARCHITECT_OR_TESTER_ADAPTER` 与本判定一致——分析器无法证明，不是分配存在的证据。

### 补充：hotAllocationSites 的 1 处真分配

`P011ParticleField.totals()` :367 对象字面量 `{heavy,light,count,resting}`（`P011ParticleTotals` 返回值）。它在主门禁下就是可达真分配，**与诊断无关、baseline 前就存在**（`remainingSiteCount=35 = 34 unresolved + 1 allocation` 的构成即此）。它在热路径的调用点：`captureFailure` 参数 :529（冷路径）与 `liveCount` getter（P011ParticleM2Field.ets :116 `get liveCount()`——被页面查询，非子步内）。子步内主路径不调 `totals()`。**处理**：登记为已知冷分配（COLD_DIAGNOSTIC_COMPAT 同类），主门禁判定时归入 legacy registry 口径（§2b 判据边界），不因它把 ZERO_ALLOCATION 判 FAIL。

## 2. 门禁接入方案（选 (b)：独立子探针，不动主门禁判定）

**方向选择理由**：(a) 扩展主分析器识别「开关保护调用」需要分析器理解 `if(d!==null)`/`d?.` 守卫与 `work.poolProgress` 的 null 判定链（需跨过程数据流），实现复杂度高且引入误判面（把「守卫后的真分配」也误放行）；(b) 独立子探针只需静态扫描三个诊断模块的源文件 + 主门禁把「未解析调用按被调模块归属」分流，改动小、判据清晰、与 T116 §4 的「诊断开销独立披露」语义一致。

### 2.1 子探针 `probe_p011_particle_diagnostic_gate.cjs`（新文件，test/）

**判定对象**：`P011PoolProgressDiagnostic.ets`、`P011FieldColumnEvidenceV2.ets`、`P011MergeExactIndex.ets` 三个模块（**诊断模块群**）。

**判据 1（诊断 OFF 零可达分配）**：对三个模块全量 AST 扫描分配节点（new/object-literal/array-literal/closure/template-string/string-concat/exception，与主门禁 :30-34 同一分类器），要求：
- (i) 模块内**方法体**无分配节点（允许：构造器 `constructor`、`exportEvidence`/`rowJson`/`getPoolProgressDiagnostic` 等导出/序列化方法——用方法名白名单**显式列举**并逐个标注「冷/导出/构造」）；
- (ii) 调用侧（P011ParticleHot.ets 等已列文件）对诊断模块方法的每个调用点，其守卫必须匹配三种模式之一：`if(d!==null){...}` 包裹 / 可选链 `d?.m(...)` / `if(x!==null)try{...}`（captureFailure 模式）——**守卫模式匹配失败**即 FAIL（这是防止未来开发者写出「守卫外裸调用诊断方法」的回归闸）。

**判据 2（诊断 ON 分配计入独立分母，口径披露）**：子探针输出 `diagnosticOnAllocationSites`（模块内方法体分配点 + totals() 对象字面量等跨模块冷点），每处标注 `{site, kind, path: 'COLD_EXPORT'|'COLD_FAILURE_ONLY'|'VIEW_CONSTRUCT'|'CTOR'}`。**这些不计入主门禁分母**——主门禁的 ZERO_ALLOCATION 判定保持「生产热路径零分配」原语义不变。

**误报/漏报分析**：
- 误报面 1（守卫模式匹配过严）：开发者重构守卫写法（如先 `const d=...; if(d)` 简写）会 FAIL——这是**故意的**（强制守卫模式统一），FAIL 信息提示回归守卫写法；
- 误报面 2（方法名白名单遗漏）：新增导出方法不在白名单 → FAIL 并提示登记——同样是故意收紧；
- 漏报面 1（诊断模块内的间接调用）：诊断模块 A 调模块 B 的方法（如 evidence 调 `particleMassHAt`）——子探针扫描诊断模块时其调用按「模块内已知零分配方法」解析（判据 1 已覆盖其方法体），跨到生产模块（pool.hotCanonicalBuffers 等返回预分配数组引用）零分配——已在源码核对；
- 漏报面 2（守卫内的真分配）：`if(d!==null){ const arr=[]; d.input(arr); }` 这类守卫内分配**子探针不查**（判据 1 只查模块方法体）——由主门禁覆盖不了（未解析）……**补丁**：判据 1 (ii) 的守卫块内再做一次分配节点扫描（守卫块内的 object-literal/new 即 FAIL，可选链调用本身不算）。本补丁覆盖 T154 新增的 32 处守卫块，漏报面闭合。

### 2.2 主门禁扩展（最小改动，不改判定语义）

`probe_p011_particle_hot_graph.cjs` 只改一处——`audit()` 输出新增 `unresolvedDisposition` 字段：把 `unresolvedCalls` 按 `text` 中被调方法名归属到 `{module: 'P011PoolProgressDiagnostic'|'P011FieldColumnEvidenceV2'|'P011MergeExactIndex'|'UNKNOWN', guarded: boolean}`。**`status` 计算不变**：`remaining = legacyHot + allocations + unresolved`（保持 35 原语义）——但 `disposition` 文案改为 `STATIC_GATE_UNRESOLVED_ROUTED_TO_DIAGNOSTIC_SUBGATE; MAIN_GATE_VERDICT_ZERO_ALLOCATION_FOR_PRODUCTION_HOT_PATH`（当且仅当：`unresolvedCalls` 全部 guarded 且归属三诊断模块、`hotAllocationSites` 全部为 `totals()` 类冷点）。**ZERO_ALLOCATION/NOT_ZERO_ALLOCATION 判定本身一字不动**——「主门禁 FAIL」在子探针 PASS + disposition 路由成立时，由合并报告（§2.3）解释为「生产热路径零分配已证」，但**主门禁 exitCode 仍为 2**（不删门禁不加白名单红线：门禁保持独立 FAIL 直到分析器补齐模块清单——见 §5 终态）。

### 2.3 合并报告格式（tester/T155 消费）

```json
{
  "mainGate": { "probe": "probe_p011_particle_hot_graph.cjs",
    "status": "NOT_ZERO_ALLOCATION", "remainingSiteCount": 35,
    "productionHotPathZeroAllocation": true,
    "unresolvedDisposition": "ROUTED_TO_DIAGNOSTIC_SUBGATE",
    "hotAllocationColdSites": ["P011ParticleField.totals()@367 (COLD: failure-capture param / page query)"] },
  "diagnosticSubGate": { "probe": "probe_p011_particle_diagnostic_gate.cjs",
    "status": "ZERO_ALLOCATION_WHEN_OFF",
    "guardViolations": 0, "moduleMethodAllocations": 0,
    "onPathColdSites": [ {"site": "...", "path": "COLD_EXPORT" } ] }
}
```

两门禁判据边界：主门禁=「生产热路径（非诊断模块可达集）零分配」；子门禁=「诊断 OFF 零可达分配 + 守卫模式合规 + 诊断 ON 冷点披露」。**两个 PASS 不互相替代**：子门禁 FAIL（如守卫被改坏）时即使主门禁 disposition 路由成立也不能宣称零分配。

## 3. baseline 2 处遗留路由 + mergeIndex 生产点

| 遗留 | static-gate.json 位置 | 归属 | 路由 |
|---|---|---|---|
| T149 captureFailure 调用 | baseline :523 → 当前 :525 | T149 V2 诊断（冷路径，仅 FIELD_COLUMN 失败时执行） | **T146 修复链已含**：T146 归因报告 §5 修复后 FIELD_COLUMN 命中本身消失（谓词对齐 F1），captureFailure 调用点转为不可达（谓词不再失败）→ 类 B 自动降级为「守卫合规冷点」；**不单独立卡**。若 manager 采 F2/F3 方案，谓词仍在，调用点仍冷——两种结局都归子探针判据 1 (ii) 的守卫模式检查覆盖。 |
| T123 mergeIndex.find 调用 | baseline :936 → 当前 :949 | T123 合并索引（常开生产路径，非诊断） | **归主门禁补模块清单**：这是唯一「生产路径未解析」——主探针 `files` 数组需追加 `P011MergeExactIndex.ets`（该模块方法体零分配：Int32Array 原地操作 + 可选链诊断调用；追加后 :949 变为已解析边，`unresolvedCalls` 降为 33 全诊断守卫点）。**此项属于门禁扩展一并处理**（T155 实施卡范围），不回退 T123/T135 链路（T135 已交付关闭，其 index 调用语义无缺陷——本路由只是把模块加入分析范围）。 |

## 4. T155 准入判据（何时从「门禁 FAIL」转「可验证」）

T155（audit 负载诊断独立测试）可从看评论转开工具卡，当且仅当：

1. **C1（子探针就位）**：`probe_p011_particle_diagnostic_gate.cjs` 按 §2.1 落地且对当前 HEAD 输出 `ZERO_ALLOCATION_WHEN_OFF`（守卫违例=0、模块方法体分配=0）；
2. **C2（主门禁 disposition 路由就位）**：`probe_p011_particle_hot_graph.cjs` 输出 `unresolvedDisposition=ROUTED_TO_DIAGNOSTIC_SUBGATE`（追加 MergeExactIndex 进 files 后 33 处全 guarded）；
3. **C3（合闸证据）**：合并报告（§2.3 格式）作为 T155 的静态部分证据归档（`test/reports/t155/`）；
4. **C4（T154 语义未动）**：`docs/developer-T154-pool-progress-v3.md` 描述的诊断行为（POOL_PROGRESS V3 三源 audit、排序/索引/树/range/copy 计数、单调时钟、首超长原池字节）与 HEAD 实现一致（T155 的运行时验证对象）。

**T155 验证内容**（从「门禁 FAIL」解除阻塞后的运行时判据，tester 侧）：T154 报告 §「离线实测」表的三组夹具复跑（N=16/192/16λ）、开关 OFF 时物理等价（`run_t154_pool_progress_units.cjs` 8/8 已含）、合并报告静态部分。**T155 不再验证「生产 Hot 零分配」**（该命题由 C1/C2 静态证据 + T146 修复链后的回归集共同承担）。

## 5. 终态与限制（明示）

- 本方案**不宣称**「T154 已证明生产 Hot 零分配」——T154 报告的诚实边界保留：静态门禁未过是事实，本卡提供的是「未解析=模块缺口」的证据与补齐路径。
- 主门禁 exitCode 在子探针 PASS 后仍为 2（门禁原语义保留），直到 `files` 清单补齐三诊断模块（终态：分析器覆盖全模块后 unresolved=0，主门禁独立回归 ZERO_ALLOCATION——这是长期终态，本卡不强制一步到位）。
- 诊断 ON 的 `checkLong` 视图构造（`new Uint8Array(buffer)`）在子探针披露为 `VIEW_CONSTRUCT` 冷点——它不是数据复制，但 ArkTS 运行时是否为视图构造分配头对象**未实测**，T155 真机阶段可实测一次（首超长窗口单次），不作为静态判据。

## 6. 变更记录

- 2026-10-27 v1：初版。34 处逐处判定（32+1 诊断守卫短路、1 生产模块缺口）、方案 (b) 独立子探针设计（含误报/漏报分析与守卫块补丁）、baseline 路由（T146 链含 T149 capture、门禁扩展含 T123 index）、T155 准入判据 C1-C4。
