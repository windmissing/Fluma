# T60：DEMO 拒绘归因与最小修复任务书 v1

日期：2026-10-26。角色：designer。任务：`6ade982ce4b017ab33f3c6b0`。路线：`P051_RENDER_GATE_REPAIR_V1`。状态：**归因与方案交付；T52产品仍为FAIL_RENDER_GATE；尚无修复后验证**。不使用真机，不修改业务、冻结物理约束或测试判据。

## 1. 裁决

F1、F2均先分到**渲染接线/数据表示契约**，现有证据不足以判定物理生成了坏粒子或采用了错误动力学。

| 问题 | 已证实 | 尚未证实 | 路由 |
|---|---|---|---|
| F1 NONUNIFORM_SCALE | T52两次冷启页面拒绘；controller把逻辑viewport和native像素混用，首次无真实尺寸时存在确定的自阻塞路径；页面测量有边框的Stack而非XComponent自身 | T52实际那一帧的逻辑宽高、surface宽高、DPI和具体比值；是否另有真实非均匀变换 | T62修复几何契约；T61补真实controller夹具和逐帧证据 |
| F2 INVALID_PARTICLE | adapter的单个错误码覆盖state/phase/mass/xy/ID；合法Float64右边界坐标可在Float32转换后越出上传域；现有T49用例本身已登记该反例 | T52首个被拒slot及具体谓词；不能排除真正源非法、ID错误、损坏快照等其它分支 | T61先分解谓词；T62仅修证实的表示/接线问题，源非法则另派物理返修 |

特别注意controller的执行顺序：**先pack粒子，再检查尺寸，再调用native draw/swap**。因此注入后出现INVALID_PARTICLE并不意味着NONUNIFORM_SCALE已消失；F2可以在更早阶段遮住F1。两个错误都发生在当前native提交之前，不能拿同窗口其它进程的G0成功swap来证明本DEMO已绘制。

## 2. 证据身份与范围

输入报告：`docs/effect-report-P051-T52-empty-sand-demo-v1.md`。T52构建HEAD `340e7cf`，产品最近提交 `952e148`；签名包SHA256见该报告。当前读取的以下文件SHA256均与T52归档 `test/artifacts/t52-empty-sand-demo-20261025/source-sha256.txt`逐项匹配：

| 文件 | SHA256前16位 |
|---|---|
| `P013RenderController.ets` | `B401A91B4F9F7242` |
| `P013DualFieldAdapter.ets` | `CF6BBC3EF4CD0B13` |
| `EmptySandDemo.ets` | `F8A81794EB07FA22` |
| `P011ParticleHot.ets` | `895D795EB55F29AC` |
| `P011ParticleField.ets` | `DBC67379CCF6A52E` |
| `napi_bridge.cpp` | `2E50E4342D6B0229` |

另读 `egl_context.cpp`、`dualfield_pass.cpp`、`P011PublishedRenderState.ets`、WaterSandField发布接口、T49/T51开发回执和已有夹具；本轮未运行这些夹具，不把既有开发自测当独立T61结果。

对T52四份hilog只筛选报告中的两个DEMO PID（35614、38325）：每份均有DEMO旁证样本，但没有 `p013_dual_frame`回执、逐帧surface/viewport比例或被拒粒子的字段明细。日志彼此重叠，不能相加成唯一帧数。`controls.json`是控制中心展开时的UI树，未提供本DEMO XComponent节点，不能用其中屏幕1280×2832或其它Stack尺寸代替surface。当前读取时T61尚无交付报告。

## 3. F1：尺寸来源、启动闭环与单位

### 3.1 代码确定的启动缺陷

`P013RenderController.submitDualField()`当前取：

```text
sw = nativeState.widthPx > 0 ? widthPx : round(callerWidth)
sh = nativeState.heightPx > 0 ? heightPx : round(callerHeight)
sx = sw / callerWidth
sy = sh / callerHeight
abs(sx - sy) > 1e-6 => NONUNIFORM_SCALE，立即return
```

调用者的宽高来自页面Area，而native宽高来自 `EGL_QUERY_SURFACE`，二者不是天然同一个单位/矩形。`InitSession`只绑定session并激活，不创建EGL surface；`GetState`只查询已有EGL surface。首次surface未建立时widthPx/heightPx为0，controller的fallback却把**分别取整的逻辑量**当真实像素。

