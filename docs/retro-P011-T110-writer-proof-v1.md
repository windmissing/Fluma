# T110 writer证明补篇复盘 v1

2026-10-26，designer；P011_WRITER_PROOF_V1。设计交付，未实施／未用真机。

T104指出通用ρ文字不足以生产实施，本轮改为逐RN图：settle显露collect请求量与apply分相desired的比例差，top显露RN(mh+ml)与精确库存和不同，LAND显露q=0仍有接触运动副作用。三个问题不能用一个“几倍ULP”掩盖。

选向外节点包络＋固定branch域＋真实patch证书，不选逐ULP循环或把整体split当单调函数二分。min分支端点的RN逆像接口未落地时，只支持已明确定义的MIX内缩；其它branch保留RAW/ZERO，不承诺最大物理grant。最大性只对可证内域成立。

明确control q、分相函数值和实际存储变化三层，不伪造phase尾差。non-worsening基准在writer入口冻结，三候选不能各换基准；ZERO无写与CONTACT_HOLD运动分开，后者未获本篇完整证明。

未选择为了整卡“可用”而给其余writer写泛化保证。registry列出未支持路径、初始化/restore的验证式支持与invert/外源缺项；完整严格route仍不得开启。设计完成与生产闭合分开，architect据此逐核落工程／回归，旧T98不重开、旧非法快照不原地修。
