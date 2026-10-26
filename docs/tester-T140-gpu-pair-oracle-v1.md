# T140：GPU 配对独立 oracle 先行（稳定 merge、分段查询与逐 pair 基线）v1

日期：2026-10-26。角色：tester。任务卡 `6adf6a09e4b06ee35747d303`（GPU 优先级最高）。合同：T137 GP-01..GP-12（算法 `GPU_SEGMENTED_ID_PAIR_V1`，CPU 边界 `CPU_RN64_MERGE_RANGE_V1`）。执行器：`test/p140_gpu_pair_oracle.cjs`（**14/14 PASS**；测试端独立 CPU 模型实现，非 GL；与 designer 离散模型器 `design-T137-gpu-pair-model-v1.py` 及未来 T141 shader 代码互不共用比较/分解逻辑）。raw：`test/reports/t140/p140_results.json` + `p140_gpu_pair_fixtures.json` + `p140_manifest.json`（真实 SHA-256）。不使用真机；RED 语义见 §4。

## 1. 12 组合同离线覆盖（GP-01..GP-11；GP-10/GP-12 冻结供 T142）

| ID | 覆盖 | 结果 |
|---|---|---|
| GP-01 | 每 a 的 [L,R) 与 legacy 合法 Y 集合完全一致（30 随机池×全组五字段 phase/sourceKind/sourceId/state/floor(x)） | PASS |
| GP-02 | F64 谓词边：**次正规反例**（a=.2,b=−5e-324,r=.2 → 原谓词真而 rounded y±r 边界会漏）、相邻 ULP 等距、有限相减溢出→P=false 保持单调；**不上传 F32 高度** | PASS |
| GP-03 | n=0/1 CPU_EMPTY_SHORTCUT；K=pow2ceil(Nmax)：192→256、512→512（不随 n 缩） | PASS |
| GP-04 | UINT32_MAX / 0x80000000 无符号序（无 signed 翻转、无 id+1 溢出）；跨 group 重复 ID CPU 预检拒绝 | PASS |
| GP-05 | **每层每段** = 该 Y 秩区间成员按 key `(invalidFlag,id,slot,originalYRank)` 排序；写地址唯一（无洞/无重复）；padding valid=0 且 originalYRank 唯一 | PASS |
| GP-06 | 分解 ≤2h+1 个互不相交对齐段覆盖 [L,R)；查询=每段最小合法大 ID=legacy | PASS |
| GP-07 | 全局最小 pair/NO_PAIR 与 legacy 全扫描 30 池一致（含无匹配） | PASS |
| GP-08 | 串行 merge 质心漂移：每次重建后仍与 legacy 一致——**陈旧范围批量配对合同禁止** | PASS |
| GP-10 | 错 epoch/迟到/context loss/timeout/map-fail → 丢弃保错误 status；失败 slot 完成前不复用；compute error 无权威写（设备侧用例归 T142） | PASS（合同冻结） |
| GP-11 | build 比较数 O(Kh²) 实测：K=256 h=8 bound 9216/actual 16640、K=512 h=9 bound 23040/actual 41984（模型每步 2 次比较 ×2 松弛）——无 O(N²) 替换 | PASS |
| FIX | 192/512 冻结夹具：legacy 与分段 oracle agree=true；manifest 含真实 SHA-256 | PASS |

## 2. oracle 开发中抓到的 4 个合同级要点（对 T141 的直接约束）

1. **分组相等 ≠ 全序相等**：group 边界必须用 `(group[0..3], floor(x))` 相等，不能用含 y/id 的排序键相等（首版 bug 使每 Y 值自成一组）。
2. **层结构 = 每 Y 秩段内 key 序**：层 l 段 s 覆盖 Y 秩 `[s·2^l,(s+1)·2^l)` 且段内按 key 排序；**全局不是单一 key 序**（若 T141 按全局 sort 实现 build，段-查询对应即破坏）。
3. **段尾越界读**：段内 "first (invalid OR id>idA)" 线性/二分查找越过段尾时读到相邻段记录 → 伪造 pair。T141 shader 必须硬性钳制段内边界。
4. **[L,R) 含 a 自身**（半开合同），id>idA 条件排除自身；idA=UINT32_MAX 时无 id>idA → NO_PAIR，无 id+1 溢出路径。

## 3. 供 T141 开发与 T142 设备验证的交付

- `p140_gpu_pair_fixtures.json`：192/512 冻结池（含池字节 SHA-256）、legacy/分段 oracle 双结果与一致标志。
- `p140_manifest.json`：fixtures 整体 SHA-256 + 合同范围声明（离线 GP 子集；GP-10/GP-12 设备侧归 T142）。
- oracle 的比较键/分解/建表代码与 designer 模型器独立，T141 shader 差分可直接引用其函数语义。

## 4. 边界

- 模型测试不是 GL；未实现 GPU 原型 → 原型工程 RED 由 T141 承担（本卡 oracle 已就绪，不误派业务缺陷）。
- GP-12（真设备 11/12 pass 链、128B 结果、原始时序）须 T142 真机卡执行。
- 不镜像 designer 4916 组作唯一证据；本 oracle 独立重建并发现上述 4 个合同级要点，已反哺 T137 实现约束。
- 无性能阈值、无采用结论（UNPROVEN 保持）；不改 T123、不宣布卡死已修。
