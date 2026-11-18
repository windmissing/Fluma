# T578 v1 · 目标12 四档资源与证据包络独立复算

日期：2026-11-18，tester，不使用真机/GPU（复算域，T567 工程合同 + manifest + dispatch
JSON 为被检对象）。
依据：architecture-T567-water-sfp-engineering-v1.md §3/§6 + design-G12-water-sfp-manifest-v1.json
+ architecture-T567-g12-dispatch-v1.json。
证据：`test/t578/resource_recompute.py` + 本报告。全部数值独立从公式生成，manifest 参考表
仅作对照。

## 复算结果总表（四档 K/Kw）

| 项 | 公式（独立重建） | 结果 |
|---|---|---|
| state 单份 | 父 stateBytes(591872/673792) + 401664 + 128Kw | 四档全 ✓（1001728/1009920/1083648/1091840） |
| WORK 总 | **父 G9-DM v2 workBytes（含 DM 新增 24576B）** + 675840 + 128K + 256Kw | 四档全 ✓（2028160/2044544/2071680/2088064） |
| pathRows | 32·8K（沙） + 32·16Kw（J） | 两参考档 ✓（81920/196608），全四档自洽 |
| arena 单份 | eventRows·256 + pathRows·64 | 两参考档精确差 0 ✓；**另两档 eventRows 无公开公式**（K512/64 反推 eventRows=133590.625 非整）→该项 PARTIAL |
| resident | 4·arena + 5·state + WORK | 四档全 ✓（122134400/138968960/185829760/202664320） |
| ROOT 冷单槽 | 2·state + 2·arena + 65536 + 4096 | 四档全 ✓ |
| FULL_CORE | resident + 65536 + 4096 | 四档全 ✓ |
| FULL_TRIALS | FULL_CORE + 6·arena | 四档全 ✓（294850432/336850816/453409152/495409536） |
| 极值 608171136 | resident(512/128) + 2·FULL_CORE + 22528 输入快照 + 16384 页 | ✓ 精确 |
| 极值 1486197888 | resident + 2·FULL_TRIALS + **6·arena(512/128)** + 快照 + 页 | ✓ 精确（manifest 未写明的 6·arena 项被独立反推确认） |
| wetQuantum 6204 | 2·(63·32+6·8) + 2·(32·32+8) + 12 | ✓ 精确 |
| G1+F4 | 16+4·6204=24832 / 16+4·2404=9632 | ✓ |
| SOURCE16 | 新 SOURCE 是 16（非父 12） | ✓ 口径确认 |
| 几何检查量 | 7·[31N(K+1)+K(K−1)/2] | K512=15164800 ✓ 精确（K192=5489120 manifest 未列，同式生成） |

## 两个复算发现（回 architect/manifest 维护者）

1. **eventRows 公式未在 manifest 展开**：仅两参考档（192/64、512/128）给出 eventRows
   数值，K192/128 与 K512/64 两档的 eventRows 无法从公开公式推出（arena 值本身
   自洽于 arena=ev·256+path·64，但 eventRows 中间项——SOURCE 64 行/根行/七 HYDRO
   合并摘要/父事件域——的组合式缺失）。四档 arena 值因此判 **PARTIAL_VERIFIED**：
   两档精确、两档依赖表值。建议 manifest 补 eventRows 闭式或逐项分解。
2. **WORK 基线口径**：WORK 总 = 父 G9-DM v2 workBytes（T529 1286784/1289344 + DM
   24576）+ 新增 675840+128K+256Kw——manifest 只写"675840+128K+256Kw"新增式，基线
   未显式标注；复算确认基线=父含 DM 值后四档全过。建议 manifest 显式补基线说明。

## dispatch 口径确认

- SOURCE16（G12 新 SOURCE 是 16，不把父 12 少算 4）✓
- wet Q6204 / dry Q2404（父已证）/ G1+F4 wet 24832、dry 9632 ✓
- 全 pass 单 64-lane 组、空 lane 照 barrier——表口径与 dispatch JSON passCount 一致 ✓

## NOT_RUN / 边界

- 全部为静态复算；无 GL、无真机、无时间积分。设备实际可分配性、250ms 期限、
  driver/JIT/FBO/对齐/host 固定对象仍 UNMEASURED（manifest excluded 域口径保持）。
- 本复算不构成"设备可运行"或资源充足性结论——仅公式级独立验证。
