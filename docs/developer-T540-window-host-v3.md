# T540 开发交付：整数 window-host V3

2026-11-17。按 T532 工程稿，沿 T516→T469 实际生成链交付。不使用真机；遵用户指令不运行测试、oracle、平台 mock 或端到端验证。

生成：`node test/t540/generate.cjs .codex/local-test/<fresh>`。新版本 `T532_WINDOW_HOST_RASTER_V3`、`T532_PREPARE_V3`、`T532_FRAME_OBSERVATION_V3` 同步到 RasterGeometry / PrepareReadiness / CaptureGeometry / SingleSandController / 实际研究页与冷文件。旧 V2 raster 及混版本 capture 明确拒绝。原 packet 协议保持，输入只取严格通过三框校验的 actual window x/y 和整数 w/h，native tuple 全项校验继续执行。

原逻辑布局、236vp固定控制面板及92% allocation 不改。新增无padding/border/transform、Ltr/TopStart 的页面根 `t532-window-root` 与独立 sibling `t532-window-host`；inner/render/touch 三节点移入此稳定 host，脱离浮点 allocation 父框。host 本身透明命中，只有目标矩形 touch 节点接收事件，inner 严格 clip，框外可传给控制层。前景背景继续 XComponent+shader，无 Canvas/软件 GL 回退。

desired 继续原四边 ceil/floor；local=desired−实际host origin，要求精确整数。host window/screen 原点必须整数且关系精确、范围覆盖完整 desired，否则 HOST_NONINTEGRAL/DOMAIN_INVALID。实际 ComponentInfo 不 round，不写0.2补偿，不用 actual 反改 desired。三节点 window/screen 框严格等于 desired，旧51.799987…继续拒绝。1176与旧1177的ROI/calibration不得混用。

每次同步读取同一 page/window/layout 代，保 raw allocation/host/window/avoid/keyboard/vp2px；三节点保完整 ComponentInfo（size/local/window/screen/scale/translate/rotate/4×4 matrix）。每Component原件≤4096 UTF8字节；应用声明的物理父链root→host→inner→render/touch原件≤16项，每项整体≤256B，超额明确 INCOMPLETE_ANCESTRY，不截短后宣完整。显式应用父链以window为边界，未测OS内部定位因果不作证明。原始矩阵逐项身份检查；原件保原数值。

raw allocation/host/窗口等任一变化，即使 inward 四边不变也推进代。先撤旧 HOLD/Prepare layout，再写新位置；记录请求时 didLayout serial，至少等其后新 didLayout 才查询实际三节点。Area 回调不消费回调自带几何/不负责 ack，仅触发当前代同步重读；页面/窗口过时代 didLayout callback 丢弃。全程 observer 在隐藏时取消注册。新节点实际数据继续由原 PrepareFrameObservation didLayout、willDraw 与两帧锚放行；首配对 draw serial/frame anchor 固定保在该代原件，不用不断变化的采样时刻破坏两帧身份。native几何只作证据，不回灌 allocation。

T413预约、HOLD、键盘收起、OPEN首因、零副作用流程保留。几何变化/观察失败先撤权，失败保持明确原因，不拿旧token移动新touch。新包只证明实现与编译；整数host如果真机仍返回小数 actual，按原值严格拒绝并交architect，不自动偏移或放宽容差。

最终生成来源、源SHA、编译和签包见 `test/reports/t540/delivery.json`。native/ArkTS/HAP编译通过；隔离工程去除旧链过时 signingConfig 后采用既有本地签包脚本，工作区签名材料未改。测试与设备实际放置均 NOT_RUN/NOT_MEASURED。tester接新包先收T532§3全raw，严格三框/native一致后再做READY→预约→HOLD/PNG→独立新标定；旧a8不能改记PASS。
