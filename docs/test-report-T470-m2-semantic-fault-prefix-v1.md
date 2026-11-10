# T470 · GPU M2 独立语义与故障前缀补证（v1：A/B/C 三包 PASS，缺口如实）

2026-11-10，tester。不使用真机、不改产品。按 T465 §3 A/B/C 执行；B/C 用生产 GLSL/RootGate 实际路径（复用 T452 source_host 基础设施），判读独立（不 import 生产 assert/开发 self-replay）。

## 交付

| 文件 | 内容 |
|---|---|
| `test/t470/raw_reaudit.py` | A 包：60 raw 修正判读（4 判据） |
| `test/t470/fault_host.cpp` + `build_fault.cmd` | B/C 包：生产 GLSL 实际路径执行 + 原字节快照（bank/pub/work-control/work-origins）+ command 落盘 |
| `test/t470/c_audit.py` / `b_audit.py` | C/B 独立判读 |
| `test/reports/t470/*.json` | 三包判定原件 |

## A 包（既存 60 raw 再判读）：4/4 PASS

- **A1 数量/outcome 修正**：60 raw = SOURCE接受32 + SOURCE业务拒绝16 + SOURCE重放8 + QUANTUM接受4（T465 §1 表）；无 fatal 23–26 原档如实记缺。
- **A2 mutationSerial U64**（h142|h143<<32，修正 T453 误用 h141 行数）：逐接受 root 的 **serial delta == 本 root MERGE+BIRTH 事件行计数**（32/32）；fixture 边界（BASE=0 起步，serial==m+b）3 例单列（raw-16/31/46）；拒绝/重放 root serial 不变。
- **A3 nextId 链**（行字段 28/29）：非 END 行全链连续 5656/5656；END 行（kind18）不携带 id 链（24 处断链全为 END 行，wire 语义一致）。
- **A4 投影**：c[160:173]==h[132:145] 全 60。

## B 包（生产 GLSL 数学负控独立重算）：3/3 PASS

生产 GLSL 13-pass 实际路径（非 CPU 仿真），原件含 command + BASE/after bank 快照：

- **B1 出生质量**：emit 0.125Q(131072) → 单粒出生 **min(emit,PACKAGE)=122333**（质量资格截断独立重算一致）。
- **B2 满池 BIRTH**：maxBirth → 出生 K=192 粒 each 122333，无 merge（122333×194 含 header 投影，244666×0）。
- **B3 满池 merge 链**：满池后 x+2（同列半径内）→ **191 对 merge，M=122333+122333=244666==MERGED_MAX**（T446 merge_fields 独立重算一致），末粒 122333 保持——RNE z 同列 tie 语义验证。

## C 包（故障前缀与五缺口）：8/8 PASS

故障 completion 原件首次落盘（T453 缺 fatal 原档的补采）：

- **C1 fatal 23/24/25/26**：corrupt 注入 → outcome=4、reason 码正确、**bank 前后原字节不变**（整 root 拒绝、成功前缀保留）、**WORK ctrl 段 firstReason 独立读回 == completion reason**（ctrl[2]==25，等价可检验原字节证据）。
- **C2 PAYLOAD_CONFLICT**：outcome=2、reason=8（T452 精确同序 w/j 后）。
- **C3 generation 溢出**：reason=21、state 全量不变。
- **C4 SOURCE成功→Q poison 拒绝**：账保留（bank 前后一致）。

## 缺口登记（NOT_IMPLEMENTED，设施缺失非产品缺陷）

1. per-origin liveMass 聚合审计：originU64 段 raw 未含，需 B 包执行时读回（work-origins 已可读，聚合判定待续）。
2. tie pair / nearest≠最小b / radius±1 / 负数 RNE 数学负控：单粒场景未覆盖 merge（BASE 空池单 SOURCE 无 pair 对象），需两 SOURCE 精确编排后续执行。
3. 真机（设备）全项 NOT_RUN。

复跑：`python -X utf8 test/t470/raw_reaudit.py && python -X utf8 test/t470/c_audit.py && python -X utf8 test/t470/b_audit.py`（前置 fault 原件已存）。
