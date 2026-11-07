# T399 GPU 控制隔离与双源闭环真机 — T398 隔离机制真机证实；但根因停顿未除，5 run 零 accepted，V2 裁决触发证据成立

- 卡面：T398 签包解锁，按 T395 §5 探针开/关、核断档负控/迟到拒收/输入响应/双源 accepted quantum→同 token Draw/Swap；零进展则回裁隔离 V2。
- 包：T398-handoff-signed.hap SHA256 `567adf44abdce1f8689bff79dcc064d05dce4f06d0fbf1b174c02e4222f4950b`（本地逐位一致）。
- 设备：pid 18338，K192/W128，路由 `grsm1=true`；诊断面板 SHA 4×16 分段注册零丢字；探针 `TAIL_PROBE_ON_NEXT_RUN` 开启后随新 run 生效。
- 证据：`test/reports/t399/`（36 文件，3 次时间线导出全 COMPLETE：226/627/627 行，SHA 见 t399_verdicts.json）。

## 采证过程

诊断面板展开 → 开「下轮提交探针」→ 注册签包 SHA（4×16 分段）→ 应用光学并新建空场 ×3（产生 5 个独立 run：epoch 3–7）→ 真实触摸注入画布 12+ 笔（hilog `TTHNI: XComponent D6` 证实命中）→ 导出原生时间线 ×3。

## 判定（全部 5 run 同一失败型，零进展）

| 指标 | 观测 |
|---|---|
| RootSubmit(40) | 5（每 run 恰 1 次成功发出） |
| SubmitCancel(41) | 5（全部未封尾被撤销） |
| Accepted(48) / Draw(13,14) / Swap(15,16) / Presentation(50) | **全零** |
| 双源准入计数 | 0.0000 全程（source offer 从未 ENQUEUED） |

**T398 隔离机制本身真机证实（4 项）**：
1. **stage58 授权链全通**：Stage cut ×4 OK、Flush cut ×8 OK、Batch cut ×8 OK_SUBMITTING——隔离门 authorize 在三个 cut 上逐 run 记录且全部放行。
2. **迟到拒收证实**：stage59 唯一一笔——permitRev 8 < permRev 10（permit 已换代），旧 nonce 在 Stage cut 被拒，此时 deadline 仍剩 31.55ms（在窗内但换代失效）。
3. **GAP 撤销及时**：每 run 控制线程（tid 18338）在 38.7–47.8ms 判 CLOCK_GAP 并 PauseBegin，撤销动作在控制线程独立完成——控制不被 GL 阻塞（T398 核心目标达成）。
4. **时间线 stage58/59 字典**（T398_STAGE59_V1）在真机产出且可解。

**根因停顿未除（产品 FAIL 交 architect）**：每 run 最后一次 DispatchExit（GPU 线程 tid 18645）→ PauseBegin 之间 **38.7 / 44.5 / 39.7 / 46.8 / 47.8 ms** 无任何 timeline 事件；GPU 线程 DispatchExit→SubmitExit 全程 51.3ms。dispatch 仅 0.55ms/20 笔——停顿在 dispatch 完成后的封尾/完成等待族，与 T376/T384/T390 幸存停顿（31.8–57.9ms）**同一结构性族**。停顿 > 33.33ms GAP 窗 → root 永远无法在窗内封尾 → 全部被 CancelSubmit。

## 双源闭环（卡面主目标）— 零进展

真实触摸确认命中 XComponent，但 `down()` 需要 `allowed(snapshot)`（permissionState==='READY' 且 epoch===readyEpoch）；每 run 在准入窗口建立前就被 CLOCK_GAP 撤销 → `setSource` 未被调用 → 准入 0。**"至少一次成功 quantum→同 token Draw/Swap" 在 5 个独立 run 中零达成。**

## V2 裁决触发（按卡面要求明确回报）

主目标（成功 quantum→同 token Draw/Swap）跨 5 独立 run 零进展；根因停顿族（dispatch 后 38.7–51.3ms）已第三次跨包复现（T376→T384→T390→T398 隔离包），隔离机制正确不改变停顿存在。**建议触发隔离 V2 裁决**；同时移交 architect：dispatch 后停顿的精确定位（Flush/Fence/Poll/完成等待细分，本包 timeline 在该区间无事件可拆）。

## 边界

未改业务；全部触摸真实注入；SHA 分段注册；3 次导出原档入库；OS 调度轨迹（TID 匹配）不可用——控制/GPU 线程以 timeline tid 字段区分，卡面允许分项标缺。
