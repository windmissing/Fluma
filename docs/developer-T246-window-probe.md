# T246 独立真实窗口探针：部分交付

2026-10-29，developer，不使用真机。编译/未签名包和 host 协议测试已交；产品 completion 与完整真实生命周期尚未实现，SignHap 失败，状态看评论。不得作为 T247 全量产品探针已齐。

`test/t246/generate.cjs`（复用 T248 独立工程复制器）生成独立 `libt246probe.so`、SURFACE XComponent 页面、私有 EglContext 副本；未改公共 GL/NAPI/CMake 或 T236 主核。页面选择 M0 K192/512 或循环 M1四档，不存在 CPU/Canvas 上屏回退。

同一个 window context 的 compute 写整数 SSBO → storage/update barrier → fragment SSBO fetch → 背景、床、粒点示意完整画面 → FBO blit → swap。vertex 不读 SSBO。这是探针示意画面，**不是 M0 真实物理发布**。失败报告 FAULT，不把遗留可见像素报 CURRENT。原始 shader compile/link log 转 hex、三 shader SHA、owner/context/surface/token、原 GL/EGL、wait、单帧 CPU inclusive 时间输出到 T246 JSON。

六档资源直接消费 T233/T238 当前 manifest；三 bank/双 event/双publication 分成独立对象，M1 delta并回相应对象，各档总 payload断言。实际 glBufferData、全对象 compute写入、首尾word map校验，三幅RGBA8完整target（两帧+LastGood）同时存在；range base 使用 max(256,实际alignment)，内部U32仍4B。错位/越界在GL调用前拒绝。日志按对象报 bytes/rw/live/peak，不把设计limit冒实分配。program/driver/RSS字节仍 NOT_MEASURED，不称完整资源峰值PASS。context退休未确认保留计数并quarantine；不重复分配逃故障。

1024B tail目前是 `T246` **探针身份形状**，含高低owner/context/surface/lease/token及done；不是T241产品completion256，也没有产品stage/audit/stock证书。按钮cancel/lease/resize/context/late是消费前身份失配钩子；不是已发生真实GL迟到、后台重建或share-group双向fence。真实回调建/变/毁会使代失效，但完整产品尾包及真实延迟生命周期需后续集成。公共平台owner可复用私有probe入口 `run(profile,hook)`、三个XComponent回调及`protocol.h`；真实completion接点须沿T236产品wire，不可接受此探针tail作物理commit。

host：256词逐词篡改、5种旧代、高word、cancel/lease、alignment和溢出全部断言通过。shader/window执行 NOT_RUN。ARM64 native/ArkTS/PackageHap通过，SignHap 00303116密码字段长度错误，保留配置原字节。实际工程 `.codex/local-test/t246-evidence`；复现：`node test/t246/generate.cjs .codex/local-test/t246-NEW` 后同 T248 hvigor命令。`test/reports/t246/` 归档原构建/签名错误与 ELF/HAP身份。产品尾包、真实迟到测试面与平台owner接线是实质剩余开发，不是仅等设备。
