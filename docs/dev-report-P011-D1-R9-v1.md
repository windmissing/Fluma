# P-011 D1 B 段 R9 收口交付记录

日期：2026-10-24。原因：用户转交“adapter桩+零分配+旧退款守卫，R9契约已到位”卡；依据 architecture §13.2 R9、manager `decision-P011-dual-end-refund-adapter-v1.md`。**R1/R4 已实现自验；R3 完成部分缓冲收口及待裁提案，严格零分配尚未通过；D1 不关闭。**

## 修改文件及理由

- `P011ParticleRefundAdapter.ets`：R9 精确 Receipt/Adapter 接口；预分配 sidecar，保存票据各端同额真实扣款与累计退款；预检全部端点及 total，两个端点各退 amount、total 循环外仅退一次，bottom 不读写；超额/NaN/过期/重复关闭/未扣足/坏 out buffer 均在写前拒绝。REFUNDED 的 netDeltaValid=false，CLOSED 的净差按既有1e-9审计；out 固定2槽、caller-owned复用。
- `P011ParticlePrototype.ets`：桩 budget 的 refund/close 统一映射 adapter；reserve 数学不改，新增 sidecar 注册；外层失败恢复 sidecar 及收据缓冲，避免把回滚票据的旧收据误发为成功。无 WaterSandField 接线。
- `P011ParticleField.ets`：新增 `warmUpTransactions/copyStateTo`；prepare 复用完整 scratch 池和 before/after TypedArray 状态缓冲，保留旧 snapshot 独立诊断副本语义。只做缓冲复用，不改六事务物理判据。
- `run_p011_particle_field_units.cjs`：只按 manager 唯一授权修改旧 pair 退款对照口径：真实 reserve 网格逐值相等仍为硬断言；adapter 全额退款另验；旧平分差值仍逐值断言并输出 negativeControls，不删旧函数或掩盖差异。
- `run_p011_particle_refund_units.cjs`：11例，覆盖R9双端WAIT、单端部分授予、分次/零退款、各种拒绝零副作用、sidecar代际/溢出、out别名、adapter正常路径局部零分配、旧函数行为及源码hash守卫。
- `probe_p011_particle_allocations.cjs`：独立全链分配缺口探针，不能把局部 PASS 冒充整条热路径达标。

## 自验结果

| 通道 | PASS | FAIL |
|---|---:|---:|
| M1 A+B（含600s/36,000子步） | 33 | 0 |
| R9 adapter/旧退款守卫 | 11 | 0 |
| gesture/planning/S3/S4/input/advection | 275 | 0 |
| 普通单元合计 | 319 | 0 |

旧退款负对照固定示例：request=1，reserve=0.4，WAIT退0.4；adapter两端netUsed=0/0、total=0；旧函数两端netUsed=0.2/0.2、total=0。按manager裁定属于已知不兼容证据，不算失败。旧 `refundLocal/materialGuardRefund` 方法体规范化hash及平分/保守不重分配语义均被测试钉住；产品旧函数零改动。

adapter局部分配验证：10,000次跨子步正常注册/refund/close，TypedArray/Array/Map/Set构造及JSON.stringify trap无触发；其正常路径AST无对象/数组/闭包/new表达式（抛错路径除外）。此证据**只覆盖 adapter**。

R3全链探针：预热后100次prepare/publish，scratch和before/after缓冲引用保持，TypedArray构造数0；全M1静态审计仍列60处潜在分配站点，输出 **NOT_ZERO_ALLOCATION / exit=2**。此项仍未满足当前硬性交付条件，不能用319/0替代。后续方案见 [R3提案](proposal-P011-D1-R3-buffer-contract-v1.md)，A保持严格门槛、补热接口；B有界分配降级仅供manager裁定，未自行采用。

构建/类型验证：默认 HAP hvigor assembleHap 零错误成功；bundled TypeScript strict/noEmit通过。额外ohosTest依赖图覆盖新增模块，仍受既有测试目标175个错误阻挡，不宣称ohosTest HAP成功；本卡文件无新增ArkTS错误。详细日志 `.codex/local-test/d1-r9-{build.log,ohos-build.log,field.json,refund.json}` 和 `d1-r3-allocation.json`。

## 交接

退款接法争议已由R9解决；后续阻点是R3严格零分配接口/验收决策。未改冻结阈值、旧退款函数、注入λ/η、真实物理route、renderer或页面；不做真机操作。不因本次普通单元全绿标D1完成，不自行启动D2。
