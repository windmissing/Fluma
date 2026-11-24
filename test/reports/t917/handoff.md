# T917 · Demo 续沙与 G13 建管线离线判据

2026-11-24 · tester · Windows PC 离线。机器收据见 [offline.json](offline.json)。

- **续沙**。检查真实 callback/receipt 时钟缺口、旧授权关闭、同 run/geometry/device/queue/fence/generation 退役到 Idle、旧额度与旧 offer 终态、新 baseline，以及同一次 Down 的 capture/材料/几何与新授权 epoch。首拍只建锚，第二个真实相邻消费拍才可建 Source offer。按真实时间重算 Q20、核旧 permit/nonce 不复用、SOURCE 零物理步及库存/提交变化。覆盖 Up、越界、失捕、换料、resize 及满池零量。原 T912 同一次按压在 gap 前实收 16 笔、2064374 Q20。callback/receipt gap 分别 39.1816/39.1817 ms，旧 fence 成功退役，新 session 2 后 51 个同根 QUANTUM 持续呈现，SOURCE 未续发。
- **G13**。校验 A/B 的实际 argv、PE、加载模块路径/版本/摘要、设备/队列、前置角色序与模块摘要、WORK 范围和完整 OP_48 创建结构。同路由换 PE 和同 PE 只切 snapshot 分开核。T912 两个不同进程只有 ENTER，第三份停点文件是同一原件。T890 control 在 11.1103 ms 返回，T901 在 1226.6961565 s 返回，均随后到 F02 业务拒根及晚退役。历史缺实际 argv/ICD 及同路由前缀，未通过严格 A/B 输入资格。缺 EXIT 只表示原件无返回记录；判断是否超过监督截止须同时看监督原件。
- **复现**。运行 `python test/t917/check.py selftest`，复读原件并执行 14 项定向拒绝、5 项取消对照及正常续沙、容量零量、两类 A/B 正例。T916 到件可按脚本 `m1_fixture()`、`pipeline_fixture()` 的字段从原 trace 提取 JSON，分别运行 `python test/t917/check.py resume <JSON>` 和 `python test/t917/check.py pair <JSON>`。合成数据仅用于判据验证。实际同持按续沙和冻结 A/B 由后续 PC 采证确认。
