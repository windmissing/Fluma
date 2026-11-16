# T516 开发交付

2026-11-16，developer。不使用真机。按 T509 v2，新增 `test/t516/generate.cjs`，沿实际 T469 生成链修改 RasterGeometry、PrepareReadiness、P013SingleSandController 和 P051DerivedV2Demo。通过生成器启用该 revision，保留历史 T469 代码与档案。

四边分配统一在 window 物理 px 域计算 ceil(x/y)、floor(x+w/y+h)。原父框得到 (52,990,1176,1716)，inner 使用父 content 局部 position (0.79998779296875px,0px)，显式 Ltr，三子节点用同源整数尺寸。计划保存原 allocation、desired window/screen、localPx、三实际框及 screen 框、native tuple、geometry/generation/layout ack。packet 使用已核 actual x/y 与整数尺寸，ROI 沿已有 actual CaptureGeometry。allocation 原值、window/scale/原点变化推进代，旧观察拒绝，实际三框逐值匹配。原键盘、两帧锚、reserve 零副作用和 OPEN 首因保持检查。

实际生成页面和 helper 非机检查 44/0，含精确 FAILED 样本、51/1177 与52/1177拒绝、相邻浮点/半像素与2.625/3.5、触摸screen差异、transform、EGL/packet、旧代回调/permit、OPEN失效与键盘门。页面自测仅替换 ArkUI/window/native 平台绑定，未验证设备 paint。ArkTS/native/PackageHap BUILD SUCCESSFUL，30 tasks，签名及 verifyApp PASS。保留上游编译警告。

完整来源、HAP/ELF SHA、44用例原件位于 `test/reports/t516/`。签包 `.codex/local-test/t257-sign/T516-release/T516-release-signed.hap`，SHA256 `6f49d06da5cf4a019d96c3f55a694d6a606fc0730e538406c58e142e68ec1c22`。

复跑命令在仓库根执行。生成目录须为新路径。

```powershell
node test/t516/generate.cjs .codex/local-test/t516-new
node test/t516/page_units.cjs .codex/local-test/t516-new
```

在该生成工程使用现有 DevEco hvigorw 执行 `--mode module -p product=default -p module=entry@default default@PackageHap --no-daemon`，JAVA_HOME 为现有 DevEco jbr、PATH 含其 bin。签名沿 `test/t257/sign.py`。T408 tester 接新签包串行测实际 position/layout、三框全tuple和packet/EGL，两帧后再 READY/HOLD、时钟、PNG及新几何重新标定。旧1177宽ROI不得套用新1176宽。若平台位置不匹配，保原值回 architect，不增加容差。
