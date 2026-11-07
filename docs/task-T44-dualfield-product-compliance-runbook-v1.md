# T44 双场产品与合规 run 复测执行案

日期：2026-10-25（Asia/Shanghai）  
责任：effector  
任务卡：6addc1e3e4b06ee357290762  
设备：VYG-AL00（hdc 62T0225B10010020，API 26 / OpenHarmony 7.0）  
构建基线：worktree 隔离构建 @ `d941e7f`（= T43 `13c7690` + T45 决策文档），entry/src 树指纹 `0fbeec00b7df5084`  
结论状态：**待执行**（本执行案为采集与判据； verdict 以回执报告为准）

## 0. 范围与边界（执行前必读）

1. **T44 不可改产品代码**（用户 2026-10-25 指令，已登记 `docs/effect-cases.md` 变更记录）：本卡只做采集、验证、出报告；凡遇产品缺陷只记录并移交，另派 developer 修复，自身不改 `entry/src` 业务代码。`tools/`、`test/` 下的采集脚本、分析器与回执/用例文档不受此限。
2. **code=5 规则**（T46 归因结论）：运行中若观测到 `TILT_PLANNING_BLOCKED`（ledger `context.commitStatus='blocked'/'rolled_back'` 或 `audit.errorCode`），该步/该 run 记 invalid（fail-closed），不计主观 PASS/FAIL 分，交 T47/tester 补证分流；**不得与容量准入拒绝混为一种 code5 统计**（容量拒绝另有 `rejectedReason`/CAPACITY 语义，分别记录）。
3. **peak 协议未生效**：`peakProtocolEffective=false`，宣布权在 manager；本卡只出双算数据与状态分流，不宣布生效、不单独放行。
4. **endpoint 协议双口径并报**：协议 v1 已 APPROVED 但实现未经 manager 确认适用范围；新口径（`M=U+P`、末 1s 每子步）与 legacy 口径并列披露，任一单口径不单独放行。
5. 真机与 T42 串行规则沿用：同机不并发操作；本执行案不占 T57/T52 的设备窗口。

## 1. 构建与隔离

- **隔离原因**：并行会话有未提交/进行中的改动（T48 材质导出、T49 新管线渲染、T50/T51 空容器 DEMO），在主工作树构建会得到混合态包，混淆 T44 归因。故用 `git worktree` 在 `D:/hmprojects/t44-build` 检出 `d941e7f` 后构建，主工作树产品代码零改动。
- **基线语义**：`d941e7f` = T43（设备采集链装配：`P011DeviceRunAssembly`/`P011EndpointCaptureSession`/worker 迁入 `ets/workers`/同 run 停摇/双场 route 入口）+ T45 决策文档。T44 验证的正是 T43 装配在设备上的可达性。
- 产物：`entry-default-s142-signed.hap`（签名同 s141 密钥），安装前核对 HAP SHA256 与构建日志；树指纹与上述不一致时重新构建并重算。

## 2. 设备操作入口（T43 交付，cold-start/onNewWant 均经既有单一物理输入链）

