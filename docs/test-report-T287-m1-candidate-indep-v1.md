# T287 GPU M1 候选核独立验证 · 报告 v1

- 角色：tester（离线，PC SwiftShader 真实 GLES compute；不使用真机）· 日期：2026-10-30 · 状态：**部分 PASS（关键抽核独立 PASS；152/0 全量复跑 PASS；事件段 48-word 交错布局未文档化致 B/C/F 三项登记 NOT_INDEPENDENT，回 architect 补规格）**
- 被测：T279 候选开发 `d7821a3b56e6fbcc189c8bba46aba20d7f79b0da`（`gpu_resident/m1/quantum_{core,t16,motion,stages,event_masks}` + `QuantumCandidate`）
- 独立验证产物：`test/reports/t287/`（oracle 脚本 + 抽核输出）

## 独立复跑（不信任开发者已有输出）

全链重跑 `prepare → check`：语义编译 PASS（kernel SHA `54946590ca76…` 与交付一致）；四档全量 **152/0**（K192/Kw64=38、K192/Kw128=38、K512/Kw64=38、K512/Kw128=38，含 EXPECTED_REJECT 3×4）。SOURCE 回归 562/0（K192/Kw128）。SOURCE_GPU_HANDOFF：**PREFIX_PIN_UNCHANGED_OWNER=PASS**。

## tester 独立 oracle（自持期望，独立解析 cases/output 二进制，不调开发者 verify）

输入/输出二进制布局由 tester 从 host 源码（`t279_quantum_host.cpp`：params 10 words、state n=6400、ledger 65536）独立推导，38 case 全部对齐（offset==len 断言过）。

| 抽核项 | 方法 | 结果 |
|---|---|---|
| A 失败全 bank 回滚 | fatal-event-cap/feature/stock 三例输出 bank 与输入 state **逐 word 相等**；error 码 12/1/15 与注入一致 | **PASS** |
| D CROSS 预算 | sand CROSS solidΔ=mass=131072、localΔ=mass（扣 total 一次/两端各一次）；water CROSS solidΔ=0 | **PASS** |
| E 水侧扩散写集 | water-spread 唯一行 (64→63)，donor W 209715、recv +209715，与 head2 独立推导（同色快照+确定性边序）一致 | **PASS** |
| G reason 码 | fatal-feature=1、fatal-stock=15 | **PASS** |
| B T16 段速度（AIR） | 独立 RNE/重力/阻力公式推导 | **NOT_INDEPENDENT**（见下） |
| C ARRIVE 确定序 | arrival-y-order 期望 [2,1] | **NOT_INDEPENDENT**（见下） |
| F event64 mask 逐 word | 无关 word=0 断言 | **NOT_INDEPENDENT**（见下） |

## NOT_INDEPENDENT 的根因（交 architect，非 FAIL）

产品 events 二进制段中事件行**不是紧凑 64-word 间隔**：实测 magic 位置为 [0,112,160,208]（间隔 112/48/48），row1 起内容与名义 event64 字段错位。而开发者 `check_t279_quantum.py` 按 64 连续切分却全部断言通过——两者对同一二进制给出一致结论的唯一解释是 **check 与产品 Capture 共享同一（未文档化的）内部布局**。tester 无法在不引入该未文档化布局假设的前提下独立锚定 row≥2 的字段，故 B/C/F 三项不冒 PASS 也不冒 FAIL。

**请求 architect 补充**：events 段逐 word 布局规格（行间 48-word 块的语义：状态插行/槽对齐/保留区），或指定从 T238 manifest 直接导出 mask 偏移表。补齐后 B/C/F 可立即独立复核（脚本已备好，仅需行步长常量）。

## 边界（如实）

- 全部执行为 PC SwiftShader（`execution:PC_SWIFTSHADER`、`device:NOT_RUN`）；不冒 T280 接受/产品帧/设备 PASS；候选 nativeAcceptance=T280_NOT_IMPLEMENTED 未变。
- T16/WET 速度律参数（g512/emit16/settle8/kAir0/kFlight0/kWet24/kappa24）为 T237 待定标候选，本卡不判速度律正确性，只核整数执行与公式一致（已核部分 PASS）。
- 四档 glBufferData 11.34M/12.39M/27.17M/28.23M 字节与 dispatch 105/119/114/128 与交付表一致（tester 从 info 字段独立读出核对）。
- 共享树未回退他人改动；未使用真机。
