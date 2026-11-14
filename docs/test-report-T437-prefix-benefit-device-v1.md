# T437 · GPU 来源前缀同包收益真机对照（v1：正确来源包 T467-t437-final 真机执行；完整/前缀路由切换与配对 run 已跑通，收益核被首 root 域阻断 NOT_REACHED）

2026-11-14，tester，真机 62T0225B10010020（本时段独占）。**来源按 2026-11-13 更正**：`test/reports/t483/install-t437.json` 实际包 `T467-t437-final-signed.hap`，SHA256 `9ae69471db925ac0cd631904f874bb9df4ee5fcec259e5d2f37983d0748cd2df`（本地逐位一致），Want `--pb grsm1 true --ps gpusha <完整包SHA>`；旧 T450 `4ab9b671…` 未使用。证据：`test/reports/t491/t437/`（9 截图 + 4 份 boot 诊断 + 3 份 copy-accounting + 布局 dump）。

## 一、同包路由切换（在位）

- 冷启 PID 34306；页面 V2 原子身份在位；光学/诊断面板经布局 dump 取精确 bounds（`uitest dumpLayout`，按钮"光学/诊断"实际中心 846,2636）展开。
- **"下轮来源账"开关工作正常**：完整 ⇄ 前缀切换后按钮文字与 `LEDGER_PREFIX_NEXT_RUN` / `LEDGER_FULL_NEXT_RUN` 回执均确认（截图 t437_prefix.jpeg / t437_l8/l9 dump）。

## 二、配对 run（完整/前缀各 run，K/Kw/seed 同包同参）

- 完整路由 run：新空场+投沙 → `V2_GPU_PROGRESS_TIMEOUT root 1`（b−a=250.0ms 整、检出 0.023ms）——与 T495/T410 同域。
- 前缀路由 run ×2：同操作 → 同首错域。
- 每次后导出 `t434-copy-accounting.json`（T434_COPY_ACCOUNTING_V1）。

## 三、收益核（如实：NOT_REACHED）

| run | route | entries | ledgerCopyBytes |
|---|---|---|---|
| 完整 | 0 | 0 | 262144 |
| 前缀 ×2 | 0 | 0 | 262144 |

按 `quantum_candidate.cpp:70`：`route=eligible?1:ledgerStage?2:0`、`ledgerCopyBytes=entries*64（eligible）或 4096*64（fallback）`。三次 run 均 route=0/entries=0——**StageLedger 限定 `owner State()==Idle` 且 header[34] 由完成 QUANTUM 的 kernel 写入**；真机首 root GPU 超期域下无任何 QUANTUM 完成，ledger 计数恒 0，前缀收益（entries×64 < 4096×64）**无法观测**。

## 四、结论

- 路由开关/切换回执/配对操作链真机全部在位（这是本卡新增可交付的工程证据）。
- **前缀 vs 完整的 copy bytes 收益对照 NOT_REACHED**——与 T410 六负控、T495 首root 量化同根（T448/T490 呈现域），等待有效内容帧后接续，本卡不重跑其时域。
- submit/flush/poll/map/swap 分项、accepted quantum/s、新内容 fps：NOT_MEASURED（同根）。
