# T226 明水 C V1 开发交付

> **T254 工程返修接口**：T252 真机首帧 `CW_V3_PARSE`，在 `DrawClearWaterSceneV3` 的 `MaterialText(frameToken)` 就会被通用 `String()` 的256B本地缓冲拒绝，六层 GL 未执行。C V3 专用 `frameToken/configDigest` 按 T222 修订做有界两次 UTF8 读取及完整字节比较；不改通用 String、不截断、不用 hash-only 代替身份。加入真实 `clearWaterCConfig()+clearWaterCBConfig()` 经 ArkTS→NAPI 的回归接缝，覆盖边界/超限/嵌 NUL，成功后由独立 tester 复测实际窗口。未验证前 C 设备显示仍 FAIL。

任务：6ae1c075e4b0c3103c10d6ef；2026-10-28。不使用真机。依据 T219 工程稿与 T222 §6 补篇（optics16、面积高光与重力接受）；交付仅阶段 C 基础，真实在途水包与 V4 属 T227/T228。

## 实现

- Engine 增加 busy fence 下的只读 `copyCurrentSceneInto`，绑定当前 canonical、run/stateEpoch、源提交和独立历史 sealed B；源成功后 B 失败仍显示真实当前源，第二步失败保留第一成功角。最近失败三元组独立记录，reset 清终态可见性。旧 A/B 序列化、receipt/status 与物理数学不变。
- 新 CTypes 与双 publication：固定 profile8/proof8/origin/wait/xy/meta/header64/ledger64/optics16，拒绝 owner/sibling alias、截短、offset、detach；ID 顺序归约原始 x64，预分配投影 scratch，完整校验/原始字节 FNV1a 摘要后切换一次。摘要仅辅助完整性，FrameToken 仍逐字段核对。
- `copySourceReceiptInto` 仅查原 dedup 终态，精确 key+raw payload；NOT_COMMITTED 独立枚举。不 enqueue、不 drain、不增加扫描计数、不插入终态。ACK 不能反馈成功。
- FrameClock 原始差分后换秒，不 clamp/补偿；SourceController 单 pointer、每 active tick 最多一次，MOVE 只更新位置，切源/UP/CANCEL/后台/geometry/surface 失效清 pending 和时钟，切源后必须新 DOWN。水源 V1 直接进入 W。
- GravityPort 冻结当前 tick 传感器样本，目标显式饱和到 ±30，按补篇 tau/omega 逐成功子步接受；失败不更新角。stale/无效/倒退暂停整 tick、停源；synthetic 仅显式离线入口。shake 只记诊断，候选均 PENDING_CALIBRATION。
- 独立 `drawClearWaterSceneV3` 六参数入口；旧 V1/V2 parser/layout/config57 不复用。九个 typed view 全量复制到 native 定容包后，严格 F64 profile/原 x64 ID 顺序 P/守恒摘要/投影/配置绑定/完整 token/原始字节摘要校验。重复或倒退帧在 GPU 前拒绝。
- 新六层 XComponent+shader：BACKGROUND → SAND_BED → PARTICLES → CLEAR_WATER → WATER_SURFACE → FOREGROUND。沙床/沙粒复用 DualFieldPass；水透射按真实深度，面积高光按物理像素与列矩形交集，邻列贡献一次求和。无旧水雾、无 Canvas、无读正在写的 FBO。
- native Begin/六层/Present/Swap 全成功才 Commit 目标与 last-good/token；采用 FAULT 策略，未实现旧帧 STALE replay。设备显示验证不能由 swap 回执代替，`displayPresentVerified=false`。
- 独立 `pages/ClearWaterDemo` 与 Index 导航，已有 DEMO 默认入口不变。页面背景/前景均由新管线绘制；透明输入节点不绘制。controller 定容 256×24 trace 和 32 counters，GPU 时间未测。

## 开发验证

| 检查 | 结果 | 证据 |
|---|---|---|
| C Node 自测 | 14 PASS / 0 FAIL | test/reports/t226/selftest.json |
| 生产 C++ validator/atomic/oracle，MSVC Od strict | 29 PASS / 0 FAIL | test/reports/t226/native-host.txt |
| 同一生产 C++，O2 strict | 29 PASS / 0 FAIL | 同上 |
| 既有 A source 回归 | 26 PASS / 0 FAIL | test/reports/t226/a-regression.txt |
| 既有 B physics 回归 | 35 PASS / 0 FAIL | test/reports/t226/b-regression.txt |
| 真实 ArkTS + arm64 native + PackageHap | BUILD SUCCESSFUL，零错误 | test/reports/t226/package.txt |

native 测试直接消费 Engine canonical capture 生成的二进制帧包，覆盖原始 x64 跨列边界、header/profile/proof/optics/token 破坏、全六层及 Begin/Present/Swap 失败不提交、重复帧、W0/sigma0/深水/水下透射、浅水面积、镜像、Fresnel 和相邻两列面积相加。Node 覆盖 ACK/终态、查询无副作用、切源/时钟/源窗口单次、失败 B、alias/detach、K192/K512 和保留槽。没有修改独立 tester 用例或把预试 NOT_IMPLEMENTED 改成产品 PASS。

本机没有执行 GLES runtime shader compile/link、设备颜色空间、可见显示与交互端到端，后续 tester/effector 负责。编译通过不代表这些产品指标已通过。未使用真机，也未安装构建。

## 产物与成本

unsigned HAP SHA256：`4980ede4ff50d3e212393657ae58409fa0543b38eb9c9287bb3377f4b885662a`。
HAP 内 arm64 `libp013renderer.so` SHA256：`97afbbf7b7c79f6c0413c34c9436b9860c23ba99643f37326cd1ca9c243d0a13`。
源码逐文件 SHA、构建/资源口径见 `test/reports/t226/delivery-manifest.json`。

native 双包固定最大 K512，主机 `sizeof` 双包为 110464B（含对象头，不含 string 的外部分配）；K192 也保留该容量，不冒称模板 K192 内存。ETS 两份实际长度包另各有 128×F64 fold 与 4B projection scratch；controller trace+counters 为 49408B。native profile/bed staging 6144B，GPU profile 双纹理 8192B、sand 转换纹理 2048B、粒子 VBO 16K。

C 使用独立整帧双目标，新增 `8×widthPx×heightPx`；旧场景目标可能同时存在，必须另计，不声称复用/零增量。GL program/VAO、字符串/视图、驱动、context/resize 重叠、RSS 与时延均未测。无 GPU 性能或设备效果 PASS 声明。
