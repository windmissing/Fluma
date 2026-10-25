# T51 空容器DEMO开发进展

PARTIAL_IMPLEMENTATION_WATER_CONTRACT_DECISION_PENDING。不使用真机。T49/T48独立交付；本卡不宣告全部完成，保留原任务进行中由manager登记。

入口：冷启动EntryAbility，Want字符串emptysanddemo=1，加载pages/EmptySandDemo。独立页面与P051EmptySandDemo，不修改SandCanvas。XComponent.TEXTURE实际消费P013RenderController.submitDualField，同一publication画高度场/真实粒子，无probe/Canvas沙兜底；失败显示等待/拒因不伪装成功。

已实现：零沙/粒子/源初态；触摸以T36 P011GestureRateClock把帧ns明确转秒，首帧零、不按回调次数加沙，8unit/s候选演示率；真实injectGestureSand→particle route→field.step→LAND床。Up/Cancel/隐藏/Ability后台均停源，不积后台债；reset清场和过滤器，物理失败后停源/输入直至重置。T50方案落盘后接入P051GravityInputAdapter：XYZ低通、XY方向atan2(-x,y)/低投影0.3/±89边界、XYZ残差单独shake，tick前成组设置tilt/shake；WaterSandField发布effectiveTiltDeg进P011PublishedRenderState，渲染不从原始传感器猜角。EMA alpha0.6复用现值；Aon=1、Afull=6m/s²、20ms回调、250ms stale为明示NOT_CALIBRATED候选，仅适配器配置，不改物理ramp/预算。诊断demo_gravity_config/empty_sand_demo输出配置、输入、提交和实际角。

离线：实际空场→粒子→LAND→同包床及停源/重置/Float32边界5/5；T50方向与shake分离/stale/有效角6/6；gesture18/input124回归通过；ArkTS+arm64 native编译通过。报告test/reports/t51-developer-progress.json。

待裁定的真实冲突：T51原卡只要求容器无沙，T50 §5生命周期行却要求水/沙/粒子均零。现事务NO_WATER会拒绝干场，不能兼得。本候选保留不绘制的waterTop160/cap256，沙/粒子/源账本为0；水库存20480是真实存在，不能写为空水。触点y高于waterTop时现源出生位置会夹到水域边界，尚不能宣称整个空气区精确触点出生。已经向用户请求裁定并反馈T50原卡；未绕过NO_WATER或直接增高。若要求真干场/空气发沙，需designer给最小源事务接口后实现；若允许透明水库存，则按裁定修正文书并补交完整完成回执。T52最终效果前置尚不能仅凭本进展解除。
