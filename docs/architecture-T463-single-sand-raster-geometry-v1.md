# T463 · 单沙连续布局与整数渲染几何同源化

2026-11-10，architect，P4，不使用真机。修订 T426准备合同与T449观察接线，保留两帧稳定、键盘/窗口、许可与hold严格门。

## 1. 实际来源与结论

T455-release最终签包SHA256为 `7b13fb3e4fecca3867e16d484539f7ea6f659e02549d5c42c5755c3ca2725d33`，与收据及本地文件重算一致。冻结生成页 `.codex/local-test/t455-release-final/entry/src/main/ets/pages/P051DerivedV2Demo.ets` 在外层 **Stack** 上设id=t426-surface、92%宽和layoutWeight。内层真正的XComponent是id=p013_lmat_surface、100%宽高。当前readPrepareSnapshot测的是外层Stack，不是XComponent的离散buffer。

`onAreaChange→area` 保存vp几何并在尺寸/位置/vp2px变化时推进geometry；`readPrepareSnapshot` 用vp2px把其转为window/screen连续px，再用getRectangleById取得Stack浮点px。native值来自 `P013SingleSandController.prepareReadinessCold→native.getState.widthPx/heightPx→EglContext::QuerySize→eglQuerySurface(EGL_WIDTH/HEIGHT)`。QuerySize直接保存整数EGLint；项目没有把1177.5999755859375手动round成1178。`OH_NativeXComponent_GetXComponentSize`在本renderer只是hint，不是viewport权威。

两份冷档 `test/reports/t408/attempt5/t408_cold{2,_failed}.json` 的连续框相同，vp2px=3.5，宽vp=336.45713588169644，乘积1177.5999755859375；component宽相同，native viewport为1178×1716。native packet已绑定同session/surface/context且packet.width==native.width，nativeGeometry==geometry==3。完整编辑路径键盘与窗口条件满足，最后被float!==integer拒；未编辑路径仍因KEYBOARD_VISIBILITY_UNKNOWN先拒，不能说两份都已READY。

