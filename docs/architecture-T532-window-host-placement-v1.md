# T532 单沙0.2px定位差首次归因与工程修订 v1

2026-11-16，architect，P4，不使用真机。消费T408 attempt8/T516实际生成页、T509 v2/T525原件及SDK26 ComponentInfo声明。**原OUTSIDE已修，严格PLACEMENT_MISMATCH拒绝正确；工程不再以浮点allocation父框为物理定位原点。** 新建独立整数window-host布置合同，不放宽容差、不round实际框、不换旧1177 ROI。原件/有限运算见 `analysis-T532-evidence-v1.json`。

## 1. 可证事实与不能提前定因的部分

实际T516签包6f49d06d…及包内renderer ELF3a0f207e…核对来源。prepare-a8中window=(0,0,1280,2832)，parent.windowOffset=parent.screenOffset=(51.20001220703125,990)，w1177.5999755859375/h1716，vp2px3.5。desired window/screen=(52,990,1176,1716)，local=(0.79998779296875,0)。actual/inner/touch及三screen框均=(51.79998779296875,990,1176,1716)。因此x差为−0.20001220703125px，三个节点一致；w/h正确且整体在allocation内，原外框错误确已消失。

父框51.2+local0.8在数学上恰52，但 **actual.x−local.x=51**。这与父布局原点在子position合成前被像素对齐为51的假设相符，不能据此断言ArkUI所有版本都使用floor；原件没有localOffset、完整transform4×4、逐祖先原点、布局/绘制同拍原件，无法唯一确定发生在哪层。viewport/window screen原点本例均0，不能将这次差值默认归screen/window混淆；新旧宽高3.5缩放也未显示相同比例误差。页面rasterRect直接使用ComponentInfo的windowOffset/screenOffset/size，没有再乘3.5，故不是该函数重复vp2px。

SDK26 ComponentInfo定义localOffset相对父、windowOffset相对window、screenOffset相对screen；现页只保后两者，并用scale/translate/rotate检查身份而未保存完整matrix。声明不保证浮点父windowOffset与物理子.position的叠加必逐位相等，不能把仿真mock的线性加法当平台已证明。native controller/context/packet/viewport仍0是prepare失败之前的NOT_REACHED，不据此另判EGL坏。

## 2. 新定位策略：逻辑allocation与真实window-host分开

修订版本 `T532_WINDOW_HOST_RASTER_V3` / `T532_PREPARE_V3`。保原percentage allocation节点作**逻辑可用区域与visibility/键盘布局来源**；不再把render/inner/touch置为这个浮点父框的后代再加0.799987px来凑整数。页面根提供稳定独立position host（`t532-window-host`），与allocation布局作为同一window下兄弟层，host无padding/border/translate/scale/rotate、Ltr/TopStart。以实际host的window/screen原点为唯一定位基准，要求x/y为精确整数px、host可覆盖desired及当前可见区；否则RASTER_HOST_NONINTEGRAL/DOMAIN_INVALID明确拒，不能取整其测量。

`desiredWindowRect`仍严格由原始allocation四边ceil/floor得到52,990,1176,1716；`localOnHost=desiredWindow.xy−host.windowOffset.xy`，必须整数，宽高仍1176/1716。inner/render/touch三节点均放到该稳定host，位移整数px；任何transform/padding/祖先差异显式不支持，不用旧actual反馈调offset、不写隐藏+0.2修正。逻辑allocation虽不再作为物理父，仍要求desired/actual的完整矩形落在它内部，真实clip目标为desired矩形，不能绘制/接触控制按钮区域。

host是页面根布局的一个固定层，不随prepare config分支或诊断文本重建；窗口可见/keyboard避免区/导航/rotation变化先撤原geometry/permit和旧HOLD，再重新观测host及allocation。若旧node/Surface尚未退休，走既有生命周期；不能边保旧token边移动一条新touch层。研究页输入节点命中只在actual目标框，同尺寸/同源position；host透明命中策略须让框外事件穿给页面，不创建全屏吞手势透明遮罩。

