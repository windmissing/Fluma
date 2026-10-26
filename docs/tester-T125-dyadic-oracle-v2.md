# T125：dyadic 独立测试 V2（真实位型、有效反例与基础模块验证）v1

日期：2026-10-26。角色：tester。任务卡 `6adf424be4b0127832c267fb`。合同：T120 §1/§3 + T104 §2/§7。执行器：`test/p125_dyadic_oracle_v2.cjs`（测试端独立 BigInt oracle；不镜像开发自测）。raw：`test/reports/t125/p125_results.json` + `p125_dyadic_fixtures_v2.json`。目标模块：T112 db1be20 `P011ExactDyadic.ets` / `P011StateCertificate.ets`。**14/14 PASS**。不使用真机；不改 H/阈值/旧 raw。

## 1. 真实位型与 ULP 恒等（T120 §1 修订落地）

| case | 内容 | 结果 |
|---|---|---|
| V2-01 | H=`3fe279a74590331c`、nextUp=`…331d`(+1ULP)、nextDown=`…331b`(−1ULP)、T86/T100 真实 raw=`…3320`(**+4ULP**)、T111 合成=`…3321`(**+5ULP**)，独立 DataView 解码全对；H 处 ULP=2^-53 实测 | PASS |
| V2-02 | 机械 nextUp/nextDown(H) 位级复现 331d/331b（非合成 H+5e-16） | PASS |
| V2-05 | 真实 T86/T100 边 (+4ULP raw) 在精确存储值比较、hardMax=tan30(331c) 下被拒绝——**4ULP 只是历史越界的描述计数，从不是允许域** | PASS |
| V2-14 | V1 冻结夹具保留：夹具 nextUp hex 本就是真 331d（错标 320 只在报告文字）；合成 3321 与真实 raw 3320 区分保留；V1 文件零改动 | PASS |

## 2. 有效反例（T120 §3：区分精确谓词 vs 先 RN 再减流水线）

| case | 构造 | 精确判定 | RN-first 判定 | 结果 |
|---|---|---|---|---|
| V2-03 | Sright=1, Sleft=2^-54, u=0, H=nextDown(1) | 精确残差 1−2^-54 > H → REJECT | RN(1−2^-54)=1，超出量被吞 | PASS（双断言：resLEQ=false ∧ RN 值=1） |
| V2-04 | Sright=1, Sleft=2^-54, u=2^-54, H=nextDown(1) | 精确三项 =1−2^-53=H → **ACCEPT** | 先 RN 再减两次舍回 1 → **REJECT** | PASS（两谓词结论相反，可击穿 RN 流水线） |
| V2-06 | certifier 全边链：边斜率恰 ±H 合法；一条边 +H+1ULP(2^-52) → STRICT_EDGE | —— | —— | PASS |

## 3. 基础模块对独立 oracle 的验证（非开发 17/17 作证据）

| case | 内容 | 结果 |
|---|---|---|
| V2-07 | 乘积物化：1×H=H、H×2=2H 精确相等；0.1×0.1 vs 0.01 由独立精确算术定号（+1），生产一致（OR-03 闭环：乘积对齐单位后与第三操作数比较，不再"只算不用"） | PASS |
| V2-08 | 容量边：max×max 容量内（正号）、subnormal×subnormal 恰在 −2148 最低指数精确、8 项最坏求和 OK（OR-07 闭环：最高位/carry 结构实测） | PASS |
| V2-09 | 非有限在 sum/product/rate 三路全部 NON_FINITE（无符号位偷渡） | PASS |
| V2-10 | 速率门 7 点矩阵：边界（|diff|=ω·dt → 0）、内部（−1）、+1ULP（+1）、dt=0（0/1）、ω=0（1）、±0 目标（0），全部与独立 oracle 逐符号一致 | PASS |
| V2-11 | 四项和内 ±0 规范化：(−0,0) 与 (0,−0) 均 sign 0 | PASS |
| V2-12 | 白名单：unsupportedExpression() 真实调用返回 ARITHMETIC_EXPRESSION_UNSUPPORTED（typed，非硬编码 true；OR-09 闭环） | PASS |
| V2-13 | 模块身份：两文件 fs 存在 ∧ 加载实例化成功（T120 §3 RED 纪律：fs 缺失与加载失败分类） | PASS |

## 4. 测试工程自身修正（非判据变更）

1. **共享 result 对象快照**：`P011ExactDyadic`/`P011StateCertificate` 每次 compare 返回同一可变 result/certificate 对象；执行器首版保存引用导致后调用覆盖前值（V2-06/07/08/10 假 FAIL）。修正为调用后立即快照 `{code,sign}`/`{legal,reason,firstEdge}`。**这是测试侧断言方式问题，不是生产数值缺陷**；但已记录为集成注意点：生产消费方若持有跨调用的 result 引用将出现同类覆盖。
2. **rate oracle 单位公式修正**：首版误将 diff 再乘 2^2148 与 rate 比较；修正为 diff（2^-2148 单位整数）直接与 rateUnits=sigω·sigdt·2^(eω+edt+2148) 比较。
3. **V2-06 构造修正**：基数 24 的 ulp 太粗无法精确存储边差 H（24+H 舍入为 +4ULP 斜率，恰好复现 T86 缺陷——反向印证）；改用基数 1 binade（ulp=2^-52），独立验证 U(1+H)−U(1)=U(H) 后构造。
4. V2-10 首版 oracle 复用 `U(o)*U(dt)` 单位错位，改为按各自 exponent/sig 展开对齐（V2-07 同）。

## 5. 边界声明

- 期望全部由本文件独立 BigInt oracle 派生，未复制 `run_t112_exact_dyadic_units.cjs` 断言。
- V1 的 11 例与 raw/fixture 保留（`test/reports/t111/`），勘误按 T120 §1 记录于本报告 §1。
- 成功仅覆盖基础模块（阶段A）；writer/证书工程/生产 route 未启动（T104 §6/§7 前置不变）。
- 无 ±4ULP 容忍、无 epsilon、无判据放宽。
