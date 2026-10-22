# P-011 配置机制回执报告 v1（tester 独立复核）

- **日期**：2026-10-22；责任：tester。对应任务【P-011·配置机制回执】（滴答 6ad96080）。
- **性质**：参数外置配置机制（10e4838，§27）的 tester 侧独立复核。**阶段一 CLOSED 评估的最后一项输入。**
- **依据**：algorithm-P011-user-threshold-config-v1.md §7 五项证据清单 + priority.md 23:06 修订 + architecture-P011.md §27。
- **复核方式**：Node 通道全量重跑（实际产品 .ets 经 bundled TS 编译执行，非模型复写）+ 产品代码逐路径审查（WaterSandField configure/applyThresholdConfigFromParams/onConfigurationChanged、SandCanvas apply/persist/restore、EntryAbility load/drop）+ D-1 资产链复跑。真机抽验未做（登记为未覆盖项，见 §7）。

## 1. 配置机制回执（§7 清单第 2 项）

### 1.1 U1~U13 复核（Node 通道全绿，本次实测）

`test/run_p011_input_units.cjs`：**117/117 PASS**（U1~U13 + 本次新增 U14 + 既有回归）；`test/run_p011_planning_units.cjs`：**100/100 PASS**。U 用例逐项核验：

| 用例 | 覆盖 §7 清单项 | 复核结论 |
|---|---|---|
| U1 默认武装 | 26/20 来源正确 | dual-user-configured + factory-recommended + recommendationVersion v1 + configRevision=1，首次 configure 即武装 ✅ |
| U2 零停止角 | 合法边界 | stop=0.1（deadZone=0）合法，slopeStop=tan(0.1°) 精确派生 ✅（设计"0≤stop"开放域兑现） |
| U3 三十起动角 | 合法边界 | start=30 合法，slopeStart=tan(30°)=硬上限值（允许按设计，非法域只是 >30）✅ |
| U4 反序/相等拒绝 | 完整 pair 校验+无副作用 | (20,26)/(25,25) throw；active config、params 快照、材料 bh/bl 逐列不变（拒绝无副作用）✅ |
| U5 非 finite/缺字段/legacy 带对 | 校验完备 | NaN 拒绝；legacy 无 pair 合法；legacy 带 pair（歧义）拒绝 ✅ |
| U6 运行中换对 | 配置原子生效 | CONFIGURATION_RESET=1 + configRevision=2，状态机清空，新阈值下子步重新评估（清理≠物理 stop）✅ |
| U7 reset/生命周期 | 生命周期 | 重新 initialize 保留 pair 与 source=user（config 域独立于材料域）✅ |
| U8 非法持久化恢复 | 持久化损坏恢复 | >30 拒绝且保留前一合法配置 ✅ |
| U9 元数据失配 | 身份分离 | metadata pair ≠ active config → mismatch 旗标（arm 入口拒启动），匹配后清除 ✅ |
| U10 revision 单调+preview 守卫 | 原子生效 | revision 单调递增（同值重配=身份事件）；planning telemetry 携带 revision；preview-commit revision 失配冻结子步（结构性断言）✅ |
| U11 legacy 显式模式 | legacy 锚点 | legacy-single：双角机关 off、activeThresholdConfig=null、executedThresholds null；240 步锚点等价（p011_d1_legacyTrajectoryEquivalence240Steps）复用 D-1 资产 PASS ✅ |
| U12 双角默认不启 correction | 路线隔离 | route=null → correctionRouteActive=false；显式绑定 relax-correction-weighted-v1 才武装 ✅ |
| U13 预算/守恒不变 | 安全回归 | 合法换对材料总量 Δ<1e-9，后续子步预算限额照常执行 ✅ |

### 1.2 产品代码逐路径审查（UI 应用/恢复/持久化，代码审查路径）

| 设计要求 | 实现验证 |
|---|---|
| 草稿→应用，中间态不进物理层 | `setThetaStart/StopDraft` 只改草稿；`applyThresholdPairDraft` 才走 configure ✅ |
| 拒绝整个 pair、不 clamp/不交换 | configure 三段校验（enum→pair presence→finite→成对 derive）全部 throw 于**状态复制之前**（代码顺序核验：8 处 throw 全部位于 copy block 之前），拒绝无副作用 ✅ |
| 恢复推荐显式提交 26/20 | `resetThresholdToRecommended` 走同一 applyThresholdPair 事务，source 切回 factory-recommended，不隐式触发 correction ✅ |
| 持久化失败不显示"已保存" | persist 失败仅 hilog.warn"applied, not saved"，UI 不谎报 ✅ |
| 损坏持久化丢弃+会话默认 | EntryAbility 载入时 finite+0≤stop<start≤30 校验，非法 drop+warn，26/20 会话默认保持 ✅ |
| 持久化防抖 | 500ms debounce 写 preferences ✅ |
| 版本迁移/首次初始化 | 无历史键→26/20 factory-recommended（授权的默认初始化，非掩盖非法值）✅ |

### 1.3 TEST/EXPERIENCE 共用与方向翻转

- 模式切换不丢 pair：`initField()` 每次重建都从 AppStorage 恢复持久化 pair 后再 configure（SandCanvas 1404–1419 行逐行核验）；resetField→initField 同路径。TEST/EXPERIENCE 共用同一设置 ✅（U7 field 级 + 代码路径双确认）。
- 方向翻转参数不变：forceinvert/testmode want 不触碰 theta pair（EntryAbility 无相关写入）✅。

## 2. 三态语义核验（§7 tester 清单第 10 项）

