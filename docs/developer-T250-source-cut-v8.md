# T250 原source precheck与逐cut接点回执

2026-10-29，developer；不使用真机。隔离T250-v16。

cut1 BEFORE暂时损坏真实请求的heavy[0]=-1，原injectGestureSand有限/非负precheck返回原null回执，finally恢复原输入；原txCode仍0，不伪造业务失败码。只在冷faultPlan指定root/ordinal启用，后续query/档案不重做请求。两freeze臂14/0核原null回执、完整authority前后原字节相同、撤销注入冷对照COMMITTED。cut6两臂24/0与聚焦59/0仍通过。源码和字典只加隔离生成器，生产无fault hook。

`test/reports/t250/cut-inventory.json`逐一保留20个原cut的file/line/context与Before/After接点状态。除已实现cut1/cut6 BEFORE外，其余18 BEFORE是尚未编写隔离损坏/rollback-readset适配，不能归因缺工程或设备。20 AFTER需精确区分“成功检查后的观测边界”和“后续原消费方拒绝”：T256只给边界名，没有给每个AFTER的下一拒绝谓词/取消与原firstCode归属。直接throw或改已返回码会制造伪业务故障；该合同问题已在原卡精确反馈architect，非笼统把所有切片写缺失。Before实施不因此整体停下。

证据v16-source1.json.gz/v16-cut6.json.gz/v16-focused.json、v16-manifest.json、v16-package.log.gz与t257/T250-v16.json。**仍部分交付，看评论**，2/40边缘有真实注入，原readset全覆盖/可见peak接口还未齐，不能宣T205整体PASS。native/ArkTS/PackageHap与签包均以附带日志为准，未执行真机。
