# T914 · PC DM2/R1 SOURCE 候选等价与时标复采（effector）

2026-11-24。按 [T913 开发回执](../../docs/developer-T913-v1.md) 末节，在开发集显 PC 用分族冻结输入交错复采。全程 Vulkan GPU 驻留、shader 渲染，无 Canvas 回退，无真机。

- **输入**：`.codex/local-test/t894-model-source.u32le`（SHA-256 `3580ccb1a4199f41708bd9a790f8997f77c4af87dac1743b08c636264e22f55a`，全臂冻结）
- **GPU**：Intel UHD 630（`8086:3e98`，driver `31.0.101.2140`，vendor 32902/device 16024/driverVersion 1656924，BOOT 行带真实 deviceUUID/driverUUID/LUID）
- **工件**：六组 PE 全部与 T913 文档 SHA 一致（DM2 baseline `c709d183…`、v1 `ef8ecd4e…`、split-v2 `2b13c952…`；R1 baseline `5839b002…`、v1 `72030900…`、split-v2 `e15d2581…`）
- **矩阵**：3 变体 × 4 开关组合（plain / `--source-timestamps-detail` / `--source-prewarm` / 两者）× 2 族 = 24 臂（`test/reports/t914/runs/`，argv 记录于 `manifest.json`）
- **驱动脚本**：`test/t914/run_t914.py`（stdout 直写文件，无管道停摆）；**判定器**：`test/t914/check_t914.py`（T905 sand() 的版本感知扩展：roleSetSHA 取自各臂 BOOT 行，dispatch 数按 sourceSchedule 裁定，接受根跳过退役链）

## 结论总览

| 项 | 结果 |
|---|---|
| 基线/v1 全臂结构门（同 fence、安全退役、时序、poll、query 绑定） | **22/22 PASS**（`gate-results.json`） |
| DM2 typed 250ms 接受 | **baseline 与 v1 各 1 个接受 SOURCE 根**（集显首次；v1 余量 28ms） |
| R1 typed 接受 | 未达（全臂 250–263ms；v1 GPU 段总和已降至 233ms 但加 host 记录 ~50–85ms 仍超界） |
| split-v2 复采 | **受阻**：两族 8/8 在 `owner.Open` 返回 VkResult=-3（VK_ERROR_INITIALIZATION_FAILED），任何 stdout 之前 |
| 同输入原字等价 | **结构性等价 PASS；逐字等价不可达**（模型包络盖章墙钟 `now`→inputSHA 每跑必变），需冻结时间重放，归 developer |

## ① DM2/R1 基线与 v1 交错复采（16 臂 + 6 链式尝试）

- **同 fence / 安全退役**：全部拒绝根 `RETIRE vkResult=0`、state `RETIRED`、fence/nonce/plan/generation 全绑定；时间戳臂一律 `LATE_RETIRED_DIAGNOSTIC` 后非阻塞读，DM2 27/27、R1 41/41 点位全可用（36bit/83.333ns）。
- **逐槽原 tick**（`slot-tick-tables.json`，detail 臂同队列 GPU tick 差）：
  - DM2 v1 vs baseline：SLOT_9（147,968 字比较→64 lane）**30.26→1.05ms（−96.5%）**；SLOT_8 73.4→64.5ms（−12%）；派发总和 **246.1→209.3ms（−15%）**。
  - R1 v1 vs baseline：SLOT_8/13（两个 APPLY→按列掩码）**146.7→67.5 / 144.2→64.2ms（−54%/−55%）**；SLOT_14（比较）30.5→0.99ms（−96.8%）；派发总和 **426.3→233.1ms（−45.3%）**。
- **prewarm/detail 开关**：prewarm 在贴线臂上最多省 ~12ms（dm2-baseline 263→252）；detail 的查询开销 <2ms。flag 行为与 T913 描述一致（`descriptorPrewarmed`、`SOURCE_DESCRIPTOR_PREWARM` 行 vkResult=0）。
- **typed 接受**：
  - `dm2-baseline-detail-prewarm`：stop=+244.5ms（余 5ms）、`TAIL_ACCEPTED`、`tailExecution/Expected/Barrier=12/12/12`、`RETIRED` commit=1 quotaBalanced=true——**baseline 角色集在集显上的首个 typed 接受根**。
  - `dm2-v1-detail-prewarm`：stop=+221.6ms（余 28ms）——**v1 候选集首个 typed 接受根**，接受余量比 baseline 大 5.6 倍。
  - 接受后链（`--steps 1` 续 QUANTUM）尝试 6 次：SOURCE 窗全部 250–262ms 再拒——接受是 250ms 边缘事件，链式续跑在集显上不可稳定复现；ROOT/FULL 与 WORK/Present 续链未采到，列缺项。
