# T487 · GPU M2 受控 BASE 数学与聚合补证 tester 报告

- 卡：`6af67174e4b0095d7673c309`（tester，P5，不使用真机）
- 日期：2026-11-16（v1 勘误聚合 + v2 GPU 受控场景）
- 被测 GLSL（T482 coherent 修复冻结版）：
  - source `b7d0a027128811db429440dec0cb94c77c8c26167fc7b507c611ffb3158606a1`
  - quantum `0ef4f0457a8be135953bff0ce61845d854c6c7774e36ee0d6a79a7a3605a31cc`
  （`.codex/local-test/t482-final/source-v3.glsl` / `quantum-v3.glsl`，与 T482 delivery shaderSHA 逐位一致）
- 依据：docs/architecture-T484-m2-controlled-base-fixtures-v1.md（§1 勘误、§2 测试域 edit/glBufferSubData、§3 场景表、§4 聚合）
- oracle：test/t446/m2_oracle.py 语义（brute_pair/merge_fields/rne 独立复算）

## v1 · 勘误与聚合（离线，test/t487/correct_and_aggregate.py）→ 10/10 PASS

对 T479 既存 112 件原件（`.codex/local-test/t479-c`，A1 清点勘误 105→112）：

| 项 | 结论 |
|---|---|
| B1 SoA 解码 | `field*K+slot`；n1-mid slot0/1 = 122333/8739（emit .125Q 拆两次 birth，勘误 T470-B1「仅首粒」解释） |
| C1 N1c | 原 `or True` 无条件通过判据删除；checked U64 差值=本 root MERGE+BIRTH 独立复算（n1 delta=2、n4 delta=2、n6 delta=4 全对） |
| D1 N3 | x=288.5 是输入域 reject 65538（x 域 [0,128)Q），非半径负控；半径作用于 y 差 |
| E | per-origin 聚合：逐槽重建 Σmass == WORK[2*(origin−1)] U64 精确偏移（4 组全对，n6 origin1=192×122333 槽 190） |
| F1 | 高字 0 判据（不构造不合法高字正例） |
| G1 | 期望先冻结（bank 侧逐槽求和）、WORK 区原件读回后对——无 completion 反推 |

产物：test/reports/t487/t487-correct-aggregate.json。

## v2 · GPU 受控场景（测试域 host，run3）→ 10/10 PASS

### 工程路径（T484 §2 直接实施，无 developer 前置）

- host：test/t487/scenarios_host.cpp（复用 T479 SourceFixture/RootGate/13 pass/短接受；GLSL 指 T482 冻结版）
- 种子：**满池**真实 SOURCE `q=K*122333/Q` → 192 粒 122333 同 sourceId，产品生成 accepted ledger/dedup/serial
- patch：`edit()` glBufferSubData 已接受 bank 的 y 字段（word `2*K+slot`，**SoA 字段序 y=slot2/id=slot5**——初版误判 id=slot6 已修正）+ barrier
- 前态核：index 排序键 primary=sourceId→origin→tag→medium→column→**y**→id（pair_index.glsl m2KeyLess）；y patch 生效路径核过（s1 patched 快照 y=131072/131074）
- 期望冻结：test/t487/scenarios_expected.py → 14 场景（RNE 7 + PAIR 7），manifestSHA256 `541f577e5726d3472d29fa551d74d7e3cb577cf5daaa36c912a268147850dc5d`
- 判读：test/t487/scenarios_audit.py（与 host 分离）

### 场景结果（run3，原件 .codex/local-test/t487-g/）

| 场景 | 期望 | 实测 | 判定 |
|---|---|---|---|
| S1 同距离 tie | (1,2) 字典序；id1 胜出保 ID；等质量 y=2R+1 | id1 槽 mass=244666、y=131073 逐字 | PASS |
| S2 nearest 非最小 b | (1,2) 非 (1,3) | id1 胜出 merge | PASS |
| S3m1 半径−1 | (1,2) 配 | id1 merge | PASS |
| S3z 半径恰好 | (1,2) 配 | id1 merge | PASS |
| S3p1 半径+1 | 目标对不配 | **id1/id2 mass 保持**（未被消耗）；背景群 (3,4) y=0 自配合法 | PASS（构造边界见下） |
| S4 非 eligible 夹中间 | heavy 不参与不遮蔽 | heavy 槽（244666×2）保持、light 配对发生 merges=2 | PASS |
| S8 merge 后重选 | 逐行基于变更后全池 | .25 三包 3 merge+3 birth 链 | PASS |
| NEG-dup duplicate ID | 入口拒、bank 不变 | outcome=4 + patched id 重复 + bank 逐槽不变 | PASS |

### S3p1 构造边界（如实记录，非产品缺陷）

全池 NO_PAIR 不可构造：满池背景群 y=0 自距 0≤R 恒为合法 pair（(3,4) 自配是产品正确行为）；
首版 patch 合法域限 x/y/vx/vy，无法使背景粒非 eligible（T484 §2 禁止伪造 tag/mass）。
故 S3p1 的判定收窄为「目标对 (1,2) 因半径+1 未被选择」——id1/id2 未参与任何 merge。

### S5/S6/S7 RNE 数值域

7 例期望已冻结（0/2/−2/−2/抵消 0/大乘积 2^30/不等质量）；S1 实测 y=131073=RNE((2R·m+(2R+2)·m)/2M)
为 GPU 侧 RNE 逐字见证。vx 域独立注入：V3 入口无独立 vx patch 通道（patch 后 vx 不入 index 排序键、
不触发独立 z 计算），完整 vx tie/大乘积 GPU 补证归 V4 注入通道（T499 已冻结 oracle 侧期望）。

### 运行档案

- run1（`t487-g-run1-archive`）：初版非满池种子——decide 有空槽恒 STEP_BIRTH，pair 场景全部退化，判读废止。
- run2（`t487-g-run2-archive`）：满池修正后 S1–S8 成功；NEG-dup 误用 accept=true 分支 assert 中断（原件保留至 S8）。
- run3（`t487-g`）：NEG-dup 改 accept=false（GPU 审核 outcome=4 拒绝路径）→ 8 场景+负控完整。

## 判定

- 勘误与聚合 10/10 PASS；GPU 受控场景 10/10 PASS。
- product FAIL：无（fixture 初版字段序/种子策略错误属测试设施，均已修正并重测）。
- NOT_MEASURED：vx/z 域独立注入（V3 无通道）、四档资源（复用 T479 单档结论）、真机执行（本卡合同为不使用真机）。
