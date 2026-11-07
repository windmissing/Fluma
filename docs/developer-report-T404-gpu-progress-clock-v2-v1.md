# T404 独立进度时钟 V2 开发交付

2026-11-07，developer。不使用真机。依据 `architecture-T401-gpu-progress-clock-v2-v1.md`，本卡为隔离实验实现，250ms 不是正式产品性能阈值。

- 新增固定内存 `ProgressClockV2/PlanGrantV2`。真实 FrameCallback 首先调用 `pagePulse`，session/序号/原始 UI ns 使用严格十进制 U64；native 接收 ns 独立记账，两域任一断档先撤权。show/hide 更换/关闭 session，status/sensor/触摸不续心跳。页面显式检查版本，V1 不受新增 pulse 门影响。
- 隔离生成器启用 `GRS_PROGRESS_V2_EXPERIMENT`；页面冷选 V1/V2，下一新 run 生效。生产默认 V1，专用 2ms 监督线程仅隔离宏启用。初始化不生成配额；Ready 后新 pulse 建立计时基线。
- 整数 120Hz 配额、4 个 pending、丢最新超额；授予独立活动计划，SOURCE 不消耗量子账。`RootIsolation.accept` 短锁内与 owner commit 一并更新 accepted/activeRemaining，公开账没有 Pump 返回后补计的第二来源。reset 先登记旧账，carry 清除独立记录。
- 每个 SOURCE/QUANTUM root 从提交意图建立绝对 250ms deadline；同 nonce BUSY/续批不续期。控制新鲜度在 authorize/accept/present 门即时复查。UP/STOP 仅变 source boundary，拒旧源、保量子；原 owner 隔离目标/排空复用 T398。
- 新增 60–65 事件，冷档字典 `T404_STAGE65_V1`；4096 有界环、SHA 分页及截断拒判保持。周期 pulse 每16笔聚合原始 UI/native 最大间隔；首错不省略。GAP 的 origin/delta 使用真实最后页面接收点。

自验（离线，不当真机性能证据）：

| 检查 | 结果 |
|---|---|
| `cmd /c test\t404\build_clock.cmd` | 100000 pulse 整数 oracle、carry 边界、过载、撤销、reset、重放、双域 GAP、生成溢出/身份耗尽拒绝通过 |
| `node test/t404/build_app.cjs` | 实际 App 许可 5/40/65/200/250ms 通过、250ms+1ns 拒绝；BUSY 固定期限、UP 只拒 SOURCE、量子继续、迟到监督时 authorize 拒 GAP、owner 换代通过 |
| 同上，实际 ANGLE + App + 完整 SOURCE 链 | driver cut 保持65ms且持续脚本心跳，accepted=1、sourceCommit=1、acceptedQuantumCount=0；停止 pulse 后 accepted=0，晚返回被拒；两者配额账闭合 |
| `cmd /c test\t375\build_host.cmd` | 两档 K/Kw 原回归各53/0，原 Run/bridge/App 套件通过；T389/T398 隔离字节回归通过 |
| `node test/t404/build_parity.cjs` | 真实 Run V2 grant 对 V1 同输入 SOURCE+QUANTUM 字节对拍（原始结果见 `test/reports/t404/parity.txt`） |
| `node test/t404/ui_units.cjs` | 实际输入控制器25/0，含 V1/V2 与未知版本拒绝；实际 App 另断言 V1 页面 session/pulse 不改许可代、不写 V2 字典事件 |
| `node test/t404/cold_units.cjs` | 22/0，含新字典六事件与越界拒绝；既有 T324 冷档24/0、T384 判读回归通过 |
| Hvigor `default@PackageHap` | native + ArkTS 零错误，BUILD SUCCESSFUL；既有告警保留 |
| 本地签包、verify-app、verify-profile | 全通过，见 `test/reports/t257/T404-complete.json` |

App host 的 clock 是显式 SCRIPTED_OFFLINE，真实 ANGLE 执行配合可控 native/UI ns，避免把 PC cold shader 墙时当设备控制时间；65ms 是真实 host driver cut 保持时段。5/40/200/250ms 等由实际许可入口的可控时钟验证，未宣称这些时段已在设备测得。监督目标2ms不构成调度准点保证，最终接受门会立即复查。

签包：`.codex/local-test/t257-sign/T404-complete/T404-complete-signed.hap`，SHA256 `e72786aa8d621b8b40342639f9cdfcd051f6a26b886e0843c3632f4e1ecac095`。包内为 `GpuResidentM1` 隔离路由；先选 V2 再新建空场。状态应为 `GRS_M1_PROGRESS_CLOCK_V2`，模拟秒由 acceptedQuantumCount/120 计算。T404 不安装设备；tester 按 T401 同机 V1/V2、沙/水、主动控制断档、GPU 超期、late swap 等独立端到端取证，性能是否达标保持未测。
