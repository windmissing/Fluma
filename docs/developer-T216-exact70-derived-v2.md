# T216：exact70 全 writer 与同 commit publication 开发交付

日期：2026-10-28，developer；不使用真机。原卡 `6ae19c8ae4b0d25fef69c7e1`。消费 T214 `c280602` 与 T207；保留 T211/T212。仅新 XComponent+shader 含前景与背景；禁止 Canvas 上屏或故障回退。生产 Index、原 EmptySandDemo/V1 默认入口不切换，新增 `pages/P051DerivedV2Demo` 为显式新 run 隔离入口。

## 实现和支持组合

新目录 `entry/src/main/ets/physics/derived/` 实现 Exact70、DerivedView/实际 tuple barrier、严格 dry domain、完整投影、双 publication、冷 checkpoint、私有 facade/demo。C128/capacity256/N2..512，policy/cache/envelope/publication/checkpoint/migration 依 T214 分版；私有 facade 仅接收 `isolated=true + P011_DRY_CACHE_EXACT_V2 + CPU_ORIGINAL_F64 + T216_DRY_WRITERS_V2`。无 wet、clearwater、GPU 或未知 writer/插件/alias/callback 配置入口。参数深持有，旧 raw/schema4 不冒 V2。物理 invert、live route/config/legacy pool mutator不通过 facade；reset 构建新 owner 后切换，V1→V2 live migration 未启用。T212 cold adapter 额外检查 numericPolicy，V2 在 native configure 前显式拒绝，不改 wire/native 字节或隐式 CPU fallback。

Exact70：70×U16 以2^-1074为单位，原 bits 解码，显式 carry/borrow、整数 guard/sticky ties-even RN64；生产无 BigInt/字符串数值转换/F32/FTZ，不改原 pMass/床/位置/速度表达式。live zero/负值/非 finite/非法phase/column/质量>256拒绝；数学 primitive 可编码±0。overflow/underflow预演后零写入，tuple失败恢复已改派生原words，由原 root 恢复物理域。

每次真实 birth/merge/LAND/DEGRADE/crossing/motion 读 old/new stored tuple，按旧成员全部remove、新成员全部add；原cache增减只留V1分支。membership不变的motion/WAIT不做A减加，仍更新内容身份。merge使用实际RN survivor质量与实际floor。token私有构造、当前引用一次消费，含owner/view/branch/root/cut/serial/slot generation/old ID；foreign/reentry/stale拒绝。copyPoolHot、canonical↔Hot transfer、外层backup/journal/motion同policy拷贝sidecar，不每seed重建A。原数值audit/guard/预算阈值及错误顺序不删除。

ParticleField保留无import的纯池边界，使用池内P011ParticleDerivedBindingV2窄接口依赖倒置；没有修改既有无依赖断言。原 ParticleField 非Hot allocate/retain/prepare/applyMotions 等旧写路径在绑定V2后明确拒绝；实际原Hot已全接屏障。copy/restore只接受已注册same-policy bank；raw checkpoint在独占cold candidate验证，不能向live owner注入。床/U实际直接写点接dirty标记，Sink原reserve/guard/stock顺序后按当前canonical cache严格flush。原legacy wet-only渠道在closed facade配置不可达，不能下一UI帧扫描补cache。

## 分支、投影与资源选择

采用独立同步 whole-bank sidecar，沿原source/λ/seed/preview/Hot/外层恢复边界配对，未宣与T211共享同一套派生backing或已启WORD_UNDO/D2证书。实际注册9个运行bank（canonical、Hot frozen、patchBefore、patchScratch、patchAfter、particleBackup、journalBefore、backup.particleState、motionSnapshot.particleState），另1独立audit。**cold view容量选12，未把实际第9套漏报为8**；对应实际逐buffer资源见allocation.json。原whole-bank业务源/预算/field/event恢复仍权威。

