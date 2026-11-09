# T443 · GPU 归档检查器版本化修复报告

2026-11-09，tester，P5，不使用真机。按 T441 §4–5 合同重写独立归档检查器，修复 T411 `t409_check.cjs` 全部已归因缺口。

## 交付

- `test/t443/archive_check_v2.cjs` — 新检查器（`checkerRevision: T441_ARCHIVE_CHECK_V2`，输入协议不变 `T409_EVIDENCE_ARCHIVE_V1`/`T409_EVIDENCE_CAPTURE_PAGE_V1`；CLI `node test/t443/archive_check_v2.cjs <dir|manifest>`，`--selftest` 内嵌 13 个 fixture）
- selftest 结果：P0 正例 PASS；12 个反例全部按目标门拒绝

## 真机两档回放（T441 §1 误报消除验证）

| 档 | 旧 T10（t409_check） | 新 T441_ARCHIVE_CHECK_V2 |
|---|---|---|
| v1-attempt2 | FAIL `缺: BEGIN,END`（把 string[] 当对象读） | **ARCHIVE_INTEGRITY_PASS**（activity: zeroAccept/noToken/idleWindowOnly，exercisedKinds=[BEGIN,END]） |
| v2-attempt2 | 同上误报 | **ARCHIVE_INTEGRITY_PASS**（同 activity） |

原档原件零改动；activity 分项如实标注零接受/无 token/纯空闲窗——**不产生双源或 30fps 通过**（T441 §4.2 合法空闲档语义 + NOT_EXERCISED 提示）。

## 修复项映射（T441 §3 → §4/§5）

| T441 缺口 | 新 checker 修复 |
|---|---|
| T10 required 读对象 | requiredEvents 必须 string[]；对象/混合/非字符串/重复/未知 kind 全拒（N1） |
| 未核实际事件 kind | 按页 cursor 与存储顺序扫全部 EVENTS 行，serial 严格 1..rowCount 不排序（N 系列覆盖） |
| required 从未推导 | 从 summary flags/counters 独立推导 R：BEGIN/END 恒必需；readySeen/sourceAttemptSeen/sourceOfferSeen/acceptedSeen/faultSeen/revokeSeen 与实际事件双向一致（N3 flag=true 缺行、N4 flag=false 事件在场均拒）；offerCount>0 相别各需 FIRST_OFFER；enqueued≤offer；acceptedSeen=true 而 tokenCount=0 拒；声明集与 R 欠/过声明均拒（N2 声明代真行拒） |
| INCOMPLETE 退出 0 | INCOMPLETE/dropped>0 → verdict `UNJUDGABLE`，CLI 退出 1（N5） |
| 页数公式错 | 正式 `1+ceil(rowCount/4)+tokenCount`（N6） |
| T13 文件名猜页 | 按 cursor/type 定位页序，next 链与 complete 仅末页；重复/缺 cursor 拒 |
| scope/账未核 | 每页 scope 七字段与 manifest 一致（N9）；SUMMARY 与 manifest.summary 逐字段相等（T7）；首 BEGIN/末 END 唯一（T9）；BEGIN 账=initial、END 账=terminal 逐字段（N8）；TOKEN 页数/结构/ATTEMPTED|NOT_ATTEMPTED（T14） |
| 数值 `\|0`/宽松 | 严格整数或规范十进制字符串（接受真机档的数字计数与字符串 U64 双形态，禁 NaN/负/前导零/2^32 回绕）（N12）；dataBytes=页字节和、totalBytes=页+manifest 原 UTF8 字节、1MiB 预算（N 系列文件门） |
| 未知 revision SKIP | 返回 `UNSUPPORTED_INPUT` 退出 1（N7）；空目录/缺 manifest `INPUT_ERROR`（N11） |

## 不变项与边界

- 旧 T306 timeline parser 保留：`run_t306_timeline_units.cjs` 20/0 仍绿；旧 T399/T405 拒判语义继续由原 parser 承担，T409 不借旧 parser 避开新校验
- 原 t409_check.cjs 未删除（其旧夹具 `t409_good` 按裁定转反例——新 checker 的 N 系列已覆盖该伪声明形态）
- 全部 fixtures 标 `SYNTHETIC_PROTOCOL`，不当真机
- 结果交 T410 消费：两 attempt2 档现为可离线消费的完整档，但**双源、负控、lease、30fps 仍需各自实证**，工具修复不代替真机项目
