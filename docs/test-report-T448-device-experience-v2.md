# T448 · GPU 显式沙水入口真机体验报告 v2（T467 体验签包续测：恢复链已打通，GPU 首 root 250ms 绝对期限真机不可满足，仍无内容帧）

2026-11-13，effector 续测，真机 62T0225B10010020，设备串行（当日无其他真机卡占用）。被测包：T467-experience-final-signed.hap，SHA256 `1a3d8d5313480cc5bfbcac54d12ff461d2db2ef8cb79da70572ad9687b62f6f2`（与 test/reports/t467/delivery.json 登记逐位一致，8,708,044 字节），`hdc install -r` 安装成功。本报告为 T448 前次 BLOCKED（docs/test-report-T448-device-experience-blocked-v1.md，T438 包）的续做段，结论：**T467 显式恢复/首 root 接线在真机上确实修复了前次 `EglWrapperHookLayer init Failed` 的 SURFACE/GL 初始化失败，恢复状态机全链走通（SURFACE→EGL→TRANSPORT→脉冲→SESSION_HANDOFF→READY_COMMIT→ROOT_INTENT）；但 GPU 首帧在 250ms 绝对 root 期限内无法完成，V2 期限撤权触发（V2_GPU_PROGRESS_TIMEOUT→V2_PULSE_CLOCK_GAP→CONTROL_REVOKED），run 锁死在 `PAUSED_CLOCK CLOCK_GAP`，画布仍无任何内容帧**。体验主目标（掉沙/注水/侧扩散/水质感/录像/主观）继续 NOT_MEASURED。

## 一、已真机完成部分（较 T438 轮新增证据）

| 项 | 结果 | 证据 |
|---|---|---|
| 签包核验+安装 | SHA256 本地逐位一致，安装成功 | `sha256sum` 输出与 delivery.json 一致 |
| 首页菜单入口 | 「进入 GPU 沙水体验」按钮可达，点击进入体验页 | e0_home.jpeg / l0.json |
| capability 行 | `T438_EXPERIMENT_V2 · K192/Kw128 · GRS_GPU_SCENE_Q20_M1_V2`，V2 | s4.jpeg |
| 初始恢复态 | `STATE_LOST WAIT_RETIRED / run 0:0`（T467 恢复状态机初始相位，run 未启动） | s4.jpeg |
| XComponent SURFACE | 创建成功，查询尺寸 1280×1609 | boot2.json rows：EGL_QUERY_SURFACE 1280/1609 |
| EGL 全链 | GET_DISPLAY/INITIALIZE/BIND_API/CHOOSE_CONFIG/CREATE_CONTEXT/CREATE_SURFACE/MAKE_CURRENT 全部成功（result=1） | boot2.json rows；本轮 hilog **零** `EglWrapperHookLayer init Failed`（T438 轮反复出现） |
| TRANSPORT | 创建成功，capability 1073741824，9 个 buffer 注册（含 1.34MB/521KB/11.8MB） | boot2.json rows TRANSPORT_* |
| 页面脉冲 | 468 笔 PAGE_PULSE 记录，中位间隔 16.63ms（60Hz），最大 19.74ms，无 >250ms 断档 | boot2.json recovery.events 统计 |
| 恢复状态机 | RESET_REQUESTED→RESOURCES_READY→SESSION_HANDOFF→READY_COMMIT 全部到达（epoch 3 / session 1 / resetSerial 1） | boot2.json recovery.events |
| 新空场/触摸注入 | 「新空场」「实验 V2」按钮、画布 swipe 均执行 | l2.json、InputKeyFlow 日志 |

## 二、BLOCKED：GPU 首 root 250ms 绝对期限真机不可满足（首错定位）

**现象**：点「新空场」后 run 0:1 启动，状态行恒 `PAUSED_CLOCK CLOCK_GAP / run 0:1`，准入/拒量/在途/落水全 0，画布纯黑（s5–s9 截图逐张一致）。第二次启动与首次模式完全一致（boot1/boot2 两份诊断 attempt3 首错相同）。

**首错链**（boot2.json attempt3，237 行 rows + recovery.events，时间为该域单调 ns）：

