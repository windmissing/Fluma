# T455 · 单沙准备两帧观察

T428 的隔离生成页使用 `T449_FRAME_OBSERVATION_V2`。仅 PREPARING 绑定当代页面、窗口、UIContext 和动作分支。观察使用该 UIContext 的 `didLayout`、严格递增的 `onFrame` 和随后消费一次的 `willDraw`。两帧各重读原生与页面快照。布局代或状态变化会清除稳定样本。READY、失败、配置、隐藏或窗口代变化结束观察。预约前同步复核动作锚点及原 T426 许可。

```powershell
node test/t455/generate.cjs .codex/local-test/t455-release-final
node test/t455/frame_diff.cjs .codex/local-test/t455-release-final
```

在隔离目录执行 `hvigorw --mode module -p product=default -p module=entry@default default@PackageHap --no-daemon`。返回仓库后运行 `python -X utf8 test/t257/sign.py T455-release .codex/local-test/t455-release-final`。签名标签须唯一。此次编译通过，最终 HAP SHA256 `7b13fb3e4fecca3867e16d484539f7ea6f659e02549d5c42c5755c3ca2725d33`。

T432 期望值复用，按新回调生命周期调整桩的驱动点。实际生成页验证 65 项一致、静态接线 7/7。缺帧、缺布局、缺绘制均有独立失败原因，恰好 3000ms 仍失败。重复 draw、旧观察回调和失效的动作锚点无法签发许可，后者经真实 reserve 门保持请求和 phase 零变更。

T408 续测使用签名回执的同一 HAP，归档安装回执和 SHA。依次跑无键盘、编辑后固定键盘收起、静止页面的两帧、重准备及后台旧回调。采集 `t455-prepare-*.json` 和 `WS_EVENT t426_prepare` 的文件 SHA 回执。新冷档外层为 `T449_FRAME_OBSERVATION_V2`，`observation.stages` 固定记录各阶段首末 app/帧时间与序号，`readiness` 保留原谓词、快照和 permit。取得同代许可后继续原 T408 hold、独立时钟和 PNG/颜色链。开发自验未用真机。
