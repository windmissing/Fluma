# T406 校准与配对工具交接

不使用真机。T394 旧生成器和 `test/t394/pair_display.py` 保持原语义；T406 在生成目录追加独立校准 shader、冻结计划入口与浮点几何采集，不修改常态材质 shader。前后景仍仅 XComponent+shader。

1. `node test/t406/generate.cjs .codex/local-test/<fresh-dir>` 生成隔离工程；按通常 Hvigor 流程构建。已签交付见 `test/reports/t257/T406-complete.json`。
2. tester 在包的独立标定图模式获取实际窗口尺寸/浮点全局 box/vp2px；预约三组 native ROI（各边≤64、总双目标≤98304B），分布于左下、中间、右上。手动预约后导出 manifest、SHA sidecar、两目标原RGBA和阶段/hold 封存。校准 shader 版本/hash 在原生 manifest 中显式输出。它是诊断图，不能作为材质效果样本。
3. 整个截图工具调用必须位于 hold 中。用 `python test/t406/capture.py <full.png> <call.json> <program> <args...>` 包装实际无损截图程序（无需 shell）；输出原始 Python monotonic 调用前后 ns、stdout/stderr、退出码、PNG SHA。不要将文件拉取放到封存结束后，也不要用手填 captureWallMs 代替原始调用区间。
4. 建 capture spec（相对 spec 的路径），包含 `manifest/hold/png/capture`、真实 `deviceId/toolConfiguration`、`bounds:[l,t,r,b]`、`screenshotSize:[W,H]`、`rotation:0/screenshotScale:1`，以及独立来源的 `clockMap:{source,offsetMs,uncertaintyMs,hostToDeviceRate}`。clockMap 把 Python ns/1e6 映到页面 p013NowMs 域；不得从被测图或手点时刻拟合，必须保留外部同步证据。整个映射后区间连误差包络均须被 hold 包含。
5. 仅标定 spec 增加 `registration:{source:"INDEPENDENT_GEOMETRY_PREDECLARED",models:[...]}`。每模型预注册 `native:[nativeW,nativeH],coverage:[left,top,screenWidth,screenHeight],rule:"PIXEL_CENTER_NEAREST",mirrorX:false,mirrorY:false`。模型候选来自独立边界/浮点几何，最多64个，不能观察 A/B 粒子后补候选。正向 native y 为底左，截图 y 为顶左，转换由工具完成。旋转/非 nearest/未证明的过滤或颜色规则返回不可判。
6. `python test/t406/screen.py calibrate cal-spec.json calibration.json`：25 个预登记网格点分训练/保留验证，再验证全幅不对称横纵 cross。必须唯一模型，candidate 必须精确命中独立 shader，back 颜色模型需精确匹配已声明 IDENTITY 或 sRGB-decode；不容差拟合。冻结 ID 绑定设备、窗口/context/surface/geometry、原生尺寸、浮点 box、截图配置。任何身份变化需重标。桌面 ANGLE back 有1码偏差而不满足已声明规则时，校准应拒绝。
7. 从 authority/typed profile/XY/meta 和独立几何预先挑三个 screen 核心，写 `cores.json` 为 `[[x,y,w,h],...]`。动态按旧粒子、新支持区、背景顺序；静态按 BED、AIR、BG 顺序。`python test/t406/screen.py plan calibration.json cores.json plan.json` 冻结 planId，计算每个核心的完整 native 反查包络（含所有 nearest 必要边界列），任一 tile>64 或超双目标预算拒绝。不能事后削掉坏点或补猜背景。
8. 将 `calibrationId|planId|x,y,w,h;x,y,w,h;x,y,w,h` 填入包的冻结计划入口。动态 A/B 自动预约共用此计划，不能另算逐臂偏移。静态手动预约也必须用该 native tile；不相符工具拒绝。切回原材质 shader，分别采静态中性/非零 gains 和 CONTROL/LOCK A/B。导出的材质 manifest 必须带相同 calibrationId/roiPlanId；原始 screenshot receipt 与 hold 都独立采集。
9. 静态 `python test/t406/screen.py single spec.json calibration.json plan.json`；动态 `python test/t406/screen.py pair A.json B.json calibration.json plan.json`。输出不同的 V2 结果码；逐像素 RGB 直接对 back，不放宽1码；candidate/back RGBA 差和 native alpha 独立记录，截图不用于验证 alpha。保存映射后全部样本数、差异坐标、理论投影 mask hash/像素数/边界不确定坐标。候选算术域0.0003px，边界点仍参与全部 RGB 比较；不由理论/实际覆盖数差推断 AA。

动态 EMPTY_BED/AIR 夹具额外核理论旧/新支持与无污染背景，用预登记背景核心的唯一 back 码识别原生支持占据，排除算术不确定点只用于几何解释，绝不排除 RGB 样本。非均匀背景或缺投影数据显式不可判；不把该 AIR 专用占据解释套到厚床。single 输出完整投影证据与三ROI直接配对，不据此宣称材质设计效果通过。

复跑：`python test/t406/units.py`；`node test/t406/controller_units.cjs <generated-project>`；`node test/t406/build_host.cjs <generated-project>`；旧 `python test/t394/pair_units.py`。全部为非机工具自验。真实机中性厚床、纹理和 CONTROL/LOCK 的系统显示结论由 tester 另交，不改写 T396 历史 NOT_JUDGABLE。
