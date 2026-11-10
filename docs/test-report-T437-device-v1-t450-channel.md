# T437 · GPU 来源前缀同包收益真机对照（v1 阶段报告：T450 双路通道已证，配对采集被设备 GPU 域阻断）

2026-11-10，tester。设备 62T0225B10010020，T450-t437 双路签包 SHA256 4ab9b671e068eabeb425b9bac2fd1eb0bc645471ee2632ec715f5f3818f35f9f（与卡面一致，`install_input.py` 校验 PASS，`WRONG_T434_VARIANT` 反核通过）。

## 已证（本轮新增）

1. **T450 双路冷启通道端到端打通**：冷启 Want `gpusha=4ab9b671…`，诊断页只读 `SHA_DECLARED 4ab9b671…f35f9f`；
2. **T437 特有路由 UI 在场**：`下轮来源账：完整`（setLedgerPrefix 冷选切换按钮）与 `导出复制计数`（T434_COPY_ACCOUNTING_V1）确认在生成页；
3. 安装回执、bundle/versionCode 与 t410 版互斥核（T434 变体仅在 t437 包）通过。

## 阻断（完整/前缀同包配对采集未行使）

与 T410 v3 同根因：设备 native owner 持续 `CLOCK_GAP`（冷启+静置 60s 不恢复，物理 0:0），readyEpoch 无法等于 epoch，arm 被 `STALE_OR_UNREADY_EPOCH` 拒绝（fail-closed 正确）。GPU 线程 `EglWrapperHookLayer init Failed` + queue 循环重建（T448/T451 域）。submit/flush/poll/map/swap、byte/pass、accepted quantum/s、新物理内容 fps 全部未测——**如实 NOT_MEASURED，不用字节推 fps**。

## 移交

- 完整/前缀配对采集需设备 READY 窗口（T451 分流修复后）重跑；离线路由计量（ledgerCopyBytes=64n/262144 回退、route==1/2）已由 T436 四档逐字节差分覆盖，不重复。
- 证据：`test/reports/t437/`（冷启、诊断页、layout dump）。

未冒 PASS；未完成项如实。
