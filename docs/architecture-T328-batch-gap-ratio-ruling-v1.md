# T328 · 分片×GAP 配比裁定与 T324 合同修订 v1

日期：2026-10-31。责任：architect。任务卡：滴答 `6ae54054e4b069bbd882aab3`。
状态：**SINGLE_RECOMMENDATION / CONTRACT_AMENDMENT_READY / TWO_HANDOVERS_RESOLVED**。
依据：`test-report-T318-partial-submit-device-v1.md`（真机 41 阶段分账）、`test/reports/t318/`（原 ns 数据）、当前实现源码（84bf66e 的 `m1/full_transport.cpp`/`m1/run.cpp`/`app_bridge.cpp`）、`architecture-T324-gpu-partial-submit-contract-v1.md`（原合同）。纯设计零代码。

## 0. 裁定总览

| 项 | 结论 |
|---|---|
| 配比方案 | **唯一推荐方向 (a)：Pump 内多批续发循环**（每轮 Pump 循环调 ContinueSubmit 直至 Pump 级预算耗尽或封尾），13 批压缩到 1 轮 Pump ≈3.5ms，结构性消除 104-127ms 超窗 |
| GAP 判定 | **不豁免不放宽**（33ms 窗原样）——方向 (a) 使 root 在窗内封尾，无需动窗语义 |
| UINT32_MAX | **补显式 End(16)**（一行：SubmitQuantum lambda 尾 `return result;` 前加 `observed.End(uint32_t(result));`，与 SubmitSource :22 对称） |
| 手签口径 | **tester 补签合法且为现行唯一可行路径**；developer 保留 SignHap 00303116 登记义务；包 SHA 归档义务归**实际签包者**（本轮=tester），delivery.json 必须登记手签事实 |

## 1. 根因定量复核（T318 数据 × 源码互证）

**批间 ~8-9.9ms 间隔的来源不是 GL，是主循环的 wake 节奏**：
- `app_bridge.cpp:116`：主循环每次迭代 `wake.wait_for(l, 8ms)` ——每轮 Pump 前固定睡 8ms；
- `m1/run.cpp:24-29`：submitting 分支每次 Pump **只调一次** `ContinueSubmit()`（一次一批 10 op）；
- T318 实测批 wall：首批 1.712ms（含 staging/PinCandidate/BeginQuantum）、续批 0.098-0.228ms——**GL 提交本身极快**；
- 合成：每批一轮 Pump ≈ 8ms tick + 0.1-1.7ms 批 wall ≈ 8.1-9.9ms；130 op ÷ 10 op/批 = 13 批 → **104-127ms**，与 T318 两 run 的 Begin→Cancel 32.97/41.08ms（5 批后被 GAP 取消）外推一致。

**结论**：配比问题不在批预算（8ms 没用满，实际每批只花 0.1-0.2ms），在**「每 Pump 一批」的实现选择与「每 Pump 前睡 8ms」的循环节奏相乘**。这是 T324 合同 §2.3 的语义漏洞——合同写了「Run 层 continuation：未完 → OK_SUBMITTING → 下轮续」，把续批节奏绑死在 Pump 轮次上，未规定单轮 Pump 可发多批。

## 2. 四方向对照推算（裁定依据）

| 方向 | root 封尾总墙钟（推算） | vs 33ms 窗 | 风险/语义代价 | 判定 |
|---|---|---|---|---|
| **(a) Pump 内多批循环** | **~3.5ms**（13 批×0.15ms + 首批 1.7ms，单轮 Pump 内完成；预算护栏下最坏 2 轮 Pump ≈ 11.6ms） | ✓ 远低于窗 | 无语义代价：GAP 判定、anchor、Advance 全不动；批语义（nonce/序/取消）不变 | **✅ 唯一推荐** |
| (b) GAP 窗内提交豁免 | 104-127ms（不变） | 需豁免 | CLOCK_GAP 的语义是「检测主循环/UI 停摆」——豁免 BATCH_ACTIVE 等于把停摆检测盲区扩大到 127ms；真实卡顿（如线程被杀前兆）将被误判为正常提交期；违反「不钳 delta 隐藏长根」精神（长 root 的墙钟代价被窗语义吸收而非披露） | ❌ 拒绝 |
| (c) 提高 kBatchMaxOps（10→65） | 2 轮 Pump ≈ 16-20ms | ✓ 在窗内 | 单批 wall 随 op 数线性涨（0.15ms/op×65≈10ms 超 8ms 预算，预算检查又切回多批）——实际效果退化为 (a) 但粒度更差；且 65-op 批的取消粒度变粗（CancelSubmit 丢弃更多已发工作） | ❌ 被 (a) 支配 |
| (d) 缩减 130-op 表 | 不确定（需 shader 重构） | 视结果 | pass 表是 K192/W128 档物理量子工作的结构表达（T318 实测单 dispatch 最大 691µs，无浪费证据）；合并 op=designer 侧 shader 重构，风险大周期长，且 (a) 已把 13 批压进 3.5ms——表的规模不再是瓶颈 | ❌ 不必要 |

