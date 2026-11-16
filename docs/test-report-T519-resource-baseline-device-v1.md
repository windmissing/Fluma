# T519 · GPU 资源账与缓存基线真机相位采样 tester 报告 v1

- 卡：`6afa5284e4b0711ec48fdd9b`（tester，P5，真机，设备串行 62T0225B10010020）
- 日期：2026-11-16
- 被测包：**T502-m1-delivery**（含 T501 ledger），HAP `362a3cb66f452d1f…`（本地逐位核验 MATCH），
  包内 libgrsm1.so `bd2fab0d4a432e15…`；`bm clean -d` 后冷启，PID 42442
- 证据：`test/reports/t491/t519/`（mem 四相位、vm 两相位、execution/timeline manifest、UI dump、截图）

## 签包/来源核对（卡面要求）→ PASS

| 项 | 值 |
|---|---|
| HAP SHA256 | `362a3cb66f452d1f4e35b7c29be4c0599f268aa30c66e20d55e7d33d0a5b0e4a`（本地==声明） |
| 包内 libgrsm1.so | `bd2fab0d4a432e15ebccfad867c3d89cf30f37188a51da5ba57ab3915b494af6` |
| actual route | `--pb grsm1 true` → GpuResidentM1；timeline identity 逐字回显 `packageSha256=362a3cb6…, pid=42442` |
| timeline 身份源 | `dcf105d…+T492-overlay-v1` 内嵌声明（与 T505 §1 记录一致） |

## 相位采样（系统可达指标）

| 相位 | VmRSS (kB) | hidumper Total Pss 类 (kB) | 动作 |
|---|---|---|---|
| P1 冷启后 | 373,840 | 1,590,410（Total 页，含共享） | `bm clean` → 冷启 12s |
| P2 新空场后 | （见 vm-after-reset.txt） | 1,538,879 | 新空场 click，GPU_WORK_PENDING |
| P4 导出后 | 396,424 | 368,027 | boot+timeline 导出完成（释放后回落） |

相位锚点（execution 档 stages）：RESOURCES_READY=2、SESSION_HANDOFF=2、ROOT_ACCEPT=3107、
EGL_SWAP_RETURN=3107（Swap 闭环仍成立）、FIRST_REVOKE=1、PAGE_PULSE=21463、WAIT_ENTER=140299。
timeline 导出：4096 行 missing=0 duplicates=0（state INCOMPLETE reason=NATIVE_DROPPED——ring 满
属有界合同，不冒完整）。

## 逻辑请求/host 容量/系统物理/driver 四分账（卡面要求）

| 账域 | 本轮可达性 | 结果 |
|---|---|---|
| 逻辑请求（T501 resourceState） | **NOT_MEASURED** | napi `resourceState`/`cacheStorageState` 已导出（index.d.ts:43/45）但页面无任何调用/UI 按钮——真机无触达通道 |
| host 容量（T501） | **NOT_MEASURED** | 同上 |
| 系统物理（RSS/PSS/GPU） | PASS | hidumper/VmRSS 四相位采样在档（上表） |
| driver 未知域 | NOT_MEASURED（合同明确） | driverAccounting=DRIVER_NOT_MEASURED |

同代前后活跃/退休账与 tmp 残留"有来源"要求：resourceState 未接线 → 前后账差分 NOT_MEASURED；
tmp 残留只能由 cacheStorageState 给出 → 同 NOT_MEASURED。按卡面"无设备指标记 NOT_MEASURED
回 architect"。

## 未达相位如实记录（卡面：不能只采空场就判长稳）

- SOURCE/Swap 后相位：本轮 GPU 状态域仍非 CURRENT（GPU_WORK_PENDING/GAP 族，与 T510 同根）——
  **未达 SOURCE/有效 Swap 呈现相位，长稳/泄漏判据无输入**，不采空场冒长稳。
- resize/缓存失败注入相位：页面无触发入口（product 通道），NOT_MEASURED。
- M2 包（ead86b52…）未装：设备串行，本轮 M1 优先；M2 相位采样待同法接续（不混入本报告）。

## 判定

- 签包/安装/ELF/route 核对：**PASS**
- 系统物理指标相位采样：**PASS**（RSS/PSS 四相位可溯源）
- T501/T502 逻辑账真机采样：**NOT_MEASURED**（napi 已导出但页面未消费——接口未接线，
  前置=developer 页面加 resourceState/cacheStorageState 导出按钮后复测，回 architect 分流）
- 长稳：**NOT_MEASURED**（CURRENT 域未达成，不采空场冒长稳）
