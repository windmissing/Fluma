# T128：精确索引独立差分（无匹配上界、连续 merge 与事务等价）v1

日期：2026-10-26。角色：tester。任务卡 `6adf424be4b0127832c26800`。被验对象：T123 `469ee37`（P011MergeExactIndex）。oracle：**旧 52a0700 `mergeOldestHot` 真实扫描**（T116 §5 指定，逐谓词复刻：ID 升序选 a → 全池 b 扫描带 group 五元组+floorX+|ΔY|≤radius+最小 b ID → afterId 推进）。执行器：`test/p128_merge_index_differential.cjs`（**6/6 PASS，287 个差分样本**；独立随机流 `0x51fab123^0xabcdef`，与开发 155 组自测不共享用例）。raw：`test/reports/t128/p128_results.json`。不使用真机；不改判据。

## 1. 差分矩阵

| case | 覆盖 | 结果 |
|---|---|---|
| D1 随机差分 | N∈{4,8,16,32,64,192,512}×40 池：稀疏 active（20% 空）、两 phase、两 sourceKind、4 sourceId、radius∈{0,1e-9,0.25,2}、ID 含 4294967295 邻域；**逐 pair 一致（a 与 b 的 slot、不止 found 布尔）** | PASS |
| D2 无匹配上界 | 对抗池：同 group、Y 间隔 10>radius、ID 升序 → legacy Θ(N²)=36,864 候选检查 vs 索引 sortComparisons+treeVisits+rangeQueries+aSelections ≤ 64N=12,288——两者都答"无匹配"，**每 merge O(N log N) 上界实测成立** | PASS |
| D3 连续 merge | 6 步链：每步合并后幸存者 Y=加权质心漂移、被并 slot 清除；**每步重建后与 legacy 一致**（无冻结索引/陈旧 group 缓存） | PASS |
| D4 rollback/restore | 移除 b 后 find 一致、恢复后 find 一致——重建语义覆盖 rollback/恢复/λ 重试形状 | PASS |
| D5 非法输入 | 重复 pId → invalid=true 拒绝；NaN 坐标拒绝；负 radius 拒绝——不产静默错对 | PASS |
| D6 group 键完整性 | 同 X/Y 半径 5 下 state 不同 / sourceId 不同 → 分属不同 group 不合并（a=ID1/b=ID3 模式，**非 ID1/ID2 易配对例**） | PASS |

## 2. 判定与边界

- **等价结论**：T123 索引与旧 52a0700 扫描在全部 287 个差分样本上逐 pair 一致，无匹配/单配/多候选最小 b 语义完全保持；每 merge 操作计数在 O(N log N) 上界内（D2 实测 ≤64N）。
- **未证范围（如实声明）**：
  1. 上界是**每次 merge**，不是整步——B 次 merge 仍 O(B·N log N)，含 λ/seed/preview 放大；设备端是否够快仍需 T129 设备取证（host 秒数不外推）。
  2. `mergeCandidateChecks` 在新算法恒 0（它只计旧 b 全扫），诊断语义已由 T123 文档声明，本卡确认不据此称"包含 a/sort/tree 开销"。
  3. journal/lineage/预算/事件流在差分中由 mergePairHot 原函数保证（本卡 mock 合并仅 D3 链的质心/质量/状态语义，完整事务审计属实现侧原路径）。
- 开发自测 155 组与本卡 287 样本互不替代；两者均为离线证据，设备首因另卡。
