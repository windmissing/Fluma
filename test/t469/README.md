# T469 单沙整数光栅计划

从仓库根目录生成并检查实际 T455 派生页面，输出目录须为新目录。

```powershell
node test/t469/generate.cjs .codex/local-test/t469-check
node test/t469/page_units.cjs .codex/local-test/t469-check
```

在生成目录用 DevEco Studio 的 Hvigor 执行 `--mode module -p product=default -p module=entry@default default@PackageHap --no-daemon`。签包与验签收据见 `test/reports/t257/T469-release.json`，离线检查见 `test/reports/t469/delivery.json`。

- T474 可从生成页检查分配、实际内框、旧帧、预约零副作用和 hold 首因。离线夹具代替 ArkUI 回调和原生状态。
- T408 使用本次签包重新标定。先编辑并确认固定键盘再准备，从 `t469-prepare-*.json` 保存连续外框、实际 XComponent 框、整数计划、packet 与 EGL 整数及身份。若实际尺寸与计划不一致，保留原始三域数据和拒因交 architect。
- PNG、ROI 和时钟继续使用原冻结窗口合同，映射依据为实际整数内渲染区。真机布局、采样和端到端配对由独立测试执行。
