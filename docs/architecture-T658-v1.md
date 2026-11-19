# T658：单沙 owner 快照方法丢失与准备崩溃首次归因

2026-11-19，architect，不使用真机。结论 **PRODUCT_ENGINEERING_PROTOTYPE_LOSS**，工程修订后交developer，不转设计/放宽准备门。正式表`architecture-T658-contract-v1.json`、证据`analysis-T658-evidence-v1.json`与共用有限复核脚本已交；未执行JS、产品或设备测试。

## 已证首错，不扩大历史

本次raw jscrash原件：pid35376、unique id8041312369877275180、16:03:28.166，TypeError `same is not callable, same is undefined`；栈noteChange→changed→P051DerivedV2Demo键盘avoid-area回调。源模板test/t426/PrepareReadinessV4.ets与t598-final生成件相同，bindOwner将对象JSON序列化后存ownerV4，noteChange/bindOwner随后调用`.same()`。类型断言as只改静态类型，不给plain JSON对象补方法；错误与本次栈闭合。

T592-rev2原unsigned ba8980ef…/signed eeb39a4f…及modules.abc签前后相同，具体SHA/字节关系在证据。它证本次版本这条路径有缺陷；只有owner已绑定且相应change/再bind触发才走到方法调用，不能无条件写“每次点击必崩”。v1两轮缺对应同版本jscrash/attempt关联，原PREPARE_OBSERVATION_FAILED只能保历史，不能自动改成两轮都同因。HOLD/PNG预约链本次未到达，不撤销reserve disabled的安全事实。

## 唯一修复实现：纯字段、私有持有的不可变值

选择**method-free owner DTO+显式字段复制/比较+固定tuple key**。保PrepareAttemptOwner构造名供原page builder使用，但owner身份不依赖该对象原型；不推荐structuredClone保自定义类方法，不用Object/JSON重建后继续调用.same，不catch TypeError后继续READY。

完整身份十字段：prepareId/pageGeneration/branch/windowId/windowGeneration/controller/session/surface/context/run。前九沿现有精确整数域、run为真实非空string；需要原producer已知/合法，不能以0/undefined或重复某个代号填未知。非零必要项沿现有controller/session/surface/context规则，窗口有效性仍依真实registered/window引用+generation与原predicate，不能另发明windowId数值阈值。来源宽度不可表示时拒绝未知身份，不能Number舍入后比较相等。

新增纯函数`copyPrepareOwner`：校验后显式拷贝十个primitive至新对象，内部私有持有、不向外暴露可写引用；`samePrepareOwner`逐字段严格相等，不受原型或属性插入顺序影响；`prepareOwnerKey`按机器表唯一顺序序列化十primitive数组。key只用于原件/票据绑定，不替身份有效性或授权。任意原输入后续改值不得改已保存before快照。

bindOwner第一次保存一份并checked推进ownerSerial；之后新鲜o逐字段等值则保持原快照，变化则OWNER_CHANGED硬失效且不换成新owner继续用旧permit。新begin/show遵原清理；hide/退役撤销owner许可。noteChange只比较真实不同时间的before/after值；`changed(reason)`直接保父类变更/observationSerial/READY撤销语义，不能把同一ownerV4传两次当已核fresh身份。sampleFrame每次绑定真实native/window读数，不修改物理字段。

**身份相同但readiness/布局/输入等变化**仍走原PrepareSnapshot和严格predicate，nativeReady下降、窗口不active/registered、ROI/几何/焦点/键盘/订阅变化不得被ownerEqual短路。PREPARING合法软变化沿原规则重采样且不重置begin deadline；硬owner/window/page变代撤销；READY任何原需失效的变更仍失效。3000ms只约束PREPARING，不另加READY TTL。

checkForReserve在原fresh snapshot/predicate通过后出本次prepare/page/observation/input/ownerSerial/ownerKey的data ticket。consumeTicket核同一READY、未消费和全部字段及当前ownerKey，成功一次后才consume；JSON owner对象序列化不能作为方法宿主。预约仍同一UI栈fresh read→check ticket→实际reserve→仅RESERVED后consume；中间无await/stopEditing/改布局。consume失配立即使capture失效，不保HOLD；reserve失败不消费或伪RESERVED。

## 回调、首错和冷原件

订阅/帧/准备闭包捕获实际window引用+generation、prepareId/pageGeneration/branch；入口与读回后都核同代，旧回调只记discard，不撤销新attempt。window同数值ID但引用/代变同样不能复用。原父observe/record等API语义不全局重构，修复仅覆盖owner存储/比较/票据及触发本次异常的合法回调。

键盘/窗口/帧回调中的owner.copy/compare/key/票据阶段错误须先保存T625具名raw Error code/name/message和before/after owner+scope，再对**当前**attempt fail/disable；不能仅console.error、吞掉并继续或把旧attempt异常写新首错。非异常的invalid DTO也具名OWNER_IDENTITY_INVALID，不产生许可。首/末原错各一条，message UTF8各≤1024B/截断明确，沿prepare冷报告≤16KiB总预算；异步冷写有catch/finally，不在热UI回调落盘。调用方不能用“无冷文件”覆盖已有raw jscrash。

## developer 最小完整范围与后继

改真源`test/t426/PrepareReadinessV4.ets`及`test/t574/t574_prepare_v4_overlay.cjs`、实际page/frame/window preparation glue；生成t598等产品路径，不能只修local-test输出。清除所有ownerV4/before的`.same()`依赖，新增纯field helper/唯一key字典，并核bind→changed→sample→check→consume→report全部使用同一权威gate。父PrepareReadiness只作必要衔接，原predicate/回调安全和响应式reserveEnabled不削弱，勿顺手全局替换JSON clone。

developer完成完整修复及编译即可；T592 tester独立按同一正确新包采before/after同字段、各单字段变更、同ID不同window generation/引用、同scope但ready下降、输入/observation变、过期回调、票据重复消费、reserve失败/consume失配及生命周期重入。再跑真实固定键盘确认→prepare→reserve/RESERVED→HOLD→**严格PNG**与原标定；JPEG、截图早点击或强enable均不代替。架构列检查点不是本轮已执行测试，也不加开发设备门。

共同执行约束见`architecture-T655-T658-boundaries-v1.md`；本次首归因及修复合同完成，设备复测/原产品指标仍未通过声明。
