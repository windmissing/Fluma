# T239 GPU 沙水 M1 独立规格 · WM01–12 报告 v1

- 角色：tester（独立验证，**离线**，未用真机）· 日期：2026-10-28 · 状态：**结束待验收**
- 消费：`algorithm-T237-gpu-water-M1-v1.md` §2–§10 WM01–12 合同 + T238 engineering manifest V2
- 执行器：`test/p239_m1_water_independent.cjs`；原始 JSON：`test/reports/t239/independent.json`；机器可读规格：`test/reports/t239/wm-spec.json`
- 独立性：自持 BigInt RNE（r×2 vs den 精确比较、ties-even）/整数时间格（T=65536, D=7,864,320）/head2 通量/光学公式 oracle；**不调用产品 helper、不引用 T237 designer python 模型**；designer 模型不作为产品通过依据

## 结果：12 PASS / 0 FAIL / 1 NOT_IMPLEMENTED（预测试 RED）

| 用例 | 内容 | 结果 |
|---|---|---|
| WM-00 | 产品实现面符号扫描（10 个 M1 符号） | **NOT_IMPLEMENTED**：GRS_Q20_WATER_M1_V2 / SEGMENT_T16 / SOURCE32_V2 / EVENT64_V2 / SCENE_Q20_M1_V2 / FIXED120 / waterIdExhausted / arrivalSerial / DIRECT_W / FLIGHT_J **全部缺失** → 实现面用例 RED 归工程 |
| WM-01 | source 双选事务判定（y<S 拒 POSITION_IN_BED；S≤y≤eta DIRECT_W；y>eta FLIGHT_J；q=floor(requested·Q)，q=0 拒 NUMERIC_RESOLUTION_REJECTED；waterBefore/After=W+J） | PASS（7 判定，判定函数自持） |
| WM-02 | ARRIVE 恒等式：J−m/W+m，R 逐位不变，eta 抬 m | PASS |
| WM-03 | head2 通量公式独立实现：q=min(qReq, floor(δ2/4), W[d], 头差上限, receiver free 含 P/J)；守恒 200 步随机 0 漂移；头差不反转；等头无方向 | PASS（consFails=0） |
| WM-04 | AIR/WET RNE 积分核：AIR v1=RNE(Dv0/(D+kAir·r))+RNE(a·r/D)；WET 隐式阻力无二次重力；梯形位置；Vmax 越域 FAIL 不 clamp | PASS（v1=−21251140 ≈ v0−447k，θ=0 a=−gQ 语义） |
| WM-05 | LAND：P−q/S+q/W 不变/eta 抬 q；跨列 P 转移不排水；receiver free=Cq−S−W−P−J | PASS |
| WM-06 | 到达排序低到高单扫：抬面吞高包（y=12 包被抬后 eta=14 吞入），y=20 包保持 AIR；同 y 按 id 升序；退役 slot 无重复 | PASS（arrived=[1,2,3]，终 eta=16Q） |
| WM-07 | 事件时间 ceil(t=⌈rSeg·λ⌉ 3 组精确)/零点 tick 二分/waitTicks=T−τBefore−tMotion 账目；eS48>eW16 结构 | PASS |
| WM-08 | event64 V2 namespace bits18–19 编解码（SAND/WATER/FIELD/SOURCE 往返）；stock 按 kind 映射（ARRIVE W/J、LAND mh/P、BED mh/mh、WATER_EDGE donorW/receiverW）；FIELD 行 slot=id=0 | PASS |
| WM-09 | root 生命周期：quantum 失败 candidate 全回滚且已接受 source/前序 quantum 保留；ARRIVE 不增 I_water/sourceCommit；stop 切源保留已成功 J/P/W；dt=0 无写 | PASS |
| WM-10 | 光学 depth=clamp(eta−max(y,S),0,W)（2 判定）+ T=exp(−σ·depth) 域；同 token ARRIVE 同帧退 glyph 语义 | PASS（公式层；产品 scene RED 由 WM-00 覆盖） |
| WM-11 | §9.1 dispatch 常量冻结：K192kw64=114 / K512kw64=123 / K192kw128=128 / K512kw128=137 dispatch/quantum；source G1/F4=488/526/544/582；water bank 增 16·Kw 字节（64→1024B、128→2048B）；≤5 root/tick | PASS（L(K) 公式文本歧义如实记录，显式数值冻结） |
| WM-12 | isolation：T238 manifest schema/featureBitmap=3/NEW_EMPTY_RUN_BH_ZERO 核对；M0 reader 拒 M1 rev；旧 F64 waterTotal=W+J 仅参考 | PASS |

## 关键独立结论

1. **WM03 head2 通量公式在独立实现下守恒成立**：200 步随机 W 值对（含随机 kappa=480 与四界 min 裁量）总量零漂移、头差不反转、等头零流、receiver 容量含 P/J 裁量正确。
2. **WM04 隐式阻力积分正确性**：WET 公式 v1=(D·v0+k·r·vT)/(D+k·r) 数值上严格介于 v0 与 vT 之间（无过冲），且不叠加 g——与"无双重重力"合同一致。
3. **WM06 低到高单扫语义**确认：抬面正确吞入先前高于旧面的包；结束后活包 y>eta 不变量成立。
4. **实现面全部缺失**（WM-00）：M1 是全新 policy 栈，产品核 0 符号命中——所有实现依赖用例在工程接入前保持 RED，与 T237"T237 交付是算法与测试合同完成，不是 M1 实现"一致。

## 边界与诚实声明

- WM11 中 §9.1 文本 L(K)=hK(hK+1)/2 与显式数值（Lw21/Lw28、114/123/128/137）不能同时反推吻合，按文档显式数值冻结并在执行器注释记录差异，留 architect 澄清。
- WM10 仅为公式层 PASS；产品 scene（fragment SSBO/同帧 glyph 退役）未实现，不构成任何视觉效果通过宣称。
- 独立设备/端到端归后续设备卡；本卡不含真机。

## 失败上报

无阻断。
