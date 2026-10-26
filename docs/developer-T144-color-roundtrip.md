# T144 色彩 roundtrip 探针修复：开发回执

日期：2026-10-26。角色：developer。不使用真机。

新增独立 `t144_color_roundtrip.cpp`，执行 default GL_BACK 绘制与 readPixels，把实际读回的四字节上传至 `SRGB8_ALPHA8` 纹理，再以真正 `sampler2D` 输出到明确 `RGBA8`/LINEAR FBO。记录 default 与目标 encoding、FBO complete、纹理/FBO 绑定、逐阶段 GL 错误及清理。失败输出 FAULT，不将第二次常量绘制冒作 sample。

`run_t144_color_roundtrip.cjs` 每轮清除旧结果后编译/执行，按 sRGB EOTF 独立校核采样字节。完整 fixture 以 `1536 × sizeof(float32) = 6144B` 归档并 SHA256；旧 V4 `1536B` 前缀 hash 标为 `REVOKED_AS_FULL_FIXTURE_HASH`，未覆盖旧 T126 报告。manifest 记录本轮文件长度/摘要、exe 和源码摘要。T126 已成立的同 SRGB 产品 MRT/raw 三 SHA 未改动。

离线验证：`node test/run_t144_color_roundtrip.cjs` PASS，ANGLE host 实际读回 `triRead=[128,128,128,255]`，真实采样后 `sampled=[55,55,55,255]`，目标 LINEAR、complete、绑定正确，逐阶段错误均 NONE。完整 fixture SHA256 为 `32870e766b0d501e17d0093edd00336969354dd2ebae141183b9233d3cccc353`。仅证明本独立 host 环境链路；后续 T145 独立补证由 tester 负责。
