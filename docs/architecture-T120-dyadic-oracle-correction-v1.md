# T120 严格态oracle位型与测试合同到件核对 v1

日期：2026-10-26。architect。原卡 `6adf3c1de4b03273ad85d1a8`。不使用真机。消费T111 980e047执行器/fixture/raw、T104与T100。**核对交付完成：存在合成fixture错标与断言强度不足，BigInt基本解码未见对应公式错误；生产T112基础继续，不改H/判线**。

## 1. 位型必须区分实际raw与合成输入

本轮以DataView/Buffer独立读取给定位型并算术核对（只读复算，未运行T111执行器）：

| 身份 | binary64 hex | 值/与H距离 |
|---|---|---|
| 原H | `3fe279a74590331c` | 0.5773502691896257 |
| 机械nextUp(H) | `3fe279a74590331d` | 0.5773502691896258，+1ULP |
| 机械nextDown(H) | `3fe279a74590331b` | −1ULP |
| T86/T100真实边差 | `3fe279a745903320` | 0.5773502691896262，**+4ULP** |
| T111 `H+5e-16`合成值 | `3fe279a745903321` | 0.5773502691896263，**+5ULP** |

H处ULP=2^-53=1.1102230246251565e-16；真实差为2^-51=4.440892098500626e-16。T111报告称nextUp为3320、真实T86=5ULP错误；**冻结fixture实际nextUp已是331d，机械函数本身正确**。错误在以`H+5e-16`重新构造并命名`t86Slope`，不是原raw提供了另一个新事实。OR-04的within4=H+4*ULP恰为真实旧raw，下一合成值是第五ULP，二者都严格超H；“4ULP在文档边界内”只能指描述性的ULP计数，**绝非允许域**，T104从未允许H+4ULP合法。

## 2. BigInt oracle哪些成立，哪些未证

`decompress64`正常数e=expField−1075、sig=hiddenbit|frac；次正规e=−1074、sig=frac；符号零规范比较相等；toCommon shift=2148+e非负，能精确表示有限binary64的整数倍。这个数学解码与T104一致，未因fixture错名推翻其BigInt加减能力。

但11/11不能支撑全部文字主张：

- OR-01仅检查有限样本与排序，不是穷举每个有限数；最小shift为1074，不需再减52。其数学一般性来自解码证明，非样本数量。
- OR-02只断言res>−H，`(res!==rnResidual ? true:true)`恒真；没有严格比较+H，也没有强制产生精确差与RN差不同的反例。不能用该PASS证明四项predicate已完整验证。
- OR-03没有实际断言`productCommon(1,H)`与H的等价，只计算未使用；OR-07仅看sigBits≤106，未把product对齐并检查最高用位/多项carry。productCommon返回sign/sig/exp，不是已物化共同单位整数。
- OR-09硬编码true，是合同说明而非白名单拒绝执行证据；signed-zero用例不等完整rate/directed门的边界矩阵。
- RED通过require异常判断缺模块，没有区分文件不存在、转译/加载失败；历史当时未实现身份可保留，未来文件出现不能因为加载环境不支持.ets继续写MISSING。NOT_IMPLEMENTED预测试不派业务缺陷返修。

这些是测试输入身份/断言覆盖问题，**不是已证生产数值算法缺陷**。T112按T104 exact predicate合同实现，不能为了追fixture错名扩大H或改实际T100输入。

## 3. 给tester的版本化精确修订书

不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅自身行状态有限例外；不改滴答同名清单、不回退他人修改。所有渲染仅新XComponent+shader含背景前景，无Canvas回退，失败FAULT或显式STALE旧有效新管线帧。

保留T111_V1 raw/fixture/11项历史结果，新建T111_DYADIC_ORACLE_V2及显式V1勘误：rawT86Slope直接从T100 hex3320解码，syntheticHPlus5eMinus16另起名3321，nextUp/down机械位操作从331c得到331d/331b。逐值保留bits/十进制/精确diff及ULP计数，实际strict expected均按±H，不存在4ULP容忍。

将OR-02换成真正四项残差比较±H、恰界/邻值拒绝及一个可证明不同的rounded-vs-exact反例：`Sright=1,Sleft=2^-54,u=0,H=nextDown(1)`，精确残差1−2^-54大于H，RN(1−2^-54)=1（值不同但均拒绝）；另取`u=2^-54,H=nextDown(1)`，精确三项1−2^-53=H应接受，而先RN(1−2^-54)再减2^-54均舍回1，错误逐步残差会拒绝，直接区分predicate。每例给独立BigInt expected，不用“相同也PASS”的三元式。必要选择能击穿生产错误实现的反例，不镜像同一生产函数。

OR-03明确物化product与单位对齐比较，OR-07检查max×max及最多8项的实际highestBit/负向/carry不超T1044352容量、subnormal×subnormal最低指数−2148，nonfinite拒绝。rate门用精确差/精确omega×dt，边界内外ULP与dt/omega0、±0/相反target；白名单unknown表达式真实调用并断言ARITHMETIC_EXPRESSION_UNSUPPORTED。生产模块未交时oracle自身可先验证期望，生产阶段标NOT_IMPLEMENTED，不把硬编码true当绿证据。

RED先fs.existsSync再分类load/compile error；按基线commit记录历史缺失与当前模块身份，禁止覆盖历史结果。修订回architect，T112引用修正位型与原合同继续，不要求重开已完成T111。

## 4. T112交接与完成状态

给T112的精确提醒：331d是真nextUp；3320是旧真实超界四ULP；3321只是T111合成值。严格predicate对它们都按实际位型算，+1/+4/+5ULP都在H外；零目标/no-op/真实patch、writer证明实质前置仍按T104/T110，不用oracle更新自动启生产route。

本轮核对目标已完成，原卡报告测试修订问题和文件/反例，自己的行结束待验收；没有将待补测试证据伪造为已补，也不把上游预测试RED当本task业务失败。未改业务/测试/阈值、未重跑测试/真机。