每bank A/count/dirty/generation/id/header原words复制，恢复不round/rebuild；field surface/dirty/revision亦配原backup/motion/Hot checkpoint保存并恢复。控制branch/content serial不因数据回滚倒退。TxWordUndo基础增加width2并测相邻U16隔离，当前运行仍whole-bank，不假称已用undo减少复制。

M2 projection额外9份完整 `A/count/dirty/cache/8field/32F64 header`，实际427392B；匹配cut先flush当前canonical视图，再捕获与比原bits。先核owner/run/root/substep/绑定与policy；preview/authoritative控制branch可以不同，不把控制serial相等误作语义相等。原plan/pool投影不删。

allocation.json 对N192/512逐条遍历本实现的真实冷owner、唯一ArrayBuffer/alias/路径/type/长度/字节；含原field/Hot、全部derived/projection、双publication、固定故障原值副本、1024×40F64根work表。该数值是backing分项，**不是RSS/JS heap/native/driver峰值总量**。对象/引用容器/closure/receipt/token/seal/临时byte-view仍有分配，不宣零分配。外部冷checkpoint/复制无限持有为UNBOUNDED_EXTERNAL，runtime/native/driver总量NOT_TOTALLED。

245个T214历史静态review点逐条标SUPPORTED/EXPLICIT_BULK/V1_ONLY/REJECT_V2，原文件/行/函数/理由保留；含读/复制，并非245个独立writer的自动alias证明。当前AST另登记127个直接写点。支持证明还依closed facade与实际copy/barrier接点；旧公开field/page仍是V1，未用readonly注解冒充无alias。

## 严格域、最终 verifier 与封存

同view固定 `mix=RN(bh+bl)`，`top=RN(RN(mix+mh)+ml)`，`P=RN(cacheH+cacheL)`，`envelope=RN(RN(RN(top+suspH)+suspL)+P)`。床finite各非负、U数值0、envelope finite且≤256；无EPS/max/clamp修补。dry getWaterVolume直接+0，initial/expected水同policy为0，真实occupied单列语义。DEGRADE若产生U真实触发新域失败，不吞悬沙/强制LAND/改记外源。

原snapshot的sandTop旧按 `(bh+mh)+(bl+ml)` 重组，可能与V2左结合末位不同；V2复制改取同版本sandTopScratch，V1保留旧图。此是数值policy明确对齐，不借改renderer容差消除问题。

每最终source/成功substep准备边界一次独立audit bank从完整authority pool建A/count，校allocator/ID唯一/有限域、逐limb/count和RN cache原bits、严格field。不是每SINK_ROW或UI重建。固定radix ID验证不借新AVL作oracle。原全部logical audits及1e-9/0.001/guard/速率判据保留。

inactive先完整复制候选，再最终verify并逐buffer核复制与authority一致，拒绝“verify后同rev又写”的fault注入；这些额外比较字节计counter16，不藏为免费。proposed源epoch/summary/receipt、commit/stateEpoch/ledger key预填inactive，所有fallible复制/检查/seal构造先完成，随后callback-free原admission/源账尾部+active切换。STEP预先确定与endpoint一致的key；复制入口不在读取时向live field补数组。

双bank含完整field/pool/source summary与版本header。源成功不等于CURRENT；之后step失败不抹掉成功源前缀。renderer gate先核版本/seal/buffer identity/run/commit，原SOURCE/F32/material/native严查保留。UI拿到的是DerivedPublishedSlotV2受控adapter，仅可读token元数据/调用新管线draw；sealed原数组私有，cold导出为独立复制且标PUBLICATION_COPY_NOT_CHECKPOINT。没有承诺防任意内存篡改；可信无writer的内部bank、受控无重入copy是seal前提。测试仅在Node反射注入private bits/pointer，非生产公开hook。