这属于**工程几何域混用**。两帧观察泵已经得到30次SAMPLE且零drop；本次不再改帧源。上游ArkUI也有独立paint像素对齐过程，说明layout框不能直接当buffer整数。[OpenHarmony源码](https://github.com/openharmony/arkui_ace_engine/blob/a46eecb2cfd9133f55743df8f695cfd7d205763c/frameworks/core/components_ng/pattern/xcomponent/xcomponent_pattern.cpp#L2089) 的UpdateSurfaceRect/AdjustPaintRect会分别处理位置、边界与宽高，不能概括为任意平台 `Math.round(width)`。该源码不是此真机固件构建证明；当前原件未暴露paintRect，故不能唯一还原厂商内部取整函数。

## 2. 工程裁定：同一整数计划驱动实际渲染区

采用 `T463_RASTER_GEOMETRY_V1` 独立准备诊断与几何绑定。保留外层92%自适应连续分配框，但在其内部建立**实际整数px渲染区**，由单个RasterGeometryPlan驱动XComponent、触摸覆盖层、packet预期、prepare permit及capture geometry。原连续分配框不被抹零/覆盖，继续用于裁剪、可见性、漂移与窗口关系。

1. 在外层完成layout后读取正有限的可用window-px尺寸，计划 `rasterW=floor(availableW), rasterH=floor(availableH)`，要求均>0并满足现有尺寸上限。向下取整是本项目新明确的**分配策略**，确保子渲染区不超外框，不是假称OS使用floor。例1177.5999755859375分配**1177**，不把原1178事后判通过。
2. 内部Stack和XComponent/触摸层均显式使用相同整数px Length，避免各自再按100%从浮点父框推导。去除内层layoutWeight/百分比尺寸反馈；计划只从外层分配框产生，禁止从自己的已量化子框再次生成计划而震荡。实际渲染节点有独立element id供getRectangleById核对，不能继续拿外Stack冒实际节点。若现有层级调整为外框+内渲染区，所有area/visibility/touch/capture接线一并更新。
3. 待真实新layout及native geometry就绪，要求实际内层连续宽高**严格等于计划整数**，EGL宽高与packet宽高**严格等于同一计划**。框/尺寸不符、surface尚旧、尺寸为0都按原门拒；不加±0.5或±1px容差、不cast比较双方、不改native返回值。若平台不能实现该显式整数布局，导出实际差值和回执并回architect，不临时放宽。
4. 位置仍保存连续windowPx与screenPx，不把位置统一round来掩盖小漂移。计划含parentAllocation、actualRenderBox、vp2px、windowId/generation、orientation、layoutGeneration、geometry、session/surface/context/packet run及rasterW/H；任一变化即递增几何/计划代、清稳定与旧permit。连续位置/尺度变化即使落在相同整数尺寸，也不能复用旧permit。transformIdentity继续严格要求单位scale/零rotation/translation，不用比例误差豁免转向/缩放。
5. captureGeometry和触摸输入的作用域改为真实内渲染区。global vp位置及widthVp/heightVp由实际内框配同一UIContext换算，vp2px只做域转换，禁止对整数viewport再乘一次。ROI的window/screen像素映射必须绑定该计划和实际内框；外框空余的不足1px条带不作为有效画面/采样区。原有PNG/时间/颜色门与冻结ROI规则保留，新包重新正常标定，不用旧包PNG/旧geometry冒新配对。

T449 didLayout锚必须带planRevision和内渲染节点身份；计划提交之后等待新layoutGeneration，再开始两笔不同onFrame→willDraw样本。设置尺寸调用返回不代表已布局。READY后reserve同步重读全tuple，出现差异拒绝且零request/phase副作用；RESERVED/OPEN期间变化使hold INVALID保首因，不能自动seal或释放。变化后必须重新准备，不因当前整数尺寸相同跳过。

## 3. developer和边界测试接力

developer 不使用真机，修改实际T455生成页、必要的准备helper/类型与内渲染区接线，版本化旧T426 float==viewport规则；不改材质shader、仿真、endpoint协议或T413颜色/时间容差。保留外框布局及原诊断值，新增plan、实际内框、native整数、packet整数和原拒因冷档。完成编译、签包和实际生成逻辑非机检查，不把T408真机作为开发关闭门。

| 非机边界 | 必须结果 |
|---|---|
| available=1177.5999755859375，scale3.5 | plan=1177；旧viewport1178仍拒；实际内框/packet/EGL均1177且身份新鲜，两帧后可READY |
| n−ε/n/n+ε，n=1/1178；x.5、非整数scale | 按floor策略产生明确整数；负/0/NaN/Infinity/越界拒；不以epsilon修正观测 |
| 原生尺寸计划±1、旧surface/context、packet旧宽 | 全拒，不能用“差不多”进入READY |
| 外框连续位置/宽高变化但floor不变 | 计划代/geometry更新，两帧重新观察，旧permit失效 |
| window非零screen原点、转向、缩放、布局延后、父子事件乱序 | 各域位置正确；不支持transform拒；旧layout锚/旧frame不能消费新计划 |
| READY后reserve间变化、OPEN变化 | 拒预约零副作用 / hold INVALID首因保留 |
| keyboard仍遮挡或配置未知 | 保持原拒绝，几何修复不绕键盘门 |

独立tester离线补证直接驱动实际生成helper和页面接线，复用T458帧泵及T432许可结论，不复跑无关CPU/旧预约矩阵。后续T408或新真机卡带“真机”标签同设备串行，先编辑→确认固定键盘→准备，保存连续外框/内框/计划/packet/EGL完整原件；READY后继续原hold/独立时钟/PNG/严格标定和颜色配对。若实际内整数尺寸仍不一致，带三域值与geometry先回architect。

执行者要么完成约定范围及角色必要检查，要么遇到具体不可继续问题才停止，局部受阻先做其它项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态例外，不改滴答同名清单。CPU停投。前景背景仅XComponent+shader，禁止Canvas上屏或回退，失败FAULT/标STALE旧完整有效帧。成功原卡报告且本人行结束待验收；失败/关键缺证原卡留未完项、复现原错、证据、尝试、不能继续原因、前置与接手角色，本人行看评论。

本卡完成来源核验、工程裁定及可执行修订。厂商内部精确取整实现仍未知，本方案通过显式整数布局消除对该猜测的依赖；产品READY与后续标定仍待实施和独立取证。
