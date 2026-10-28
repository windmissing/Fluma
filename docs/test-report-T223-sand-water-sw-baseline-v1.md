# T223 沙水DEMO独立规格与基线报告（v1）

日期：2026-10-28；tester；不使用真机。消费 T221 方案 `algorithm-T221-sand-water-demo-v1.md` §10（SW01–14）与 T219 既定合同，与 T222 对齐不互设循环。被测：当前树（T188 A 段 + T209/T215 B 段）；T221 专用实现未开工。执行器 `test/p223_sand_water_sw_independent.cjs`，原始 JSON `test/reports/t223/independent.json`，用例登记 `docs/test-cases.md` §20。

## 结论

**5 PASS / 10 NOT_IMPLEMENTED / 0 FAIL**（预测试 RED）。

## 实现面符号缺口（SW-00 机器可读核查）

T221 专用符号 **17/18 缺**：FLIGHT、deliveryMode、suffix16、event48、waterId、ARRIVE、arrivalSerial、emissionPolicy、flightPolicy、waterProof、optics16、profileRev、FlightVisualConfig、GravityAcceptanceToken、SAND_WATER_SOURCE_V2、vEmit、dragFlight、gFall。现 WATER source 分支：y>S[c] 即接受、全部按列局部 `W[c]+=q`（无空中 FLIGHT 水包出生、无 deliveryMode），与 §4.1「WATER and y>eta: FLIGHT」合同不符的部分全部属 T221 新增工程，归 developer 侧。

## 分项

| 编号 | 内容 | 结果 |
|---|---|---|
| SW-00 | 实现面符号覆盖核 17/18 缺 | PASS（缺口如实） |
| SW-01 | 库存/源：直接注水 W+=q 现语义可对照；**FLIGHT 当次 W 不变分支零实现** | NOT_IMPLEMENTED |
| SW-02 | 弦段碰撞 oracle + 到达 W+=/J-= 恒等 | NOT_IMPLEMENTED（无水包池/轨迹核/ARRIVE 收尾） |
| SW-03 | 容量/侧壁：A 段列局部部分准入（free=1→q=1 PARTIAL）对照 PASS；整包跨列/墙反弹零实现 | NOT_IMPLEMENTED |
| SW-04 | 水沙独立：空水 W=0、仅沙不增 W、混合 eta=S+W 精确成立 | **PASS**（对照基线） |
| SW-05 | 双侧：平床中央 W=8，20 步内左右累计外流均>0（W63>0/W65>0） | **PASS**（对照基线） |
| SW-06 | 止源：停源后纯 step 8 步 Wtotal 保持（重分布继续、无源增减）；凹槽/高脊水包场景零实现 | **PASS**（守恒部分）/ RED（水包场景） |
| SW-07 | 幂等：同 key replay 不重复记账、payload 冲突 IDENTITY_CONFLICT；新 DOWN/切源/emissionPolicy 零实现 | NOT_IMPLEMENTED（物理层幂等 PASS 作对照） |
| SW-08 | 生命周期：reset 入口存在、无 restore 假成功入口（与 T221「无 restore 假成功」一致） | **PASS**（对照基线） |
| SW-09 | 原子性：拒绝 step（负 dt）sourceCommit 不动 | **PASS**（对照基线） |
| SW-10 | 到达排序（column,y,id）单次排序/attempt、ew/tape 耗尽 | NOT_IMPLEMENTED（无水包排序；B 段沙事件有界已核） |
| SW-11 | optics CPU oracle（footprint 覆盖/Fresnel/预乘）+ optics16/V4 | NOT_IMPLEMENTED（全缺） |
| SW-12 | FrameToken desired/captured/rendered、source-only+B 失败、GL/swap 故障 | NOT_IMPLEMENTED（C 段/页面入口零实现，T215 B-14 同口径） |
| SW-13 | 重力接受 token（±89→saturated±30）、速率滤波、shake 仅诊断 | NOT_IMPLEMENTED |
| SW-14 | V1 reader 显式拒 V2 row | NOT_IMPLEMENTED（V2 revision 未实现，无可拒对象） |

## 基线复跑

T188 26/26、T209 35/35、T215 12/12、T189 15/15 全绿。

## 边界与交接

- **T221 designer 设计模型（design-T221-sand-water-demo-model-v1.py）不是产品 oracle**：本执行器自持 Float64/精确算术。
- 全部 T221 新参数（vEmit/dragFlight/gFall/mRef/widthPx/gain/sigma/tint）**PENDING_CALIBRATION**；不自造 fps 阈值；不贴未标定像素/主观分数。
- E-SW1..4 效果需求（四段连续同 run 录像/逐 FrameToken 回执、source/arrival/WATER/床库存对照）交接 effector；设备验证另卡。
- A/B 段离线 PASS 不冒明水 DEMO 可视效果；SW-04/05/08/09 是现实现对照基线，不是 T221 DEMO 通过。
- T221 实现到件后由本执行器扩展逐条断言（规格已机器可读登记）。
