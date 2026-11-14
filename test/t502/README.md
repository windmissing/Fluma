# T502 非机复跑

Windows/MSVC C++、DevEco JBR/SDK、CokePLUS ANGLE，项目根目录，不使用真机。所有输出目录必须为新目录。

```powershell
node test/t502/build_store.cjs .codex/local-test/t502-store-evidence
node test/t502/regressions.cjs .codex/local-test/t502-regression3
node test/t502/generate.cjs .codex/local-test/t502-m1-final m1
node test/t502/generate.cjs .codex/local-test/t502-m2-final m2
```

前两项保 stdout/stderr 为同名 .txt；store 输出12项和两个独立进程 seed/restart，metrics.ndjson 保存故障时实际占用。回归复用原 T475/T476 源码，不更改判据。仅测试宏注入写/remove故障，生产无故障开关。

两项目分别以 DevEco jbr 为 JAVA_HOME、jbr/bin/tools/node 加入 PATH，用 hvigorw `--mode module -p product=default -p module=entry@default default@PackageHap --no-daemon` 完整构建，日志保为 t502-m1/m2-final-build.txt。

```powershell
node test/t502/build_graphics.cjs .codex/local-test/t502-m2-final m2route .codex/local-test/t502-graphics2 faults
```

加入 CokePLUS 到 PATH 后运行 `scene.exe <fresh-cache-directory>`，日志为 t502-graphics-run.txt。共原13+新增3检查，合法 GL program 在 remove/queue/pin失败下仍保留；不是只测文件 Put/Get。

```powershell
node test/t485/legacy_compute.cjs .codex/local-test/t502-legacy-compute
node test/t501/build_app.cjs .codex/local-test/t502-m1-final .codex/local-test/t502-app-m1 m1 .codex/local-test/t485-app-m1-exact/cache/gpu-program-cache/v1
node test/t501/build_app.cjs .codex/local-test/t502-m2-final .codex/local-test/t502-app-m2 m2route .codex/local-test/t485-app-m2-exact/cache/gpu-program-cache/v1
python -X utf8 test/t257/sign.py T502-m1-delivery .codex/local-test/t502-m1-final
python -X utf8 test/t257/sign.py T502-m2-delivery .codex/local-test/t502-m2-final
python -X utf8 test/t502/collect.py
```

App只种既有compute binary，三graphics cold→新进程warm，并复核17/20 buffer/五program与零冷quota。各host输出重定向为同名 .txt，collector读取明确固定路径；变更复跑路径须同步collector输入。最终只交 T502-*-delivery 签包。物理GPU/driver/RSS不是缓存磁盘预算，均不得由这些离线计数推断。
