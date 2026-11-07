# correction 矩阵正式 108run —— 用户指令停机回执（P-011）

- 停机时间：2026-10-23 16:48（GMT+8）
- 停机原因：用户显式指令「把你的脚本停下」，非哨兵触发、非失败停批
- 后台任务 ID：0i5uvz（运行 8h53m 后 killed）
- 采集脚本：`tools/run_p011_corr_matrix_formal.ps1`
- 冻结计划：`calibration/p011-corr-matrix-formal-plan-v1.json`（master seed 20261022）
- 产物根目录：`test/artifacts/p011-corr-matrix-formal-108-20261022`

## 1. 停机现场清理（已执行）

| 项 | 动作 | 结果 |
|---|---|---|
| 后台采集进程 | TaskStop | killed |
| 宿主机残留 hdc 进程（PID 3704，10-13 遗留孤儿） | Stop-Process -Force | 已清，hdc 进程数归 0 |
| 设备端应用 | `aa force-stop com.example.helloworld` | force stop process successfully |
| 设备端残留 `hilog -x \| grep` （PID 15514/15517） | kill -9 | 已清 |

设备 62T0225B10010020 在线，未关机、未熄屏处理（如需长期停机另做 power-shell）。

## 2. 进度：82 / 108 完成，全哨兵通过

按 `sentinel.json` 计数：

| stratum | A | B | C | D | 小计 |
|---|---|---|---|---|---|
| G1 | 12 | 12 | 12 | 12 | **48（全）** |
| G2 | 9 | 8 | 8 | 9 | **34** |
| G0 | 0 | 0 | 0 | 0 | 0（未开始） |
| 合计 | 21 | 20 | 20 | 21 | **82** |

哨兵（82/82 全通过）：`d1bClosure` 82、`identityOk` 82、`prefixOk` 82、`hardSafetyOk` 82、`invariantWithinTan30` 82、`runIdUnique` 82。

- 单 run 墙钟 mean 390 s（min 383 / max 403），n=82
- 已耗时 8.87 h；剩余 26 run 预计 2.81 h
- 目录 84 个（含 1 个 `-partial-<ts>` 保留的首 run 重采前目录），trimmed 账本 173.3 GiB，目录共 192 G；D 盘剩 183 G

## 3. 需要继续注意的两条观测（登记，非结论）

1. **tan30 边界接触**：82 run 中 61 个 `maxSandTopInvariantError` 贴在 0.5773502691896xx（浮点抖动），全部为 B/C/D（修正臂）；A 组为 0.4997–0.5010。全部仍在引擎闸门内（`range=True`），但修正臂余量已被用尽。交 designer/tester 裁定是"正常饱和"还是"余量告警"。
2. **stratum 因子未激发**（`docs/cnd-verify-receipt-P011-v1.md` §1）：G1/G2/G0 三档 conditioning 命令序列相同，跨档差异/同档重复差异 = 1.12。统计时三 strata 只能当「同一 working state 下的可交换 block」，G2 稳健性、G0 控制两条判定作废。

## 4. 续跑方式

脚本带断点续跑：已存在且 `sentinel.json` 完整的 run 会 SKIP。直接重跑同一条命令即可接上：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File "D:\hmprojects\HelloWorld\tools\run_p011_corr_matrix_formal.ps1"
```

注意：G2 剩 5 个 block 未闭合（b05–b12 中部分变体缺），G0 3 个 block 完全未采；若只想要"完整 block"，需由 tester/manager 先确认是否接受 34/48 的 G2 子集。

## 5. 未写回项

滴答清单 MCP 本轮仍为 `unauthorized`，任务 6ad9e9b6 的回执与评论未写回；本地等价物为本文件 + `docs/cnd-verify-receipt-P011-v1.md` + 产物目录 `batch-log.csv` / `summary` / `manifest`。
