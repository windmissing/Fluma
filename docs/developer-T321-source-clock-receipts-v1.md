# T321 · 参考明水 GAP 停源反馈与源时钟收据交付

2026-10-30，developer；依据 `architecture-T315-clear-water-longpress-attribution-v1.md`。不使用真机。交付范围仅 C F64 参考线，不冒 GPU M1 效果。

## 已实施（对应 T315 工程合同逐项）

1. **有效长按终态**：`ClearWaterSourceController` 新增 `sourceState`（0 空闲/1 供给中/2 已停止）与 `stopReason`；GAP/UP/CANCEL/BACKGROUND/时钟异常全部显式终态，重复无 pointer 维护性 stop 不抹掉已锁存的 GAP/UP 终态（KIND_SWITCH/RESET 等管理性停止仍执行原 generation 递增，不改原 stop 语义）。GAP 清 pointer 原策略不变；跨 GAP 不续旧代、不补跑、1/30s 上限不放宽（`raw threshold remains 1/30` 用例原值 33333334ns delta 无钳值）。恢复需新 DOWN，DOWN 后第一 tick 仍为零差 tick，与原 controller 逐位等价（见回归）。
2. **用户可见停源反馈**：`feedbackMessage` 区分 GAP/UP/CANCEL/BACKGROUND/时钟域变化/倒钟/非法钟，均提示"请重新按下"；页面 `ClearWaterDemo` 新增独立反馈行（与 CURRENT 状态行分离），并明示 `configuredWaterRate`（实际 rateWater=1）与"墙钟按住时长不等于有效供水时长"，不冒 8 units/s 或 30s 有效供水。
3. **固定有界源时钟收据**：新增 `P011ClearWaterSourceTrace`——512 行最近环 + 64 行首边界，32 列含原 rawNS/delta（换算前的差值）、unit/domain、pointer 前后、gesture、input/run generation、sample/terminal/feedback 计数、sourceCommit、requested/admitted/rejected、GAP 相关原始值；固定可复用 packet，record 后不引用可变时钟/请求对象。丢失显式化：环覆盖 `overwritten`、边界 `boundaryDropped`、超长元数据 `metadataDropped`、`incomplete` 汇总。冷导出为带 checksum 的单 JSON（18,432 标量固定），覆盖声明 `RECENT_WINDOW_PLUS_FIRST_BOUNDARIES_NOT_FULL_RUN`，不冒完整 run。
4. **观察不影响权威**：trace 是只读旁路，每操作后 engine 权威 SHA 与旧 controller（`eee20b3` 基线）逐位一致（ON/OFF 双对照 60+ 步操作序列）；观察关闭时 sequence=0。`P011ClearWaterEngine` 仅新增只读 `currentSourceCommit` getter，不构成第二权威。

## 非机证据（共 61 项，设备 NOT_RUN 如实登记）

- `node test/t321/units.cjs` **14/0**：T315 原 NS 双窗复算（有效 0.033359337s、rate1、两笔 commit、GAP 终态行原值）、GAP 后无追赶/无自动恢复/新 DOWN 语义、连续 pointer 身份（MOVE 不产生量、错误 UP 忽略、正确 UP 显式）、四类时钟关闭均保原 raw/domain 且需新 DOWN、1/30 阈值不钳、terminal 记实际 admitted/rejected 与成功计数（非墙钟时长）、重复采样不发请求、STOP/CANCEL 与 rate 配置精确、观察 ON/OFF 权威逐位一致、固定环/首边界显式丢失、超长元数据显式丢证据且冷导出不改权威、页面终态独立显示+显式冷导出+无 Canvas。
- 既有套件原样重跑（仅重定向证据目录）：`run_t226_clear_water_c_units` **14/0**、`t309/regression` **26/0**、`t309/selftest` critical lanes **7/0**。
- 生产工程 touch 强制重编：CompileArkTS 实跑通过（仅既有无关 WARN），PackageHap 通过；SignHap 00303116 已知签名配置失败如实保留，未手签不冒有效签包。
- 隔离项目冷生成重建（`test/t321/generate.cjs` → `.codex/local-test/t321-final`）：全量构建 CompileArkTS+PackageHap 通过，unsigned HAP SHA256 `d3939a4d5056656afa680e1f8df9c04bf1064afd3f094d4bcca4e9803ce3416c`；final 项目上四套件再次全绿。

## 边界与未做

- 不改水物理/光学；不跨 GAP 恢复（若产品要求长墙钟恢复策略，归 designer 另立，本卡不隐含）。
- 非机源量/停源/新 DOWN/观测等价已交付；设备端真实触摸/调度停源观察、观感与 GPU M1 效果均 NOT_RUN，归后续 tester/effector。
- 生成器产物只重定向证据目录，未改 t309/t248/t270 原始脚本本体。

## 修改文件

| 文件 | 用途 |
|---|---|
| `entry/src/main/ets/physics/clearwater/P011ClearWaterSourceTrace.ets` | 新增：固定 512+64×32 源时钟收据环与冷导出 |
| `entry/src/main/ets/physics/clearwater/P011ClearWaterSourceController.ets` | 终态/反馈/观察旁路；GAP 与 1/30s 语义不动 |
| `entry/src/main/ets/physics/clearwater/P011ClearWaterEngine.ets` | 只读 currentSourceCommit getter（注释标 T321） |
| `entry/src/main/ets/pages/ClearWaterDemo.ets` | 独立反馈行+速率明示+显式收据导出；仅 XComponent+shader |
| `test/t321/{units,generate,run_existing}.cjs`、`test/reports/t321/*` | 单测/生成/回归与证据归档 |
