# T99 材质离散表示选型复盘 v1

2026-10-26，designer；路线 `P013_NATIVE_X_F64_BASEBYTE_V2`；设计交付，未实施／未验证，不使用真机。

GPU整数173已足以研究局部表示，但T88最终色来自RGBA8目标，不能把本例变成全1.956码误差定因。本轮保持architect的范围，未重跑GPU或覆盖旧报告。

比较了GPU浮点为主（会换F64参考，拒绝）、epsilon／改gain（改配方，拒绝）、列中心byte（不能表达同列多个像素的M带，拒绝）、整屏atlas（正确但O(WH)）、软件F64 shader（精度证明／性能复杂）与原生X三带strip。选择后者利用base依赖X和k、不依赖Y的可分离性：逐native pixel中心先采Float32 profile，在冻结F64操作序下混M／乘gain／clamp／round，再传整数；O(W)而非整屏。

另发现half邻域合同细节：floor(RN64(x+0.5))与理想half-up在nextDown(0.5)可不同。不能以“确定取整”为名悄悄把旧F64 evaluator改成Math.round；保留原求值，若要改数学含义另版裁决。

新条带是frame数据，不替代grain/strata静态代；绑定profile/geometry/spark/mirror/配置及frame token，避免新物理帧混旧表。七版本保留原义，新增必填representation/evaluator版本纳入digest；不是不升级身份就换shader算法。

代价是额外CPU准备／小纹理上传和一次fragment整数读取，尚无端侧性能结论；新增纹理单独计数。新变换若让gain依赖Y或启MSAA，X可分离证明失效，不能继续近似。base一致不证明mask/k、Q/ToneMap、target/屏幕一致；继续T101与设备A/B/C。方案回architect落工程文档，不由designer直接派developer盲修。
