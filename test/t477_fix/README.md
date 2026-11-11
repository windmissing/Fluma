# T477 v2 修复检查

不使用真机。生成目录和编译输出目录必须为新目录。

```powershell
node test/t475/generate.cjs .codex/local-test/t477-fix-check
node test/t477_fix/build_timeline.cjs .codex/local-test/t477-fix-check .codex/local-test/t477-timeline-check
node test/t477_fix/units.cjs .codex/local-test/t477-fix-check .codex/local-test/t477-timeline-check/pages.ndjson
node test/t477_fix/regression.cjs .codex/local-test/t477-fix-check t306
node test/t477_fix/regression.cjs .codex/local-test/t477-fix-check t404
node test/t467/page_units.cjs .codex/local-test/t477-fix-check
node test/t477_fix/build_app.cjs .codex/local-test/t477-fix-check .codex/local-test/t477-app-check
```

M1 恢复回归另生成 T467 experience 工程并运行 test/t483/build_recovery.cjs。最终 M2 工程用 Hvigor PackageHap 编译，沿 test/t257/sign.py 签包验签。交付签包见 test/reports/t257/T477-fix.json，报告见 test/reports/t477_fix/delivery.json。

离线 GLES 使用现有 ANGLE/SDK shim，控制时钟来自原 App fixture。其 wall 时间不用于设备 GAP、root 期限或 fps 判定。实际设备复跑仍由 tester 执行。
