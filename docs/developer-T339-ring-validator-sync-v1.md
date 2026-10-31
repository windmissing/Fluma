# T339 · 环扩容 ArkTS 校验同步交付 v1

日期：2026-10-31。责任：developer。任务卡：滴答 `6ae5622ee4b0f69bc58244ae`。
依据：`docs/test-report-T337-fence-flush-device-v1.md`（阻塞根因源码级定位）+ T336 回执（8e2662a 遗漏说明）+ manager 2026-10-31 裁定（校验器同步为 T336 实施遗漏，随 T336 语义归属，最小改动）。

## 1. 修复内容

### 1.1 校验常量同步（核心一行）

`entry/src/main/ets/physics/GpuBridgeTimelineCold.ets` page() 校验：

- `p.capacity!==1024` → **`p.capacity!==4096`**
- `p.count>1024` → **`p.count>4096`**

与 native 侧 `bridge_timeline.h` 的 `capacity=4096`（T336/T335 §4 扩容）对齐。fail-closed 语义不变（越界仍拒、坏页不落盘）。

### 1.2 pageBudget 核实（无需放大，已注明）

`PAGE_BUDGET 16384`：一页=header+4 行 JSON，最坏情形（全部 u64 字段 20 位）约 1.7KB << 16384——**无需放大**，推导写入校验器注释。

### 1.3 随环扩容必须同步的两个预算（实施中发现，语义对位）

- **页数上限**：do-while `out.pages<=256` → **`<=1024`**（256 页×4 行=1024 行是旧环上限；4096 行需 1024 页，不改则满环导出被 MISSING_ROWS 误杀）；
- **EXPORT_BUDGET**：`1048576` → **`4194304`**（4MB）：4096 行最坏流 ~2.15MB（行 JSON 最坏 ~440B×4096+页开销 1024×~350B），1MB 会在满环时误触；4MB 留 2× 余量。1024 环时代最坏 ~450KB<1MB 成立，预算与环容量同步缩放属同一语义。

### 1.4 测试链样本同步

`test/run_t306_timeline_units.cjs` 样本页 `capacity:1024` → `4096`；满环档 `1024` 行 → `4096` 行（groups 断言同移）。t324/cold_units.cjs 链式复用同源自动适配。

## 2. 防漂移四例（T337 建议，语义对位说明）

任务卡原文「V1 1024 页构造样本 VALID」与修复矛盾（修复后 1024 必须被拒——该拒绝正是修复本身）。语义对位：卡面 V1/V2 指**样本字典版本**（stageRevision），非环容量。实施四例（`test/t339/anti_drift.cjs`）：

| # | 样本 | 期望 | 结果 |
|---|---|---|---|
| 1 | T295_STAGE23_V1 字典 + capacity 4096 | VALID（COMPLETE） | PASS |
| 2 | T324_STAGE41_V1 字典 + capacity 4096 | VALID（COMPLETE） | PASS |
| 3 | **残留 capacity 1024**（T337 阻塞根因形态） | INVALID（INVALID_PAGE） | PASS |
| 4 | 越界 capacity/count 4097 | INVALID（INVALID_PAGE） | PASS |

## 3. 非机回归（设备 NOT_RUN）

- t306 timeline units：**20/20**（修复前 13/7——8 失败全部是旧 1024 样本被正确拒绝）
- t324 cold units：**23/23**（含 T324 41 字典三例）
- T339 防漂移：**4/4**
- hvigor CompileArkTS+PackageHap 绿（SignHap 00303116 已知如实保留）

## 4. 红线遵守

- 只动 GpuBridgeTimelineCold.ets 校验常量与两个随环预算 + t306 样本同步 + 新增 t339 测试；
- native 侧零接触（bridge_timeline.h 4096 已正确）；
- 分页导出协议与 fail-closed 语义不变（越界仍拒、坏页不落盘）；
- T336 的 flush/环改动零接触。

## 5. 交付后路径

T337 无需新卡——tester 沿原 K1-K5 判据换包含本修复的包重测（真机队列首位不变）。包由执行 tester 按 T151 流程补签。

## 6. 变更记录

- 2026-10-31 v1：初版。校验 1024→4096 两处+pageBudget 核实无需放大+页数上限 256→1024+EXPORT_BUDGET 1MB→4MB（随环缩放语义）+防漂移四例+t306 样本同步；非机 20/20+23/23+4/4+构建绿。
