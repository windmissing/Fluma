# T254 明水 C 长 frameToken 拒绘首次归因

2026-10-29，architect；原卡 `6ae2a419e4b0d25fef8cf085`。消费 T252 正式报告与 `entry/src/main/ets/render/P013ClearWaterTypes.ets`、`P013ClearWaterController.ets`、`entry/src/main/cpp/p013renderer/napi_bridge.cpp`。不使用真机、不重跑测试、不修改业务代码。

## 证据链与归类

真实 C 页面在 `P013ClearWaterController` 把 `JSON.stringify(clearWaterCConfig())+';'+JSON.stringify(clearWaterCBConfig())` 作 configDigest，`ClearWaterNativeFrame.bind()` 再将它和完整 run/epoch/geometry/commit 等字段拼入 frameToken。T252 记录 frameToken 约999 UTF8字节、`sess=1/context=1/surface=2`、每帧 `INVALID_ARGUMENT @ CW_V3_PARSE`。native `DrawClearWaterSceneV3` 的布尔链在任何 GL 前调用 `MaterialText(frameToken)`；该函数调用通用 `String()`，其 `char b[256]` 在 `n>=255` 时拒绝。故障**可由当前源码和真实长度独立推出**；T252 未保存每个条件逐项布尔值，不能把其它字段的潜在超长也宣称已排除。`configDigest` 同样走 255B 上限，实际约700B，所以仅扩大 frameToken 不足以打通 parse。短 `offline-explicit-fixture` 回归遗漏了真实配置长度。此为桥层工程缺陷；GL/shader/物理均未执行，不能判其失败，也不转 designer。

## 工程合同与返修

已在 T222/T226 顶部修订：仅 C V3 的 frameToken ≤8192 UTF8字节、configDigest ≤4096 UTF8字节走专用有界读取；`napi_get_value_string_utf8` 先查真实字节数，checked 加1后一次分配，再读并核实复制长度、无内嵌 NUL、非空、无截断/超限。完整字符串须按原字节保持，在 ArkTS→NAPI→native scene receipt/LastGood 间同 token 严格比较；FNV/payload hash 不替代身份。通用 256B `String()` 保持其余字段/路由的既有约束，不做无界全局扩容。明确区分 `CW_V3_PARSE_IDENTITY_LENGTH` 诊断与其它 parse 问题，原 `INVALID_ARGUMENT` 状态码可不改；请求内存随本次调用释放，长 token 不长期多份保留。

**developer 接力，不使用真机**：仅修改 C V3 专用 `napi_bridge.cpp` 读取分支及必要的诊断/测试入口，维持公共 GL/NAPI hunk 唯一 owner；新增真实 `clearWaterCConfig()+clearWaterCBConfig()` 构造的 ArkTS→NAPI 测试，断言当前约999B frameToken 与 configDigest 全字节一致并能越过 parse；另测 255/256、8192/8193、4096/4097 UTF8字节界、非 ASCII 字节、内嵌 NUL、错误类型与过期 token，超限明确拒且不触 GL。编译和非真机自测后交独立 tester 定点真机复测 s144 同路径，记录新包 SHA、真实六层 GL/CURRENT/swap/像素与旧 FAIL 是否解除；仍有故障按首阶段回 architect。全部前/背景仍只新 XComponent+shader，故障 FAULT 或显式完整 STALE，禁 Canvas。签包前置沿 T253，不重复修签名。
