# T113：整数条带测试先行 v1（冻结 F64 evaluator oracle + 资源合同基线）

日期：2026-10-26。角色：tester。任务卡：T113。合同：`docs/architecture-P013-T105-base-strip-and-color-routing-v1.md` §2/§7（T99 冻结 evaluator；版本 `P1_FROZEN_F64_EVAL_V1` / 输出 `P013_NATIVE_X_F64_BASEBYTE_V2`）。执行器：`test/p113_base_strip_baseline.cjs`。raw：`test/reports/t113/p113_results.json`。**10/10 PASS**。不使用真机；不改主参考/生产。

## 1. 冻结 F64 evaluator oracle（逐 case）

| case | 断言 | 结果 |
|---|---|---|
| EV-01 | T88 点：冻结 F64 evaluator（binary64 op graph）**= 172**；旧 F32 路径（fround gain）**= 173**——两个身份都保留，无任何容差 | PASS |
| EV-02 | W=128/256/1178 条带 byteLength=12W（三 RGBA8UI 层） | PASS |
| EV-03 | 小数 box（origin 16.5 / width 128.7，W=384）：validX 按 `0≤qx0<boxWidthPx`，129 列有效 | PASS |
| EV-04 | mirror **恰一次**：镜像条带 = 未镜像条带的水平翻转 | PASS |
| EV-05 | 有效边界：j=0..127 有效，j≥128 RGB=0/A=0 | PASS |
| EV-06 | M 带同模型列多 native pixel：W=256/boxWidth 256 → 256 条目（逐 pixel 求值，不合并） | PASS |
| EV-07 | spark：ON 只改提升列区域（150 基 → clamp 248），OFF gain=1 精确（base=palette 字节 150） | PASS |
| EV-08 | 半整数邻域：**150×1.15 在 RN64 = 精确 172.5** → floor(+0.5)=173；**上传 Float32 gain（1.149999976158142）乘积 172.49999642 → 172**（T88 V2 点）；clamp 248；零基 0 | PASS |
| RED-01 | `P013BaseByteEvaluator.ets` / `base_strip_uploader.h` / `stageBaseByteStrip` 桥入口**全部缺失** → **NOT_IMPLEMENTED**（T114 预测试 RED，非业务修复失败） | PASS |
| RES-01 | 资源合同用例登记（供 T114）：stage descriptor 不可变 + payloadSha256 有效字节域；pump 65536 总预算 base 优先固定策略；迟到请求丢弃 un-leased PENDING 记 discardedSerial；材料+条带双 lease 原子 Commit/Abort；context 丢失 GPU 句柄失效 CPU 保留重验证；baseStripTextureLiveCount≤2 独立计数；非法 budget 结构化 400 零副作用 | PASS |

## 2. 执行器自身期望修正（非判据变更）

- EV-07 首版误用 palette 红 216（×gain → clamp 248 掩盖 spark 差异）→ 改蓝 150；首版 OFF 期望误用 gainF32 → 生产 frag OFF 时 gain=1.0 精确（base=palette 字节）；
- EV-08 首版误记 150×1.15=172.4999…——实际 RN64 **恰好 172.5**，半边界命中（173）；上传 Float32 gain 才落在下方（172）。172/173 分支对即 T105 §2 所述"half 整数邻域的加 .5 舍入效应继承 F64 evaluator"的文档化夹具。

## 3. 对 T114 的交接

oracle 参考实现（`evalStrip`，binary64 逐 op 与生产 frag 次序一致）+ 全部边界夹具 + 资源合同清单已冻结在 `test/p113_base_strip_baseline.cjs`；T114 实现 `P013BaseByteEvaluator.ets` 后以本 oracle 逐 j×三 k 零字节差验收；禁止 1 码容差。旧 V1=173 身份保留为历史对照，不冒称 V2 结果。
