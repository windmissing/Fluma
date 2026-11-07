# T406 独立标定与屏幕配对 V2 开发交付

2026-11-07，developer。不使用真机。依据 `architecture-T402-single-sand-screen-mapping-v1.md` 实施工具及隔离签包；本轮不宣称系统显示端到端通过。

`test/t406/generate.cjs` 基于 T394 只在生成目录追加诊断能力：独立不对称 XComponent 全屏 shader、显式诊断标记及 SHA、同次 candidate/default-back 读回、浮点全局 box/vp2px 归档、冻结 calibrationId/planId 的三ROI预约。普通材质 shader、普通 draw 零读回及既有 GL 状态/失败恢复保持原合同。没有 Canvas 上屏或回退。

`screen.py` 将空间与颜色分开：预登记候选 nearest 模型、25个覆盖网格点训练/holdout及额外横纵跨组件验证，唯一通过后冻结设备/窗口/context/surface/geometry/浮点box/截图配置身份。模型只从独立标定图选取；A/B 阶段不搜索位移或半径。根据预登记 screen 核心计算完整 native 反查 tile，所有核心像素必须有原始样本，守64×64及双目标98304B。原 T394 工具及旧结果码未改。

V2 输出 `DISPLAY_CALIBRATED_A_AND_B_PAIRED`、`DISPLAY_REPEATS_A_IN_B_HOLD` 或显式不可判；静态 `single` 单独输出同次三区配对。逐项 RGB 零容差、native candidate/back RGBA 差及 alpha 分列，保存每个差异坐标/对应native索引/样本数。独立 CPU 投影 mask 验旧/新支持与无污染背景；候选算术域0.0003px 单列边界不确定坐标，全部边界点仍参与 RGB 比较，未从覆盖数量差推出 AA。动态 AIR 夹具的原生占据另作 oracle 检查，非均匀背景未知规则拒判。

`capture.py` 记录实际截图程序调用前后 Python monotonic ns、原始工具输出、退出码、PNG SHA。判读要求整个经独立 clockMap 映射后的区间连误差包络都在同token/frame/sequence hold内，且所有几何/预约身份未变。操作者手填时刻不能代替 receipt。真实设备跨时钟映射由 tester 保留独立同步来源；工具不从粒子图拟合时钟或变换。

| 自验 | 结果与证据 |
|---|---|
| `python test/t406/units.py` | 1:1 118→118、1178→1177 最近邻正控；旧/新支持分别位于组件1/4、3/4，后者跨缩宽对应的索引差；静态 single 正控通过 |
| 同上负控 | B整体1px、A/B同时1px、内部常量错1码、native与screen同时改半径、漏B、重复A、错误width/比例/半像素原点/双轴镜像、漏native列、换context/geometry/device、截图越hold和RGBA SHA污染均拒绝或给对应重复A码。详见 `test/reports/t406/units.json` |
| `node test/t406/controller_units.cjs <generated>` | 实际 ArkTS controller 两静态臂/两动态臂、冻结计划解析、native失败、hold换代、正常不读回、原始输入/沉降证据通过 |
| `node test/t406/build_host.cjs <generated>` | 真实 PC ANGLE/GLES：candidate 原生三ROI所有像素精确命中独立CPU诊断pattern；back完整、alpha255、时间顺序；原ROI界限/预算/read-pack恢复/目标不全/failed swap LastGood/context重建回归通过 |
| 旧 T394 controller / `python test/t394/pair_units.py` | 原静态/动态控制器、原 paired/repeated/clock/mapping/SHA 反例均通过 |
| Hvigor `default@PackageHap` | native + ArkTS 零错误，BUILD SUCCESSFUL；保留既有告警 |
| 本地签包及验签 | 见 `test/reports/t257/T406-complete.json`，verify-app/profile 均通过 |

PC ANGLE 的独立诊断 back 相对精确 sRGB-decode 模型有1个通道1码偏差，未用近似容差接受。native host 记录该原值，candidate精确性和双目标采集/恢复通过；该环境完整颜色模型不能据此宣PASS，实际运行 `calibrate` 规则不匹配就拒判。工具正反控为合成原始manifest/RGBA/PNG/receipt夹具，非设备截图，不能替代 tester 真机结果。

交接用法与完整 spec 字段在 `test/t406/README.md`。tester 独立标定后用原材质 shader 采中性厚床三ROI、非零gains、CONTROL及LOCK A/B，保留全部反查列与原始截图时间证据。旋转、非 nearest 过滤、未证明颜色转换均显式不可判；这些模型没有被本轮扩成已支持。T396 旧档的 NOT_JUDGABLE 保留。

最终签包：`D:\hmprojects\HelloWorld\.codex\local-test\t257-sign\T406-complete\T406-complete-signed.hap`；SHA256 `bb7b0558f4118c57896ca58024829034cf981b196e89dfeba47a0397d1f57cc9`。
