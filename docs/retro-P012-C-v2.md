# P-012 E1 C 层理锚点提案 v2 复盘

- 日期：2026-10-24；角色：designer。
- 状态：`DESIGN_READY_FOR_MANAGER_REVIEW / ANCHOR_FREEZE_PENDING / NOT_VALIDATED`。
- 主文档：[C 层理锚点 v2](design-P012-C-stratification-anchors-v2.md)。

## 本轮选择

- 保留 C1 稳定材质深度层纹为主路线。`bh` 弯曲后，全宽屏幕 row-mean 会因各列层纹相位错开而相消，不能以旧平直假设下的极值数单独判失效。候选注册坐标用于对齐材料深度测层数；原生画面脊线到 `bh` 的曲线跟随单独判弯曲。
- 纳入 S4 OFF/ON × C OFF/ON 四格，并把物理、C 渲染、联合观感设为互不替代的三道出口。S4 sweep 的亚像素振幅变化必须与测量重复误差比较，不因有变化就宣布可辨。
- 将冻结证据逐项标出当前状态。目标 JSON 与 sweep 可作为输入，但同口径参考复算、厚沙 C OFF 当前基线、独立弯曲/固定屏幕 oracle 和真机采样排期仍待执行。

## 不选路线与边界

- C2 历史代理栈需要无法由当前库存差唯一辨认的沉积批次/输运历史，本轮不扩展物理或渲染账本。
- C3 2–4宽层难以代表候选细层数量，也易与 E2 整体明暗塑形混淆；只可作为辅助消融，不宣称完整 C。
- E2 顶光属于整体受光/背光，D 属流束与前缘纹理，C 只负责沉积层理；三者分开消融、分开报告。

## 后续

manager 验收口径 → tester/effector 同口径参考复核与 C OFF 基线、tester 独立 oracle/方法预注册 → designer 提正式数值 revision → manager 冻结 ROI/mask/methodId/阈值 → architect/developer 实现 → tester/effector 进行冻结后四格与观感验证。缺任何冻结前证据时维持 pending；不在本稿冻结候选数字或宣称效果已验证。

