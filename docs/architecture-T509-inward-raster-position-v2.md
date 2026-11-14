# T509 · 父框、整数渲染框与位置同源化 v2

2026-11-14，architect，P4，不使用真机。接续T408 attempt7、T469-release实际生成页和T463，修订宽高及位置分配合同。[原件及有限算术索引](analysis-T509-evidence-v1.json)保来源/FAILED/布局/状态hash和边界计算。只工程稿，不修改业务、旧档或测试实现。

## 1. 确认的工程问题

重算T469-release签包SHA256 `cab5081663aab4626b8b3dad9cbd203e724f1d2224c05db1ed744ac9180eacdf` 与T257收据一致；unsigned输入 `.codex/local-test/t469-handoff`。该实际生成`RasterGeometry.ets`与测试模板一致：allocate只floor父框width/height，内Stack TopStart显式整数宽高，但**没有显式位置计划**。quoted页面源码和实际签包分别留hash，不把工作区后续代码当已安装。

`test/reports/t491/t408/prepare-FAILED.json`：parent window-px为x51.20001220703125/y990/w1177.5999755859375/h1716，实际XComponent、inner、touch全为x51/y990/w1177/h1716，window(0,0,1280,2832)、vp2px3.5。故实际左边51严格小于父框左边，原rasterNodeReason准确报RASTER_OUTSIDE_ALLOCATION；不是加epsilon即可宣布包围成立。

FAILED档prepareId1/geometry5/layoutAck1，native controller/session/context/packet/viewport均0，surface1。几何门先阻止draw/prepare，所以不能把native零值另猜成原生建面故障。status/status2等HOLD仍IDLE、holdId空；预约disabled、OPEN/PNG/标定/跨域时钟配对未达，不属于它们已测FAIL。旧键盘确认、两帧稳定和许可约束保留。

T463只明确宽高向下取整，未冻结连续父框位置与设备paint位置之间的分配关系。T469非机fixture的父/子位置都是整数（如x50），覆盖不了本次分数原点；责任归工程位置合同缺口，先本稿修订再developer，不直接交tester放宽门。

## 2. 新revision与四边向内分配

新增 `T509_RASTER_GEOMETRY_V2`、`T509_PREPARE_V2`，不把旧V1 FAILED事后判通过。连续parentAllocation/外框原值继续保留，不取整覆盖它。计划使用**window物理px**为唯一计算域，对父content框P=(x,y,w,h)定义半开区间：

```text
L=ceil(x)       T=ceil(y)
R=floor(x+w)    B=floor(y+h)
rasterX=L      rasterY=T
rasterW=R-L    rasterH=B-T
localX=L-x     localY=T-y
```

这是项目明确的新分配策略，不猜ArkUI内部round规则；四边都在父框内。有限值、加法无溢出、safe integer边界及产品已有尺寸上限先核；w/h>0、W/H≥1，否则RASTER_ALLOCATION_EMPTY或INVALID，不强行最小1px。P须完整在window client边界内，继续严格拒超窗/遮挡/transform；不能先与window随意裁交集掩盖原布局越界。

本次P右边=1228.7999877929688，故新计划 **(52,990,1176,1716)**，localX=0.79998779296875px、localY0。不能只把x改52仍保持1177宽，那会使右边1229超父框。也不能把父框x改51伪造原分配事实。与T463的floor(width)相比，新宽最多再少1px，这是版本化分配变化，需要新正常标定/ROI；不修改物理/材质半径或颜色阈值补偿。

位置即使变动但ceil/floor结果相同，allocation原值的任何改变仍推进planGeneration/geometry、失效旧permit；不能只比较四整数。屏幕位置=window原点+计划window位置；window原点变化/窗口代/屏幕scale/方向/键盘布局变化均属于身份。screen位置允许window原点的原域小数（若平台确实返回），不能再round screen坐标；raster的整数性只定义在window-px。

## 3. 实际ArkUI接线及位置验证

保持92%＋layoutWeight外allocation Stack，**其自身content无padding/border/safeAreaPadding**；祖先偏移已由getRectangleById的windowOffset体现，不能再累计一次。若必须有内padding，先独立零padding allocation host并测其content框，不把border框拿来计算local。以下为定位结构，不是已实施代码：

```text
outer allocation (continuous; measured once, not child feedback)
  inner raster .position({x: localX+'px', y: localY+'px'})
               .width(W+'px').height(H+'px')
    XComponent .width(W+'px').height(H+'px')
    touch      .width(W+'px').height(H+'px')
```

仅inner设置父content内absolute position；XComponent/touch在inner TopStart、无再offset/百分比/weight。使用px Length，不能将local当vp再乘scale，不能用translate、offset、scale、markAnchor伪装位置。layout-direction固定物理左上坐标，不让RTL改成另一基准；不支持的镜像transform继续拒。

