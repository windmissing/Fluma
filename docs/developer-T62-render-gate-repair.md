# T62 拒绘最小修复交付

2026-10-26，developer。不使用真机。对应 T60 §3–6、T61 bb2b534 冻结反例。

显式 `prepareDualFieldSurface(sessionId,expectedSurfaceEpoch)` 在原 mutex/owner 下配置 basic composition，调用无 caller 尺寸依赖的 `PrepareCurrent` 创建/查询真实 EGL surface，再解绑；不 draw、不 swap。回执返回真实像素与配置后的 epoch；原 `EnsureCurrent` 继续严格检查 epoch/尺寸。DEMO 新入口 `submitDualFieldSurfacePx` 先 prepare，复核尺寸和 epoch 后以 SURFACE_PX_V1 完整容器提交。旧逻辑入口不变，真实非均匀仍拒绝。XComponent 自身测量和触摸共享原点，移除外层布局边框，radius 使用 UIContext.vp2px(1)，geometryEpoch 随测量更新。

`COLUMN_PRESERVING_F32_RENDER_V1` 只给新入口 opt-in。权威 Float64 源先严格检验，再将 X 投影至原列内最近 Float32；Y 保持闭域。两遍处理后才改 packet，拒绝保留旧包、原物理发布不写回。RAW_F32_V1 默认及原边界拒绝用例保留。新增 p013_dual_geometry（真实宽高/epoch/commit/投影数量及误差/首修正样本）和限频 p013_dual_rejected（首拒 slot/ID/state/phase/mass/原值/Float32）日志；原 p013_dual_frame 平坦回执不改。

离线结果：repair 6/6；原 dual packet 11/11；P0、composition 全绿；真实 C++ lifecycle/bridge host 全绿（补显式 prepare、严格尺寸、过期 epoch、后台重建自测）；hvigor ArkTS/native/PackageHap BUILD SUCCESSFUL，零错误。证据在 test/reports/t62。native/平台边界为合成桩，未使用设备。

T61 历史证据和执行器不改；其源码身份校验是修复前身份，不能在业务修复后冒称仍与 T52 六文件相同。新增执行器用同边界值进行 RAW 拒绝与新投影通过、相同小数几何旧启动拒绝与显式路径成功对照。

设备缺口仍交 T63：真实 surface/DPI、首拒谓词与同 commit 屏幕、落床/松手/reset/手持重力、前后台/resize。离线成功不等于产品 DEMO 已通过，T52 FAIL_RENDER_GATE 不在本卡自行改判。T54 上传接口、T59 干场均未混入本提交。
