# T276 诊断导出开发交付

2026-10-29，developer，不使用真机。依据 architecture-T274-v2-directed-evidence-attribution-v1.md §3，仅生成隔离 T266 诊断工程；生产物理、clock/dt、shader 均未改。

## 交付

`node test/t276/generate.cjs .codex/local-test/t276-v5` 从 T266 配方生成隔离包。T266_CLOCK_V3 优先输出：首生、每60个推进帧、首非零床、since≥8秒锚点。每条 JSON 上限1400字符，包含完整 run/epoch/frame/commit、原样保存的 firstBirthAcceptedSeconds、accepted seconds/since、成功子步数、请求/准入/拒绝、丢行和锚点原因。未首生为 null；源停止不停止推进时钟。

大对象改为 T276_COLD_V1，UTF16代码单元十六进制分片；每片最多256单元、JSON上限1800 ASCII字符，包含 exportId、index/count、原字节数、分片与全载荷 FNV-1a32 摘要。摘要用于诊断完整性而非安全认证。完整 payload 保留原完整 token。`node test/t276/reassemble.cjs <hilog.txt> <outdir>` 重组 JSON 并产 index.json/SHA256；缺片、重复、截断或摘要错误显式 INCOMPLETE/退出2。无把空/缺档当成功的路径。正常帧不构造每帧大对象。

LAND 有界256行 writer 在原 recordEndpointCommit/COMMIT_FINISH 后记录真实事件；以 hot stepEpoch 去重（该路线 committedPageToken=0，不能拿0当有效封存租约），记录 run/epoch/substep、当时实际 physicsCommit、粒子前后状态、bedDelta、原首码0。Facade 完成时仅封存已保留前缀；derived 根撤销丢弃本根候选行。超过容量显式 dropped/INCOMPLETE。非机实跑捕获3条正质量 LAND；不再依靠末页为空推断历史无 LAND。

首非零床锚点提供一次 arm，候选渲染同 token 在 present 前采4×4 SRGB8_ALPHA8 编码RGBA，最多首帧及其后三帧；不切换FBO/GL状态。冷档携带原完整 token 的十六进制、frame/FBO/ROI/raw GL/耗时。必须与同 token 的完整 CURRENT receipt 联结；候选像素本身不代表 swap 成功，设备缺采不能宣产品PASS。背景前景继续新 XComponent+shader，失败原 FAULT/STALE。

## 验证与交接

- 原 T266 非机开关等价、真实部分子步成功前缀、epoch重置等 **24/0**：test/reports/t276/physics.json。
- T276 冷档/CLI缺片与破坏/源停增长/精确首生基线/有界与回滚writer/真实LAND **16/0**：export-writer.json。
- native mock GL 默认零调用、一次arm、4帧上限、原token/RGBA **9/0**：native.txt。仅模拟GL门控，不是真机采样结论。
- hvigor PackageHap **BUILD SUCCESSFUL**：package-v5.log.gz；签名及验签成功，已检查签包 ArkTS/native 接点存在。

签包 `.codex/local-test/t257-sign/T276-v5/T276-v5-signed.hap`，SHA256 `e9c0b37c66b6013849f12292d2bc3b2b128fe98e4f522d79447e9d6361885380`。签名记录 test/reports/t257/T276-v5.json；生成manifest在 test/reports/t276/manifest.json。交 T277 使用同一包续证；本卡无设备执行、无DD12通过结论。
