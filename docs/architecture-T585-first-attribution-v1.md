# T585：M2真实手势到SOURCE空账本首次归因

2026-11-18，architect，不使用真机。输入T510 v4原件、T575实际t537生成树、签包收据、T553；证据 `analysis-T585-source-evidence-v1.json`。不将G9 T572 adapter当M2实现。

## 原件裁定

T579b-witness包/ELF字节已核。execution原件确有SourceClaimWitnessV1、witnessBytes176、designatedEnqueueSeq0、dropped0、rows=[]。这证明字段/空结构可导出，**不证明enqueue/claim/terminal功能已经通过**。inputStages只有NEW_RUN相关3行；其scope为run1/context3/session1/permission4，顶层execution却为run2/context4/session2/permission5。1187 accepted空Q及后继CLOCK_GAP不能替不同scope的真实源消费。typedType=-1是未观察到typed decode的默认事实，不是typed拒绝原因。

M2实际页已有`touch`→model.down/move/up，controller.down核active/ready/坐标、setSource，**在按住后的frame tick**等sourceOpen和第二个递增pulse才组32U32 offer；Up只是停源。它不是G9的UP一次IMPULSE模型。短swipe、未命中、未打开源、重设geometry/时钟GAP均可能无enqueue。当前没有同scope Down/allowed/tick/offer原件，不能断言swipe一定不触发或页面未接线。

## 已证工程风险（独立于本轮空账本原因）

准确t575-witness-tree bridge在持`App::mutex`的FrontLocked区域，再用`std::lock_guard`锁同一个非递归mutex调用onClaim；首次真实队头命中可自锁。当前空rows没有走到该分支，不冒现场已经死锁。该hook还在`run->Offer`结果之前写CLAIMED，拿latest.nonce同时冒planId/rootNonce并按其非零猜rootKind，可能是上一根Q；尚未取得真实SOURCE事务。

Ledger忽略enqueueSeq参数、仅选最老未claim行，不按完整scope核准；没有SOURCE terminal hook。8×176B的行池也不是原T553单份≤256B窄见证，最坏8条JSON可能超过附加2048B，不能靠global2MiB writer代替局部限额。claimCopy nothrow分配后无null分支/独立HostScope就解引用，同样是具体工程缺口。保原开发已交登记，这些进入后续修复，不回写T575已完成行。

## 修订接口/真源

修改 `test/t537/overlay.py`、实际bridge head/Offer回执及 `input_diagnostic/source_claim_witness.h`、cold_diagnostics serializer，不能只改另一个未生成bridge。App队头在入队时保存稳定enqueueSeq与intentSeq；已有App锁内用`OnHeadClaimLocked`无再次加锁，或先拷不可变head/scope后锁外记录再重核，统一锁序，禁止观测为取证唤醒owner。

状态严格分ENQUEUED/ELIGIBLE/CLAIM_ATTEMPT/CLAIM_ACCEPTED/ROOT_STARTED/TERMINAL；Offer返回OK前不能写CLAIM_ACCEPTED，成功后绑定实际SOURCE owner分配的planId/rootNonce/kind/seq，不能用latest旧nonce。NO_CLAIM保存第一次具体门（not ready/sourceOpen/queue freshness/epoch/route等），失败Offer不Pop、不冒消费；真正terminal处记completion/outcome/sourceCommit或具名未消费。scope退休单独封见证，后继空Q不覆盖。

恢复原**一个指定seq、≤256B核心见证、附加JSON≤2048B**预算；全域计数可留固定scalar，不能放大8条后隐瞒成本。显式`designate(enqueueSeq)`只影响观测选择，不改队列算法。按(scope,intentSeq,enqueueSeq,inputEpoch,sourceGeneration)匹配，避免跨run最老行；seq单调耗尽具名失败，晚回scope先拒。冷copy先gate/信用/null检查，使用现有caller-owned诊断块，锁内定长copy、锁外JSON，超限完整INCOMPLETE，所有HostScope与异常RAII计入原4MiB总信用。

## T593精确可执行取证

先保同包/PID/boot/window/scope与实测touch overlay bounds、pixelMode=false、页面active/sourceKind/ready/sourceOpen及geometry稳定；在目标层中心真实Down**按住约150–250ms跨至少三个真实pulse**再Up。不是伪造timestamp、合成SOURCE或延长GAP；若pulse/permission坏按正常拒绝保原件。采Down handler enter→model.allowed与坐标→setSource回执→首/次tick/delta/sourceOpen→raw32/hash→typedOffer回执→enqueueSeq/head→实际SOURCE owner/terminal。每步同一次intent，缺哪步精确标NOT_OBSERVED，不再只导五分钟后的最后状态。

第一轮可用已有page/inputStages接口先取Down/门/typed原件；若首Down都没有，保存调用UI设施/位置/节点可交互状态，不预断平台不触发。若已有ENQUEUED则在上述死锁修复前不把空ledger宣通道完成；真正产品缺口由developer处理，tester不负责实现队列。新证回来architect再分设施命中/合法拒绝/观测链/物理源事务。独立签名构建部署采集由tester，不作developer测试关闭门。

所有接力包含 [共同约束](architecture-T581-T589-handoff-policy-v1.md)。当前只完成首次归因/工程闭包，原同seq链仍未证，未运行产品测试。
