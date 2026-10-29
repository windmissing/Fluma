# T250 续工：选对计数身份反例

2026-10-29，developer；不使用真机。接续 v4 报告，隔离生成 `T250-v9`，不改变生产默认路由。

修正 `M_FIND/M_FOUND/M_NO_PAIR/M_ERROR` 边界：`M_FIND` 在原 `selectPair` 的有效 `try` 内计数；`M_FOUND/M_NO_PAIR` 仅在原配对谓词与状态校验成功后计数；原谓词拒绝或选择器异常归 `M_ERROR`。此定义使逐调用身份 `M_FIND=M_FOUND+M_NO_PAIR+M_ERROR` 成立。新增非法 `PAIR` 反例，证明拒绝时 `FIND=1, ERROR=1, FOUND=0`，不是仅靠正常样例通过。

聚焦自测 **59/0**，见 `test/reports/t250/v9-focused.json`；独立 hvigor native/ArkTS/PackageHap **BUILD SUCCESSFUL**，见 `v9-package.log.gz`。独立 unsigned SHA `ab20cf35e63c53dec831f4d8c9d76d04c58d433755479ab3d717750b7e82d5ff`，T257 原授权流程签包 SHA `4e0087e99df8b55b9be2061f51c8e6aed11f5b2f008dba99216e71d719bf8449`，verify-app/profile 均 PASS，见 `test/reports/t257/T250-v9.json`。清单 `v9-manifest.json`、`v9-dictionary.json` 与该包对应。

**仍为部分交付，看评论。**20 原 cut 可控故障入口、逐 cut 的原始回执与冷归档、完整父子作用域/资源峰值等仍缺；当前 20 cut 显式 `NOT_IMPLEMENTED_NO_ORIGINAL_CUT_INJECTOR`，不能宣 T205 全合同或设备 GPU PASS。上一 v4 报告其余边界继续适用。
