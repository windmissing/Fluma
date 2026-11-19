# T656：共享 broker ContextRetired 票据失败首次归因

2026-11-19，architect，不使用真机。结论 **TEST_FIXTURE_INPUT_AND_ORDER_ERROR**，不改生产broker语义、不转designer。正式表`architecture-T656-contract-v1.json`、证据`analysis-T656-evidence-v1.json`及共用只读复核脚本已交；仅静态ELF元数据/源码/保存日志核对，未运行harness或产品。

## 真正的首拒绝

生产GPU Reserve的入参门是ticket指针有效、**`*ticket==0`**、context/bytes合法；已存在全局GPU ticket也拒。Freed和ContextRetired收**按值**ticket/context，清内部匹配租约，不会把调用方变量写零。ContextRetired实际实现对匹配当前context清gpuTicket/gpuContext/gpuBytes，并非只标“context亡”。opaque context操作仍不证明真实eglDestroyContext或driverfree。

实际driver：A首次Reserve后gA=1；B正确Freed(gA)后以gB成功Reserve；**立刻Freed(gB)**；之后ContextRetired(ctxA)已没有该活动租约；最后却用仍为1的`&gA`再次Reserve。因此最后false首先由非零输出变量入参门决定，不能检验ContextRetired是否释放活动票据，也不能与前一个B.Freed后成功作所谓只差一个条件的对照。gB实际值未逐项日志化，按实现可推下一serial但不能补成已观测输出。

现成broker715f314d…/200824B与T608 final release逐字节同，静态dynsym有全部8符号，两adapter DT_NEEDED同broker；无需新产品编译或endpoint。两adapter源码相同但确为独立DSO。driver装载B时实际优先选择其A_命名exports，所以发生跨DSO操作，但报告的“B前缀wrapper”字面不准确；后续须记录真实dlsym名/地址/所属模块。

日志实际**38行、两个seq0..18的19行片段**；去重后21不同字符串不是单次21行序列。每片段15条M1/M2/M3/M5 bool观察，不能无调用账将其扩大成17步16项总体PASS。保留这些真实Reserve/reader/BuilderDone/Freed交叉成功与拒绝事实；纠正最后伪失败不抹已有窄域证据。M4 route/process累计信用测试在driver和日志均未执行，不能概括为已测；driver物理free、生产route集成亦未测。

## tester 原 T627/harness 完整修正矩阵

每个Reserve探针使用**独立初始0的输出变量**，记录其before/after、context、bytes、routeBudget/route、request/trace全部真实输出和bool；失败不假设API清零。Freed后若复用变量必须由调用方显式置0并记录；主正控优先new零变量避免别名混淆。

GPU顺序：A reserve(ctxA,gA=0)成功→分别错ticket Freed、错context Retired后各以fresh0探针仍拒→B正确Freed(A票据)→B以fresh0 gB reserve成功→**保留gB活动**→B.ContextRetired(ctxA)→A以fresh0 gC reserve成功。之后用新活动票据分别验证错context不释放/正确释放；异常成功或失败先保原件再只清本进程已拥有票据，不靠ResetAll或猜生产票据。此矩阵直接复用T636正确语义，不改变用例期望，不需要manager重裁或designer重新设计。

先修loader对全部8个函数指针的校验（现return条件漏GpuFreed/GpuCtxRetired），严格按真实签名调用；void返回不能套bool函数指针，A/B别名与独立DSO身份明确分列。新的唯一attempt create-new日志，每行含boot/pid-start/attempt/seq及调用、票据/context/input/output、真实结果；不能再次append两轮后去重当一个run。原2个片段保留历史，不重写为新正确序列。

按T636补M4完整隔离：同route4096/8192后4097拒、4096准；独立清空后不同route charge=524375552−4096、另一route4097拒/4096准，所有routeBudget合法且只有一个slot占用，去掉单builder/reader等更早拒因。只模拟组件信用，不真分配数百MiB，不宣driver内存测量。

同样，builder期间ReadBegin且另slot已有reader同时为两道拒门，原单个false只能保行为，不能称已隔离builder拒因。需要时独立设置“无其它reader、只有该builder→ReadBegin拒→BuilderDone→同票ReadBegin准”；全局reader门则“目标已BuilderDone、另reader持有→拒→释放另reader→同目标准”。所有更早前置分别对照，不将source分析冒新设备实测。

预算仍≤30s、≤128条、每行≤512B、log≤64KiB/manifest≤16KiB。设备部署/运行与双DSO/生产broker加载映射由tester，真机串行；本architect不重跑。新原件按完整键分域报告M1–M5，不拿组件成功宣产品集成或driverfree。当前无需developer接口；若新原错显示生产ABI/实现与正式退役合同矛盾，再回architect修工程分流。

共同执行约束见`architecture-T655-T658-boundaries-v1.md`；本卡归因完成，原报告宽结论收窄，不新增验收门。


## 2026-11-19 T667 续归因

[T667](architecture-T667-v1.md)确认新run的D2仍是设施票据/reader/清理错误，非Freed破坏持读合同；M5匹配ctx退休正控窄证保留，M4未执行。原T627按architecture-T667-contract-v1.json完整独立矩阵续办；所有成功探针票据必须入ledger清理，M2缺End2不能假End旧1，M3首ReadBegin必须检查bool。本文早先Reserve初始0的简写只对GPU绝对成立；host既有合法票重建保留，新host探针fresh0用于隔离。无需新developer接口/设计或manager门。
