# T409 离线开发自验与采集交接

本卡不使用真机；host 证据不证明设备帧率、真实触摸或系统呈现。架构依据 `docs/architecture-T407-gpu-v2-evidence-triage-v1.md` §4，落地格式见 `docs/developer-interface-T409-evidence-capture-v1.md`。

## 离线复跑

在仓库根目录执行，沿用工程已有 MSVC/ANGLE SDK 和 Node TypeScript 依赖。

```powershell
cmd /c test\t409\build_capture.cmd
node test/t409/ui_units.cjs
python -X utf8 test/t409/prepare_u64.py
cmd /c test\t409\build_u64.cmd
node test/t409/build_app.cjs
node test/t409/cold_units.cjs .codex/local-test/t409-actual-pages
node test/t409/cold_units.cjs .codex/local-test/t409-late-pages
```

`build_capture` 生成独立合成协议分页；`build_app` 生成真实 App+ANGLE 分页及 late-Swap 反例，脚本时钟均显式标记。不要并行运行同输出目录的原生编译。`--controls-only` 跑控制断言及 late-Swap 反例，使用独立目录。

冷导出输出 manifest 路径；可以使用 `python -X utf8 test/t409/check_archive.py <manifest>` 重核磁盘实际文件。此开发校验器不代替 T411 独立 checker。提交档仅包含对应 manifest 引用的全部分片；同目录其它历史导出不属于该档。

## 正式测试方采集操作

1. 使用最终签包实际 SHA，在页面选择实际 V2 和需要的手动模式；显示当前 native revision，切换选项不代表已经生效。完成 reset/新 run，收起展开诊断面板，先稳定 XComponent viewport。
2. 输入包 SHA 后使用面板外常驻采集按钮。默认采集 2 秒，API 允许 2–5 秒。开始回执必须 ARMED 且身份匹配；BUSY 立即拒绝，等自然空闲后重试，不注入额外假 pulse 或修改物理期限。
3. 按测试方案在窗口内操作。首错/到时自动冻结，显式 end 幂等；reset 或换代封存旧窗口。窗口期间保持正常页面真实回调，不轮询 dumpLayout 或外部 status 干扰健康窗口。
4. 冻结后点导出，保存回执与 manifest 及所有引用分片到每窗独立目录。导出是冷路径，不再读实时 status。取消、缺页、超预算或身份混用必须保留 INCOMPLETE，不手工补齐或筛选成功行。
5. 校验所有字节/SHA/游标/完整 scope/事件 serial/终态账及 ≤8 组 token。`SOURCE_ATTEMPT` 是 UI DOWN，`FIRST_OFFER` 是 native offer，二者均不等于正准入；正准入看 SOURCE receipt。被节流 root 没有 scene token，不能借其它 root 补成呈现链。

原生 `STEADY_NS`、UI `FRAME_CALLBACK_NS`、导出 `WALL_MS` 分域，不能直接跨域相减。host 标记为 `HOST_SCRIPTED_OFFLINE`，与设备档区分。`rootToken` 是接受身份，`requestedToken/renderedToken` 来自真实 scene；原生 Swap 成功且 deadline 过期必须拒绝 CURRENT，系统呈现仍未由此证明。

## 构建与签包

`node test/t409/generate.cjs <新隔离目录>` 生成构建工程；按已有 Hvigor SDK 环境运行 `--mode module -p product=default -p module=entry@default default@PackageHap --no-daemon`，再用 `python -X utf8 test/t257/sign.py <唯一标签> <隔离目录>` 签包。交付包及校验值见开发报告与 `test/reports/t257/T409-finalized.json`。