- **R1**：v1 把 GPU 派发压到 233ms（−45%），但 host 记录段 50–85ms（reset 为主）叠加后 288ms+ 仍超 250ms 界；两臂结构一致，首因均为 `GRANT_DEADLINE`。

## ② split-v2 复采受阻（原始错误与复现）

- **复现**：DM2/R1 各 4/4（plain/detail/prewarm/组合）全部退出码 1、stdout **零字节**、stderr `<FAMILY> VULKAN_ROLE_PIPELINES VkResult=-3`——失败点在 `VulkanOwner::Open`（host.cpp 的 `RequireVk(owner.Open(...))`），任何日志行之前。
- **已排除**：模块文件 SHA/魔数逐个核过（48/56 个 spv 全匹配 roles.json，SPIR-V magic 全对）；exe 内嵌模块名与盘上一致（DM2 48/48）；新模块 capability/entry/LocalSize(64,1,1) 与旧模块一致；maxStorageBufferRange（134217728B）> arenaBytes；v1 同生成器同日构建同机通过。
- **未排除**：`CreateComputePipelines` 对三个 DM2 新角色（`role_00000380/381/382`，381 为 166KB）或 R1 七个新角色的驱动侧拒绝；需逐模块创建探针定位。
- **前置/接手**：T914 无法验证 v2 的 14/27 派发与 31/57 细点位。**交 developer**：用单模块逐个创建管线二分（或加驱动日志），修复合入后本卡判据（`check_t914.py` 已按 14/27+31/57 参数化）可直接复跑。
- 证据：`runs/manifest.json`（8 条 stderr 原文）、`runs/{dm2,r1}-split-v2-*.jsonl`（0 字节）。

## ③ 等价与差分

- **结构等价**：v1 与 baseline 在同一冻结输入、同一 GPU/驱动上 22/22 通过同构门（时序链、poll 纪律 ≤2ms、query 根绑定、退役链、1024B 热尾、250ms=deadline−grant 恒等）——候选未改变任何业务门与账目结构。
- **逐字等价不可达**：`ModelEnvelope(command, now)` 以墙钟入输入 → inputSHA 每跑必异（实测 dm2-baseline/v1 两接受根 `bde70e00…` vs `360e8bad…`，同文件同 nonce）。**逐字差分需要冻结时间/固定种子的重放通道**，与 T912 M1 同类缺口，归 developer 设计。
- **收益方向已证**（见①数字）：v1 的两处优化点（64-lane 比较、APPLY 掩码）恰好落在 T913 声明的角色上，其余槽位在噪声范围内。

## 复现入口（用户独显）

```powershell
$root = '.codex/local-test'
$input = "$root/t894-model-source.u32le"
# 24 臂矩阵
python test/t914/run_t914.py
# 单臂（示例：DM2 v1 + 两个开关）
& "$root/t913-dm2-t913-v1/pc_sand_dm2_host.exe" --modules "$root/t913-dm2-t913-v1/sand_assets" --source $input --source-timestamps-detail --source-prewarm
# 结构门
python test/t914/check_t914.py DM2 test/reports/t914/runs/dm2-v1-detail-prewarm.jsonl
```

独显上按同一入口复跑：预期 DM2 接受稳定、R1 接受转正、链式续跑（加 `--steps 1`、R1 加 `--present --witness <file>`）可采到 QUANTUM/WORK/Present。

## 缺项清单

| 缺项 | 原因 | 接手 |
|---|---|---|
| split-v2 全部复采（14/27、31/57、v2 角色标识） | owner.Open VkResult=-3 阻断（8/8） | developer |
| 接受根的归档原字差分（--archive） | 接受+归档未同时命中（8 次尝试全拒；250ms 边缘） | 独显复跑或重放通道 |
| QUANTUM/WORK/Present 续链 | 6 次 `--steps 1` 尝试 SOURCE 窗再拒 | 独显复跑 |
| 逐字等价 | 输入包络含墙钟 | developer（冻结时间重放设计） |

GPU 驻留、shader 渲染全程；迟退役时间戳仅作诊断，接受一律按原 250ms typed 门裁定。