| 操作 | 命令 | 说明 |
|---|---|---|
| 冷启动 | `hdc shell "aa start -a EntryAbility -b com.example.helloworld --ps rendermode 4 --ps testmode true --ps p011particle 1 --ps p011capture 1 --ps wssnapshot false [--ps shake 0|1]"` | `rendermode 4`=MODE_WATERSAND（默认 1=细沙 CA，不带则物理场不步进）；`testmode=true` 传感器不接管、重力全由输入构造；`p011particle 1` 武装真实粒子 route；`p011capture 1` 装配 endpoint 采集 session + observation store；`wssnapshot=false` 保持 GLES 双场渲染（true 会强制 Canvas） |
| 同 run 起摇/停摇 | `hdc shell "aa start -a EntryAbility -b com.example.helloworld --ps shake <0..1>"` | 运行中再发 `aa start` 走 `onNewWant` → shake-only 信封（仅 shake 单参，`ohos.*` 平台元数据自动过滤）→ 既有面板 setter → `field.setShakeInput`；不重置 run/初值，不重新冷启动 |
| 触摸注沙 | `hdc shell "uitest uiInput swipe <x1> <y1> <x2> <y2> <ms>"` | 盒内 Pan 拖动 → `routeGestureSand` → `injectGestureSand`（8 unit/s 速率积分、±2 列三角核、H/L 0.5）；全宽 swipe（沿用 T11 标定 100→1200）保证落入盒区 |
| 离开页面（正式收尾） | `hdc shell "uitest uiInput keyEvent Back"`（备选 `aa force-stop`） | `aboutToDisappear` → store.close → worker `closed` 回执 → hilog `P011_OBS_STORE_CLOSED`（records/admissions/status=COMPLETE）。R0 先验证哪条路径能触发 |
| 拉取 store | `hdc file recv <filePath> <本地>` | filePath 来自 hilog `P011_OBS_STORE`；同目录 `-admissions.ndjson` 为原始准入侧流 |

关键 hilog 事件：`WS_EVENT particle_route`（route 武装结果）、`WS_EVENT shake_effective`（含 boundary：SHAKE_OFF 实际 elapsed 与 off+8000/末 1s 窗）、`WS_EVENT gesture_sand`（逐笔 admittedH/L/rejectedReason）、`WS_EVENT shake_lift`、`WS_METRIC`（250ms 抽样）、`WS_FRAME`（帧分段耗时）、`P011_OBS_STORE`、`P011_OBS_STORE_CLOSED`、`P011_RASTER_LAYOUT`（盒区坐标）。

逐子步账本（NDJSON，schema v7）字段覆盖 endpoint/peak 全部必需项：`context.endpointSampleElapsedMs`、`context.endpointPhaseOccurrenceId`、`context.shakeOn/OffEffectiveElapsedMs`、`context.peakWindow*`、`context.sourceInjectedH/L`、`context.sourceEpoch`、`context.particleRouteActive/Id`、`context.commitStatus/rollbackReason`、`stateAfter.{bh,bl,mh,ml,suspH,suspL}`、`particles`（post-commit 粒子质量摘要）、`audit.{heavy,light}TotalBefore`、`audit.conservation/errorCode`。

## 3. 场景矩阵

| 臂 | 目的 | 时序 | 时长 |
|---|---|---|---|
| R0 | 冒烟与装配验证：route/capture/worker/store/退出路径全通 | 冷启动 shake=0 → 10s 观测 → 退出页面 → 等 CLOSED → 拉 store | ~40s |
| R1 | 真实粒子落床累积（触摸注入 → 飞落 → 床层累积） | 冷启动 shake=0 → 5 次全宽 swipe（间隔截图）→ 20s 静置 → 截图 → 退出 | ~60s |
| R2 | 同 run 停摇 endpoint 合规 run | 冷启动 shake=1 → 10s → `--ps shake 0` → ≥10s 静置（覆盖 off+8000 边界与末 1s 窗）→ 退出 → 等 CLOSED | ~50s |
| R3 | TC-WS-08 全时序：静置 3s → 摇晃 3s → 静置 8s，peak 双报 + endpoint 末窗同 run | 冷启动 shake=0 → 3s → `--ps shake 1`（t_on）→ 3s → `--ps shake 0`（t_off）→ ≥10s → 退出 → 等 CLOSED | ~55s |

时序纪律：动作与连拍交错（先动作后连拍会漏拍动态渐变）；每臂开始时 `hilog -r` 清缓冲，臂末 `hilog -x` 落盘；USB 弹窗（240,620）在每次冷启动后点击关闭；防熄屏 `power-shell timeout -o 600000`，收尾恢复。

## 4. 验收锚点与判据

### A. 装配与身份（硬闸门，失败即 BLOCKED 不算产品分）