官方[ArkUI position说明](https://github.com/openharmony/docs/blob/master/en/application-dev/reference/apis-arkui/arkui-ts/ts-universal-attributes-location.md#position)定义其相对父content定位、区别于offset绘制偏移；本机SDK26支持该接口。这只支持选用该API，**不证明真机最终paint恰等于计划**。真实新layout后分别读取inner/XComponent/touch window/screen框，要求三者与计划 **x/y/width/height逐值严格相等**，screen关系也严格一致；EGL/packet整数只比较W/H，不拿不存在的native x/y造证。未达到则明确RASTER_PLACEMENT_MISMATCH并留值，不能±1px通过。

新计划字段必须包含revision/planGeneration/geometry、原allocation/key、windowId/generation/原点、vp2px、desiredWindowRect/desiredScreenRect/localPx、三实际框、native tuple、layoutGeneration/ack。allocate只用外框，observe只核结果；父/子事件先后乱序按代拒旧观察，不能observed子框反写desired位置造成振荡。

设置位置尺寸后至少等新didLayout匹配plan代，再沿原两笔不同onFrame→willDraw观察；不能调用setter返回即ack。每次观察读取同一window/scale及完整tuple，读中代变化丢该样本。refresh任何actual/screen不符先使准备无效，不在缺native时“先假Ready”。

## 4. 触摸、packet、ROI与许可保持同源

触摸局部坐标属于实际touch框，模型/packet globalX/Y、widthVp/heightVp必须取**已验证actual**/同UIContext scale，同时带desired计划身份；不能只换width而保旧x。屏幕ROI映射先screen→window→实际内框一次，外框至内框空条不算可采样区；旧包PNG/旧plan ROI不可套新1176宽证据。整数坐标平移并不改变渲染尺度或半径，归一化/像素转换仅按现有同源框进行。

关键负控：父框移动δ但ceil四边未变，nativeW/H也未变，仍必须使旧prepare permit失效；只保floor尺寸相同不得继续OPEN。READY→reserve前同步重读全tuple不匹配则拒且requestId/phase/hold无副作用。RESERVED/OPEN时位置、window原点、keyboard、scale或layout代改变按原hold INVALID保首因，不自动release/seal。恢复后重新配置/两帧/预约，不能用旧OPEN/旧PNG续配。

## 5. 可执行developer与测试接续

developer，P4，不使用真机。修改实际T469生成链的RasterGeometry helper/页面State/inner position、PrepareReadiness及capture plan字段、触摸/packet/冷档接线；保原native renderer/shader和endpoint、T413 PNG/颜色/时序门。原fixture覆盖整数父框太窄，新增本机FAILED精确样本和n±ε位置/两轴组合，测试应实际加载生成helper/页面，不重写另一个predicate冒通过。编译、非机必要检查及新签包/完整来源即可交付，T408真机另行接续，不作为开发关闭门。

必要非机检查：

- x/y在n−ε/n/n+ε、半像素、非整数scale2.625/3.5下，四边包含、尺寸正或明确EMPTY；ε选可表示相邻float，禁止先epsilon修正输入。
- 本机精确样本得到(52,990,1176,1716)；旧actual51/1177仍拒，52/1177也拒右溢；仅三实际框及packet/EGL一致、身份新鲜两帧后才可READY。
- 浮点parent小平移而四整数相同仍换代；window screen原点非0、窗口resize/转向/键盘升降、UIContext scale改变，desiredWindow/screen关系准确，位置/尺寸前代回调拒。
- 内层padding/translate/rotate/RTL不满足计划时拒；position是layout位置，不把localOffset当transform.translate误拒；错误screen原点、touch错1px、只native尺寸正确都拒。
- Ready→reserve零副作用拒、OPEN→INVALID、旧frame/layoutAnchor/permit不被新代消费；键盘未知和避让未满足仍拒。父框变化不从子actual反推新计划，不产生无限layout反馈。

tester后续带“真机”标签，串行核新签包HAP/ELF及源码hash；按T408原编辑/固定键盘/prepare流程，留plan原值、local、desired/actual三框/native packet与EGL、布局代和两帧锚。若平台未能表达global整数位置，保存ArkUI localOffset/windowOffset/screenOffset/transform和位置setter参数、前后layout，而非补容差；回architect决定是否需要独立零原点host或更换布局容器。预计可用position完成，但本轮不把该预计当设备结果。可READY后继续原HOLD/时钟/PNG/标定/颜色配对，未达项真实保留。

执行prompt共用：要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部受阻先完成其它项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列/滴答同名清单。CPU停投。前景背景仅XComponent+shader，禁止Canvas/软件GL产品回退，失败FAULT或标STALE上一完整有效帧。成功原卡回执且本人行结束待验收；失败/关键缺证写未完项、复现原错、证据、尝试、不能继续原因、前置/接手角色，本人行看评论。

本卡实际原件/来源、位置根因、同源四边分配和完整工程接力已完成；未改产品或测试，未跑真机，不宣prepare/HOLD/PNG通过。
