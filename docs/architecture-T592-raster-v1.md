# T592 v3 · raster/native 门首次归因

2026-11-19；architect按用户“#592的问题定位”处理。不使用真机，不运行产品/测试/构建，不修改T592的effector状态或整份清单。已读原卡最新评论、T592-v3、T659开发与实际签包、fix9原件/截图、实际生成页/controller/predicate及真源模板。原字节证据 `analysis-T592-raster-v1.json`。

**当前已定位到raster/native实际身份、geometry或尺寸的配对门；尚未定位其中哪个字段。报告所谓“rasterRevision V3与planRevision9版本失配”不成立。** 原T659的.same()崩溃本次未复现，不能由此宣十字段/票据全部负控及生命周期已通过，更不是HOLD/PNG通过。

## 已证与纠正

实际T592-fix9签包SHA `8a3238635649c65f72dc0dacca151060b929d98ac90f60bf5a6e153e2682f854` 与签名收据相符，包内ABC及libp013renderer与T659给出的28077347…/4a18eb40…逐项核对。它不是旧same()崩溃包。设备实际加载资格仍按本次原始收据，不由离线包检查代填。

人工查看fix9-screen4.jpeg：失败为`PREPARE_TIMEOUT:RASTER_NATIVE_MISMATCH`，capture仍IDLE、预约视觉disabled；下方可见`rasterRevision="T532_WINDOW_HOST_RASTER_V3"`、`planRevision=9`、`raster.generation=9`、`raster.geometry=11`、1154×1644。**协议名、plan generation、geometry是三个不同量**；planRevision9与generation9当前恰好一致，不需要把协议字符串改成9或把generation改成3。截图是JPEG观察证据，不是严格PNG标定。

`test/t540/RasterGeometry.ets::rasterPredicate`先通过协议/plan、didLayout/draw/layout acknowledgement、当前geometry和节点放置等门，再检查以下十个条件的合取，任一不满足都只给同一个RASTER_NATIVE_MISMATCH：

| 字段 | 必须满足 |
|---|---|
| native.controller/session/surface/context | 各自真实非零 |
| native.run | 真实非空 |
| native.geometry | 等当前准备geometry（截图显示raster.geometry11，但native值未显示） |
| native.packetWidth/packetHeight | 等rasterW/rasterH（1154/1644） |
| native.viewportWidth/viewportHeight | 同样等rasterW/rasterH |

prepare snapshot中的native来自`P013SingleSandController.prepareReadinessCold`：controller=controller session，session/geometry/packet尺寸/run来自实际preparation.packet，surface/context与viewport来自native.getState。它们不是revision字符串；校验之后还有独立nativeReady/currentToken/swap成功等门。因此仅错误标签既不能证明GPU尺寸不符，也不能证明owner身份丢失，更不能称这是HOLD链“唯一剩余门”。

## 证据缺口和最短下一步

fix9_l1–l8布局原件的非空text主要是系统状态栏时钟，未包含应用完整readiness payload；不能声称这些布局已保全native数据。JPEG只显示JSON开头，raster.native十项看不到。本轮保存目录没有当前fix9的`t540-prepare-*.json`；唯一查到的t408vA旧档不是本次attempt，不混用。

**优先读取已经自动导出的同attempt冷文件，不先改产品/重复点准备。** 实际页prepareUpdate在状态切换时调用prepareCold，文件名为`t540-prepare-<session>-<pageGeneration>-<prepareId>-<state>-<observationSerial>.json`，内含readiness.report/current snapshot、raster.native、frame observation和owner/API错误；总UTF8≤16384B，写错亦有具名prepare.cold.write记录。这是现有采集路径，不需要先加产品日志或准备新包。

当前职责：effector在原T592读取同包当前失败的app filesDir原文件与关联WS_EVENT t426_prepare回执，保原字节/SHA/prepareId/pageGeneration/ownerSerial/observationSerial/window/context及单调时间；离线解析/脚本/已有包签名交tester T671。**新v4的准确路径是`readiness.authority.snapshot.raster.native`，其它准备量在`readiness.authority.snapshot`、API在`readiness.authority.api`**，不要照旧结构查report后把undefined当0。先核`readiness.detailTruncated`、`observationTruncated`及PREPARE_COLD_UTF8_LIMIT；超过14336B时v4摘要可能不保authority，这时明确FIELD_NOT_CAPTURED/TRUNCATED，不能声称原始数值已齐。读取旧文件无需重跑prepare。若该进程或文件已不存在，明确NOT_CAPTURED_DIFFERENT_ATTEMPT，再以同一正确包一次fresh prepare，在FAILED前后立即取完整自动冷档，保持真实同scope，不拿旧截图补字段。

离线报告必须列出上述十项的actual/expected和首个失败项，同时保存所有不等项、packet生成geometry和native viewport更新的真实顺序；比较的是数值/身份，不是协议名对generation。若冷档不含该字段、写失败或读取权限失败，附精确endpoint/路径、原始code/name/message和字段缺失，先回architect定义那个缺口的最小有界只读接缝；不得笼统宣没有观测机制。

## 工程分流边界

- 未初始化controller/session/surface/context/run：核实际init/pack/target/presentation首失败，再按真实owner/资源链分流。
- native.geometry落后：核geometry发布与packet生产/同代读取顺序；不能把最新geometry写回旧packet伪配对。
- packet或viewport尺寸不等：核实际组件px请求、EGL_QUERY_SURFACE回值及resize/准备顺序；不能扩大容差或用逻辑vp替真实px。
- 所有十项相等但仍同理由：先检查是否取到另一个observation/attempt的报告，确属同代再由architect修工程观测/谓词。

这些是精确续证分支，不是当前已确认的产品缺陷。源码observe已明确把native.geometry排除在allocation change-key之外以避免纯geometry反馈；不能凭单图就宣无限循环，亦不能把geometry加入key形成追赶循环。没有证据要求designer改算法、提高3000ms、强开READY/预约、去掉pairing gate、改协议版本或重做渲染。

T659已有修复保留，T592完整真实prepare→check ticket→实际reserve→仅RESERVED后consume→HOLD→严格PNG/原标定仍未完成。developer只在具体工程缺陷证据齐并由architect修订后实施+编译；本轮不默认退developer，不设manager/设备关闭门。

执行prompt：不使用真机（本architect）；effector真机读取/执行串行，tester离线脚本/构建签名解析。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅执行者本人状态有限例外），不更新滴答同名清单。GPU驻留优先、CPU停止、目标10暂停、水13→14→12；全部背景前景仅XComponent+shader，FAULT或明确STALE，无Canvas/软件GL产品回退。禁止JPEG转PNG冒严格截图，不降低几何/身份/时限门。
