# T149 FIELD_COLUMN 原值诊断 V2 开发回执

日期：2026-10-27；角色 developer；不使用真机。原卡 `6adfe799e4b017ab340ad992`。

## 实现范围

- `P011ParticleHot.beginStepHot` 原逐列短路谓词、阈值、返回码不变。命中时清理前将该列八个原数组 IEEE754 bits（从原 buffer/byteOffset 读，NaN payload 与 -0 保真）、三次左结合和、完整 badMask、Hot/池身份写入独立首错槽。池的 13 个 canonical buffer 原 bytes、storageValid、live/free/nextId 同点锁存，避免旧 frozenPool 被误作本次 before。
- `P011FieldColumnEvidenceV2` 在诊断启用时冷分配两份 `8×128×8B` 场快照和 32 条固定 metadata 环。只读阶段观测标明候选/authoritative、writerStage、run/attempt/epoch/commit/source 身份；记录最后合法与首次非法，首次已非法明确为 `PREEXISTING_IN_WINDOW`。绑定改变、观测/捕获错误显式标状态，首错不覆盖。
- M2/trace 只透传锁存结果，`driftMeasured`、`rollbackExecuted` 和来源字段区分默认零与实测/真实回滚；旧 V1 字段保留。FNV32 digest 明示仅供相关性，不能作为完整状态 SHA。surface 绑定 sandTopScratch 的身份由场端补充。
- 关闭诊断时 Hot 循环只做空指针路径，窗口不分配；未增加物理 guard、audit、rollback 或派生更新调用，不修改 renderer。

## 离线结果

- `node test/run_t149_field_column_evidence_units.cjs`：13/13 PASS，包括八有限性、七非负性、左结合溢出、-0、NaN payload、[-EPS,0) 合同差异、同列多异常、首列顺序、清理后冻结、32 环溢出、窗口、分片重组与关闭开关无写。
- `node test/p118_t117_verify.cjs`：6/6 PASS；`node test/run_t85_tilt_trace_units.cjs`：7/7 PASS；`node test/run_p011_particle_hot_units.cjs`：11/11 PASS；`node test/run_p011_particle_m2_units.cjs`：13/13 PASS。
- hvigor `BuildNativeWithNinja`、`CompileArkTS`、`PackageHap` 成功；`assembleHap` 最后在现有 `SignHap` 配置失败：`build-profile.json5` 的 storePassword/keyPassword 少于 32 位。未改签名配置。完整日志 `test/reports/t149_build.txt`，无签名 HAP SHA256 `0764046fd18ab180cbf14b37a313ca7b09dc4cb9ea13c853125f85c1045a9bf6`。

构建源码 SHA256：Evidence `426e89788547b9749d03a8ce7b3823f6dc31c4d1f6e990985580a851e25c3f6f`；Hot `754657c84f6a1c81e322ef8bad00d56113205f5d2f05719f9b319a202cb4cac5`；M2 `d39af95badd3d34ec879d5edcef47fb089f206549af6c42463c9be15cfff9966`；Trace `99946368181a5fc5fbbbb8f0103705a473e4a19bc6d1af2cb4043b139013fe25`；Field `c85a0c9d4e3c247eda955664389e38a19f738313f75020c94d2f8f4840bb22f1`。

V2 只提供定位区间和原值，不指认具体 writer；真实设备链、正式独立测试与签名包仍由 T150/T151 接续。
