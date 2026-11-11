# T483 可复跑检查

不使用真机。先生成四个新目录，再核冻结签包与源码。原目录保留。

```powershell
node test/t467/generate.cjs .codex/local-test/t483-regular regular
node test/t467/generate.cjs .codex/local-test/t483-experience experience
node test/t467/generate.cjs .codex/local-test/t483-t410 t410
node test/t467/generate.cjs .codex/local-test/t483-t437 t437
node test/t483/build_recovery.cjs .codex/local-test/t483-experience .codex/local-test/t483-recovery-final
python -X utf8 test/t483/install_units.py
python -X utf8 test/t483/audit_sources.py
```

生成和编译要求新输出目录。已存在的本轮输出可直接运行最后两条命令。来源核对固定保存本轮冻结收据范围，若今后有源码变化，先核差异再更新范围。

安装输入助手兼容旧用法。新签包显式指定新收据，例如以收据中的实际 HAP 路径执行 `python -X utf8 test/t450/install_input.py t410 <hap> --receipt test/reports/t257/T467-t410-final.json`。输出中的 Want SHA 从所校验的 HAP 计算。命令仅生成输入，不执行安装或启动。
