# T492 非机复跑

Windows / DevEco SDK 与 JBR / Visual Studio C++ / CokePLUS ANGLE；项目根目录执行。不使用真机。所有生成目录、host 输出目录、签包标签均须为新值。设备独立由 T495 接续。

```powershell
node test/t492/generate.cjs .codex/local-test/t492-m1-delivery m1
node test/t492/generate.cjs .codex/local-test/t492-m2-delivery m2
```

在各生成目录执行 `hvigorw.bat --mode module -p product=default -p module=entry@default default@PackageHap --no-daemon`。使用 DevEco jbr 作为 JAVA_HOME，jbr/bin 与 tools/node 加入 PATH。保留构建日志为 `.codex/local-test/t492-m1-delivery-build.txt` 及 M2 对应路径。两路线均先完成构建，宿主明确优先包含该项目实际 CMake 头文件。

```powershell
node test/t492/build_controls.cjs .codex/local-test/t492-m1-delivery m1 .codex/local-test/t492-delivery-controls-m1
node test/t492/build_controls.cjs .codex/local-test/t492-m2-delivery m2route .codex/local-test/t492-delivery-controls-m2
node test/t492/page_units.cjs .codex/local-test/t492-m1-delivery .codex/local-test/t492-delivery-controls-m1 test/reports/t492/page-m1.json
node test/t492/page_units.cjs .codex/local-test/t492-m2-delivery .codex/local-test/t492-delivery-controls-m2 test/reports/t492/page-m2.json
```

控制测试各执行原 T467 的 23 项和本卡 20 项。控制 fixture 使用可控 native clock，并停掉它的真实 supervisor，以免手动时钟与线程竞争；生成 App GL 测试保留真实 supervisor。页面测试执行实际 ArkTS reader/controller 和页面方法，消费 C++ exporter 原页，保存每次 fsync 后的文件和 SHA 至 controls 目录 `reader-evidence/`。拒页保存为 rejected-page，不能当有效行。

```powershell
node test/t492/build_app.cjs .codex/local-test/t492-m1-delivery .codex/local-test/t492-delivery-app-m1 m1 .codex/local-test/t485-app-m1-exact/cache/gpu-program-cache/v1
node test/t492/build_app.cjs .codex/local-test/t492-m2-delivery .codex/local-test/t492-delivery-app-m2 m2route .codex/local-test/t485-app-m2-exact/cache/gpu-program-cache/v1
node test/t492/build_source_gl.cjs .codex/local-test/t492-m1-delivery .codex/local-test/t492-delivery-source-m1 .codex/local-test/t485-app-m1-exact/cache m1
node test/t492/build_source_gl.cjs .codex/local-test/t492-m2-delivery .codex/local-test/t492-delivery-source-m2 .codex/local-test/t485-app-m2-exact/cache m2route
```

App 准备检查只从既有 cache 种下三个 compute binary，三 graphics 仍 cold→新进程 warm；检查 READY 前准备及首真实 host pulse 的零冷 quota。SOURCE GL fixture 复用 T477 实际 App 测试及既有暖 cache；执行真实 Offer/compute/接受/lease/Scene/Swap，验证放手后 J→W，导出新诊断。其 native clock 为受控 host 域，不能用执行耗时证明设备 250ms 或帧率。ANGLE 仅测试设施，产品无软件 GL/Canvas 回退。

原 live endpoint 与恢复回归：

```powershell
node test/t477_fix/build_timeline.cjs .codex/local-test/t492-m2-delivery .codex/local-test/t492-legacy-timeline
node test/t477_fix/units.cjs .codex/local-test/t492-m2-delivery .codex/local-test/t492-legacy-timeline/pages.ndjson
node test/t467/page_units.cjs .codex/local-test/t492-m1-delivery
node test/t467/page_units.cjs .codex/local-test/t492-m2-delivery
python -X utf8 test/t257/sign.py T492-m1-delivery .codex/local-test/t492-m1-delivery
python -X utf8 test/t257/sign.py T492-m2-delivery .codex/local-test/t492-m2-delivery
python -X utf8 test/t492/collect.py
```

将各 host stdout/stderr 重定向至同名 `.codex/local-test/<host-directory>.txt`，legacy units 至 `t492-legacy-units.txt`。collector 读取固定交付目录；换目录复跑时同步修改显式输入。只交付 `T492-*-delivery` 签包收据，早期 handoff 是保留的迭代证据。
