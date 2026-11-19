# T623：真实触摸到 SOURCE 的证据断点

2026-11-19；architect；不使用真机。结论：目前没有同代Down/typed enqueue/消费证据，不能判定只能真人操作，也不能把SOURCE_SELECT或acceptedQ当SOURCE成功。先修tester操作与取证；真实事件入口观测不足按以下工程seam分段，不注入合成SOURCE冒真实交互。

## 实际路径与原件

消费T585/T546/T597、test-report-T593-typed-source-v1.md、test/reports/t491/t593a原件及 `.codex/local-test/t597-fixed` 实际页/controller。SURFACE XComponent `grs-m1-experience` 的onTouch只遍历changedTouches：Down→model.down、Move→move、Up→up；Cancel→stop。down先判active/reportAllowed/READY/pointer/坐标，已有拒绝reason和SOURCE attempt；接受后setSource，首hold pulse仅建时间锚，下一个有效pulse才生成offer，原生接口从ArrayBuffer解包。本卡不能直接套T621 typedarray length缺陷。

执行snapshot的当前permissionRevision=5，但inputStages属于revision4并有retiredScopes=2；该域仅SOURCE_SELECT和NATIVE_ACTION两条，dropped=0，没有Down。witness rows为空，184B文件合法形状不证明非空链路健康；source seq/消费证据均未出现。SOURCE_SELECT仅是选源，无法说明Down命中；reset到选源相隔多秒，不能拿那时READY推触摸时READY。原报告也未留足uitest具体坐标、坐标单位、Down/hold/Up脚本回执，尚不能归因平台不支持自动化。

## tester 最小复现与所需证据

固定真实HAP/ELF/boot/page/session/context/permissionRevision，重新进入实际体验页，选源后先导出同代状态：active、暂停、reportAllowed、sourceOpen、selectedSource、完整XComponent局部/屏幕bounds、vp/px比例、遮挡层和inputStages基线。布局树未暴露SURFACE节点时从既有组件rectangle记录取bounds，不能使用旁边“水”按钮坐标替代surface。

在bounds内部1/2处使用真实平台触摸Down，原样记录screen px及转换后的局部vp、pointerId和事件时间；保持150–250ms并至少跨三个真实page pulse再Up，避免只有点击或只有首pulse建锚。记录完整脚本命令/返回/截图布局SHA；不伪造pulse或延长已冻结GAP，若暂停/过期先修正常操作。立刻导出本代inputStages/actions/typed入队返回以及相同seq的native claim、SOURCE root和witness；退出/重置前封存。需至少一条实际非空SOURCE消费，不以selected/acceptedQ代替。

## 工程观察合同（现有记录不能区分时交developer）

在onTouch入口、遍历changedTouches**之前**增加PAGE_TOUCH_HANDLER_ENTER：type、touches/changedTouches数量、pointer/坐标和同代scope；在model.down保现有具名拒绝、在accepted→pulse→offer边界关联同一pointer/intent/seq。每代固定64条、每条≤256B，页侧总≤16KiB，首次/末次和dropped计数显式；scope换代清理并记录retired，不把旧revision拼新手势。异步冷导出，不在触摸热路径IO/GL查询、额外物理推进或二次offer。若ArkUI handler完全没进，再在已有native SURFACE DispatchTouchEvent边界加只读同意图观察（同一预算分配，不双倍新增），不将observer变成另一SOURCE producer。

按证据分流：无handler且native有真实事件→组件接线/遮挡/平台传递工程；二者都无→tester操作/坐标/注入设施；handler有但changedTouches0→具名记录平台事件形态，由developer在真实平台合同下修转发，不能任造Down；down拒绝→按原reason修操作或具体产品状态；down接受但无有效pulse→调度/页面active链；offer接受无claim→native队列/同代scope工程。每条必须绑定相同intent，缺证先交准确补证而非猜因。

真人/effector可另证真实手势，但不是“自动化必不可能”的结论。若另需test-only typed接口，仅独立测试包/route标记SYNTHETIC_TEST、不同身份，证明接口/队列层；禁止冒REAL_TOUCH、真实SOURCE资格或绕过clock/同seq witness门。T593及依赖真实消费的下游只按非空原件推进；其他研究不新增此门。

续作prompt：developer不使用真机，完成工程范围和编译；tester/effector负责真实操作、签名部署、测试采证，真机串行。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外）或滴答同名清单。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。阻断回执列未完成项、原始错误/复现/证据/尝试、不能继续原因、前置与接手角色。GPU驻留优先、CPU停投、XComponent+shader唯一上屏，失败FAULT/显式STALE，禁Canvas/软件GL产品回退；目标10暂停，水线13→14→12，旧湿M1暂停。
