# T693 T703 原映射身份与三时域复验

2026-11-20，离线。两份隔离 MSVC host 构建均通过 layout gate 和链接。主构建以冻结 T492 所用源和原 GL 调用为底，重放构建另启用 T692 实际接受态读取。构建输入、PE、已加载模块、缓存输入和守护进程证据均随臂保存。

## 证据与判定

每笔原 completion 以包装器实际分配的调用序关联 ENTER、RETURNED、1024 字节和 EXIT，再以 context/share group、GL name、对象代、存储代、绑定代和 map token 关联实际 unmap 的 GL_TRUE。逐根比对实际接受切点的 1024 字节，核对范围、参数、根身份、bank、publication 与事件 arena。重放中的 bank/event 读单独标记，并以 T692 返回的名称、指针、范围、access 和状态联核。

32 字节行保存进程 steady、函数、边、原调用序和 stamp 字典引用。脚本 ID 与声明时间结合现场送达及 owner 实际消费 checkpoint。transport 保存许可开始、期限、revision、nonce 和原授权或 Stamp 观察值。原业务诊断保留了冷启动、批次及轮询的 TailObserver steady 起止样本，重放另存 T692 原 `TailObserver::Now` 起点。父守护使用独立 monotonic 起点/边界，并通过 PID/start 与 ActivePhase 命中实际子进程调用。

首次补证 OFF 在 120 秒业务窗只送达 39/52，退出 8，仍有 69 个接受根的映射和接受字节可逐一关联。第二次 OFF 送达 52/52，在第 90 个接受根后触发 `TRACE_EVIDENCE_BUDGET`，API 已提交前缀 10,266,678 字节。字典达到原固定配额，随后在 12 MiB API 分区内改用 110,000 行预留，将差额用于字典。外层 420 秒、业务 120 秒、排空 30 秒及 API/root/business 12/16/4 MiB 合同保持。

## 离线负控

102,733 行历史前缀逐字节往返，历史主/重放四臂的 92 根仍可复核。MSVC shadow 单测覆盖代际、线程转移、共享及非共享 context、复用/改绑和范围拒绝。七项实测 trace 副本负控均被判拒，包括变更 access、offset、map 指针状态、unmap 返回、对象代、clock mode 和接受字段。容量测试在首拒绝序 5281 保留 5280 行可读前缀，损坏块 SHA 被拒绝。原结果见 [回归](../test/reports/t693/t703/regression.json) 与 [负控](../test/reports/t693/t703/witness-negatives.json)。

## 同版四臂结果

| 臂 | 耗时（秒） | 动作 | 原映射匹配根 | 额外重放读取 | API 字节 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 主 OFF | 147.062 | 52 | 92 | 0 | 10,238,001 |
| 主 ON | 132.234 | 52 | 92 | 0 | 10,281,633 |
| 重放 OFF | 173.094 | 52 | 92 | 138 | 10,747,816 |
| 重放 ON | 145.516 | 52 | 92 | 138 | 10,598,854 |

四臂正常退出，每臂覆盖四个指定 busy cut，ON 两臂各保留 55 个完整 FROZEN 页。两次主↔重放分别匹配 92 根的原 completion，重放双臂的 bank、event、completion 字节相同，接受账本、token 和排空终态逐项匹配。四臂均无未知 transport 锚点，原件总量均在 32 MiB/臂之内。32cycle 为 NOT_MEASURED。

汇总见 [四臂回执](../test/reports/t693/t703/four-arm.json)。原件见 [主 OFF](../test/reports/t496/t693-t703-compact-primary-off/)、[主 ON](../test/reports/t496/t693-t703-compact-primary-on/)、[重放 OFF](../test/reports/t496/t693-t703-compact-replay-off/) 和 [重放 ON](../test/reports/t496/t693-t703-compact-replay-on/)。
