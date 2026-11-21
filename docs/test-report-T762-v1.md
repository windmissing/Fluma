# T762 v1 · 目标 20 DM SOURCE 超期边与退役真机续证

日期：2026-11-21，effector（tester 兼，含部署采证），真机（011124264R000337）。
被测：T758 driver（receipt-T758-compile-v1：g9_dm_differential 990744B
buildId 652b030a、libgrs_dm_t647 sha 2aa71299、libc++ 配对——三件套设备
md5 重发后核对）。单次 K192/S40 原 SOURCE（dm22-out）。证据：
`test/reports/t491/t738/v8/`（dm22-final.jsonl 27 条含
T758_DM_SOURCE_TRACE_V1 原件、v8 判定）。

## 身份（ATTEMPT_IDENTITY schema 同票）

pid/processStartTick/driverMappedBuildId/run 7/owner 1/context 11 全齐
（BOOT 段 22 条 + SOURCE 段 5 条）。

## 超期边精确定位（本轮核心成果）

**T758_DM_SOURCE_TRACE_V1 逐 guard 原件**：

| guard | 结果 | 时点 ns | dispatch |
|---|---|---|---|
| **lastGuardSuccess** | PUMP_ENTRY_GUARD **PASS** | 97842143237812 | 12 |
| **firstGuardFailure** | PRE_EXEC_POLL_GUARD **DEADLINE** | 97842143239531 | 12 |

**两次 guard 仅差 1719 ns（≈1.7 微秒）**——第 12 次 dispatch 的
PRE_EXEC_POLL 在最后一次 PASS 后 49 微秒启动、1.7 微秒后过界。250ms
deadline 的**越界点被夹逼到单一 guard 迁移**。

## fence 与退役

- executionFence：created=true，polls=13069，lastWaitRaw=37147，
  **signaled=false**
- retirementFence：reusedExecution=true，polls=62，lastWaitRaw=37146，
  **signaled=true**（退役用执行 fence 复用）
- boot-work-tail.u32 再证 [0,6559303]（第 4 次）

## NOT_REACHED（如实）

1024B WORK 尾/typed proof/209715 Q20/commit+1/FIFO 消费——map 未返回
（PRE_EXEC_POLL 在 dispatch 12 过界），按合同"达到 map 才取"，T761
reader 不适用。

## 回执

- T754 归因**原字级确认**：250ms 窗口在 PUMP_ENTRY_GUARD(PASS)→
  PRE_EXEC_POLL_GUARD(FAIL) 1.7μs 间于 dispatch 12 越界
- 归 architect：窗口放宽/两段式 claim 决策（证据夹逼已完备）
- exitCode=6（DEADLINE）与 T751/DM DEADLINE=6 一致