**唯一推荐：(a)**。理由：数学上一步到位（104-127ms→3.5ms，30 倍压缩）；语义上零代价（所有已真机验证的条款——nonce 一次消耗、单调 pass 序、BATCH_ACTIVE 取消、逐批 8ms——原样保留）；工程上改动最小（Run 层 submitting 分支从「调一次」变「循环调」，transport 层零改动）。

## 3. T324 合同修订（Amendment 1）

对 `architecture-T324-gpu-partial-submit-contract-v1.md` 的逐条变更：

### §2.3 修订：Run 层 continuation（Pump 内循环版）

```
原：submitting 分支每次 Pump 调一次 ContinueSubmit，OK_SUBMITTING 返回 → 本轮 Pump 结束。
改：submitting 分支在单次 Pump 内循环调 ContinueSubmit，直到下列任一：
    (i)  返回 OK（封尾完成）→ inflight=true; submitting=false; 退出循环（现行后续原样）；
    (ii) 返回 OK_SUBMITTING 且 Pump 内累计 wall < kPumpBudgetNs → 继续循环；
    (iii) 返回 OK_SUBMITTING 且 Pump 内累计 wall ≥ kPumpBudgetNs → 返回 OK，下轮 Pump 继续；
    (iv) 返回 BUSY/错误 → 现行语义原样（BUSY 不应发生在 submitting；错误 → Fail）。
```

**新常数**：`kPumpBudgetNs = 8,000,000`（8ms，与 kBatchBudgetNs 同值但独立声明——两者语义不同：批预算是单批护栏，Pump 预算是单轮 Pump 护栏，防御性上限）。Pump 级预算耗尽时的批次边界自然截断（ExecuteBatch 返回 OK_SUBMITTING 后检查），无半批风险。

**不变式增补**：
- INV-5（Pump 循环）：单轮 Pump 内的 ContinueSubmit 调用数无上限（受 Pump 预算约束），但**每批的 SubmitEnter/Exit 遥测照常逐批发出**（41 字典语义不变，carry=批号全局递增）；
- INV-6（GAP 检查位置）：GAP 检查在 PumpEnd 之后（app_bridge.cpp:187 现行位置）——Pump 内循环不绕过该检查，单轮 Pump ≤8ms 预算保证检查频率不低于现行；
- INV-7（首批仍在 SubmitQuantum 内）：首次 Submit（含全部前置校验+BeginQuantum+首批）结构不变——Pump 循环只作用于续批。

### §2.5 修订：总墙钟合同（配比后语义）

- 分片目标更新为「**单轮 Pump ≤8ms 且 root 封尾总墙钟（Begin→FenceEnter）≤ 一个 GAP 窗（33ms）**」——配比后的定量预期：K192/W128 档 13 批 ≈3.5ms（单轮 Pump）；
- 超窗取消语义不变：若未来更大 K 档（如 K512 表更长）使 Pump 循环仍超 33ms，CLOCK_GAP→CancelSubmit 原样触发，submitTotalWallNs/submitCancelled 分账原样披露——**不钳 delta 不藏长根**（方向 (b) 被拒的同一原则）；
- 遥测增补一个字段：`submitPumpWallNs`（本轮 Pump 内循环累计 wall），与 submitTotalWallNs（Begin 起总墙钟）分开——用于验证「单轮 Pump 封尾」预期。

### 与 T324 既有条款的兼容性

| 已验证条款 | 兼容性 |
|---|---|
| nonce 首批消耗（真机 PASS） | ✓ 不动（循环在 nonce 消耗之后） |
| 逐批 ≤8ms（真机 PASS 10/10） | ✓ 不动（ExecuteBatch 及其内部预算原样） |
| pass 序单调不重排（真机 PASS carry 0..49） | ✓ 不动（RunPassRange 序不变） |
| CancelSubmit 两路径（真机 BATCH_ACTIVE PASS） | ✓ 不动（循环中途任意点可取消，取消语义与批位置无关） |
| submitting≠inflight 分离 | ✓ 不动（OK 退出循环才 inflight） |
| 41 字典 | ✓ 不动（每批 Enter/Exit 照发；新增字段走 bridge status 非 timeline stage） |

### 实施/验证规格

