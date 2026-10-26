# T115：色彩目标身份纠正补证 v3（同 SRGB8 附件对照与真实 SHA 清单）

日期：2026-10-26。角色：tester。任务卡：T115。合同：`docs/architecture-P013-T105-base-strip-and-color-routing-v1.md` §1/§6（版本 `T101_IDENTITY_V3`；T101 067fd84 原 raw/diff0 保留不改标签）。执行器：`test/p013_native/t115_identity_probe.cpp` + `build_t115.cmd` + 驱动 `test/p115_color_identity.cjs`。证据：`test/reports/t115/t115.json` + `p115_results.json`。**10/10 PASS**。不使用真机。

## 1. 目标身份纠正（T105 §1 的核心错标）

| 项 | T101 实际 | T115 修正 |
|---|---|---|
| attachment0 | **GL_RGBA8**（错标 SRGB8 pbuffer 目标） | **GL_SRGB8_ALPHA8 texture FBO**，attachment0 actual encoding **查询 = SRGB**，complete |
| RGBA8 组 | — | 独立 RGBA8 FBO actual encoding = **LINEAR**（查询） |
| default GL_BACK | — | **查询返回 SGB→SRGB**（host 支持 GL_BACK encoding 查询；查询值与功能值分列） |
| EGL colorspace | — | **12425 = 0x3089 = EGL_GL_COLORSPACE_SRGB** 如实记录——不以"缺 KHR 扩展名"推断降级，推翻"本 host 无真正编码路径" |

**决定性结果**：真实 SRGB8_ALPHA8 目标上生产 MaterialPass (88,78) = **[247,219,157]**（与 T55 归档字节一致）；debug 常量 shader（以该像素 ToneMap 线性输出播种）同点 = **[247,219,157]**——**sRGB 写时编码确认**（线性 0.934/0.709/0.335 → 编码 247/219/157）。全图 diff=168676 为**预期**（常量 vs 场景内容不同，非同类比较），不作等价主张。

## 2. 真实 SHA256 清单（sourceHex 正名）

manifest 全部身份字段为**真 64 字 SHA256**（C++ 内置 SHA-256 实现，probe 内计算）：production/debug/stage shader 源码、grain/strata fixture、fixture bin、prod/dbg 全图 raw、uniform Float32 位型（含 hex 位型串）。`sourceHex`（源码字节 hex 文本）字段废除——不再以源码 hex 冒充摘要。

## 3. 半透明与其余 T87 缺项

- fractional-alpha：实际 back 编码字节（128）+ EOTF 解码 dst（0.215861）+ 上传 alpha Float32 位型（0.25f=0000803e）+ blend/dither 状态（dither=1）落盘；精度表仍观测项无新容差；
- 正确第六常量：OETF(145/255)=0.215760（F64 位型归档），历史错误值保留历史 id；
- 逐阶段 error 独立记录不覆盖（srgbFbo status 0x8cd5/enc SRGB/draw NONE；rgba8Fbo status/enc 分列）；
- EGL 版本/查询值/扩展来源落盘。

## 4. 边界

- 未重跑 T80/T88/T101 历史（各自 raw 保留）；probe raw 以 SHA256 覆盖（全图 dump 文件未归档——hash 覆盖为身份用途，如需字节复现可由 manager 另派归档卡）；
- 本 host 有真编码 sRGB 路径（推翻 T101 的"无编码路径"推断）；设备 A/B/C 仍需；
- 未改产品/shader/配方/阈值；不加 gamma；未用真机；无 Canvas；结果回 architect。
