# T235 明水 C 基础独立验证 · 报告 v1

- 角色：tester（**离线**，未用真机）· 日期：2026-10-28 · 状态：**结束待验收**
- 消费：T226 0eeaa8d 开发交付（C 基础/optics16/V3 帧包/切源页）
- 执行器：`test/p235_clearwater_c_independent.cjs`；原始 JSON：`test/reports/t235/independent.json`
- 独立性：透射/预乘/Fresnel/深度语义/相邻列面积均自持实现，不调用产品 helper；光学公式层与 T221/T222 已定视觉数学一致

## 结果：8 PASS / 0 FAIL

| 用例 | 内容 | 结果 |
|---|---|---|
| CW-01 | C 基础符号面 8/8 在位（copyCurrentSceneInto / copySourceReceiptInto / drawClearWaterSceneV3 / ClearWaterPublication / SourceController / FrameClock / GravityPort / CW_B_EVENT40_V1） | PASS |
| CW-02 | `P011ClearWaterEngine(clearWaterCConfig(192))` 真实构造成功，公开面完整 | PASS |
| CW-03 | optics16 域（sigmaWater=0.025/opticsRev=1/tintLinear[3]/**PENDING_CALIBRATION 不冒已标定**）+ 独立 oracle：depth=clamp(eta−max(y,S),0,W)、W0→T=1、T=exp(−σ·d)、预乘 body=((1−T)tint, 1−T)、Fresnel(1)=F0 | PASS（9 判定） |
| CW-04 | `ClearWaterSourceController(engine,1,1,1)` 构造 + stop/cancel/down/move/up 生命周期 API 在位 | PASS |
| CW-05 | **T227 FlightV2 / T228 V4 全代码库 0 命中**——NOT_IMPLEMENTED 显式；V1 水直进 W 是现行基础 | PASS |
| CW-06 | 面积高光独立参考：相邻列 x 不重叠（交集 0）、W=0 → 面积 0 无高光 | PASS |
| CW-07 | FrameClock 原始差分：去注释后代码无 clamp/补偿（源文件头注释"No clamp"与实现一致） | PASS |
| CW-08 | source dedup 机制 + NOT_COMMITTED 独立枚举在位 | PASS |

## 边界（诚实声明）

- 本卡为**公式/结构层独立验证**：V3 native packet raw 字节、F32 跨列投影、同 token 六层、Present/Swap 失败原子性等属于设备/native 端到端面——T226 native-host 29×2 已自测，本卡不重复宣称；设备显示验证（displayPresentVerified）仍归设备卡。
- 面积高光 Fresnel 求和、真实切源生命周期行为序列（stop/reset/后台/context 失效清 pending）仅核 API 面与公式，未逐场景驱动引擎（可与 T226 selftest 14 PASS 互补，但不替代）。
- CW-03 显式确认全部光学参数 PENDING_CALIBRATION，未自造"已及格"阈值。

## 失败上报

无阻断。
