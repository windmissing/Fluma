# T456 · GPU 启动与首帧冷探针

基于 T438 的两个隔离配置。普通菜单进入体验页和 `grsm1=true` 直达诊断页共用一个 `libgrsm1`、一条 GL owner 路径。两页的“导出启动诊断”读固定容量快照，写 `t456-boot-*.json` 及文件 SHA 回执。NAPI 可读进程中已失去窗口的 attempt。签包和验签回执见 `test/reports/t257/T456-*-finalized.json`。

收据 revision 为 `T451_GPU_BOOT_PROBE_V1`。`attempts` 最多保六代，`transitions` 最多保 64 项。每代最多 256 条首阶段记录，`firstFailure` 与 `latest` 独立保存，`dropped` 显式标记不完整。窗口代号仅在同进程内有意义。时钟字段使用原时域，V1 anchor、V2 pulse 和 V2 job deadline 分列。`SUBMIT_OBSERVER.a` 为 `SubmitStage`，`TAIL_OBSERVER.a` 为 `TailStage`。各首根 dispatch 仅保第一组进出及最近进度。`TRANSPORT_BUFFER` 的 a/b/c 为请求字节、对象类及累计逻辑字节。对象类 1–6 为 SOURCE bank、event、publication、completion、WORK、scratch，7–9 为 QUANTUM candidate、WORK、event。分配未完成时从最近进出阶段结合终态错误判读。

```powershell
node test/t456/generate.cjs .codex/local-test/t456-regular-final regular
node test/t456/generate.cjs .codex/local-test/t456-experiment-final experiment
node test/t456/build_host.cjs .codex/local-test/t456-regular-final
```

在两套隔离目录执行 `hvigorw --mode module -p product=default -p module=entry@default default@PackageHap --no-daemon`。用 `test/t257/sign.py` 取不同签名标签。开发验证用 ANGLE pbuffer 与脚本时钟，确认生成桥的 EGL、READY、接受及 Swap，另核对同线程取 EGL 首错、旧代隔离、无 exit 时读最近 enter、有界溢出和有界 shader log 转义。此证据没有现场 Surface 时序，真机首错仍由 T457 取证。

T457 按同一实际 HAP 做菜单 V1 与直接 Want V1 配对，K192/Kw128，匹配 viewport 和输入。冷启后先无触摸观测，再导出诊断。另取新空场及真实背景/尺寸变化。experiment 中明确切实验 V2 并核 `actualClock`，单列采样。保每次文件 SHA、目标包、PID、epoch、surface generation 和原状态。无首帧时仍从冷出口取首错，禁止把旧代最新状态当本次失败。无真机开发操作。