这消除了依赖“浮点父框物理原点=布局测得浮点值”的工程假设；**不保证设备必逐位落52**。真机若整数host方案仍返回小数actual，继续保原值拒绝，带§3新增有限原件回architect/平台域，不再加容差或试探性循环偏移。

## 3. 必须保存的原始观测与代号

扩展 `RasterAllocation/Plan`：原allocation与host各保ComponentInfo完整size/localOffset/windowOffset/screenOffset/4×4 transform及scale/translate/rotate、window id/generation/rect、vp2px、orientation、layout observation serial、didLayout/willDraw/frame anchor。新增hostOriginWindowPx/hostOriginScreenPx、requestedLocalOnHost、所有node id与父链id；原值保存，不以expected覆盖observed。有限祖先链≤16项、每项≤256B，超限INCOMPLETE_ANCESTRY，不盲称transformIdentity。

refresh按单个layout generation采所有字段，异步Area/Draw回调只接受当前prepare/page/window/geometry代；配置写入后至少等新didLayout观测及原两帧锚，不能立即查询旧节点然后宣布请求位置已生效。layout原值哪怕四边ceil结果没变也须推进代，原native tuple不能作为allocation输入制造反馈。native packet只读已通过实际三框的actual.x/y和整数w/h；EGL/viewport维持1176/1716，后续CaptureGeometry/触摸和冷文件使用同一新revision/id。

纯函数边界：

```text
planInward(rawAllocation) -> desiredWindowRect (T509原数学)
planOnIntegerHost(desired, rawHost, rawWindow) -> integer localOnHost or explicit reason
observeNodes(generation, rawComponentInfo[render,inner,touch], nativeTuple) -> actual snapshot
placementPredicate -> strict desired==actual==inner==touch and screen relation; then native tuple gate
```

不得在placementPredicate把51.79998779 round成52，或用actual反改desired以求通过。若window非零screen原点，先window域整数布置，再精确screen=windowOrigin+windowRect核对；screen lattice不是另一套可以任意取整的计划。两轴均同规则，vp/px仅在ArkUI属性输入/类型转换处做一次，直接ComponentInfo原px不再转换。

## 4. 实施、必要非机检查与精确真机接力

developer沿T516→T469实际生成链，不直接改历史旧包；同步RasterGeometry/PrepareReadiness/页面/SingleSandController/CaptureGeometry和签包声明，原T413预约/HOLD/键盘收起/OPEN首因/零副作用保留。新增window-host稳定层及定容原始观测，完整V3拒旧档混用。编译/实际生成页非机检查/签包即可交付，不追加真机作为开发关卡。

非机：原a8在V2严格拒，V3同逻辑allocation仍给52/1176，host0与整数偏移原件/非零window origin、2.625/3.5、负/非finite/overflow/空域、非整数host、祖先transform/RTL/padding、旧代Area/Draw、新旧width相同但allocation变更、原生tuple不符、HOLD中布局变化撤权。特别加入“先把父原点量化为51再加0.799987”的平台mock，证明旧linearity mock为何漏掉本例；mock不是平台PASS。

tester新签包串行真机只先收一组§3全raw：同拍allocation/host/三node的local/window/screen/matrix、requestedLocal、代号/窗口与真实显示尺寸；严格actual52/990/1176/1716、三框/屏域/native tuple全齐才进入READY→预约→HOLD/PNG→独立重新标定。原a8不重标PASS，旧1177 ROI/旧calibrationId不可套新1176。若仍有差异，以这些有限原件一次回architect，不能再仅报0.2px或替换成松容差checker。

本卡首次归因、替换定位假设及精确工程范围完成，平台内部像素对齐的唯一因果点仍未证，不捏造设备修复成功。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下，局部受阻先做其它项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列/滴答同名清单。CPU停投，前景背景仅XComponent+shader，禁止Canvas/软件GL产品回退，失败FAULT或标STALE上一完整有效帧。
