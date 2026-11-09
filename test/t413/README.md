# T414 隔离单沙采集交接

T414 实现位于 `test/t413`，按 T413 §4 生成独立包。`T413_DISPLAY_HOLD_V2` 和 `T413_CAPTURE_CALL_V2` 由本目录 `screen.py` 严格读取。T406 的 V1 工具仍用于历史档。

## 离线自验

```powershell
node test/t413/generate.cjs .codex/local-test/<fresh-directory>
node test/t413/controller_units.cjs .codex/local-test/<fresh-directory>
python -X utf8 test/t413/units.py
python -X utf8 test/t413/runner_units.py
node test/t406/build_host.cjs .codex/local-test/<fresh-directory>
```

对隔离工程运行 `hvigorw --mode module -p product=default -p module=entry@default default@PackageHap --no-daemon`，使用 `python -X utf8 test/t257/sign.py <新标签> <隔离工程>` 签包。构建日志、离线结果和交接签名报告分别位于 `test/reports/t414` 与 `test/reports/t257/T414-finalized.json`。

## T408 采集准备

1. 核对签包 SHA，打开 `P051DerivedV2Demo`，选择独立标定图、厚床或动态 A。中性/纹理、ROI、冻结计划及 `maxHoldMs` 均在配置区设置。ROI 必须为三组原生底左坐标，每边至多 64。B 在 A 成功封存、归档、释放后通过“注入B”准备，实际注入在 B 预约成功时执行。动态 A/B 使用同一冻结计划；每次用计划的三组 ROI 文本预约。
2. 点“收键盘并准备”，等待 native 几何与页面最新几何代一致。核对完整 XComponent 未裁切、无键盘或遮挡，以同一稳定 layout 读取控件 id、整数 bounds、截图尺寸和 `t413-status.json` 的 `status.geometry` 浮点原件。状态控制区高度固定。配置、可见比例、位置、尺寸、缩放、surface/context 或前后台变化后重新登记，不使用失效窗口。只在捕获前做 layout/可见性核对。
3. 用下面的配置启动 `python -X utf8 test/t413/capture_hold.py <配置.json> <新输出目录>`。预填真实设备/路径/点击坐标和截图 spec。`hdc_action.py` 读取应用写入的冷文件，使用 `hdc -t <设备> shell cat <应用文件绝对路径>` 和 `file recv`。先在 tester 环境验证该签包的沙箱文件可读取；若设备权限要求不同取回方式，可用等价 `commands` argv 传输，保留实际原输出和文件字节。以下模板中的坐标、尺寸、几何和误差是假值，采集前必须替换。

```json
{
  "hdc": "C:/path/to/hdc.exe",
  "deviceId": "实际设备ID",
  "toolConfiguration": "uitest_screenCap_native_png_rotation0",
  "appFilesDir": "/实际可读的应用files路径",
  "clockUncertaintySource": "填写独立测定的app ACTIVE时钟量化误差来源",
  "clockQuantizationMs": 0.001,
  "taps": {"reserve":[0,0],"status":[0,0],"export":[0,0],"clock":[0,0],"seal":[0,0],"cancel":[0,0],"release":[0,0]},
  "spec": {
    "bounds":[10,400,1010,1400],"screenshotSize":[1080,1920],"rotation":0,"screenshotScale":1,
    "registeredGeometry":{"revision":"T413_GEOMETRY_V2","globalX":10,"globalY":400,"widthVp":1000,"heightVp":1000,"vp2px":1,"geometry":3,"visible":true,"configuration":"FIXED_PANEL_XCOMPONENT_V2"},
    "registration":{"source":"INDEPENDENT_GEOMETRY_PREDECLARED","models":[{"native":[1000,1000],"coverage":[10,400,1000,1000],"rule":"PIXEL_CENTER_NEAREST","mirrorX":false,"mirrorY":false}]}
  }
}
```

各动作控件 id 为 `t413-reserve/status/export/clock/seal/cancel/release`。显式 `commands` 可替代 HDC 默认模板，键还包括 `screenshot/pull/terminal`。支持 `{output}`, `{remotePng}`, `{localPng}`, `{actionId}`, `{attemptId}`, `{holdId}`, `{requestId}`, `{phaseSequence}`，作用域在预约成功后确定。`reserve` 输出独立的 CaptureReply。`status/release/cancel` 输出原生页面 HoldStatus，`clock` 输出原始 ClockSample，`export/seal/terminal` 输出文件 `{name,bytes,sha256}` 并取回原件，`export` 也取回同名 sidecar 和原 RGBA。`screenshot` 写唯一远端路径，`pull` 取回 `{localPng}` 临时文件。脚本校验无损 PNG、字节和 SHA 后才在新目录原子改名落盘。任一步失败保留全部原件和 failure 回执，状态仍需按当前身份显式取消/归档/释放，不能点新预约解忙。

runner 采两个截图前时钟样本及一个截图后样本，保留每笔完整命令前后 host monotonic ns 与 app ACTIVE 毫秒原值，计算所有可行正斜率映射的最坏 L/U。设备或 host 休眠、时钟回拨、不同 boot/session 等失去连续性证明的窗口均重采。仅 hold 与 capture 全部身份/区间匹配才进入空间/颜色检查。

```powershell
python test/t413/screen.py calibrate <窗目录>/spec.json calibration.json
python test/t413/screen.py plan calibration.json cores.json plan.json
python test/t413/screen.py single <窗目录>/spec.json calibration.json plan.json
python test/t413/screen.py pair <A窗>/spec.json <B窗>/spec.json calibration.json plan.json
```

冻结模型和三组核心、完整反查列及 RGB 零容差沿 T406。独立负控另开窗口，先 seal 后运行截图和完整拉取，应只因 `CAPTURE_INTERVAL_OUTSIDE_HOLD` 拒绝。离线工具正反控是合成证据，系统截图可见性和真实时钟关系由 T408 记录。