由此可直接推出确定的坏路径：Area含非整数宽高→两轴独立取整比例不相等→在native draw前return→EGL无机会建立→下一帧仍widthPx/heightPx=0→重复拒绘。这是代码逻辑证明，**不是T52实测宽高复原**。即使实际已有EGL尺寸，整数像素独立量化也通常不满足绝对比值误差1e-6，且页面/子节点矩形不一致还会产生另一种差异。

### 3.2 页面矩形风险

`EmptySandDemo.ets`在带2vp边框的外层Stack上读取Area，并在该Stack接触摸；native surface属于内部 `XComponent(width=100%,height=100%)`。源码能证明两个来源是不同节点，不能单凭源码断言设备上恰好相差多少像素。若边框参与布局，外层尺寸、子节点内容区和触点原点会同时错位。DPI缩放本身应为同一标量；边框/内容区错误不能当作DPI误差放宽。

### 3.3 推荐修复：先prepare真实surface，再显式像素几何

推荐T62为本DEMO增加明确的surface像素输入路径；原逻辑viewport路径及其NONUNIFORM_SCALE保护保留，不用增加任意epsilon解决。

1. **先获取真实几何。** native提供显式 `prepareDualFieldSurface(sessionId, expectedSurfaceEpoch)`（名称可同义），在既有owner线程、锁和生命周期约束内完成basic composition配置、EGL创建/绑定/`eglQuerySurface`/解绑；返回非零整数widthPx/heightPx、sizeSource=`EGL_QUERY_SURFACE`、配置后的surfaceEpoch/contextEpoch。prepare不绘制、不swap、不制造physicsCommit。将创建/query过程与帧宽高校验拆成共用内部流程；`EnsureCurrent`继续严格检查提交帧的epoch与真实尺寸。不能改用全屏、Hints或round(vp)冒充真实surface。
2. **消除启动互锁。** 未拿到prepare成功回执前输出WAIT_SURFACE或具体失败码，不提交假尺寸帧；配置composition会递增surfaceEpoch，下一包必须使用prepare返回的新epoch。尺寸变更、前后台和context重建后重新取得几何。
3. **明确像素输入版本。** 新controller入口/显式选项 `geometrySpace=SURFACE_PX_V1`，只供DEMO opt-in。完整容器box为真实 `(0,0,Sw,Sh)`，输入确实处于像素域，转换矩阵为恒等/同一标量，native的box包含关系仍严格生效。不能先把原逻辑宽高偷偷改写为surface宽高，再宣称旧逻辑域校验通过；类型、来源和诊断必须能区分两条路径。
4. **同节点触摸。** 在XComponent自身或严格与其内容区重合的无边框输入层测量逻辑宽高Wvp/Hvp和本地触点；边框放在外部装饰层。触点映射为 `xMaterial=128*xLocal/Wvp`、`yMaterial=C*(1-yLocal/Hvp)`，等价于先归一化到同一个surface像素box。外层Stack边框原点不能继续混用。
5. **保留非均匀变换保护。** 新路径接受真正surface局部像素域，不接受未知/非均匀变换、过期epoch或source不明的尺寸；旧逻辑路径仍拒绝非均匀矩阵。若后续继续支持逻辑域到像素域转换，须用同节点、同UI密度以及明确的整数边界量化契约验证，不能仅把1e-6改大。material坐标128×C到容器box的映射与DPI单位转换是不同关系，不应拿前者的纵横比判定DPI拉伸。
6. **粒子半径维持物理像素语义。** 现radiusPx=sx隐含“1逻辑单位乘密度”。新像素路径应显式传经同一UI密度换算的候选radiusPx，不能无意退化成1物理像素；颜色、mask、深度、二值alpha及T49两个pass均不改。

这修的是几何来源/启动/API契约；不授权非均匀缩放P1光照、不改P1材质算法或冻结外观判线。

## 4. F2：源合法性与Float32表示

### 4.1 可证明的表示缺口

池的权威坐标是Float64，合法X域为 `[0,128)`。Hot路径将右边界位置限制为 `cols−1e-9`，阻断跨列时也可保留 `c+1−1e-9`。这些在Float64中合法，但：

