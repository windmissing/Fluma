# T237：GPU水沙M1方案复盘 v1

2026-10-28，designer；路线`gpu-resident-water-M1-v1`；不使用真机。主稿`algorithm-T237-gpu-water-M1-v1.md`，证据`design-T237-gpu-water-M1-evidence-v1.json`，模型另文。未实现/运行产品或shader，非设备PASS。

## 候选与选择

1. **CPU T221/T222先做效果再搬GPU：REJECTED为本次最终路线。** 用户已明确所有新增仿真效果优先GPU；旧CPU仅参考/兼容，不能默认先投产。T226 C基础工作保留，不回退他人修改。
2. **把T221 F64公式改GLSL float：REJECTED。** 不能保留旧RN/顺序/阈值；water/J/source/audit相差会被隐去。选择继承Q20整数质量与新运动版本，量化拒量显式，内部水沙exact0漂移。
3. **整quantum冻结AIR/WET不处理入水切换：REJECTED。** 最多8.3ms阻力延迟，小浅水层可能整步穿过而没有湿阻力；不满足本轮明确湿沙分段合同。
4. **65536固定微步或CPU求碰撞：REJECTED。** T16仅是事件时间格，不是65536 dispatch/每包CPU读回。GPU有理弦/ceil时间直接找事件，事件数按运动性质/冷cap有界。
5. **整数事件分段＋隐式阻力＋GPU统一event64：SELECTED。** 水/沙各ID序冲突解析、水边两色、床三色、末尾低到高ARRIVE，在同候选/重放/原子接受中运行。新增shader/math与CPU指数积分不等价，全部单独rev/新run。

## 关键修正/风险反例

- 直接W源不能表达高处下落；J成功出生总账一次、到达内部换W不再计源。
- 水头列中心有半网格，用head2/bed2保精确；q equalize界是delta2/4、越床界除2，不能把原实数公式直接套到double-height单位。
- 水/沙pool不能共用ID/slot含义，事件需namespace+generation；MAX_ID要有显式exhausted位，不能nextId wrap0。
- 只按waterId到达会漏掉先跳过、后来被低包抬面吞入的高包；按column/y/id低到高末扫，整数转换R不变，合法ARRIVE无需RN无写WAIT。
- MEDIUM/碰撞需要真实剩余时间；T16 ceil偏差明示，WAIT消费余时有事件，重复零面无进展整root失败。
- 仅重算事件速度却用整段末速度画弦，可能让弦方向与当时速度相反，不能据此证明只有一次方向反转。增加速度零点TURN切分和宽域梯形位置，<=16次二分/坐标的成本与事件行均计入。
- 事件上界依赖固定角/冻结水面、同向力/终速、无反弹/随机shake；若将来加流速/扬沙/压力面，需要新证明，不照搬48/16。
- 新water16、pub水20/列16和大event arena不能假装M0预留Kw就够；完整预算增至23,342,736/55,111,056B，非driver/RSS。
- Kw64在约100Hz源且首落约0.9秒时会先满池；推荐Kw128连续DEMO候选，另算25,471,888/57,240,208B与128/137 dispatch/quantum，不靠降频/合并源或假动画掩盖拒量，也不对任意负载承诺无拒绝。
- frozen medium诊断和ARRIVE后的当前eta不是同一个面；渲染遮挡按当前profile，不能用旧medium bit画错水。

## 验证/接力边界

本轮自持整数/Fraction模型核水头五界/守恒、隐式湿速度/时间格/碰撞示例、到达顺序、ID耗尽与事务模型、事件上界和资源/dispatch。独立tester须自持oracle且跑真实实现，不能以designer模型替产品验证。参数、可见水层/速度、GPU性能/平台资源与新管线同帧效果仍待独立/设备证据。

architect接T233/T236扩展M1manifest和共享hunk，不覆写M0；新效果GPU优先、CPU参考保留、未实现明确feature拒绝；失败先architect，设计缺陷再回designer。仅自己行状态/原卡回执，其它管理账本与任务不改。
