# 本轮验收与待建卡回执（2026-10-27）

【接口恢复后已解决】T178=6ae09088e9adb600000007ff、T179=6ae090f6e9adb6000000080c已通过接口查询确认，补齐角色/渲染线/P5；T180已通过接口创建=6ae0914fe4b0d25fef445f37（含真机标签）。T171/T174已补远端关闭，T177/T172/T122本轮正式关闭，同名清单已同步。下文是当时未完成状态和prompt历史，不再代表待建卡。

滴答读取连续无响应；本轮未获得原卡新评论、未执行远端关闭/建卡/同名清单同步。不得声称已同步，后续先查询卡片及标题去重。

T171本地正式报告docs/developer-T171-gpu-isolated-v3-v1.md与提交961d837已交；Native/ArkTS/PackageHap通过，SignHap既有00303116失败诚实，四fixture三路64/128实际merge离线一致，GPU为CPU stand-in，设备未测。登记本地完成并解锁T172；远端关闭待同步。
T176部分归因1a863f0：CPU材料准备catch，不是已证GL/六图上传故障；缺同帧原bits/内部原因，保留看评论，按T178→T179→T180补证回原T176。
T168部分工具续作及T116真实双场负载缺证保留；本轮未重新读取远端评论，不将旧回执冒新核验。T174/T177进行中状态保留，不凭Git出现新提交擅自关闭。

## 待建卡prompt

### T178 【T178·材质首错旁路诊断】developer：同帧原bits、内部拒绝原因与STALE回执

状态：未建卡（未向远端发送创建请求）。角色：developer；优先级P5；标签：developer、渲染线；前置：T176工程已交。

不使用真机。工作目录D:\hmprojects\HelloWorld，共享树不回退他人改动。所有渲染含前背景仅新XComponent+shader，禁止Canvas上屏/回退；失败FAULT或显式STALE。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md；有限例外仅本人行状态：接单进行中+原卡评论已开始，成功结束待验收，失败/FAIL/BLOCKED/关键缺证看评论+原卡写问题/复现/证据/前置。不得改其它行列/队列或滴答同名清单。评论不可用明确未同步留本地报告。manager不新增验收门。
按docs/architecture-T176-scene-material-failure-routing-v1.md§4/4.1实施P013_SCENE_MATERIAL_REJECTION_V1 sidecar，仅诊断，不修数值。所有权P013ScenePreparation、MaterialDataPreparation、FrameAdapter、SceneController及必要native scene_compositor/napi_bridge诊断字段；页面仅导出接线按hunk协调。保持原拒绝谓词/sourceFailure总类/code100/配方/上传及scene原子性。冷定容记录run/commit/publication/pageFrame/scene token与geometry；首错stage/innerReason/column/channel/predicate mask/数组binding及原buffer bits。失败位置固定封存10组F64场与2组cache及有效F32阶段，最早成功+首失败+最近8摘要；不再跑pack/step/audit/copyStateTo或更新派生场，hash冷导出。未知exception与缺测诚实，不被诊断错误覆盖。native拒绝/STale兼容mask、replay/Present/Swap原码分开，禁可见旧粒子冒局部成功。报告开发自测/编译及身份，设备不作开发关闭前置。

### T179 【T179·材质首错诊断独立测试】tester：拒绝分支、只读锁存与同帧身份

状态：未建卡（未向远端发送创建请求）。角色：tester；优先级P5；标签：tester、渲染线；前置：T178。

不使用真机。工作目录D:\hmprojects\HelloWorld，共享树不回退他人改动。所有渲染含前背景仅新XComponent+shader，禁止Canvas上屏/回退；失败FAULT或显式STALE。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md；有限例外仅本人行状态：接单进行中+原卡评论已开始，成功结束待验收，失败/FAIL/BLOCKED/关键缺证看评论+原卡写问题/复现/证据/前置。不得改其它行列/队列或滴答同名清单。评论不可用明确未同步留本地报告。manager不新增验收门。
实质前置T178诊断交付。按T176§5独立构造LAYOUT/VALUE各量/PROFILE源及F32层序/最终F32 overflow/未知throw，核首错原因与原bits、源数组不写、首错不覆、容量/新run清理、publication token贯通，P0/P1 wet/dry拒绝结果保持。只测试夹具注入，NAPI真实分支与host桩分列。STALE兼容mask与旧帧身份、未执行null+NOT_RUN，firstFailure不被replay失败覆盖。按影响复用T56/T91窄回归，不照开发脚本改名。测试FAIL先architect，未实现预测试RED登记NOT_IMPLEMENTED不冒产品通过。

### T180 【T180·材质首错真机窄补证】effector：控制臂与同frameToken失败材料快照

状态：未建卡（未向远端发送创建请求）。角色：effector；优先级P5；标签：effector、渲染线、真机；前置：T178+T179。

需要使用真机，真机串行。工作目录D:\hmprojects\HelloWorld，共享树不回退他人改动。所有渲染含前背景仅新XComponent+shader，禁止Canvas上屏/回退；失败FAULT或显式STALE。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md；有限例外仅本人行状态：接单进行中+原卡评论已开始，成功结束待验收，失败/FAIL/BLOCKED/关键缺证看评论+原卡写问题/复现/证据/前置。不得改其它行列/队列或滴答同名清单。评论不可用明确未同步留本地报告。manager不新增验收门。
需要使用真机，真机标签，串行排队；实质前置T178有效诊断包与T179独立无阻断结论。消费T176§5，先空场控制再同T95拖动注入至首个SCENE_MATERIAL_DATA，立即锁PID/run/package/source，保存同frameToken sidecar/首错原bits/完整native receipt；能复现即停宽矩阵，必要R2仅一次确认。不读live snapshot重写派生场，不拿离线值补设备原bits；未触发则登记缺证。保存原s150身份声明与新诊断包实际manifest，包找不到明确缺失，不重建冒历史包。沿合法签包与设备队列，不改权限或SELinux，GPU同bundle覆盖记录清楚。补证回T176续判唯一数值根因，不自行归因或放宽EPS/clamp。
