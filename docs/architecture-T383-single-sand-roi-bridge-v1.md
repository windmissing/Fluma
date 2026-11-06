# T383 · 单沙合法 ROI 被拒首次归因

2026-11-06，architect，P4。离线核对 T373 原卡、原始冷档、T370 合同和 T372 隔离签包的生成源码。工程返修范围在隔离窗口探针及测试入口。

## 1. 首个分歧在 N-API 长度单位

T372 本地签包 SHA256 实核 `98339521d6eea6ab58fe6799167d6ca7c78ba350af3243665d94b91359066992`，四个探针源文件与 `test/reports/t372/generator.json` 匹配。实际签包构建目录的生成版 `single_sand_napi.inc` 有同一旧门。`test/reports/t373/t372-cold-receipt.json` SHA256 `9c288098ada747191951f5bd2ee5ec955bf0b6fb300c6dd540d259984a6e94fe`。T373 使用未编辑的默认文本 `0,0,16,16`，页面接受预约。冷回执的帧 serial=177、count=239、窗口 1178×1186，requested/actual token 相同且渲染 CURRENT、swap 成功。探针回执独立为 `CAPTURE_FAILED/INVALID_ROI`，readbackBytes=0，ROI 空，两份像素附件均缺。

| 边界 | 源码及证据 |
|---|---|
| ArkTS 解析与预约 | `test/t372/SingleSandWindowCold.ets::parseWindowRoi` 将默认值变成 4 元素 `Int32Array`，期望字节数 16。隔离 controller 把它作为第五参送入下一次通过 prepare 的 draw。 |
| Native 第五参 | `test/t372/generate.cjs` 生成的 `n==5&&!windowProbe.available` 分支设置 `INVALID_ROI`、清空读回字段并绑定当前 packet；完成后写入同帧 `windowReceipt`。原档带该帧与状态，证明这条第五参分支已到达。 |
| TypedArray 解码 | 该分支把 `napi_get_typedarray_info` 的 length 直接用作元素数，先判 `length<=12`，再传 `Configure(coords,int(length))`。本项目的 Harmony Ark N-API 按字节报告长度。现有 `p013renderer/napi_bridge.cpp::TypedView` 对同调用的 32 位视图按 `expectedCount*4` 核长度，同次 CURRENT 已通过该门。T42 真机回执也记录 512 个 Float32 元素到 native length=2048。默认合法 ROI 的预期 native length=16，旧上限 12 使它在调用 Configure 前被拒。2/3 个 ROI 的 32/48B 同样被拒。 |
| 几何及目标 | 默认矩形在 `ProbeRoi::Fits(1178,1186)` 界内。只有 Configure 成功才把探针指针传给 Draw；同次有效 shader/swap 可继续，candidate/default back 的 pre-swap readback 均未执行。T372 host pbuffer 直接调 Configure，跳过有单位错误的 N-API 入口。 |

据签包源码、同回调既有 32 位视图合同和 T42 真机对照，归因为隔离探针的字节/元素转换错误。旧回执把 N-API status、类型、长度、空指针和 Configure 拒绝折叠到同一状态。T373 的第五参实际返回码和 length=16 仍须由修订包直接实测记录。

## 2. 隔离探针修订合同

1. 保留普通四参绘制。第五参先查 callback status、参数个数、`napi_is_typedarray`、`napi_get_typedarray_info`、`napi_int32_array`。按原生字节契约只接收 16/32/48B，验证非空数据、未 detached、4B 对齐 offset、有效 backing range 和 data 与 base+offset 一致。复用 `TypedView` 的现有校验逻辑与平台约定。字节长度验证后只做一次除以 `sizeof(int32_t)`，得到 4/8/12 个元素，复制到有界 native 值再交 Configure。按窗口实际像素边界检查 ROI 与实际绘制尺寸一致；沿用底左原点、整数坐标、1–3 个不超过 64×64 的半开矩形。拒绝保持 fail-closed。
2. 冷回执版本升级。记录同一有界请求 ID、JS 侧元素数/字节数、native status/type/raw byteLength/offset/backingBytes、有效元素数、矩形、窗口/几何及请求与实际 token。分开报告 arity、typed 信息/类型、长度、buffer、bounds、预约失效、各 GL 目标和 swap/unbind 的首失败。早退、重建或换 epoch/fixture 要终结或取消该请求，保留一次可取的失败结果；导出重试只重试保存同一回执。已 arm/已完成未导出时拒绝第二笔。正常准备尚未生成有效 packet 时可等待下次有效帧，消费时绑定该帧身份。
3. 只在 T372 隔离页面加可直接点击的预填控件。保留默认 `0,0,16,16` 作为桥接冒烟，另用 T370 的冻结夹具提供 BED/AIR/BG 三矩形预填及可见的确切坐标。按当前 viewport 和实际 profile/粒子几何配准，验证三个 ROI 确有各自支持集；内容不符或几何变化时明确报夹具未就绪并重新预填。三矩形通过同一解析与预约入口，常规手输可用于负控。T373 的 uitest 字符丢失和键位污染单列为自动化输入问题。
4. 只在有效预约帧的真实 candidate FBO 和窗口 framebuffer=0/`GL_BACK`、present 后 swap 前读取同一 ROI。记录实际读点/format/encoding/stride/rowOrder、原始 GL/EGL 码、状态恢复、swap/unbind 与完整同 token CURRENT。两份 RGBA、manifest 和 SHA 冷导出。上限仍为双目标合计 98,304 B，常态帧零 readback。探针拒绝与渲染回执分别记录。渲染故障只发布 FAULT 或有完整旧 token 的 STALE，背景和前景均走 XComponent+shader。

## 3. 最小回归与派工

| 责任 | 最小交付 |
|---|---|
| developer，离线 | 对生成签包入口同一解码逻辑使用 Harmony 字节语义的替身，默认 4 元素返回 16B 先红后绿；补 32/48B、非零合法 offset、有意按元素报 4/8/12、错误类型、detached/越界、0/超过 3 ROI、尺寸边缘和 off-by-one。原来通过的 pbuffer Capture 继续覆盖双目标及状态恢复。复用实际 controller/预约入口验证重复 arm、early return/epoch 变化、只消费一次、普通四参零读回，以及导出失败可重取同一回执。交编译、离线自测和带 SHA 的隔离签包。 |
| tester，独立设备 | 签包齐后，串行复现未经编辑的默认 ROI，先核 JS 4/16、native 16B、归一为 4 元素、bounds 通过与两份 1024B RGBA（每目标）。随后用预填三 ROI 的同一冻结夹具采中性和纹理两臂，逐臂记录 token、两目标原像素/SHA、swap/unbind 和实际 GL 编码；补一个固定非法 ROI 以及不预约零读回。按 T370 分别判 candidate、真实 back 和交换呈现，再按颜色校准与独立 oracle 判断颜色链。系统截图单列。 |

T373 原档的材质切换和渲染身份结论可保留；窗口颜色链仍缺 candidate/default back 原像素，待签包复测。T370 的 host 像素矩阵沿自身前置继续。
