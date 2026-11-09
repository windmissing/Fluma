# T438 · GPU 沙水体验入口

两个隔离 HAP 使用 T421 `0228a7d` 的 M1 transport、shader、控制器与场景。普通启动显示入口菜单，选择“进入 GPU 沙水体验”才打开同一 `libgrsm1` owner 的新空场。仓库应用入口仍由原公共配置管理。隔离生成目录的 `frozen-source.json` 记录基线文件的 SHA256。

| 构建 profile | 初始时钟 | 实验 V2 按钮 |
| --- | --- | --- |
| regular | V1 | native 返回 `PROGRESS_V2_UNSUPPORTED`，显示拒因 |
| experiment | V1 | `PROGRESS_V2_NEXT_RUN` 后请求新空场，实际时钟来自 native 状态 |

`experienceCapabilities()` 输出 `GRS_M1_EXPERIENCE_V1`、实际编译宏、配置的 K/Kw、features=3、布局/数值/场景 revision 和 `GRS_GPU_SCENE_Q20_M1_V2`。两包初始 K192/Kw128。V2 选项 `T438_M1_EXPERIMENT_V2` 仅作用于隔离工程的 `grsm1`。250ms 属实验 V2 时限。M2/M3 回报 `UNSUPPORTED_M1_CONFIG`。

页面使用 native run 身份和接受库存显示状态。沙水选择切断上一个源。DOWN/MOVE/UP 经原控制器传入，UP 关闭源，已接受 J 继续在同一 run 下落。手动模式停止传感器订阅，模式切换后重新按下才发源。退到后台释放页面 pulse session，恢复使用新 session。若 native 要求新 run，使用“新空场”。模拟场景由 XComponent 和 GPU shader 绘制。无 CPU 或 M0 snapshot 导入路径。

```powershell
node test/t438/generate.cjs .codex/local-test/t438-regular regular
node test/t438/generate.cjs .codex/local-test/t438-experiment experiment
node test/t438/page_host.cjs
node test/t438/build_host.cjs
```

生成目录须为 `.codex/local-test` 下新目录。两目录分别执行 `hvigorw --mode module -p product=default -p module=entry@default default@PackageHap --no-daemon`，用 `python -X utf8 test/t257/sign.py <唯一标签> <生成目录>` 签名验签。报告记录最终签包 SHA256。页面 host 运行生成页的输入和生命周期方法，native host 使用 ANGLE pbuffer 和脚本时钟运行同源 App 及真实 shader。真机触摸、系统呈现及体验属于独立测试。

最终包回执为 `T438-regular-release.json` 与 `T438-experiment-release.json`。签名后执行 `python -X utf8 test/t438/check_signed_profiles.py` 对照包内 native 能力分支与冻结来源。
