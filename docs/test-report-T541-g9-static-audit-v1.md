# T541 · 目标9 GPU 三态产品独立差分 v1（静态同源审计 + 差分骨架）

日期：2026-11-17，tester，不使用真机。
被测：T529 交付（签包 `9290f550ee92a617101bbfa826537d0b03ec7e868c08055ef8df72838f5e8e82`，与卡面逐字一致）。
estimand：T515/T524；独立基线：T520 oracle（45 项 G9xx 语义锚点）。

## 本轮已证（静态同源审计）

1. **同源链成立**：unsigned hap 内 libgrsm1.so = 签包 so = `9ff4844a…`（逐位）。
2. **g9spf 49 文件源 SHA 逐个核**（development-compile.json sourceSha256）：
   `checked=49, mismatch=0`——交付工程 g9spf 目录与 developer 声明逐位一致。
3. **packagedLibraries 与签包一致**（libc++/grsm0/grsm1/p013hello/renderer 五 so 全对）。
4. **研究参数域边界核**（T515 estimand vs 实现 Config.Valid()）：
   g=16Q20 ∈[120,2^30] ✓、muStart=32768≤262144 ✓、muStop=16384≤muStart ✓、
   band=Q20≤CQ ✓、entrainRate=4Q16=262144 **恰好==上限（边界值 ≤ 合法）** ✓、
   shear=Q16=65536 ✓、settleTicks=384>0 ✓、revision=1 ✓——T520 基线的参数域全部落在
   实现支持域内（T520 oracle 可直接作为 GPU 差分的期望来源）。
5. **frozen-source.json 是 T421 fork 基线**（0228a7d），g9spf 为 fork 后新增——其 SHA
   以 development-compile.json 为准（已核 49/49）。

## GPU 差分主体：NOT_RUN（工程量边界，如实声明）

卡面要求的完整差分域（S/F/P writer、局部/全局动量能量、来源/互斥/容量、输入去重、
J 资格/首成功/拒绝前缀/replay、typed completion、两 K 档、ROOT/FULL 冷档、
EVENT/PATH、资源界）需要 tester 自建 GPU harness——T529 developer 只交付编译，
无 harness（对比 T522 交付了 11 个 .cmd/.cpp harness）。

实现规模：pipeline.comp 1927 行主 compute（2024 pass QUANTUM + 72 pass SOURCE）、
wide-integer 四 limb U128/S128、五 bank、G9_SPF_COLD_V1 双 slot——完整差分 harness
是数天级工程（等价重建 T523 模式的 11 个 harness）。

**本轮不启动半成品 harness**（避免以编译冒 PASS）；骨架与锚点已就绪：
- 期望来源：T520 oracle 45 项（capture p+=m·vx、dShear 6.25Q、Hamilton 键序、
  proximal 摩擦 −5→FLOW、负功不抵扣、+0/−0 dedup）——参数域已验证落在实现支持域。
- 被测对象：签包 9290f550 + frozen g9spf 49 文件（SHA 已核）。
- 待建：pipeline.comp 分段 dispatch harness（SOURCE72→QUANTUM2024→completion→
  cold capture/reader），预计需独立工作轮。

## 前置与建议

- T529 GPU 差分 harness 工程量大，建议 manager 裁决：①tester 独立建 harness（数天）；
  或 ②developer 交付 harness（同 T522 模式）后 tester 复跑（T523 模式，效率高）。
- 软件后端保护拒与真实设备性能分列（卡面）——设备域归 T542。

## NOT_RUN 清单

全部卡面差分域（除本报告静态审计外）NOT_RUN——非 SKIP，工程量边界如实声明，
不冒 PASS。
