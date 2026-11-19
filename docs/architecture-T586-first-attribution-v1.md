# T586：真实READY与禁用预约按钮/未接入v4的分流

> 2026-11-19现役v4准备路径jscrash工程修订见`architecture-T658-v1.md`及contract表：owner由纯字段值复制/比较/固定tuple绑定票据，不在JSON对象调用.same，不以structuredClone保原型；唯一gate/响应式reserve、实际RESERVED再consume、原predicate和3000ms仅PREPARING不改。

2026-11-18，architect，不使用真机。输入T408-ready-v12、两份prepare原件、布局l2/l4/l6、T574准确生成页/overlay/v4类和签包，证据 `analysis-T586-source-evidence-v1.json`。

## 决定性原件与纠正

READY原件可信：现役gate revision为**T532_PREPARE_V3**，prepare1/page1、run/geometry同代，完整predicate通过、两次递增frame并发出permit。保这一次READY窄结论；它没有v4首因见证，也不能证明软/硬/晚回调/consume全域已测。

更直接的证据是三份实际布局：状态文本READY，但t413-reserve始终 **enabled=false、clickable=false**，bounds[184,388][408,514]。因此“三次预约点击”不能当作handler已执行三次；没有t413-reserve.json也不能独立证明handler早退。源码按钮读`this.prepareGate.state==='READY'`，prepareGate是普通非响应式类字段，而文本单独绑定@State prepareText；这支持**启用状态未响应式刷新**的工程原因。没有本轮运行栈，不把每次系统点击分发路径伪称已观测。

原报告两个推断须纠正：reserve guard阻挡的是evidence **!==null**，不是===null；当前现役elapsed仅在PREPARING检查3000ms，READY状态不会由它自动三秒失效。未接入v4 helper虽写了READY超时，也不是本次原件的实际许可路径。不能靠“更快点击/延长准备窗”绕过禁用按钮。

## v4实现缺口

T574 overlay仅另建prepareV4侧对象：active初值false，begin被插在旧gate.begin之前，拿到旧prepareId；owner辅助函数返回0/空，windowGeneration冒windowId、controller/session/surface复用同值。页面没有noteChange/bindOwner/sampleFrame/checkForReserve/consume调用，状态/按钮/预约仍用旧gate。故READY首证不等于T570完整v4已接线，不能让tester仅再截两张图给该合同整体PASS。现有v4.ready可直接置READY、consume不比ticket的prepare/page/serial/input，亦须在实际集成时修正，不能把不受原predicate约束的旁路投入使用。

## 工程闭包与接口

修改真源 `test/t574/t574_prepare_v4_overlay.cjs`、`test/t426/PrepareReadinessV4.ets`、t540/t455 page/frame生成链；不用只修local-test生成件。以**单个权威事务**集成v4，复用原predicate/三框/frame observation，不保并行两个gate各自READY。owner按真实page/branch/window generation/controller/session/surface/context/run分别取值，page show/hide明确激活/撤销；首次有效native owner可按T570只绑定一次。soft重采样、hard终止、晚回调拒绝和原deadline原件同源。

新增响应式 `@State reserveEnabled:boolean` 或等价可观察状态镜像，在同一次publishPrepareView中更新state文本、reason、subphase和canReserve；按钮绑定该可观察值。canReserve须由真实READY、configuring/exporting/evidence/capture状态得到，不是手动永远enabled。所有合法/失效路径均更新，UI affordance不代替reserve内鲜读许可。3000ms仍仅约束本次PREPARING从begin到READY，不续期；不凭本卡给READY另造TTL，READY后仍按原身份/变化和各自既有freshness门严格重核。

预约同一UI栈执行：handler entry→前置gate/ROI/fixture/capture状态→fresh snapshot和check ticket→原singleSand/capture.reserve→仅RESERVED后consume同ticket。ticket须匹配prepare/page/observationSerial/inputSerial及完整owner，不能只验valid=true。中间不await/stopEditing/改布局；若reserve失败保真实reason、不消费permit、不伪HOLD。仍保存capture原来的几何/token/maxHold/归档/PNG要求。

添加冷 `reserveAttemptWitnessV1(id)`：handler入口分配单调id，保存BEGIN/首次具名拒绝/实际reply/consume/文件IO状态，包含before/after gate与输入/owner serial、ROI是否合法和三busy谓词。每份核心≤256B、first+last两份，借既有T570≤2048B header/诊断预算；附加JSON≤4096B、沿既有2MiB writer/4MiB信用，超限完整INCOMPLETE，不保整raster重复副本。每个实际handler出口均产具名receipt，外部未分发点击只可记录“disabled/not delivered”，不能伪造handler BEGIN。异步写文件用catch/finally，UI不等IO；文件失败也保内存firstCause，不将“无文件”再当唯一入口证据。

## T592步骤与前置

现有布局已足以保留禁用按钮负证，无需反复快点。正向预约须先由developer交响应式启用/唯一事务与回执接线；tester不改产品来强设enabled。新包同原用户序列达到READY，立即保同prepareId的文本、button enabled/clickable、真实onClick entry及reserve receipt，逐项走到RESERVED/consume/HOLD或首拒；独立保存IO返回和文件SHA。disabled时报告其事实，不能换坐标/调用私有函数冒用户操作。

READY→HOLD→严格系统PNG→原标定仍NOT_REACHED，JPEG不替PNG，几何/期限/GAP不改。保T574开发已交登记，此处工程缺口另派接力。完整执行 [共同约束](architecture-T581-T589-handoff-policy-v1.md)，developer不测试/不用机。
