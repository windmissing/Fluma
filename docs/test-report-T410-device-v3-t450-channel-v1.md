# T410 · GPU V2 同窗双源与负控真机补证（v3 阶段报告：T450 冷启通道已证，采集被设备 GPU 域阻断）

2026-11-10，tester。设备 62T0225B10010020，T450-t410 签包 SHA256 63b0fd25e8476fd2bf792b142f63caf52b60c3bdf0f0a1363e74fa806c4fa706（与卡面一致，`install_input.py` 校验 PASS：bundle/versionCode/invalidField/PROGRESS_V2_NEXT_RUN 均在、非 T434 变体）。

## 已证（本轮新增）

1. **T450 冷启动 SHA 声明通道端到端打通**（T442 归因的 SHA 输入阻断已解决）：
   - 冷启 Want `--pb grsm1 true --ps gpusha 63b0fd25…`，PID 7876；
   - 诊断页只读显示 `SHA_DECLARED 63b0fd25…fa706`（与安装文件一致）；
   - arm 时 `prepareCaptureArm` 组装九字段 config（packageSha256=声明 SHA、expectedRevision 跟随 run、durationNs=2000000000、maxRows=4096、maxBytes=1048576）并调 `beginEvidenceCapture`——inputText 注入通道不再需要。
2. **生产 fail-closed 语义正确行使**：arm 在 scope.epoch≠readyEpoch 时被 `STALE_OR_UNREADY_EPOCH` 正确拒绝（V1 与 V2 run 各一次，原件 `test/reports/t410-v3/`）。
3. **实际 V2 run 建立**：切"下轮进度时钟：V2"→新空场，顶行 `GRS_M1_PROGRESS_CLOCK_V2 / epoch 3`（第二轮 epoch 4），`PROGRESS_V2_NEXT_RUN` 确认；run 0:2/0:3 出现 physics commit 0:1，native 链路活。

## 阻断（未达采集 ARMED/双源/负控/30fps）

设备侧 native owner 无法恢复 READY：
- V1 冷启 run 与 V2 新空场 run 均持续 `CLOCK_GAP`（静置 70s 不恢复，物理 0:0，AnchorSet 从未发生）；
- V2 run 一旦 GAP，`progress.open=false` 永久（`PagePulseLocked` 86 行的 open 恢复仅 `!progressV2` 分支；98 行 phase 已是 CLOCK_GAP 不再触发恢复），页面无 endPageSession→beginPageSession 重建路径，pulse 永久 `STALE_PAGE_SESSION` → Fresh=false → TryReadyV2 前置失败 → readyEpoch 永不更新；
- hilog（PID 7876）GPU 线程 `EglWrapperHookLayer init Failed` + GPU queue `~QueueInternal/Termiated/Device init` 循环重建，与 T448/T451 已归因的 SURFACE GL context 生命周期域同根因；shader 编译 40.9s 必然超过 33.3ms GAP（结构性的，缓存后仍 GAP）。

## 移交

- **发现 1（新，交 architect 裁决）**：V2 下 CLOCK_GAP 后 `progress.open` 无自愈路径（V1 有），且 V1 CLOCK_GAP 后 Advance 恢复依赖 GPU 提交恢复，当前设备上不可达。恢复手段缺失属生产恢复路径设计问题，fail-closed 本身正确。
- 采集 ARMED→FROZEN 全链已在 T459/T421 离线闭环 + T421 包 v2-run3 真机档（历史原件）；双源/六负控/30fps 继续等设备 READY 窗口或 T451 分流修复后重签包。
- 证据：`test/reports/t410-v3/`（14 截图 + layout dump）。

未用旧 handoff SHA；未冒 PASS；未完成项如实。
