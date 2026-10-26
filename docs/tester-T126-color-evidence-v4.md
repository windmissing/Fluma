# T126：色彩证据 V4 重建（标准 SHA、真实 raw 与实际执行矩阵）v1

日期：2026-10-26。角色：tester。任务卡 `6adf424be4b03273ad863c01`。合同：T121 §2–§5（revision `T115_IDENTITY_CAPTURE_V4`）。执行器：`test/p013_native/t126_identity_capture_v4.cpp` + `build_t126.cmd` + 驱动 `test/p126_color_evidence_v4.cjs`。证据：`test/reports/t126/`（p126_results.json + t126.json + 3 个 raw .rgba + environment.txt）。**13/13 PASS**。不改产品；T115 v3（459bd4a）历史 raw/JSON/结果保留不动；不使用真机。

## 1. 标准 SHA 与 tinysha 缺陷定性（T121 §3）

**V3 tinysha 缺陷独立复现**（测试端 JS 复刻漏 `w[i]` 的轮函数）：对空串、24B uniformBits、25B stage 占位串输出**完全相同摘要 `da5698be…`**——摘要不依赖消息内容，定性为算法缺陷而非单字段错误。JSON 中 stageShaderSha256 与 uniformBitsSha256 同值即直接证据；prodRawSha256==dbgRawSha256 与 diff168676 并存同源。

**V4 修正**：native SHA-256 恢复消息字 `w[i]`（revision `SHA256_FIPS1804_WI_RESTORED_V1`）；驱动用 Node 标准 crypto 独立复算每个摘要。

| case | 内容 | 结果 |
|---|---|---|
| D01 | 标准 known-answer 向量：空串 `e3b0c442…`、`abc` `ba7816bf…`、FIPS 多块向量、1000×'a'；同长不同内容必须不同 | PASS |
| D02 | manifest 3 个 raw 文件逐字节重算：native 与 std crypto 全一致、byteLength 与文件大小一致 | PASS |
| D03 | shaImplRevision 落盘；3 个 `.rgba` 是磁盘真实字节 dump（非内存 hash） | PASS |

## 2. 真实 raw 归档与同内容等价（T121 §1/§4 核心缺口闭合）

| case | 内容 | 结果 |
|---|---|---|
| A01 | **生产 MaterialPass vs MRT debug 变体（同材质数学+stage 捕获）同 SRGB8_ALPHA8 附件：全图逐字节 diff=0**（两个 184,320B raw 文件独立比对，非 hash 推断）；(88,78) 均 [247,219,157] | PASS |
| A02 | 常量 debug vs 生产 diff=168676 属预期不同内容（两 raw 均归档可查）；点 (88,78) 一致确认 sRGB 写时编码 | PASS |
| A03 | 整数 stage 真实捕获：baseB=173/grainB=102/strataB=127/k=1（RGBA32UI 附件），baseB=173 与 T88 partB 归档一致 | PASS |
| B01 | RGBA8 对照组实际查询=LINEAR、complete | PASS |

**A01 是本轮决定性增量**：T121 §1 指出 V3 的 debug 是常量 uniform 色非等价变体、diff168676 不能作等价证据；V4 用真实生产 shader 数学+MRT stage 的 debug 变体在同一附件上实现逐字节等价，且原始字节落盘可复核。

## 3. 实际执行矩阵（T121 §4/§5：recorded 字符串全部换真实执行）

| case | V3 缺陷 | V4 实测 | 结果 |
|---|---|---|---|
| C01 | triangle.writeRead="recorded" 硬编码 | 三角形**真实绘制**（drawError=NONE）+ readback 128 + sRGB 采样路径 128=128 + sampleError=NONE；backEnc 查询=SRGB | PASS |
| E01 | backEncoded=128 硬编码、无合成 | **实际混合绘制**：dst=128（EOTF 种子线性值写后编码）、front 经 `glEnable(GL_BLEND)+glBlendFunc(ONE,ONE_MINUS_SRC_ALPHA)`、src alpha 0.25（bits `0000803e`）→ **front 实测 172**；dither disabled 分支真实切换验证；scissor 禁用 | PASS |
| G01 | stageErrors 是单字符串 | `stageExecution`：integerStage=EXECUTED、floatStage=NOT_RUN（附 note：连续 stage pass 不在本采集，T101 f32 bins 仍是 float 证据） | PASS |
| H01 | driver Object.values 对字符串误过 | stageErrors 为 **typed object**（7 个分立字段：srgbFboStatus/srgbEnc/prodDrawError/prodReadError/mrtComplete/mrtDrawError/dbgDrawError，全部字符串） | PASS |

## 4. 六常量矩阵（T121 §2：报告数值不再复制错标题）

| id | 值 | F64 hex（LE） | 独立复算 |
|---|---|---|---|
| correct-EOTF-linearize | 0.28314874042999213 | `e73903e51b1fd23f` | `((145/255+.055)/1.055)^2.4` 一致 |
| correct-OETF-encode | 0.7788681486941016 | `003151e57cece83f` | `1.055*(145/255)^(1/2.4)−.055` 一致 |
| t80-historical-wrong-formula | 0.21716935993161873 | `3abe4ca134cccb3f` | `1.055*(145/255)^2.4−.055` 一致 |

值+位型+id 三重断言（%.17g 全精度落盘），uploadedPaletteBits 24 字节 6 float 位型真实 digest（F02）。**V3 的 "OETF=0.215760" 错误标签不再出现**；T121 §2 的算术（EOTF≈0.283149、OETF≈0.778868、T80 错公式≈0.217169）全部独立复现。

## 5. 边界与移交

- 旧不存在 raw 标记：V3 的内存 hash raw 无法补 dump → 本轮**新 run 取证**（三文件落盘），不伪称补齐历史 run（T121 §5.2）。
- float 连续 stage 本轮 NOT_RUN（明确标注），T101 的 5 个 f32 bin 仍是 float-stage 证据；未重复执行无关测试。
- numeric 仍 MEASURED_NOT_CALIBRATED；**无 ±1 容差、无 gamma、无新阈值**；结果回 T121 architect 分流。
- 产物：`test/reports/t126/`（p126_results.json、p126_probe_run.txt、t126.json、3×raw、environment.txt）；源 `.codex/local-test/t126/` 同步保留。
