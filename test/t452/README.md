# T452 · M2 V3 developer 部分交付

当前状态为 `PARTIAL / PRODUCT_NOT_IMPLEMENTED`。交付独立 `gpu_resident/m2` 的机器 manifest 常量生成、四档资源计划、冷能力探针、单组 GPU 索引和选对、宽整数合并及 SOURCE128 行封签原语。尚未创建可用的产品 M2 run。未使用真机。

```powershell
python -X utf8 entry/src/main/cpp/gpu_resident/m2/generate_layout.py --check
cmd /c test\t452\build_host.cmd
```

真实 ANGLE SwiftShader GLES 3.1 核四档 SSBO 限制、同时分配和释放，保留每档原始 GL 错误值。四档 4/0，最高档常驻 payload 87,196,368 B，另有 27,654,144 B 冷 event64 scratch。合并原语 6 个反例通过，覆盖带符号 tie-even、宽边界、来源不匹配、heavy、半径和 unsigned MAX_ID。单组排序/重选及 SOURCE128 封行和污染拒绝检查 5 项通过。证据为 `test/reports/t452/gl-primitives.json`，不作为完整事务差分。

剩余开发范围为 SOURCE 13 pass 的输入/去重、pending origin、库存、配对出生、部分前缀、终态、独立 spare 重放及发布封尾。QUANTUM V3 还需接有效来源审核、来源累计、逐 word 重放及 SOURCE128/QUANTUM64 分轨。短接受、scene/lease、冷 trace、App 入口与签包未实现。

尝试复用冻结 M1 的审核/接受结构时核到两个明确接缝。`m1/source_owner.cpp::Valid` 对 revision3 或 SOURCE stage mask8191 返回 false。`m1/quantum_replay.glsl::quantumAudit` 把 V3 已分配的 header132–144 和来源账14/15 按 V2 保留位核零。直接接现有 owner 或改常量绕过旧审计会使合法 M2 root 拒绝或缺少 V3 不变量。现阶段代码仅供独立 V3 实现接入，未放宽 M1。续作由 developer 完成独立 SOURCE/QUANTUM 审计与 owner/scene；四档完整 root 跑通并签包前，T453 缺生产差分输入。未发现资源阈值需要放宽，也未确认外部设备或设计阻塞。
