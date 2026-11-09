# T415 · 单沙 hold 与时钟独立离线验证

2026-11-09，tester。不使用真机、不改产品。按 T413 §4/§5 合同建立合成正反控与旧资产复核。

## 1. 交付物

| 文件 | 用途 |
|---|---|
| `test/t415/hold_oracle.py` | A 状态机 oracle + B 截图时间门 oracle + C T408 旧资产复核；`--selftest` / `--audit-t408` / `--demo-clockmap` |
| `test/reports/t415/t408_audit.json` | 四轮失败分类复核（只读，不补写不挽救） |
| `test/reports/t415/t415_verdicts.json` | 结论+证据 SHA |

## 2. A 状态机 oracle（T413 §4.1，19/19 PASS）

`IDLE→RESERVED→HOLD_OPEN→SEALED→RELEASED`（INVALID/EXPIRED 分支）。覆盖：全生命周期正例；OPEN 中新预约 `HOLD_ACTIVE` 零副作用（sequence/request/state 不变）；旧 ack 三重拒绝（attempt/request 不匹配、空 SHA）；到期只 `EXPIRED` 不造成功 seal 且登记首因；重复取消保留首因；geometry 变化→INVALID；未归档 SEALED 不能 release（`ARCHIVE_NOT_SAVED`）；无效 ROI 预约零副作用。

## 3. B 截图时间门 oracle（T413 §4.3，8/8 PASS）

仿射映射 `d=a*h+b` 由 ≥3 组独立 `(h−,h+,d)` 样本约束正斜率；对每组 `a·h−+b≤d≤a·h++b`；对全体可行 (a,b) 求完整 capture 区间的最小下界 L 与最大上界 U（不取最有利单映射）。通过条件 `holdStart ≤ L ≤ U ≤ sealedEnd`。负控全过：先 seal 后抓屏 / 开始在 hold 前 / 结束在封存后 / 中心在内但包络越界（L≈952<1000 实测检出）→ `CAPTURE_INTERVAL_OUTSIDE_HOLD`；<3 样本与回拨证据 → `CLOCK_MAPPING_UNKNOWN`。不增容差消除失败。

## 4. C T408 旧资产失败分类复核（只读）

四轮全链实测拒绝顺序：**r1 无收据 → CAPTURE_TOOL_RECEIPT；r2/r3/r4 fileSha256 空 → CAPTURE_TOOL_RECEIPT**（screen.py 在时间门之前拒绝）；即使补齐收据，无归档时钟同步对 → **CLOCK_MAPPING_UNKNOWN**；再映射越界 → **CAPTURE_INTERval_OUTSIDE_HOLD**。r2 hold 跨度 87104.65ms、r4 228954.438ms 与 T413 §2 逐位一致。结论维持：T408"未运行 calibrate/未获 hold 覆盖"不因延长 hold、补写收据或倒算映射而改变。键盘展开遮挡（三张 PNG 无标定组件可见性）独立于以上任何一条。

## 5. 边界

- T414 接口未交付 → 对真机 capture_hold.py 的消费按**预期 RED** 登记，待 developer 交付后转可执行
- 本 oracle 为合成层独立实现（对照 T413 合同文本），不是产品代码；真机 T408 续测消费本工具与 T414 接口
- 无阻塞