| # | 判据 | 来源 | 通过条件 |
|---|---|---|---|
| A1 | 粒子 route 武装 | `WS_EVENT particle_route` + ledger `context.particleRouteActive` | ok=true/active=true；全 ledger 记录该字段为 true |
| A2 | 采集 session 装配 | `P011_OBS_STORE`（ready 后输出 filePath/admissionFilePath） | 事件存在且 filePath 可 pull；`endpointClockId` 非 null |
| A3 | endpoint 身份契约 | ledger `context.endpointPhaseOccurrenceId`/`endpointCommittedOrdinal` + `WS_METRIC.metricCommitKey` | 必登字段存在（缺失→`NOT_IMPLEMENTED` 分流，不冒充）；metricCommitKey 非 null 样本占比披露（对照 T11 的 0/603） |
| A4 | run 身份一致 | store 事件 runId == ledger `context.runId` == admissions runId | 全一致 |

### B. 真实粒子落床累积（R1）

| # | 判据 | 通过条件 |
|---|---|---|
| B1 | 注入准入 | 每笔 `gesture_sand` 有 admittedH/admittedL；rejectedReason 分类统计（容量拒绝与 code5 分列） |
| B2 | 外源账本 | ledger `sourceInjectedH/L` 累计 == gesture_sand admittedH/L 合计（逐位一致，T11 六项实证同判据） |
| B3 | 期望恒等 | `E = VH0+VL0+IH+IL`，`VH0/VL0` 取首条记录 `audit.heavy/lightTotalBefore`（准入前身份） |
| B4 | 落床累积 | 注入后 `D=Σ(bh+mh)+Σ(bl+ml)` 相对注入前增长，且最终增量≈累计 admitted（允许驻留 P/U 披露差值） |
| B5 | 分相守恒 | 每已提交子步 `abs(D_H+U_H+P_H−E_H)≤1e-9`（L 同理）；`audit.conservation` 全 true |
| B6 | 主观锚点 | 截图人工审看：粒子可见飞落、不穿模、松手停止、床面连续累积（EC-TT 系列锚点；主观分只在报告记录，不作为协议 PASS） |

### C. endpoint 合规（R2/R3，协议 `endpoint-protocol-P011-terminal-nondeposited-v1`）

- `t_off` = `WS_EVENT shake_effective.boundary.shakeOffEffectiveElapsedMs`，须与 ledger `context.shakeOffEffectiveElapsedMs` 一致；`t_e` = off 后满 8.0s 的最后一个已提交子步；末 1s 窗 `[t_off+7000, t_off+8000]`。
- 逐子步 `M(k)=U(k)+P(k)`（`U`=ΣsuspH+suspL；`P`=`particles.massH+massL`，post-commit 同子步）；`R=E(k)`；判定 `M(k) ≤ 0.05×R+1e-9`。
- 窗资格：`sourceEpoch` 恒定、无 reset/invert/route/mode 变化、`context.shakeInput==0`、elapsed 覆盖完整 1s；不满足→`INCOMPLETE(ENDPOINT_NOT_QUIET)`，不外推不插值。
- 输出：`endpointFraction`（终点值）、`windowPeakFraction`、`massWindowPass`、`endpointStatus`（PASS/FAIL/NOT_IMPLEMENTED/INVALID_LEDGER/INVALID_BASELINE/INVALID_DENOMINATOR/INCOMPLETE）；legacy 双值 `legacyEndpointFraction=U/A_hist`（`A_hist=D+U`）与代码实测 `suspFraction` 并报。
- 滞留诊断：终点 `state=2` 接触待准入粒子计数/质量披露；逐粒年龄字段不存在→`NOT_OBSERVABLE`，不宣称"长期不落已排除"。

### D. peak 双报（R3，协议 `peak-protocol-P011-ws08-peak-nondeposited-v1`）

