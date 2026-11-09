# T411 · GPU 同窗归档独立完整性检查

2026-11-09，tester。不使用真机。依据 T407 §2/§4.3 固定 schema 与 developer-interface-T409-evidence-capture-v1.md。

## 1. 交付物

| 文件 | 用途 |
|---|---|
| `test/t411/archive_check.cjs` | 旧 timeline 归档（T306_TIMELINE_COLD_V1）独立完整性 CLI；零依赖 node；`--selftest` / `--make-fixtures` / 目录或单 manifest |
| `test/t411/t409_check.cjs` | T409_EVIDENCE_ARCHIVE_V1 新 schema 独立检查器；`--selftest` 内置 4 组合成 fixtures |
| `test/reports/t411/fixtures/` | 合成正反控 fixtures（可复现） |

**独立性**：不调用产品断言、不读产品代码、不依赖 T409 真机归档即可自证。T410 可直接消费（`node archive_check.cjs <dir>` 退出码 0/1）。

## 2. 检查项（旧 schema，archive_check.cjs）

- C1 manifest 顶层 schema 字段齐备（12 项）
- C2/C2b 分片存在性 + `missing` 字段与独立核验一致（T407 §2：missing=0 只是导出器自述）
- C3/C3b/C3c bytes：**导出器语义已破解并复算验证**——声明 bytes/SHA = 各页对象紧凑序列化（`JSON.stringify` 无空格）以 `\n` 连接+尾 `\n`；单分片 raw ≤1MiB；`dataBytes == Σ声明 bytes`
- C4 SHA-256 按导出器语义逐位复算
- C5 serial 严格连续无缺失无重复、first/last 与声明一致（行在页对象 `rows[]` 内）
- C6a/C6b identity 齐备；行内 `epoch/run` ⊆ manifest `groups`（同窗一致，混窗必检）
- C7a–C7d before/after.count==rows、dropped 显式（静默采样不得掩盖）、state 相符；INCOMPLETE 归档输出 `INCOMPLETE_REFUSED`（完整性检查继续，语义判读须拒）
- C8 pages 与 rows 分页关系自洽（4 行/页或 1 行/页）

## 3. 检查项（T409 新 schema，t409_check.cjs）

T1 revision、T2 state、T3–T5 每页存在/SHA-256/原字节==声明且 ≤1MiB、T6 dataBytes、T7 **totalBytes==分片+manifest 实际 UTF8 bytes**、T8 预算、T9 initial/terminal ledger 八字段十进制字符串、T10 required 事件（BEGIN/END 恒必需 + seen 标志推导 READY/SOURCE_ATTEMPT/FIRST_ACCEPT/FIRST_FAILURE/REVOKE）、T11/T12/T12b rowCount≤4096、pageCount≤1033、页数公式、**files 数==pageCount**、T13 行 serial 严格 1..rowCount、T14 incomplete/dropped>0 → UNJUDGABLE。

## 4. 自测结果（合成正反控先行，全部通过）

- archive_check：**14 ok / 0 bad**——正例 PASS；缺页/SHA 错/超额/身份混窗/静默采样五类负控全部按预期指定检查项变红
- t409_check：**7 ok / 0 bad**——正例 PASS；SHA 篡改/缺页/serial 错全部检出

## 5. 消费既有归档

| 归档 | 判定 |
|---|---|
| T399 两份 `18338-2/-3` timeline manifest | **PASS（14/14 检查全绿）**——分片 5/5 在场、SHA 逐位一致、627 行 serial 连续、24 groups 同窗、dataBytes=204807 |
| T405 `tl1-manifest.json` | FAIL（INCOMPLETE_REFUSED）——0 分片 0 行，PACKAGE_SHA_REQUIRED 导出，非成功导出（与 T407 §2 裁决一致） |
| T405 `tl2-manifest.json` | FAIL（INCOMPLETE_REFUSED）——**31 个引用分片全部缺失**、`missing=0` 与独立核验（实际缺 31）矛盾、dataBytes=1543343 **超 1MiB 预算**、NATIVE_DROPPED=3663823。旧 T405 不能按 T409 格式追认（T409 接口文档明示），维持 T407 拒判 |

## 6. 移交

- T409 真机归档尚未产生（仅 `T409-handoff-signed.hap` 签包，无 `GRS_EVIDENCE_CAPTURE` 导出回执/manifest）。新 schema 检查器已就绪并有合成验证，T409 真机归档到件后可直接 `node t409_check.cjs <dir>` 出判定。
- 无阻塞问题；本卡可完成收口。
