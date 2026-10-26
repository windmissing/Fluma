# T114 原生像素整数条带开发回执

日期：2026-10-26。developer。原卡 `6adf3613e4b0e6ce82cce07c`。不使用真机。

## 交付

- `P013BaseByteEvaluator.ets` 按冻结 F64 操作次序从实际 layout2 Float32 profile 求每个原生像素的 H/L/M、RGBA8UI 共 `12W` 字节；有效域、镜像、spark、尾数、SHA 均按 T105。
- accepted scene 配置由 51 个 F64 扩至 57 个；仅新 V2 配置写入六个编码域 palette 原值，摘要包含原值及版本。旧 scene packet V1 和旧 pipeline 版本保持原义。
- `base_strip_uploader` 使用两份 CPU 槽和两张三层 RGBA8UI GPU 纹理；严格校验 descriptor、实际 SHA、RGB/alpha 域及逐列覆盖；支持部分上传、替换、租约、失效与资源计数。ETS 另有一份可复用 `12W` scratch。
- P1 scene 使用 `P013_SCENE_PACKET_V2_BASE_STRIP`，先 stage/pump 条带，再以 `65536 - 条带实际上传字节` 为 static material 上传预算。material 和条带同 token/profile/配置/几何/epoch/spark/mirror 检查，七 pass 与 swap 成功后才一起提交。故障继续报告 FAULT 或兼容的显式 STALE。
- 新 shader V2 通过 `usampler2DArray` 以原生 j 与层 k 读取整数底色，不再做第二次 palette/gain/clamp/round；V1 分支保留。全画面仍在新 XComponent+shader 管线，无 Canvas 上屏或失败回退。

## 离线证据

- `node test/run_t114_base_evaluator_units.cjs`：13/13 PASS，含 W128/256/1178/3840、分数 box、mirror、spark、三层逐字节独立 oracle，T88 新值 172。
- `test/run_t114_base_strip_core.cmd`：PASS；预算非法与 `<4`、分段、租约 abort/commit、双纹理上限、域拒绝与释放。
- `test/run_t114_scene_atomic.cmd`：PASS；失败 swap 不提交两资源、成功双提交、身份错配拒绝。
- `test/run_t114_base_strip_gpu.cmd`：桌面 GLES 真实 RGBA8UI 三层逐字节读回 PASS。
- `test/run_t114_scene_gpu.cmd`：桌面 GLES 完整 V2 shader scene 使用不同条带得到不同画面，PASS。
- T56 scene units/core/bridge/GPU 旧路径回归 PASS；T55 material units PASS。T24 的历史“尚未嵌入 shader”断言已被 T55 正式取代，不作为当前回归门。
- hvigor `BuildNativeWithNinja`、`CompileArkTS`、`PackageHap` 均成功，无源码编译错误。完整 `assembleHap` 在既有 `build-profile.json5` 签名配置因密码长度 `<32` 于 `SignHap` 失败，原文件未改。日志：`test/reports/t114_build_wip.txt`。

## 边界

本卡没有真机端到端、完整色差标定或性能结果；T115/T121 等数值取证和设备任务仍独立。签名配置阻断完整已签名包，未将其误报为源码编译失败。
