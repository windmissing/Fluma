# T54 G2 上传与接口：部分交付

2026-10-26，developer。不使用真机，不接产品页面，不宣称 G1/G2/材质效果通过。T53 冻结期望直接消费、不改 checker/fixture；本卡 **未完成、接口尚未冻结**，三项精确合同等 architect 裁定，问题已评论原卡。

## 已实现

- `material_uploader.{h,cpp}`：SHA256（空串/abc/百万 a 标准向量自测）、alpha255、精确两包长度，整包先验证再自有复制，至多 active+pending，覆盖 pending 明确 discardedGeneration。
- GL backend：RGBA8 2D_ARRAY 两对象（256×256×3 / 64×256×3），unit1/2、NEAREST、无 mipmap，PBO解绑、UNPACK_ALIGNMENT4、rowLength/skip/images清零。Pump 每次≤65536字节，只跨完整像素行/层；分配/上传失败保留旧 active。完整上传才 READY。
- 新增五个 NAPI 导出及 ETS types/controller：stageMaterialBundle / pumpMaterialUploads / canActivate / activateMaterial / discardMaterialStage。旧 P0 drawFrame 和 typed-array 域不动；Uint8 新入口单独检查真实 byteOffset/byteLength、detach 和 buffer 范围。Stage 校验七版本/apiVersion2、固定布局、必要 metadata/有限值、session/surface/context，复制整个已解析配置（含 grainBase、种子、几何、lambda、warp、hash/revision）。
- owner/mutex/current/解绑沿既有会话；激活整代指针与自有 metadata/FrameToken 原子切换，旧 GPU 命令引用由 GL 删除语义保留。没有暴露任意 JS texture handle。完整 palette/uniform/fog 的下游同代消费还需 G3/G4。
- 生命周期 Reset 保留 active CPU；恢复从自有镜像重建 pending，重传 READY 才再激活。context 失配不绑定旧 texture。generation/epoch/字节/CPU/texture/pump/discard 遥测随每个 receipt 返回。独立 fallback 回调同 tick 完整 legacy→清前景→隐藏整块 GLES；即使 legacy 抛错仍执行隐藏。页面实际接线归 G4。

## 待裁定边界（未擅自决定）

1. T53 A04：不符应报402，但 stage 的 appearanceConfigRevision 与哪个已绑定权威 revision 比较未定义；新配置本来需要新 revision，不能自行要求与旧 active 相同。
2. A05：身份冲突应报400，但 materialBundleId/assetGeneration 的唯一性范围、同身份同字节重 stage/恢复/复用规则尚未定义。当前尚未实施冲突比较。
3. C02：正整数≤65536域、无上传/保留pending已自测；精确 receipt.code 尚未定。当前无效 budget 明确抛 CONTRACT_PENDING，**不是最终 receipt 合同**，不把此例标 PASS。正常合法预算路径可独立验证。

因此不能报告 T54 完成、不能把当前五接口视为已冻结并解锁依赖完整接口的 T55。该状态是合同缺口，不是请求新增验收门。须由 architect 给精确基准/规则/码值后补三例，再交完整完成报告。

## 离线证据与边界

`python test/run_p054_material_g2.py` 运行真实 C++ core、真实 NAPI bridge（合成 JS/GPU 边界）和 ETS controller；读取 T53 23项原 expectation 生成 test/reports/t54/cases.json：15 PASS_OFFLINE、5 PARTIAL、3 BLOCKED_CONTRACT。PARTIAL 明确为 B02/C01 旧代真实 material draw 尚待 G3，D02 全配置消费尚待 G3/G4，E01/E02 产品整层回退尚待 G4；不以桩假称产品通过。

测试同时覆盖既有真实 bridge/lifecycle 全套回归，未放宽阈值。Shader/驱动/GPU 上传与色彩、生产资产效果、真机均未验证；本轮字节为明确 SYNTHETIC_TEST_ONLY，不冒充 T48 真实图。不重复实现 T48 生成器。

hvigor ArkTS + native + PackageHap 构建零错误，原有警告保留；最后构建记录 test/reports/t54/build.txt。新增源文件及 native/CMake/host runner 变更以本次 commit 为准。SandCanvas、EntryAbility、WaterSandField、P012WaterSandRenderer 无本卡变更。
