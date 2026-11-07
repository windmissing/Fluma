# P-013 P1 材质与坡度配方：设计阶段复盘 v1

日期：2026-10-24。责任：designer。状态：ALGORITHM_RECIPE_SIGNED / MANAGER_ACCEPTANCE_PENDING / NOT_IMPLEMENTED / NOT_VALIDATED。
任务6adc92ba…272e；对应[确认稿](design-P013-p1-material-recipe-slope-confirmation-v1.md)与[数学参考](reference-P013-p1-material-recipe-v1.json)。只记录算法签收，不假称实现/真机效果闭环。

## 候选取舍

- P0 raw差分：保留旧layout1/E2_APPROX_P0，不拿它代表新σ6核。
- Canvas原型盒核：实现体是单遍13tap盒平均，单位/注释不够精确；不继承为Gaussian。
- 横向Gaussianσ6/R18：选择；只对显示S算宽面法线，深度用原zOrigin1 exp，不模糊bh/mask/C层纹。
- 直接相加带基色B/C或读取已受光Canvas：会重复基色/光照且不满足材质先合成；否决。
- Q空间去基线增量：选择；M资产raw平均与实际量化零噪声基线区分，C3为固定样式灰，不作第二光场。
- 临时对255降亮、C3去均值、模糊C1或加旧二维sparkoverlay：会改变配方/色彩/资产责任；本轮不混入，需要时另版。

## 源码复核揭示的差异

1. 法线来自S而非bh；内部H/L重排不变S时不应改宽面光照。
2. DISPLAY四遍与额外Gaussian叠加，不能对同一输入漏做/重复做四遍；端点固定2dx会减坡，必须给边缘参考。
3. bandColor现包含Hi/旧shade与游程c0取色，不能直接取其输出来当Rbase。
4. M颗粒tile生成用raw半整数均值，零噪声字节则为(181,182,131)；生成前取整与减数不取整都会留下不同偏差。
5. C3实际LUT有65项、固定索引48/16对应字节206/159；两正增量及块边界不等于光照连续性已证。
6. 旧二维sparkoverlay与列级gain是两条路径；当前P1配方只覆盖后者。明确跨栈差异，交manager验收/architect登记，不能暗放Canvas后叠破坏同光场。
7. Q(248/255)≈15.31、Q(254/255)≈111.39、Q(1)=4095，局部饱和粒影响可能远大于全帧SSIM显示；必须局部分材质/迎背光披露。
8. ToneMap/OETF非线性使正确暗面增量不一定单调下降，Q_couple必须比较具体CPU预测，不自行冻结1附近保留率。

## 自检与后续

独立离线算术给出核权重、常数/坡/脉冲/边缘数据、Q样本及C3块值，供tester写用例；未改测试/代码/旧锚点、未用设备。manager验收后architect写rev2，tester重复性/RED先行，原G0–G5门序保持。颜色/局部效果/帧时均待后续双报告；效果不达标回designer迭代版本，不改冻结线求绿。
