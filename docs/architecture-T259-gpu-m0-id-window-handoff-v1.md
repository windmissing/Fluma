# T259 GPU M0 末 ID 与真实窗口接线工程裁定

2026-10-29，architect；原卡 `6ae2a5c5e4b0f11bdf398c23`。消费 T236 V2 报告、原卡末 ID 评论 `6ae29016e4b0ab67702bde28`、产品源码与 T233/T241/T238 manifest。**工程缺口已裁定，T236 离线继续；真实设备和完整产品 GR 门仍未执行。** 不使用真机、不重跑物理测试、不改业务代码。

## 1. 末 ID：合法 MAX 与 U32 耗尽哨兵

现行 `m0_source.comp` 在 `births>UINT_MAX-next` 时拒绝，因此 `next=MAX,births=1` 被拒；`RootOwner::ValidCompletion` 还以 `c[nextSandId]<header[nextSandId]` 拒绝任何 `MAX→0`。这是 T233/T241 未定末值的**工程合同缺口**，并非硬件/算法不可行证据。已在 T233、T241 与 M0 manifest 的 `sandIdExhaustion` 定版：真实粒子 ID 范围 `1..MAX`，ID0 永不合法；header.nextSandId=0 是**本 run 永久耗尽**，初始1。用 `next>0 && births>=1 && births-1<=MAX-next` 的无溢出条件接受连续分配；末真实 ID 为 MAX 时 nextIdAfter=0。sourceTerminal32 word9、sourceEvent16 word13、completion256 word92、接受后 header word39 及 receipt 投影必须一致，firstId/事件真实 ID 可为 MAX。sourceEvent、event64、完整 bank/replay 都使用真实 ID，不把哨兵当 ID；不占 reserved，不重编旧记录含义，产品 wire revisionWord=1 保持，**工程 revision/manifest SHA 已变化**。

后续新正量出生 `METADATA_REJECT/ID_EXHAUSTED=7`，原权威库存、source/physics commit、source event 不增；若新 key 且正常容量可记终态，按既定 dedup 前缀规则；同命令旧 key 仍 `DEDUP_REPLAY` 原 receipt，不能再次 sourceSuccess。量子与非出生的 metadata-only 根保持 next=0，任何 `0→非0`、饱和 MAX 复用或模回绕都拒。native 须分别验证 `next>0→更大非0`、`next>0→0`（仅成功出生且最后 ID=MAX）、`0→0`、`0→非0` 禁止，而非单个 unsigned `<` 判断。GPU source/replay/审核、terminal、completion、事件逐字段同时核；旧生成头必须按新 SHA 重生。

M1 的 M0 基线 SHA/revision 已同步，沙 ID 继承该哨兵；**水 ID 的末值不在本卡裁定范围**，不能写 M1 全域通过。M0 设计 payload、word index、资源公式与既有双 pad 不变；T236 旧 SwiftShader 结果绑定旧 manifest，需对新 revision 的末值 fixture 做非真机回归，不能沿旧结果宣新门 PASS。

## 2. 产品真实 XComponent 的唯一 owner 接线

现有 `SourceGpuTransport::Create/Submit/SubmitQuantum/Poll/Acquire/ReleasePublication`、`ResidentRun::Offer/Advance/Pump/Pause`、`ResidentScene::Draw` 是**未接 App 的 native 接口**，`GetFoundationInfo.connectedToXComponent=false`、production feature=0。T246 私有 probe 证明的是示意帧/私有 tail，不能注入 T236 accepted bank。指定唯一平台 owner 串行改 `p013renderer/egl_context.*`、`scene_compositor.*`、`napi_bridge.cpp`、CMake 共享 hunk，算法 owner只改独立 `gpu_resident/`；遵守共享树不回退他人改动。

