# T485 非机复跑

Windows，项目根目录执行；需要现有 Visual Studio C++、DevEco SDK/JBR、CokePLUS ANGLE 与 T475/T336 测试设施。不使用真机。各输出目录必须为新目录，不能复用旧进程结果。

```powershell
node test/t485/generate.cjs .codex/local-test/t485-m1-final m1
node test/t485/generate.cjs .codex/local-test/t485-m2-final m2
```

分别在两项目目录用 DevEco `hvigorw.bat --mode module -p product=default -p module=entry@default default@PackageHap --no-daemon` 构建。设置 JAVA_HOME 为 DevEco jbr，并将 jbr/bin 和 DevEco tools/node 加入 PATH。先构建后运行 host，host 直接读取本次 CMake `m1_scene_sources.h`。

```powershell
node test/t485/build_host.cjs .codex/local-test/t485-m1-final m1 .codex/local-test/t485-pixel-host-m1
node test/t485/build_host.cjs .codex/local-test/t485-m2-final m2route .codex/local-test/t485-pixel-host-m2
python -X utf8 test/t485/run_pixels.py .codex/local-test/t485-pixel-host-m1/scene.exe .codex/local-test/t485-pixel-host-m2/scene.exe .codex/local-test/t485-pixel-final-matrix
node test/t485/build_host.cjs .codex/local-test/t485-m2-final m2route .codex/local-test/t485-final-faults-all faults
```

将 CokePLUS 目录加入 PATH 后，运行最后生成的 `scene.exe <fresh-cache-directory>`，日志保存 `.codex/local-test/t485-factory-all.txt`。`factory_faults.inc` 只在测试宏启用时注入故障，不进入生成应用。

```powershell
node test/t485/legacy_compute.cjs .codex/local-test/t485-legacy
node test/t485/build_app_check.cjs .codex/local-test/t485-m2-final .codex/local-test/t485-app-m2-exact m2route
node test/t485/build_app_check.cjs .codex/local-test/t485-m1-final .codex/local-test/t485-app-m1-exact m1 <existing-compute-cache-v1-directory>
```

最后一个参数只复制 SOURCE/INTERMEDIATE_QUANTUM/FINAL_QUANTUM 的 binary，不复制任何 graphics binary；M1 三场景 role 仍冷建后新进程暖载。省略该参数可以全部冷建。fixture 的超时为 600 秒，不修改产品阈值。App 输出同时重定向至相应同名 `.txt`，供 collector 读取。

签包沿 `python -X utf8 test/t257/sign.py T485-m1-handoff <m1-project>` 和 M2 对应命令，标签/输出不可覆盖。构建日志保存 `t485-m1-handoff-build.txt` 与 M2 对应路径。`collect.py` 默认读取本次交付固定目录；复跑改目录时同步修改 collector 的显式输入，再运行：

```powershell
python -X utf8 test/t485/collect.py
```

输出在 `test/reports/t485/`；完整实现和证据边界见 `docs/developer-T485-scene-program-cache.md`。像素 fixture 冻结 publication，只验证实际 Scene 和缓存路径，不替代 compute 或真机端到端验证。