- 既有 GAP-1 `p011_hysteresis_threeStateTransitionFunction` 用**协议 pair（28/22.911）**覆盖：r==start 等号触发 START（`>=`）、r==stop 等号触发 STOP（`<=`）、两阈值之间双向保持（滞回）。PASS。
- **本次补齐 U14**（tester 侧增量，已入 Node 通道 PASS）：`u27_14_threeStateSemanticsWithUserPair` 用**用户 pair（26/20）**重复上述语义断言——证明 configure→activeThresholdConfig→派生 slope 喂给与协议 pair **同一比较器链**（无 UI 0.1° 步进舍入导致的 off-by-epsilon）；等号边界（r==slopeStart 触发、r==slopeStop 退出、±1e-9 两侧滞回保持）全部钉死。
- 结论：等号语义=start 用 `>=`（等号入 active）、stop 用 `<=`（等号出 active），两阈值之间保持前态。**与协议原文一致，且对用户 pair 成立。**

## 3. 安全回归（§7 清单第 3 项）

- U13：合法换对材料总量不变（<1e-9）、预算限额照常。
- 全配置 tan30 硬上限：deriveHysteresisThresholds 的 thetaHardMaxDeg 固定 30（applyThresholdConfigFromParams 硬编码，用户不可达）；guard/库存/容量/有限值门槛在 117+100 用例中零失败。
- deadZone 不变量重定向：dual 模式下 `bottomCreepDeadZone < thetaStopDeg`（legacy 保持 angleDeg）——stop 角度调低时死区约束自动收紧，不产生新越界通道。
- 配置变化 ΔV=0：onConfigurationChanged 只清状态机/镜像/预留缓存/epoch 递增，不触碰材料、预算 used、守恒累计量（代码逐行核验）。
- 既有回归：input 117/0 + planning 100/0（本次实测，比 developer 交付时 115+98 多出的用例为 10e4838 自带的 player/identity 用例与其他会话增量，全部 PASS）。

## 4. 路线隔离（§7 清单第 4 项）

- **用户配置不启用 correction（U12）**：默认 dual-user-configured + route=null → correctionRouteActive=false；唯一武装条件=显式 route 绑定，阈值非 null 本身不是开关。三路 apply 路由（`p011_d1_routeIsolation_threeWayApplyRouting`）PASS。
- **legacy 模式锚点（U11，复用 D-1 资产）**：本次实测 D-1 链全绿——`run_p011_obs3_d1_verification.cjs` 重出证据（240/240 okSteps，四场景路由正确）→ checker `overall=PASS`（C1 轨迹等价+C2 四路分流+C3 correction 遥测闭合，`d1_check_receipt_final.json`）。legacy 单角行为与 5e60f2d 基线位精确等价保持。
- metadata 不再驱动物理阈值：setNaturalObservationRunMetadata 身份-only（物理联动已删除），失配旗标+拒绝启动（U9）——§27 红线"probe/遥测永不设物理阈值"落实。

## 5. 与设计稿的偏差/发现（不阻塞 CLOSED，登记处置）

| # | 发现 | 严重度 | 处置建议 |
|---|---|---|---|
| N-1 | UI 错误文案"停止角必须大于死区(**1°**)"把默认死区值 1° 硬编码进文案；实际死区可配置（thick 预设等），若用户配置 stop≤自定义死区，文案数字可能不实 | 低（文案精度） | 文案改为"停止角必须大于底部蠕动死区"或动态插值实际值；不阻塞 |
| N-2 | U10 preview-commit revision 守卫为**结构性断言**（revision 快照+等值检查），单线程下无法表达真实交错复现 | 低（测试表达力限制，非产品缺陷） | 已在用例注释诚实登记；真机抽验可补 |
| N-3 | 真机抽验（s107 HAP UI 面板操作/持久化重启恢复）未执行 | 中（覆盖缺口，非缺陷证据） | developer 已完成设备端 smoke（首 record configRevision=1+executedThresholds={26,20,factory-recommended}）；UI 手动抽验建议 CLOSED 评估时注明"代码审查路径+设备 smoke"组合证据 |
| N-4 | ohosTest 侧 U 用例为同步副本（54 个 it），与 src/test 的 110 个不完全对齐 | 低 | 按既有惯例两份 List 注册同步；建议后续对齐 |

## 6. §7 五项证据清单对照总结

| 清单项 | 状态 |
|---|---|
| 既有执行链回执 | 保留（10e4838 前已交付；D-1 链本次复跑 PASS） |
| **配置机制回执** | **本报告 §1：U1~U13 全绿复跑 + 代码逐路径审查通过 + U14 补齐** |
| **安全回归** | **本报告 §3：各配置 tan30/guard/预算/守恒/库存/容量/有限值不变** |
| **状态与路线** | **本报告 §2+§4：三态语义（含等号）对用户 pair 成立；新 epoch 清理不伪报 stop（U6）；null 元数据不关停双角（U1 独立武装）；用户配置不启用 correction（U12）；legacy 显式模式锚点等价（U11+D-1 PASS）** |
| 阶段一其余 B/C | 维持原口径（不在本回执范围） |

**tester 结论：配置机制回执五项证据齐备，参数外置机制按 §7 覆盖清单验证通过。可提交 manager 进行阶段一 CLOSED 评估；N-1~N-4 登记项不构成阻塞。**

## 7. 产物与版本

- 测试增量：`entry/src/test/WaterSandFieldTest.ets` 新增 u27_14（+56 行，纯测试增量，产品代码零改动）
- 本次实测通道输出：input 117/0、planning 100/0；`test/reports/d1_check_receipt_final.json`（D-1 链 PASS）
- 未覆盖：真机 UI 手动抽验（N-3，s107 HAP 已出包可随时补）
