# T123 粒子合并精确索引开发回执

日期：2026-10-26。developer。原卡 `6adf424be4b017ab34051fa3`。不使用真机。

## 实现

新 `P011MergeExactIndex` 冷分配并复用两份 N 槽索引、slot→排序位置和范围最小 ID 树。每次 merge 从当前 `patchScratch` 重建；完整 group 键为 `(phase,sourceKind,sourceId,state,floorX)`，组内按 Y、ID、slot 排序，另一索引按全局 ID 排序。遍历 a 时先删其树叶，Y 两侧边界以原 `Math.abs(y_a-y_b)<=radius` 谓词二分，再查区间最小 ID。找到 pair 后仅调用原 `mergePairHot`，质量、质心、ID、free-list、journal、lineage、λ 与 rollback 算术不变。每次 merge 为 O(N log N) 时间、O(N) 定容内存；不宣称整步同界。

`POOL_PROGRESS_DIAGNOSTIC_V2` 明示工作区累计计数，新增 a 选择、排序比较、树访问、范围查询、索引字节、seed collect/apply、preview/authoritative 次数、最近 origin/λ 尝试及 shake 真能量。历史 `mergeCandidateChecks` 仍表示旧 b 全扫次数，新算法中为 0，不伪称包含 a/sort/tree。页面已有 gate→clock→request→inject 与 captureFinished 跳过 trace，保持原有界行数。

## 自测与边界

- `test/run_t123_merge_index_units.cjs`：155 组对独立旧搜索逐 pair 差分 PASS，含 N4/16/64/192/512、无匹配、稀疏 active、两 phase/source/state/sourceId、最大合法 ID、等高、半径 0/0.25/2、连续质心变化；比较与树访问上界断言通过。
- 既有 T96 热路径 5/5、Hot 11/11、M2 13/13 均 PASS。兼容满池 merge 与无匹配 λ 重试保持预期结果。
- hvigor `BuildNativeWithNinja`、`CompileArkTS`、`PackageHap` 成功；完整 `assembleHap` 仍仅被原 `build-profile.json5` 的 SignHap 密码长度配置阻断，未改签名。日志 `test/reports/t123_build_wip.txt`。

真实卡死池的 N/birth/seed/λ/耗时与设备首因尚待独立设备取证；本卡离线结果不声称解决设备冻结，也不授权 GPU 改写。新 XComponent+shader 渲染边界未变，无 Canvas 上屏或回退。
