# T450 · GPU 采集签包身份冷启动入口

`grsm1=true` 必须是 Want 的布尔参数，`gpusha` 必须是恰好 64 字节的 ASCII hex 字符串。冷启动模块只接收首个 onCreate 声明。诊断页面只读展示同一声明，显式点击才会读取当前 scope 并尝试预约。`INVALID_CAPTURE_CONFIG` 保留顶层错误码，`invalidField` 给第一个未通过的固定字段名或类别。原 2 秒预约、1 秒安全点窗口、GAP、V2 期限和预算沿 T421。

诊断区的“导出预约入口回执”写 `gpu-capture-launch-last.json`。归档成功预约时另写 `gpu-capture-launch-<requestId>-<captureId>.json`，内有九字段配置、声明来源和原 requestId。后续无效或旧代调用不会覆盖已匹配的冻结身份。

```powershell
node test/t450/generate.cjs .codex/local-test/t450-t410 t410
node test/t450/generate.cjs .codex/local-test/t450-t437 t437
node test/t450/generate.cjs .codex/local-test/t450-regular regular
node test/t450/page_host.cjs
```

分别在 t410/t437 隔离工程执行 `hvigorw --mode module -p product=default -p module=entry@default default@PackageHap --no-daemon`，回仓库用 `python -X utf8 test/t257/sign.py T450-<target> .codex/local-test/t450-<target>` 签名。t410 基于冻结 T421 `0228a7d`，t437 另叠 T434 `b54dc48`。regular 是拒绝分支的宿主生成夹具。安装及冷启前运行 `python -X utf8 test/t450/install_input.py <target> <实际用于安装的 signed HAP>`。

工具只验证文件并打印命令。tester 记录安装回执、设备、目标版本、冷启命令及新 PID，再读取页面只读声明，核实际生效 run/epoch/session 和预约回执。SHA 是宿主声明，安装文件哈希一致性由宿主验证。T437 在该独立包内选择完整/前缀并新建 run。设备窗口按清单排队。