| 事件 | ns | 相对前一事件 |
|---|---|---|
| ROOT_INTENT（意图起算 a=70343609929217） | 70343609933592 | READY_COMMIT 后 33.4ms |
| （GPU 队列忙：127 段场景提交链，13 次 FLUSH，其中 4 段重段各 39~53ms，段间等待合计约 2.27s） | … | 共 2455ms |
| ROOT_ACCEPT | 70343855447967 | 意图后 **2455ms** |
| DRAW_ENTER（1280×1609 首帧） | 70343855472342 | ACCEPT 后 2.4ms |
| V2_GPU_PROGRESS_TIMEOUT（**首错**） | 70343860670624 | 超过 deadline（intent+250ms）仅 0.74ms；DRAW_ENTER 后仅 5.2ms |
| V2_PULSE_CLOCK_GAP | 70343876377811 | 首错后 157ms |
| CONTROL_REVOKED | 70343876390624 | READY_COMMIT 后 299.9ms |

**定性**：250ms 绝对 root 期限（不续期）从 ROOT_INTENT 起算，而真机 GPU 队列完成初始场景提交链（ROOT_INTENT→ROOT_ACCEPT）耗时 2455ms，结构性超出期限约 10 倍；首帧 DRAW_ENTER 时期限已过期 5ms，超时检查随即触发撤权。撤权后 449 笔 pulse 全部 code=1（不再被接受），run 永锁 `PAUSED_CLOCK CLOCK_GAP`，恢复状态机停在 `FAULTED_REVOKED`（retired=true，dropped=10290）。

**与 T438 轮对照**：前次首错在 EGL/SURFACE 创建（`EglWrapperHookLayer init Failed`，grsm1 静默失败）；本轮 EGL/TRANSPORT/恢复链全部成功、grsm1 有完整诊断导出——T467 的显式恢复与首 root 接线对创建路径的修复**有效**。本轮新首错是**期限语义问题**：250ms 绝对期限未给初始场景提交预留时间，属 T401/T404 期限设计与真机 GPU 队列吞吐的矛盾，归 architect 分流（接 developer）。

**非因素排除**：页面 pulse 无 >250ms 断档（中位 16.6ms），排除页面时钟断档直接致因；EGL 全链 result 成功且 rawEgl=0（末次 T398_PRESENT_REVOKED rawEgl=0x3000=EGL_SUCCESS），排除 EGL 错误；无 shader 编译失败记录（rows 中 SHADER/PROGRAM/COMPUTE 类 stage 为 0，shaderLog 空），与 M2 SHADER_FAILED 不同因。

## 三、未行使项（不冒通过）

- 空容器/切沙水/触摸落下/停源后水到达/积水侧扩散/水面观感——画布无内容，无法观察
- 录像与 fps 证据——无有效内容帧，NOT_MEASURED
- 系统可见性、主观体验——依赖有效渲染帧，NOT_MEASURED
- 缺真人的摇晃——未到（本项持续保留）

## 四、证据清单

- `test/reports/t448-s2/`：boot1.json / boot2.json（T467 启动诊断原件，来自应用 files 目录，WS_EVENT 回执 bytes/SHA 见 hilog 转录）、e0/s1–s9 截图、l0–l3 布局、full.log/full2.log hilog 全量、hilog_after_newrun.txt、hilog_pid_touch.txt
- 复现：冷启→菜单进入→新空场→（约 5s）状态行锁 `PAUSED_CLOCK CLOCK_GAP / run 0:1`，两次启动+触摸注入均一致
- 本报告数字均可回溯到 boot2.json 具体行/事件（报告内已列 ns 原值）

## 五、所需前置（交 architect 分流）

1. 250ms 绝对 root 期限对「初始场景提交链」的适用性裁定：真机 GPU 队列 ROOT_INTENT→ROOT_ACCEPT 实测 2455ms（其中重段 flush 186ms + 段间等待 2.27s），期限设计需给首 root 前的场景装载另立窗口（如 deadline 从 ROOT_ACCEPT 起算，或对首帧放宽/分段预期），或工程上压缩初始提交链耗时；
2. 撤权后 CLOCK_GAP→FAULTED_REVOKED 的恢复可再入性：当前第二次「新空场」未使 run 递增（仍 0:1），恢复链对死 run 的再启动路径需核（T467 §`RUN_RESET` 语义 vs 页面按钮接线）；
3. 以上为 developer 域修订前置；修订签包到件后本卡续做全部体验项。
