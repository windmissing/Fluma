# T501 非机复跑

不使用真机。Windows、DevEco JBR/SDK、MSVC C++、CokePLUS ANGLE 与既有 T492/T485 宿主设施。所有输出目录须为新目录。

```powershell
node test/t501/generate.cjs .codex/local-test/t501-m1-final m1
node test/t501/generate.cjs .codex/local-test/t501-m2-final m2
```

两项目分别以 DevEco jbr 为 JAVA_HOME，DevEco tools/node 加入 PATH，在其目录执行 hvigorw `--mode module -p product=default -p module=entry@default default@PackageHap --no-daemon`。日志保存 `.codex/local-test/t501-m1-final-build.txt` 与 M2 对应文件。构建后运行：

```powershell
node test/t501/build_controls.cjs .codex/local-test/t501-m1-final m1 .codex/local-test/t501-controls-m1
node test/t501/build_controls.cjs .codex/local-test/t501-m2-final m2route .codex/local-test/t501-controls-m2
node test/t501/build_app.cjs .codex/local-test/t501-m1-final .codex/local-test/t501-app-m1 m1 .codex/local-test/t485-app-m1-exact/cache/gpu-program-cache/v1
node test/t501/build_app.cjs .codex/local-test/t501-m2-final .codex/local-test/t501-app-m2 m2route .codex/local-test/t485-app-m2-exact/cache/gpu-program-cache/v1
node test/t501/build_exit.cjs .codex/local-test/t501-m1-final m1 .codex/local-test/t501-exit-m1 .codex/local-test/t485-app-m1-exact/cache
node test/t501/build_exit.cjs .codex/local-test/t501-m2-final m2route .codex/local-test/t501-exit-m2 .codex/local-test/t485-app-m2-exact/cache
```

退出 suite 各有九个独立进程：正常退出、四个 EGL 退出切点失败保原错、四个 reset 退出切点失败后恢复。用 EGL_NO_DISPLAY 触发真实错误，不伪造清理成功；新 context 创建必须等待旧代退出完成。未退 context 随测试子进程结束，不在控制线程补毁。每进程 650s 外部守护。

```powershell
node test/t501/build_scene.cjs .codex/local-test/t501-m1-final m1 .codex/local-test/t501-scene-m1
```

加入 CokePLUS 到 PATH，创建空 `.codex/local-test/t501-scene-pixels`，执行 `scene.exe .codex/local-test/t501-scene-cache cold .codex/local-test/t501-scene-pixels`；保日志 `t501-scene-run.txt`。实际 Scene 的四场景、八拒绝路径后检查 resize/delete 全账，并注入非法 BufferData 验证失败保旧容量和 glGetError 原语义。

沿 `test/t492/build_source_gl.cjs` 对两路线运行真实 SOURCE/J→W/Draw/Swap，输出为 `t501-source-m1/m2`。stdout/stderr 保存在各 host 输出同名 `.txt`。然后使用 `test/t257/sign.py T501-m1-delivery <project>` 和 M2 对应命令签包，执行 `python -X utf8 test/t501/collect.py` 汇总。

host 的逻辑请求容量和句柄退休不证明 driver 物理回收；系统 GPU/RSS/allocator 仍 NOT_MEASURED。长序列及设备补证由独立 tester 接续，非开发关闭前置。