```text
x64 = 127.999999999 < 128
fround(x64) = 128
```

128下方最近Float32为127.99999237060547，二者中点127.99999618530273；中点附近至128的合法源坐标会舍入到128。adapter预检这个上传值并返回INVALID_PARTICLE，native也严格要求上传X<128。T49把早期native拒绝提前到adapter，保住了旧包原子性，**但尚未给合法边界源提供可持续绘制的表示办法**。T52静态有效角约30°与向右运动的旁证使该路径值得优先检验，但没有该次slot快照，不能宣布它就是设备F2唯一根因。

其它OR分支也必须逐一保留：state不为1/2、phase不为0/1、mass不有限/非正、XY不有限/越域、ID非法。`PUBLICATION_MISMATCH`、重复ID、liveCount不一致是独立错误；不可把它们都归为Float32边界问题。

### 4.2 快照与物理边界

`copyPublishedRenderStateInto()`在同一个同步UI调用中复制field与pool，再写同一run/stateEpoch/physicsCommitId；adapter比较field/context/publication commit。当前源码没有“分别重新抓一份床和一份池”的逻辑。T52也没有被拒快照，故既不能认定撕裂，也不能把现有commit检查当作设备全程正确的证明。T62不得另抓实时pool与旧field拼包；必须先验证只读publication，再生成staging。

### 4.3 推荐表示修复（仅在T61证实合法源量化反例后启用）

技术路线 `COLUMN_PRESERVING_F32_RENDER_V1`：**先严格验证权威Float64源，再在只读渲染staging中作有界Float32投影**。物理池、账本、碰撞/归列、质量、ID、state均保持原值。

对合法X令 `c=floor(x64)`，上传X选择区间 `[c,c+1)` 内最邻近的可表示Float32：通常为 `fround(x64)`；若其恰好舍入到 `c+1`，则取 `prevFloat32(c+1)`。这既保持全域 `[0,128)`，也避免内部列右缘127次类似量化跨列。对Y使用同理的闭域 `[0,C]`，本DEMO C=256可精确表示；合法源在转换后越上界时才取不大于C的相邻Float32。原始源非法/非有限时继续整帧拒绝，不能把它夹回域内。

数学误差上界为相邻Float32间距量级。X<128时最大间距为 `2^-17` 材料列单位；屏幕误差记录为 `abs(qx−x64)*boxWidthPx/128`。这是可计算的表示误差，不是新增物理容差或产品效果通过线。Y亦记录实际误差，不改重力、预算或物理坐标。

接口与兼容要求：

- 增加独立纯函数投影模块及明确projectionRevision；Float32/Uint32上传布局、native坐标域守卫保持原义。
- 为DEMO显式选择新投影，默认旧raw-Float32拒绝路径继续可测。可在controller/adapter增加可选projection选项，不必污染共用物理接口；旧T49“原值合法但Float32越域即拒绝”的raw-V1回归仍运行，新投影以独立版本正例验证，不能直接删除旧断言或改既往报告。
- 两遍处理：全包源校验与投影可行性通过后才写目标staging；任一粒子失败不得部分覆盖前一个有效包，不删除粒子、不伪造ID、不降低liveCount；mass本来不上传也必须源合法。
- 新路径必须输出投影修正数量、最大XY误差、原始/量化/最终值的诊断样本。若T61发现源本身非法，应停用该分支修复并转物理源/发布完整性归因，而不是让投影掩盖。

## 5. T61精确补证需求与T62文件所有权

T61已有独立卡，不在本稿重建或代执行。需要补：

1. 真实controller夹具：native首次width/height为0、带小数逻辑Area时，证明旧路径在native draw前拒绝且后续仍无法取得尺寸；记录draw/prepare/getState调用序列。再覆盖已就绪真实尺寸、节点边框差、纯统一密度、正常整数光栅、真正非均匀变换、resize/epoch变化；夹具必须标合成，不能用1280×2832全屏当XComponent尺寸。
2. 真实adapter夹具与完整失败对象：同run/stateEpoch/commit、slot/pId/state/phase/mass/x64/y64、容量、fround值、逐谓词拒因；合法边界及真实非法源分开。证明失败前后输入publication与上个有效packet逐字节不变，孔洞/state2/liveCount/重复ID/ID越界均维持原判。
3. T52现有日志不能补出的真实尺寸与首个slot要明示缺失。若要证明设备同一F1/F2确实消失，manager另派带真机标签的修复后同用例复测；不能把host夹具当设备证据。

