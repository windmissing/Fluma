# T188：单沙明水 A 段库存与源事务交付

原卡：`6ae0b709e4b0c3103bff77cb`。依据 `architecture-single-sand-clear-water-v1.md` §2–4/7/8、T122 算法交接与 T108 先行基线。不使用真机。仅交付 A 段，不声明整个单沙明水功能通过。

## 实现与所有权

新建 `entry/src/main/ets/physics/clearwater/` 下四模块：

- `P011ClearWaterTypes.ets`：独立 mode/route/source/row revision、配置、请求/回执、错误枚举、queue20/dedup56/event24 逐槽清单。候选物理参数必须显式提供并标为 `PENDING_CALIBRATION`。
- `P011ClearWaterState.ets`：独立 W/bh/mh 权威库存、S/eta/P/R 派生缓存、phase0 定容池；空槽栈、独立正 Uint32 ID/sourceId、递增 ID 活动链及在途列缓存。没有 legacy medium、M2 或悬沙入口。
- `P011ClearWaterSource.ets`：64 行队列、4096 行/run 幂等表、4096 行源事务事件；逐原 F64 位比较 payload（含 -0），冷端检查宿主 little endian。先拒旧 run，已有 key 查询先于当前输入/几何代检查；停止手势不撤销旧成功回执。
- `P011ClearWaterEngine.ets`：四份定容 state bank、独立 sourceBefore/stepBefore、reset、新 run/generation、显式 NS/MS 时钟窗口、停止输入屏障、固定 out 接口和冷端独立 snapshot/allocation ledger。

不修改旧 WaterSandField、旧 wet/dry/M2 容量、主页面、GPU 协议或渲染；没有页面启用入口。新文件通过编译探针显式交给真实 ArkTS 检查器，避免“未被页面引用所以漏检”。探针及 build-profile 临时注册在 finally 中恢复，不作为生产入口保留。

## 事务及数值边界

WATER 只向 floor(x) 的 W 写入，SAND 按 massPackage 拆包及真实尾包，birth 保留触点和 phase0/external-source 身份。q 受请求、显式 maxSourceVolume、本列 `C-S-P-W` 和沙空槽限制，禁止全场扩散、悬沙回退或将 rejected 计为准入。

提交前预检 dedup/event/ID/commit 空间；复制 sourceBefore/work，在 work 上写库存、池、ID/free/cache、重建派生量并审核。只有通过的候选发布 authority、成功账、源事件和冻结幂等回执。失败不推进成功序号或库存；两个 kind 独立成功计数。控制准入 q 与 RN 后实际 water/sand 差分分别保存。水正 q 若 W 位型完全未变，拒绝 NUMERIC_UNREPRESENTABLE；沙真实新粒子尾质量保持为权威库存，不因其派生归约贡献舍入而丢弃。

基准只在初始化/reset 建立，运行中 expected=initial+I_kind。保留 `drift>=0.001` 拒绝、分母 `max(expected,1e-9)` 和源真实差分绝对界 `1e-6`；W 非负、有限、单沙/cache 身份和 R<=256 严格检查，不钳零。N=128、pool 主档512。300/s、local28/s、tan30 常量保留，A 段不捏造 B 段预算/guard 已执行的证据。

`dt=0` 不消耗队列或改库存。正 dt 明确返回 `UNSUPPORTED_FEATURE / NOT_IMPLEMENTED_B_COUPLING`，保留最近接受 commit 和 failedAttempt；已有成功 source 不被这一失败撤回。A 没有可运行的半成品水流/粒子推进 writer。

## snapshot 与尚未实现部分

冷 snapshot 复制 authority/pool 全数组及标量、queue/dedup/events、环队列 head/ordinal、输入/几何/run/config、时钟窗口和失败尝试身份；调用者修改副本不影响运行库存。sourceBefore/work 等临时 scratch 不作为可恢复 authority。没有部分 restore 入口，也没有从旧 schema 的 waterTop 反推 W。

以下明确 **NOT_IMPLEMENTED**：B hydraulics/分段空气湿粒子/完整 step budget+guard+事件 lineage；C publication/渲染/输入控制器/页面；snapshot restore。源事务 event24 是 A 的源账事件，不冒充 B 的粒子七类事件、lineage 或双 plan 证明。

## 完整 A 数组资源清单

机器可读逐数组清单：`test/reports/t188/allocation-ledger.json`；字段 offset：`layout-manifest.json`。以 K512、queue64、dedup4096、event4096 实际分配计：

| 内容 | 字节 |
| --- | ---: |
| 四份 pool（每份 13 组原格式数组 + prev/next） | 137216 |
| 四份 W/bh/mh/S/eta/P/R | 28672 |
| 四份 auditMass | 4096 |
| queue 64×20×8 | 10240 |
| dedup 4096×56×8 | 1835008 |
| source event 4096×24×8 | 786432 |
| source request20/receipt56 scratch | 608 |
| 全池审核 marks | 512 |
| **A 持有的数组 payload** | **2802784** |
| 一份冷 snapshot 增量 | 2674176 |
| **A + 一份 snapshot 的数组 payload** | **5476960** |

构造时 endian probe 另有临时 4B backing buffer，不是持久表；DataView/TypedArray headers、对象、配置 JSON 克隆/字串、对齐、栈和 GC 存活时间取决于运行时，不把上述表当 RSS 上限。调用者同时持多份 snapshot 需逐份累加。没有缩减/驱逐 dedup。

B/C 尚未分配的 etaMedium、双 publication/pool/profile、bridge proof/native 副本、GPU staging/FBO、lineage/双 plan/诊断 ring 均不计入 A；完整功能应在后续实现时累计，不能把本表称为完整路线或设备内存 PASS。没有新 GPU/driver 分配，也没有测量或宣称满足整机内存/性能预算。

## 离线验证与构建

- `node test/run_t188_clear_water_source_units.cjs`：**26/26 PASS**。覆盖局部水/沙守恒、拆包尾包、P 预留、部分准入、phase0、双 kind 序号、停止后幂等、-0 冲突、queue64、4096 无驱逐、event/pool/ID/token 耗尽、ULP 无操作、水负值/缓存损坏、时钟单位/窗口、reset/snapshot 隔离、候选 audit 失败回滚、正 step 未实现边界、drift 等号拒绝、严格容量，以及热接口无 TypedArray/ArrayBuffer/DataView/Map/Set 构造或 slice。故障注入仅在 Node fixture 中替换 work.audit，不增加生产开关。
- 旧 `run_p011_gesture_inject_units.cjs`：**18/18 PASS**；旧 `run_t49_t51_demo_units.cjs`：**5/5 PASS**。
- `python test/compile_t188_clear_water_modules.py`：真实 `default@CompileArkTS` **BUILD SUCCESSFUL**，包括四个新模块。
- `hvigorw assembleHap --no-daemon`：native CMake/Ninja、CompileArkTS、PackageHap **通过**；最终 **SignHap 失败**，既有 `00303116`：签名 storePassword/keyPassword 长度不足32。没有修改签名配置/凭据，未声明完整签名构建成功。卡片要求的代码编译已通过。

证据在 `test/reports/t188/`：selftest、两项 regression、allocation/layout、compile/build 日志和 source/build manifest。unsigned HAP 记录是原主应用打包，不表示新 clear-water 页面已经集成。没有设备操作、端到端或视觉效果结论。
