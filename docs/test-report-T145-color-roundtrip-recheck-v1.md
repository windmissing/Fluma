# T145 色彩 roundtrip 定点复测（tester 报告 v1）

日期：2026-10-27。角色：tester。不使用真机，同 ANGLE 离线环境实测。目标：T144 a49f880。
执行器：tester 自有探针 `test/p013_native/t145_roundtrip_independent.cpp`（不复用 T144 exe）+ 标准加密交叉校验脚本（结果 `test/reports/t145/p145_results.json`，5/5 PASS）。

## 结论

**5/5 PASS。T144 色彩 roundtrip 链路独立复核成立；6144B fixture 完整摘要独立确认；未发现链路缺陷。** 未复跑 T126 生产 MRT/raw（按卡要求保留原状）；未执行完整六常量/alpha/dither 矩阵（NOT_RUN，不派产品返修、不签全 numeric）；不签设备 PASS。

## 复核内容

1. **T144 全链复跑**（清除旧结果后 fresh build+run）：`status=VALID`；`triRead=[128,128,128,255]` 为 default GL_BACK 实绘实际读回；`sampled=[55,55,55,255]` 经真实 `sampler2D`；目标 LINEAR RGBA8 FBO complete、纹理/FBO 绑定正确；13 项逐阶段错误全 NONE。失败路径为显式 FAULT，无常量绘制冒充采样。
2. **tester 自有 4 灰阶矩阵**（超出 T144 单点覆盖）：`[0,64,128,200] → [0,13,55,148]`，全部经 SRGB8_ALPHA8 上传实际字节 → sampler 解码 → LINEAR FBO 读回。与数学 sRGB EOTF 期望逐点偏差 0/0.07/0.04/+0.72 LSB——**200→148 的 +0.72 LSB 为硬件 sRGB 查表量化**，与 T144 同口径 1 LSB 判据内；逐点 delta 已记录在 `t145_independent.json`。alpha 255 直通（sRGB8_ALPHA8 的 alpha 通道 LINEAR 存储）。
3. **fixture 完整身份**：`flat_0_1_0_1_0_0_0.bin` = 6144B（1536×float32），标准 crypto SHA-256 = `32870e766b0d501e17d0093edd00336969354dd2ebae141183b9233d3cccc353` ✓；旧 1536B 前缀 hash `e86423b1…` 复算一致，其 `REVOKED_AS_FULL_FIXTURE_HASH` 状态正确（前缀身份保留、不冒全量身份）。
4. **T144 manifest 交叉验证**：fixture/roundtrip.json/build.txt/run.txt/exe/source 七项 SHA-256 全部与标准 crypto 重算一致（exe 320000B、source add4b194…）。
5. **数学链自洽**：128 → EOTF 解码 0.2158605 → LINEAR 目标直写 55.04 → 舍入 55。128→55 非容差凑出，而是 sRGB 硬件解码 + LINEAR 目标直写的确定性行为。

## 边界与声明

- 仅证明独立 host（ANGLE 2.1.18846/Intel UHD630）D 链路；不外推设备端。
- float 连续 stage、六常量矩阵、dither 交互：NOT_RUN（沿用 T144 范围声明）。
- 无产品代码改动、无 gamma/容差/阈值变更；T126 既有生产 MRT/raw 三 SHA 证据未触碰。
- 探针源码快照与原始 JSON 已归档 `test/reports/t145/`。

交 manager 验收；如需 float 连续 stage 补证另行派卡。