T62建议文件/hunk边界：

| 文件/hunk | 最小工作 | 协调边界 |
|---|---|---|
| `cpp/p013renderer/egl_context.h/.cpp` | 提取无caller尺寸依赖的surface创建/query准备流程；保持帧校验、owner、unbind、生命周期 | 与T54 native资源工作按hunk串行；不改P0/G0规则 |
| `cpp/p013renderer/napi_bridge.cpp` | 新显式prepare接口、回执和导出；composition配置后返回新epoch | 不混入G2材质API、shader或typed-array长度规则 |
| `ets/render/P013RenderTypes.ets` | prepare回执、geometry-space与可选projection版本类型 | 旧入口保持兼容，新增只给DEMO opt-in |
| `ets/render/P013RenderController.ets` | prepare→真实几何→完整pack→同次draw/swap；逐阶段拒因，保留旧包与lastPresentedCommit | 不把失败记为新呈现；原逻辑路径的NONUNIFORM保护保留 |
| `ets/pages/EmptySandDemo.ets` | 同一内容区测量/触摸、像素几何入口、真实density半径，geometryEpoch随实际几何变化 | 与T59同文件按hunk交接；不改其干场初始化/空气注入 |
| `ets/render/P013DualFieldAdapter.ets`及新增投影模块 | 严格源校验→有界渲染投影→原子写staging；细分拒因 | 只读物理发布，不修改WaterSandField/ParticleHot/源库存 |

T62实施前置：T60方案已交付；T61冻结上述controller/adapter客观反例后实施相应分支。T52设备实际触发值若仍缺失，仍可修复被真实生产代码夹具证明的通用工程缺陷，但不能预先宣称F2设备根因或修复成功。若必须修改物理状态、预算、源事务或干场接口，超出T62范围并回流designer/architect；T58/T59继续独立。

## 6. 必须维持的不变量与复测条件

- field/pool属于同一run、stateEpoch和physicsCommitId，field/context commit一致；复制只读，拒绝不改publication、不增physicsCommit、不写质量。
- XY native域仍X∈[0,128)、Y∈[0,C]；phase/state/ID唯一性、liveCount、有限正质量、nmax、reserved/layout/version与buffer范围校验保留。原始非法源仍拒绝。
- 全包验证后才能写staging；拒绝不覆盖上一个有效packet或lastPresentedCommit。错误码与“本帧未提交”必须对应，不能沿用旧swap回执冒充当前commit。
- surface/query大小、owner、session、surface/context epoch、防陈旧帧校验、二值alpha、两pass一次swap、LAND去粒与写床同发布、P0/G0既有回归保持。
- 不动守恒、四层预算、planner/guard、θ角域、shake映射、含水库存、dry-source/NO_WATER合同；不顺手修T58/T59或T47。

开发完成条件仍为实现、编译、非真机自测与回执；device end-to-end另卡。Tester对修复前RED/修复后GREEN给出同输入对照，分别报告geometry版本和projection版本；必须含零粒冷启、首粒、右壁/内部列边界、state2/LAND、Up/Cancel、reset、前后台、resize、真正非法源/非均匀变换的拒绝保持。

随后effector沿EC-T52-01~05在串行设备窗口复测：真实draw/swap+同commit包与屏幕对应、可见落沙与床累积、松手/后台停止、reset；手持倾斜/摇晃单独覆盖并保留NOT_CALIBRATED身份。新run固定构建/设备/配置和日志身份，记录拒因计数、掉帧/可见连续性；完整逐帧质量/预算缺失仍报INCOMPLETE，不能从抽样推全程PASS。干空场/空气出生仍按T58/T59交付状态独立判定。

## 7. 本轮完成与未完成

本轮完成T52双故障分流、同版本源码匹配、启动互锁与Float32表示冲突的静态证明、T62最小接口/文件任务书、T61精确缺口和复测条件。未实施业务、未执行测试/真机、未取得T61报告；产品状态保持FAIL_RENDER_GATE。接单/交付只更新清单T60自己的状态，整单与滴答同名清单由manager维护；原卡报告完成，由manager按通知登记。
