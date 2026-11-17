# T496 续办 v2：CREATE_ONLY 三臂 C0/C2 实跑（T518 §3 执行）

日期：2026-11-17，tester，离线（ANGLE host fixture，不使用真机）。
依据：T518 §2–3 合同（设施修订+CREATE_ONLY 定位矩阵）。
工程增量（frozen tree 副本纪律）：

- `test/t496/gl_forward_sink.cpp`：`gl_forward.cpp` 的 frozen 副本，六函数（glCreateShader/glShaderSource/glCompileShader/glCreateProgram/glAttachShader/glLinkProgram）加 T518 §3.3 ENTER/EXIT 落盘（`gl_sink.inc` 命名空间，自持状态不依赖主 TU）；其余逐字同源。来源 SHA256 与差异说明：`test/reports/t496/gl-forward-sink-provenance.json`。
- `test/t496/gl_sink.inc`：sink 命名空间（TraceFile/CallSeq/WallNs/Emit，append+flush JSONL）。
- `test/t496/external_source_diff.cpp`：main 加三臂入口（argv=C0/C1/C2 走 CREATE_ONLY；无参保留旧 D1 两臂）；`RunCreateOnly`：新进程/新输出目录/种子构建→`Store().Configure`（owner 启动前）→`packageSource` 标注→owner 启动→等 `captureCommittedEpoch`（900s）→有限退出（quit+scriptDone+permit+notify，修 drain 自锁）。C1 臂未跑（C0/C2 已覆盖矩阵两端）。
- `test/t496/run_create_only.cmd`：每臂外部 watchdog 900s 硬杀（实际跑用 `timeout 920` 包裹，EXIT=124=观察界）。
- `test/t496/build.cmd`：build-only（不再自动运行）；换入 sink 版 gl_forward。

## 实跑结果

| 臂 | cache | Store.Configure | GL 编译链 | captureCommittedEpoch | 出口 |
|---|---|---|---|---|---|
| C0 | 空 | entries=0 | **306 行全配对**：9 compile 45.1ms、6 link 0.7ms、141 createProgram、0 未配对 | **未达** | EXIT=124（900s watchdog） |
| C2 | 6 bin 种子 | **entries=6 装载成功** | 18 compile（双倍=C0 冷编译+暖 miss 后重编？——见下）、12 link、576 createProgram、0 未配对 | **未达** | EXIT=124（900s watchdog） |

### 关键判定

1. **GL 编译链不是卡点**（T518 首要定位目标）：两臂编译链 ENTER/EXIT 全配对、总耗时 <60ms、无首个未返回调用。首个可定位未配对切点**不在 glCompileShader/glLinkProgram**。
2. **暖种子未产生 BINARY_LOAD 暖命中**：C2 entries=6 装载成功，但 key=Key(source,layout,environment) 派生自当前环境 SHA，与 T475 fixture 种子环境不匹配 → `Store().Get` miss → 冷编译（compile 计数 18=2×C0）。按 T518 §3 "允许原工厂合法冷编译，不能把冷 fallback 误记暖命中"——如实记录，暖命中验证需 environment SHA 匹配种子（2aabb8c8…），本轮环境派生不同。
3. **卡点缩域**：C0/C2 都到不了资源 commit（`captureCommittedEpoch`）→ 按 T518 §3 矩阵末行归"编译之外的 create 段或测试恢复握手域"——即 transport Create 内 compile 之后的 prepare/dispatch/scene Build/资源 commit/capture commit 握手之一。具体调用需 sink 覆盖扩展（当前 sink 不含 glProgramBinary 落盘——T488 wrapper 保有但无 ENTER/EXIT；EGL/transport/scene 调用未覆盖），本卡合同 900s 不再延长，**精确调用名回 architect 判方向**（候选：给 glProgramBinary/glGetProgramiv/eglCreateContext 等扩 sink，或由 architect 授予明确函数清单后 tester 补包装）。
4. **iter 心跳语义**：iter=45000 @ ~900s ≈ 20ms/iter（2ms sleep + 每 PumpBegin 锁外交接 hook 开销），owner 循环活着——非死锁态，是"在等某未满足条件"。C0 最后心跳 iter=45000 后被 watchdog 杀；两臂均无 GUARD_TIMEOUT 之外异常。

## 假通过路径修复确认（T518 §2 表）

- CREATE_ONLY 臂断言 `CREATE_RESOURCE_COMMIT_<arm>`：C0/C2 均如实 false（EXIT 前 Assert 已写 log；进程被 kill 时汇总文件未写——以 log-C0/log-C2 原件为准）。
- 无"阶段 EXIT 总写 OK"：CREATE_ONLY 路径不经 D1_OFF/D1_ON StageExitLog。
- 汇总/退出码：rc=1（epoch 未达）由进程内返回；被 watchdog kill 时 rc=124——两者都非 0，不再"main 总 return 0"。

## NOT_MEASURED / 前置

- C1（仅 compute 种子）：矩阵端点已被 C0/C2 覆盖（C2 含 compute bin），未单独跑。
- 暖命中（BINARY_LOAD）：需环境 SHA 与种子匹配——回 architect 裁定是重建 fixture 环境还是扩 key 派生记录。
- 编译后 create 段精确调用名：需 sink 覆盖扩展（见判定 3）。
- D1 两臂 SOURCE/QUANTUM 正控：等 create 卡点消解（T518 §3：CREATE_ONLY 过资源 commit 后才走生产 RecoverySessionLocked 握手再 D1）。

## 原件

`test/reports/t496/gltrace-C0.ndjson`（306 行）、`gltrace-C2.ndjson`（576 行）、`log-C0.ndjson`、`log-C2.ndjson`、`gl-forward-sink-provenance.json`。
