# T479 · GPU M2 配对数值边界与来源聚合补证（v1）

- 日期：2026-11-11
- 角色：tester（离线卡，不使用真机）
- 结论：**PASS 7/7**。真实 GLSL 13-pass 生产路径，期望全部由 T446 oracle / 独立 Fraction 重算冻结后与 GPU completion/work-origins 原字节对照。
- 代码：`test/t479/numeric_host.cpp`（生产 shader 采集 host）、`test/t479/numeric_audit.py`（独立判读）、`test/t479/build_host.cmd`
- 证据：`test/reports/t479/t479-numeric-boundary.json` + `.codex/local-test/t479-c/`（105 件 completion/bank/pub/work 原件）
- 消费前置：T470 fault_host 基础设施（复用其 SourceFixture/RootGate/GL 链，OUT_DIR 独立 t479-c）。

## 判据与结果

| 项 | 独立重算期望 | GPU 实测 | 结果 |
| --- | --- | --- | --- |
| N1 emit 拆分 | emit 0.125Q=131072 → PACKAGE 122333 截断 + 余数粒 8739 → birth=2/次 | c[166]=2 ×2 | PASS |
| N1b pair 半径语义 | 输入 y=0 ≠ 粒子 y：同列重 emit merge=0（shader 出生 y 分散，\|yA−yB\|>radius 不配对） | c[165]=0 | PASS（语义发现） |
| N1c mutationSerial | 逐 root delta == merge+birth | c1 mut=2、c2 mut=4 | PASS |
| N2 满池 maxBirth | birth=192（K 粒全额）、stop=TARGET_COMPLETE、merge=0 | birth=192 stop=0 merge=0 rows=193 | PASS |
| N3 半径外 x+256 | 输入拒绝 c[64]=0x10002、merge=birth=0 | 0x10002 / 0 / 0 | PASS |
| N4 per-origin liveMass | 两 origin 各 U64=131072（Hi=0 双字，每 SOURCE emit 全额出生无损耗） | 命中 2 条 | PASS |
| N6 merge 后重选 | 满池后新 SOURCE 2 粒与既有粒重选 2 对：merge=2 birth=2 mutDelta=4 | 2/2/4 rows=5 | PASS |

## 关键语义发现（更正 T470-B 初版解读）

1. **emit 拆分语义**：单次 SOURCE emit >PACKAGE 时拆分为"PACKAGE 截断粒 + 余数粒"（122333+8739），birth=2。T470-B1 初版"min(emit,PACKAGE) 单粒"解读不完整——bank slot0/1 实测双粒。
2. **输入 y ≠ 粒子 y**：SOURCE command 的 y 只是列锚点，粒子 y 由 shader 出生逻辑决定；同列同参数重 emit 不构成 tie pair（merge=0）。T465 禁止的"只查全相同质量/列坐标推 RNE"正是此场景——本卡以 GPU 原字节证伪了"重 emit 必 tie"假设。
3. **origin key 含列坐标**：不同列 SOURCE 产生不同 origin 记录，liveMass 按列分记（131072×2 而非聚合 262144）。同 origin 聚合场景需要同列多 SOURCE 输入通道，fixture 未构造，如实登记缺口。
4. completion 摘要 stop（c[167]=6）与详细 stop（c[64]=0x10002）双字段并存，判读以详细域为准。

## 缺口如实

- tie pair RNE 非零分量（zA≠zB）、nearest 非最小 b、radius±1 边界逐字对照：需受控粒子 y 的注入通道（fixture 无直接写粒子 y 的 GL 通路），NOT_RUN 登记待设施。
- 极限乘积 RNE（|num|<2^48）由 T446 oracle M204d 数学证已完成；GPU 侧需上述同通道。
- 真机全项 NOT_RUN（本卡离线）。