首个新fault用P011_DERIVED_FAILURE_V2 1001–1006独立namespace，原首码字段仅原检查真实失败才填。outer UNKNOWN/M2失败是transport字段，不冒派生首因。first fault在预分配注册域冻结**未刷新**pool13/cache/A/count/field/预算及源/Hot标量；不调updateSandTop后再抓。原值导出明确仅注册域完整，不把renderer/native/未注册opaque域宣完整。新fault后owner停止，重置新owner才继续，保留首因。原物理错误单列原receipt。

## Checkpoint、工作量与复现

V2 checkpoint外层保存policy/config/原params/authority/A/count/dirty/generation/id/run/stateEpoch/commit。只在quiescent原边界导出；import先深复制到新candidate，原结构/源summary/守恒检查后重建audit逐word比较A、count、cache、包络和slot身份，版本/参数/源/NaN generation不符拒绝，旧owner不变。成功新stateEpoch大于checkpoint与当前sealed epoch，重复恢复不复用旧FrameToken。未启旧run迁移；旧raw不补V2标签。

counter区分build slot、ordinary tuple remove/add、limb、round/cache/dirty/domain/public verifier、derived copy/rollback、raw capture、publication/projection、sealed候选byte compare、λ、seed、原pool复制与build-live-add。1024个固定root work行按原source/step分母记录root/kind/ordinal/成功/commit前后/UTC wall耗时及32项真实delta；总counter=所有root delta+根外UI copy/cold checkpoint等实际项。超容量/负时钟显INCOMPLETE，不拿缺尾当零。失败λ/preview成本保留。未改变原field C²残余，也不宣整事务复杂度/设备时延收益。

开发自验目录为 `test/developer-derived-v2/`，没有修改tester独立DD01–DD12用例：

- Exact70独立BigInt oracle 3208项；tuple/domain/width2 nested undo 7项全部通过，覆盖实际值、inactive残值、相位/跨列/merge、tiny、非法tuple/重入/stale、坏cache/严格U床域、原word复制恢复。
- N192/512各10次双phase源、300step/310public roots，倾斜与LAND后strict关系全程成立，根counter闭合；各311 verifier包括cold初始1次，不按UI帧重复计。不是设备/独立产品PASS。
- checkpoint正常/篡A/坏版本/坏generation/参数图/重复恢复epoch、publication版本/指针/verify后写注入、native configure前拒V2、UI copy不build/verify、源成功后原step审核失败保留前缀均通过。
- V1 OFF/LEGACY/TX对修改前a27d92c，各N192/512×6命令共36条，原回执及中间snapshot raw bytes一致；旧基线解压在`.codex/local-test/t211-baseline`（`git archive a27d92c entry/src/main/ets`）。这仅证明所列V1回归，不把V2轨迹记对V1等价。
- N192/512在step0/123/299共6份同commit publication通过真实P013ScenePreparation+原strict material/F32准备，未替换谓词；这是离线打包，不代表GL/draw/swap或可见性PASS。
- 既有Hot/M2/particle/gesture/planning/s3/s4/input/advection/clearwater-B十通道exit0；T212桥9/9、T211树同输入回归保持。独立T217、真机T218另接力。

主工程与STRICT_O2隔离兼容工程真实PackageHap构建零错误；根包含新增显式V2页面，隔离GPU兼容包只证明旧bridge编译/回归与V2拒配置，**不是可用V2 GPU包**。源码/资源/ELF/buildId/unsigned HAP SHA、完整产物zip与构建日志在test/reports/t216。未签名、未安装、未做真机/E2E、未测GPU收益或材质长期稳定性。

复现：`node test/developer-derived-v2/run_t216_all.cjs`，`run_t216_regression.cjs`，`run_t216_legacy_differential.cjs <old physics目录>`；JBR PATH下`hvigorw --mode module -p product=default -p module=entry@default default@PackageHap --no-daemon`。GPU兼容隔离生成器沿T212：`generate_tx_gpu_project.cjs <全新目录> STRICT_O2`，产物归档沿`archive_tx_gpu_post_build.py`。后续以本报告delivery-manifest的最终源码与包身份为准，不借旧signed包。