- `t_on` = ledger `context.shakeOnEffectiveElapsedMs`；`W=[t_on, t_on+3000ms)` 左闭右开；成员=W 内实际消费该 shake-on 输入的成功提交子步。
- `peakNonDepositedFraction = max_{k∈W} M(k)/E(k)`；`peakCommitKey`=达到 max 的最早提交 `(runId,sequence,frameId,substepId)`；分项在峰值提交同时披露（H/L 分相峰不可加）。
- 分档：≥0.90→10 / ≥0.75→8 / ≥0.50→6（及格）/ ≥0.35→4 / ≥0.20→2 / <0.20→0；及格比较 `M(peakCommitKey) ≥ 0.5×E(peakCommitKey)`，不放容差。
- 双算：legacy 三值（`legacyPeakSuspReported`=现行实现全序列 max、`legacyPeakHistoricalReference`=同 W `U/(D+U)`、`legacyPeakLiveFormula`=同 W `U/max(D+U+P,EPS)`）+ `sameReferenceSuspOnlyPeak`=同 W `U/E` + `OBSERVED_SAMPLED_MAX`（WS_METRIC 250ms 样本下界）；`baselineNonDepositedFraction=M0/E` 与 `peakNetIncreaseFraction` 披露。
- 判定顺序：硬闸门（守恒/库存/容量/guard/预算/有限值/账本对齐）→ 窗口绑定与资格 → 字段完整性 → 账本/基线/分母 → 窗口资格 → 效果峰值；硬闸门失败只留原始候选峰值诊断，不签峰值 PASS。

### E. 帧健康与 store 完整性

- `WS_FRAME`：`frameDeltaMs`/`seg.physMs/renderMs/presentMs` 统计；无 >1s 间隙；max 帧时分段归因（对照 T42 R8 口径）。
- `P011_OBS_STORE_CLOSED`：`status=COMPLETE`、records/admissions 与本地文件行数一致；失配→`OBS_STORE_INCOMPLETE`，该臂证据作废重跑；强杀无 closed 不冒称完整（fsync 前缀可作诊断并披露）。

## 5. 采集与分析工具

- `tools/t44_dual_field_run.ps1`：四臂采集（前台执行；后台 PowerShell 长脚本会被杀）。输出目录 `test/artifacts/t44-dualfield-product-run-20261025/`。
- `tools/analyze_t44.py`：解析 hilog dump + NDJSON，按 §4 输出逐项 verdict、双算表、守恒统计、帧健康、资格披露；编码探测 BOM 优先（不盲猜 utf-16）。
- 截图为人工审看证据（effector Read 审看并记录），不替代账本判据。

## 6. 资格与披露（写入回执）

- endpoint/peak 采集资格：逐子步账本来自 store NDJSON（STREAMED），WS_METRIC 仅旁证（OBSERVED_SAMPLED_MAX 下界）；hilog 单条 ~4096B 截断，长行证据以 NDJSON 为准。
- `peakProtocolEffective=false`（不宣布）；endpoint 协议实现适用范围未经 manager 确认（双口径并报）。
- 若 R1/R2/R3 出现 code5 或容量拒绝：按 §0.2 分流记录，该 run 相关项记 invalid，交 T47；不移除尖峰、不重选窗口求绿。
- 构建身份：commit `d941e7f`、树指纹 `0fbeec00b7df5084`、HAP `entry-default-s142-signed.hap` 及其 SHA256；设备 62T0225B10010020；每臂起止时间戳。

## 7. 失败与重试

- 臂级失败（装配不可达/ store 不收尾/ 事件缺失）：记 BLOCKED 并列缺失项，修脚本或环境后重跑该臂；重新冷启动=新 run（runId 变），不跨臂拼接。
- run 级失败（守恒越限/硬闸门/窗资格）：如实 FAIL，按协议状态码记录，不重选窗口、不删尖峰。
- 连续两臂同因失败：暂停并报告，不盲目加时或改判据。