1. 独立实验 page route 冷开新空 M0 run/feature bit0；UI 触摸仅形成完整 sourceCommand32（run/sequence/inputTime/位置/原始量/seed），角输入与 stop/reset 是有序控制，不在 UI 阻塞 GL 或把旧 F64 pool 灌入新 bank。不可混用旧 pair、T246 probe 的 GL 名/尾包或生产默认路由。
2. XComponent window create/change/destroy 与 page show/hide 只向**同一 native GL owner**交有界异步消息；owner 在当前 EGLDisplay/context/draw surface `Create`，记录 thread、owner/context/surface/geometry epoch。输入按 `Offer` 入 FIFO16，按帧单调时钟调用 `Advance(deltaNs,frame,control,angle,capacity)`；resume 首 tick dt0、F0只控制carry，不能用墙钟补跑。`Pump` 每次至多提交/零等待 poll 一 root，UI 不调用同步 fence wait，背压/失败有明确回执。
3. `Poll` 返回 **真实 T241 completion256** 并经产品字段/尾 fence/GL/tombstone 校验后才切 bank+revision+receipt，source ACK 与视觉 CURRENT 分账。已接受 publication lease 由 `Acquire` 交 `ResidentScene::Draw`；其完整前景/背景/床/活动沙全走 XComponent+shader，只有 draw+实际 swap 成功才 CURRENT。`ReleasePublication` 以 render-release fence 退还槽，BUSY 留 owner loop 非阻塞再 poll。异常只 FAULT 或持有完整有效旧新管线 frame 时显式 STALE，禁 Canvas 和半帧。
4. resize/background/context loss/窗口销毁使旧 surface/context/lease/GL 名及 inflight token 失效；已接受历史不可伪回滚，未接受 candidate tombstone/quarantine，迟到仅退休不 map/commit。明确新 run 或从受证冷 checkpoint 恢复；M0 当前不支持仅凭 completion 摘要恢复旧 GPU pool。`Release` BUSY 时不强销/复用资源；记录真实 callback/epoch/rawEGL/GL、fence、lease 与 live/peak。后台退出/重进不得把旧 LastGood 标 CURRENT。

上述 owner/page/NAPI 接线与末 ID 修复均可**离线开发并编译/非机自测，不等待设备**。真实 window 资源、呈现、生命周期由独立 tester 真机串行取证，签包沿 T253；不把 PC SwiftShader 当硬件 GL。T247 私有 probe 与产品 GR01–09 分开，T240 等真实 M0 接口到件而非 probe 示意帧。

## 3. trace、矩阵与成本交付边界

当前 `SourceTransportStats` 仅累计 coldMapped/dispatch/barrier/fence/poll/map/upload/alloc 与首 GL 错，不能证明每阶段完整成本。T236 开发应加**固定容量**逐 root/per pass trace：run/owner/context/surface/feature/manifest+shader SHA、root kind/nonce/accepted base、source/physics commit、stage/pass ordinal、dispatch group/binding/range bytes/barrier bit、CPU submit/fence poll/map/pack/draw/swap 时间、实际 GPU timer 有效/disjoint、event与completion字节、lease/retire、首 GL/EGL/firstMismatch、成功/拒绝/fatal/迟到和 offered/accepted 分母；环满标 INCOMPLETE，不填零。source32/34、quantum59/68 是设计阶段账；真实拆 pass 须升版记录实际数。每 tick G≤1+F≤4、最多五份1024B完成包与失败冻结成本都计入，不能只报成功平均。

ID 唯一性当前串行 O(K²)，数学重算与事件写集重放两轮审计是正确性硬门；成本须计 `K192/K512 × 每root(唯一性比较数、重算、replay、event arena、fence/map、scene/swap)` 和失败帧，不能仅用 dispatch 常数推收益。若该复杂度达不到目标，交 designer 另版保持守恒/确定 ID/事务的算法，不让 developer 未经设计裁定删审计或逐热点微调后声称收益。开发补末 ID/异常、满池/重复/partial/迟到/lease/context loss、逐 stage fatal/翻位、trace 与非机自测；独立 tester 用自有 oracle 做完整 GR01–09、设备同 token 与 UI 成本。未覆盖项标 NOT_IMPLEMENTED/NOT_TESTED；developer 编译和非机自测交付即可，真机与独立测试不作其关闭前置。

**交原 T236 developer 继续**：按本稿先重生成 manifest，落实末 ID 的四类状态迁移与事件/终态/完成包一致性，再由唯一平台 owner 接真实 XComponent route 和非阻塞 pump/lease/swap 生命周期，补固定 trace/失败矩阵并构建。不要改 `docs/后续工作清单.md`，唯本人 T236 行状态有限例外；不改滴答同名清单、不覆盖共享修改。产品 M0、硬件能力与收益在 tester 到件前仍未证。
