# correction 矩阵正式 108run 收尾回执（P-011）—— EARLY_STOPPED_AT_USER_DISCRETION

日期：2026-10-23。责任：effector。状态：**CLOSED / SEQUESTERED_FOR_TESTER_EXTRACTION**（数据封存，非 VALIDATED；指标提取与判定归 tester）。

裁决依据：manager 2026-10-23 16:24 裁决 **EARLY_STOPPED_AT_USER_DISCRETION**（commit `c79d447` 在批脚本内落 `STOP_BATCH.now` 优雅停机检查），设备释放给 effector 真机窗口。停机发生在 run 82/108；用户 17:06 确认「108run 收尾 = 封存交接」，不再续跑剩余 26 run。

- 产物根：`test/artifacts/p011-corr-matrix-formal-108-20261022`
- 封存清单：`test/artifacts/p011-corr-matrix-formal-108-20261022/CLOSURE.json`
- 冻结计划：`calibration/p011-corr-matrix-formal-plan-v1.json`（master seed 20261022）
- 前序文档：`docs/cnd-verify-receipt-P011-v1.md`（conditioning 通道验证）、`docs/corr-matrix-formal-108-stop-receipt-P011-v1.md`（停机现场回执）

## 1. 数据清单

| 项 | 值 |
|---|---|
| 完成 run | **82 / 108** |
| 完整 block（ABCD 四变体齐全） | **20**：G1 b01–b12（12）+ G2 b01/b05/b06/b07/b08/b10/b11/b12（8） |
| 不完整 block | 1：G2/b04 仅 A、D |
| 未开始 | G0（3 block 全缺） |
| 变体计数 | A 21 / B 20 / C 20 / D 21 |
| 哨兵 | 82/82 全通过（身份、D-1b 闭合、前缀到位、硬安全、不变量在 tan30 内、runId 唯一） |
| 单 run 墙钟 | mean 390 s（min 383 / max 403），已耗 8.87 h |
| 磁盘 | 目录 192 G（trimmed 账本 173.3 GiB） |

## 2. 交接 tester 的可用口径（必须遵守）

1. **主比较 D vs A：21 对**（同 block、同 seed 配对，含 G2/b04 的 A/D 对）。这是本批唯一完整闭合的主比较。
2. **需要 B/C 参与的比较：只能用 20 个完整 block**（G1 12 + G2 8）。G2/b04 的 A/D 不得进入 B/C 相关统计。
3. **三 strata 不得当独立场景**：conditioning 通道虽接通（CND_ 前缀在位、正式段起点平移 65.0 s），但 G1/G2/G0 三档命令序列相同（跨档差异 / 同档重复差异 = 1.12，两组范围重叠），判定 `STRATUM_FACTOR_NOT_EXCITED`。把三档当独立 stratum 做 block bootstrap 会让 CI 虚假收窄。**G2 稳健性、G0 控制两条预注册判定本批作废**。
4. **G1 的 12 block 是预注册的完整统计单位**：若只想要一个自洽的最小集，用 G1 48 run 即可，结论标注「单 working state」。
5. 数据仅可用于**描述与区间估计**，不得写 VALIDATED / PASS——E3 锚点冻结与正式判定在 tester 侧。

## 3. 观测（登记，交 designer / tester 裁定）

1. **tan30 边界接触**：82 run 中 **61 个** `maxSandTopInvariantError` 贴在 0.5773502691896xx（浮点抖动），全部属于 B/C/D 三个修正臂；A 组为 0.4997–0.5010。全部仍在引擎闸门内（`audit.range=True`，哨兵未触发停批），但修正臂把局部界面偏差推到了硬闸门上限，A 组尚余约 0.077。需裁定：修正臂的**正常饱和**，还是**余量告警**。
2. 批脚本的不变量判据已对齐引擎自身容差（`WaterSandField.ets:1917` 判 `≤ tan30 + EPS`，EPS=1e-9，同文件 556 行）。第一版脚本写的是严格 `≤ tan30`，比引擎更严，第 1 run 误触发停批——该 run 按「不删失败 run」保留为 `-partial-<ts>` 目录并重采，已在停机回执登记。
3. 每 run 的 `invariantAtBoundary` 逐 run 记录在 `sentinel.json`，可供 tester 直接筛选边界接触样本做敏感性分析。

## 4. 未写回项

滴答清单连接器未安装（`unauthorized`，get_task_by_id / get_project_by_id 均失败），本收尾回执与封存结论**未写回任务 6ad9e9b6**。本地等价物：本文件 + `CLOSURE.json` + 产物目录 `batch-log.csv`。授权恢复后需补一条评论，内容为 §1–§3。
