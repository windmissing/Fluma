# T30 客观端到端集成验证回执

- 任务卡：`6add769ce4b017ab33e71bd7`（【T01后续·客观端到端】tester：恢复后集成验证与原通道回归）
- 日期：2026-10-25。角色：tester。模式：纯离线（编译 + 单测/分析），未使用真机，未改任何实现或判据。

## 1. 版本与构建身份

| 项 | 值 |
| :--- | :--- |
| 验证时 HEAD | `187e327`（docs: register T37 completion and T04 decision handoff） |
| 关键前置提交 | T02 = `adbcc67`（M2 生产粒子集成 + 账本 v7）；T08 = `41b90bf`（peak 采样）；T36 = `c1ae739`；T20 = `283644c` |
| 工作区状态 | 含并行会话未提交改动（286 个文件有变更）；本卡只读验证，未写入业务文件 |
| ArkTS 编译 | `default@CompileArkTS` **BUILD SUCCESSFUL**（exit 0，CompileArkTS 20.8s / 总 32.5s），日志 `test/reports/t30-run/t30-build.log` |
| T01 隔离证据复用 | `docs/dev-report-P011-D1-R3-S1-S4-v1.md` + `test/reports/p011-d1-r3-s4/`（M1 隔离 43/0、36000 步零分配、隔离编译 PASS），按卡要求直接复用，未重跑 |

## 2. 执行项与结果

原通道回归（当前工作区实跑，证据在 `test/reports/t30-run/`）：

| 通道 | 套件 | 结果 |
| :--- | :--- | :--- |
| planning | `run_p011_planning_units` | 106/106 PASS |
| input | `run_p011_input_units` | 124/124 PASS |
| gesture 注入 | `run_p011_gesture_inject_units` | 18/18 PASS |
| gesture 时钟（T36） | `run_p011_gesture_clock_units` | 7/7 PASS |
| advection | `run_p011_advection_units` | 9/9 PASS |
| S3 moisture | `run_p011_s3_moisture_units` | 14/14 PASS |
| S4 deep creep | `run_p011_s4_deep_creep_units` | 10/10 PASS |
| render | `run_p012_render_units` | 82/82 PASS |
| tracer | `run_p012_tracer_units` | 9/9 PASS |
| particle field（冷） | `run_p011_particle_field_units` | 33/33 PASS（验证中途曾 32/33，见缺陷 D-1，并行会话已在工作区修复） |
| 热接口（T01 M1） | hot/patch/sink/s4 四套件 | 11+12+17+6 = 46/46 PASS |
| M2 粒子单元 | `run_p011_particle_m2_units` | 13/13 PASS |
| M2 生产事务 | `run_p011_particle_m2_transactions` | 14/14 PASS |
| endpoint/peak Python | `test_p011_endpoint_check` + `test_p011_peak_check` | 38/38 OK |
| endpoint node | `run_p011_endpoint_units` | 9/9 PASS |
| peak node | `run_p011_peak_units` | 8/8 PASS |
| T20 视觉 | `run_p012_particle_visual_units` | 8/8 PASS |

粒子↔高度场守恒/预算/生命周期（M2 生产事务 14 例逐条 PASS）：两相守恒（gesture+shake 实子步）、真实 reserve/guard 与逐端点退款、满池 merge 无质量损失、不兼容池 degrade 无二次扣账、v7 账本与粒子摘要表征提交后池、C OFF 武装逐位保全。

失败恢复：`publish failure restores pool, field and real budgets` PASS；热接口 stale/busy/溢出/回滚类用例全绿；`plan mismatch commits nothing` PASS。

镜像边界（用户裁定 x′=cols−1−x）：有效域守恒——M2 单元 #6/#10（x′=127−x 精确、质量保留）；非法镜像原子拒绝——M2 事务 #6（`PARTICLE_INVERT_DOMAIN` 在首次写入前抛出，pool/waterTop 逐值不变）。

## 3. 缺陷与处置（按责任卡反馈，tester 未放宽判据、未代改）

- **D-1（已闭环，责任方 T20 developer `6adda102e4b017ab33e90fbf`）**：`run_p011_particle_field_units.cjs` 用例 `route exact two-condition predicate; default off; no field dependency` 的结构断言 `!/^import\s/m.test(source)` 在已提交 HEAD `187e327` 上失败——T20 提交 `283644c` 向 `P011ParticleField.ets` 添加了 `import { P011ParticleRenderProjection }`（`copyRenderProjection` 方法所需）。该断言是 D1 时代"物理场不依赖渲染"的隔离规则，与 T20 有意引入的渲染投影导出冲突。同套件前 4 条路由谓词断言本身 PASS。该失败在已提交 HEAD 上复现（两个涉事文件相对 HEAD 无差异时仍失败），非未提交改动所致。**闭环**：T20 于 15:07 提交 `9bc19ed`（fix(p012): preserve dependency-free particle pool）移除该 import 并将 `copyRenderProjection` 改为接收原始 typed array 参数，渲染依赖从物理文件消除；复跑 **33/33 PASS**（证据 `test/reports/t30-run/run_p011_particle_field_units.json`）。

## 4. 结论

**PASS**：当前稳定提交构建身份可确认（CompileArkTS BUILD SUCCESSFUL）；原通道回归全绿（particle field 套件在 D-1 修复后 33/33）；粒子↔高度场守恒/预算/生命周期、失败恢复、镜像边界（有效域守恒 + 非法原子拒绝 + 失败前后状态不变）均按既定契约通过。真机采集与端到端效果归 T31/effector，本卡不占设备队列。

## 5. 复跑

```bash
export JAVA_HOME="C:/Program Files/Huawei/DevEco Studio/jbr"
"/c/Program Files/nodejs/node.exe" "C:/Program Files/Huawei/DevEco Studio/tools/hvigor/bin/hvigorw.js" \
  --no-daemon --mode module -p module=entry@default -p product=default default@CompileArkTS
node test/run_p011_particle_m2_units.cjs && node test/run_p011_particle_m2_transactions.cjs
```

- 2026-10-25，v1：首次发布。验证版本 HEAD `187e327` + 工作区（并行未提交改动在场，已如实登记）；缺陷 D-1 归 T20，已由其提交 `9bc19ed` 闭环（33/33）。