**developer 实施卡（T329 建议）**：
1. `m1/run.cpp` submitting 分支改循环（§2.3 修订版，~10 行）+ `kPumpBudgetNs` 常数；
2. **UINT32_MAX 修复**（§4）：`full_transport.cpp` SubmitQuantum lambda 尾 `return result;` 前加 `observed.End(uint32_t(result));`（与 :22 SubmitSource 对称）——首批 exit code 变为真实 TransportCode（16=OK_SUBMITTING）；
3. `submitPumpWallNs` 遥测字段（bridge status 追加）；
4. 非机回归：沿 T317 五套替身（19/19×2、9/9、6/6、11/11、23/23）+ 新增用例——13 批单轮 Pump 封尾（断言 batchesIssued≥13 且单 Pump 内 FenceEnter 出现）、Pump 预算耗尽截断（注入慢批替身）、循环中途 CancelSubmit。

**tester 验证判据（T318 后续真机卡）**：
- J1：K192/W128 root 单轮 Pump 封尾（SubmitRootBegin→FenceEnter 同一 Pump 窗口内，submitPumpWallNs ≤8ms）；
- J2：FenceEnter/Exit ≥1 次（T318 为 0——封尾首次到达）；
- J3：root 转 inflight→Poll→accepted ≥1（双源同 token 链解锁前提）；
- J4：首批 exit code=16（UINT32_MAX 消失）；
- J5：GAP 判定仍有效：人为注入 >33ms 停顿（脚本时钟）→ CLOCK_GAP→CancelSubmit 原样触发（窗语义未被破坏的反向验证）。

## 4. 两项移交裁定

### 4.1 手签包交付口径

- **事实**：SignHap 00303116（storePassword/keyPassword<32 位）是 T311 时代登记的已知配置问题，developer 从未绕过（不冒签包，诚实）；tester 按 T151 隔离签包先例（Python 重打包绕 zip central directory 缺陷 + hap-sign-tool localSign + verify-app PASS）补签 3b428b0a 包——全流程留证在 `.codex/local-test/t318-sign/`。
- **裁定**：**tester 补签合法且为现行唯一可行路径，予以追认**。签包基础设施问题（SignHap 配置）独立登记，不在本卡修复范围（属工程构建配置，manager 可另行派小卡）；在其修复前，**需要真机验证的包一律由执行 tester 按 T151 流程补签**，交付物三件套：手签包 + sign/verify 原始输出 + 包 SHA256 归档。
- **SHA 归档义务**：归**实际签包者**（本轮=tester，T318 报告 §0 已做）；developer 的 delivery.json 若引用包 SHA，必须注明「无签包、SHA 指构建产物」或「tester 手签包 SHA」，**不得混写**（T323 已登记的引用错误教训）。

### 4.2 UINT32_MAX 首批 exit code

- **事实**：`SubmitQuantum` 的 lambda 有两处 return 路径——校验失败路径经 `}();observed.End(uint32_t(result));`（无此行——**该函数整个 lambda 结束后直接 `return result;`（:57-58），无 observed.End 调用**，与 SubmitSource :22 / ContinueSubmit :117 / Poll :144 三个函数均有显式 End 不对称）；SubmitScope 析构器兜底 `~SubmitScope(){if(!ended)observer.Emit(exit,root,UINT32_MAX);}` 发出 UINT32_MAX。
- **裁定**：**补显式 End**。修复=一行（§3 实施 2）；修复后首批 SubmitExit code=16（OK_SUBMITTING），与续批一致；失败路径（校验拒绝/BUSY）也获得真实码而非 UINT32_MAX——遥测消费者（T318 的分账工具按 exit code 区分首批/续批）不再需要特判 UINT32_MAX。**不改变任何 wall 计量**（Exit 的 ns 时戳由析构器/End 同点发出，差值为 0）。

## 5. 路由

- 本卡裁定+修订合同 → **developer 实施卡**（建议编号 T329：§3 实施 1-4，预计小卡）；
- 实施后 → **tester 真机卡**（T318 后续：J1-J5 判据，双源同 token 链首次有封尾前提）；
- 手签口径裁定 → manager 知悉（追认 tester 流程为真机验证的标准路径）；SignHap 配置修复独立登记不预派；
- 与 T313 归因稿无矛盾（T313 的「Pump 内 78-187ms」在分片+本修订后预期降至 ~3.5ms，方向一致）。

## 6. 变更记录

- 2026-10-31 v1：初版。配比四方向对照（(a) Pump 内多批循环唯一推荐，13 批 3.5ms vs 现行 104-127ms；(b) GAP 豁免拒绝——停摆检测盲区扩大违反不钳 delta 原则；(c) 被 (a) 支配；(d) 不必要）；T324 Amendment 1（§2.3 循环版+INV-5/6/7+§2.5 配比后目标+kPumpBudgetNs+submitPumpWallNs）；兼容性表（六项已验证条款全不动）；UINT32_MAX 补 End(16) 一行修复；手签口径追认+SHA 归档义务归实际签包者；J1-J5 验证判据。
