# T250 原 REQUEST_VALIDATE 故障接点

2026-10-29，developer；不使用真机。隔离工程 `.codex/local-test/t250-v13`。

实现 cut6/BEFORE 的原输入损坏注入：完整 faultPlan 身份校验后，仅在指定 root、ordinal 的 `requestIdentityHot` 原检查前临时令 plan.ownerId=-1；由原检查自行返回 `P011_HOT_STALE_HANDLE`，finally 恢复原输入，调用者继续既有失败/回滚/λ重试路径。没有直接返回或抛出伪业务码。Worker 支持该 plan；Launcher 可用 `txFaultCut=6/txFaultEdge=BEFORE/txFaultRoot/txFaultOrdinal` 构造同 build/backend/freeze/PID 身份。

每个 fixture 独立 cold reset 证据，保存原 cut 前后 authority raw archive、event/lineage 原档、叶首码、source 最终 receipt 与 root after；另封存最终 events/lineage。cold 归档实际 UTF8 字节进入 resourceEvidence，不冒 driver 峰值。

两 freeze 臂 **24/0**：原首码4、注入输入恢复后叶前后 authority/event/lineage 原字节相同；原 source 在首次失败后沿既有 λ 重试，最终准入9.6/请求19.2，累计准入计数/质量与 receipt 一致、heavy drift≈0。未注入冷对照与撤销注入再冷跑均 COMMITTED 且池原字节一致。自测初版曾错误假设整个 source 必须零写；已根据原 λ 重试合同改为验证真实成功前缀与回执，不将 fixture 的全量准入前置断言当产品首错。既有聚焦 **59/0**。这是 cut6 BEFORE 覆盖，非20 cut全绿。

证据：`test/reports/t250/v13-cut6.json.gz`（含原档）、`v13-focused.json`、`v13-manifest.json`、`v13-dictionary.json`、`v13-package.log.gz`。native/ArkTS/PackageHap 构建与新签包结果见 `test/reports/t257/T250-v13.json`。

**部分交付，看评论。**其余39个cut/edge组合仍无原码注入器，manifest 精确标 NOT_IMPLEMENTED；完整父子作用域/峰值尚缺，不能记整个T205合同或设备GPU PASS。原切片仅在隔离生成器安装，生产热路径/默认路由未加 faultPlan。
