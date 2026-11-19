# T670 离线设施与交接报告

2026-11-19；tester；不使用真机。构建/离线设施交付，不表示设备、物理或性能 PASS。开发编译基线 8fe7c2e65565350b2013dd466bb64dce189b2351；本卡新增工作区按随附源/对象 SHA 收据绑定，提交时一并入库。

独立SPF frozen T564与DM当前T648 GPU差分执行设施已交付。test/t670/prepare.py spf|dm <fresh> 各用独立源码/cache/对象：SPF生产族从7a7ee216完整恢复（原stage除换行外有candidate_v2夹具不一致，未混用）；DM按 tools/g9dm/prepare_build.cjs 从当前源完整生成。两个实际ARM64 g9_differential / g9_dm_differential 链接真实Transport/Pump/typed gate/CaptureSpan，无mock、无CPU物理stepper。DM使用真实AcceptedProof/CommitAccepted与rootBeginNs/clockGeneration，保250ms绝对gate，SOURCE12/QUANTUM2404不重标旧R1。

最终正常构建两者均PASS：SPF 10.461s、DM 10.024s增量（此前完整77.304s与完整DM对象重建记录保留）。签名verifyApp PASS，当前源/生成头/每对象/argv/rsp/cwd/工具/ABC/ELF BuildId闭包及独立可执行ELF见 test/reports/t670-spf-final/handoff.json、t670-dm-final/handoff.json 与本卡 native.json。未换SDK、未全局去重flags；真实OS argv另外存 tester-native-argv*.json，与原Ninja/rsp同时保留。

fixtures.py 按现有T552/T554机器种子表构造输入，校模板SHA，生成ROOT/FULL、0/15/30度、1..4根命令；DM20强启动、DM22 foreign-policy拒绝、DM29两根接续、DM31 FULL仅旧parent信用均可明确生成。配额仅caller声明原件，不冒Controller实际clock授权。复用T556既有七项oracle及T569静态十项，不重投CPU仿真；新增check.py对实际冷流联核来源/段SHA、S/F/P总账/column T，以及DM_NATURAL行与既有独立oracle的q/上界/二分/H/omega/u/局部守恒精确差分。已有fixture正控和篡改q拒绝通过；没有生成虚假GPU output。

交接T541/T569 effector：按 native.json 部署各独立ELF及其DT_NEEDED，SPF用config16、DM用config64+settings.u32，不交叉调用。python test/t670/fixtures.py dm 192 7680 <new-input> [--roots 2|--full|--foreign-policy|--parent-credit]；SPF同样参数但kind=spf。设备串行运行 g9_*_differential INPUT NEW_OUTPUT，原EGL不可用无回退。保存execution.jsonl/input/request/completion/cold；hash使用test/t549/receipts.py，冷流 python test/t670/check.py <root-N-cold.u32>；DM再复用 tools/g9dm/read_cold.py。普通单次设备进程240s总界、单根原250ms不改；逾期/拒绝/无有效DM事件如实NOT_MEASURED/FAIL交architect。

所有设备域DM20/22/29/31和三态演化均NOT_RUN。COLD_BUDGET负控保FULL−8MiB调用者span，CaptureSpan必须拒而不能称COMPLETE。ROOT/FULL不同预算；DM K512 full档188059648B，两个冷槽376119296B，包含GPU/host/driver/包装各自另账，禁止32bit/JS Number宽ID误用。无可视/性能/物理PASS，不以真机作为本卡关闭门。
