# T808 · G13 探针停在 CREATED 的首次归因

2026-11-22 · 目标20 · P5 · architect · 离线归因

> T820 复核。T817 实际同代进度已定位 `glCompileShader` 首个未闭合调用，输入为原 RESET 的 48,695 B `OP_00`。取消请求未确认，未退役。下一步用原 OP12/13 的最小等价闭包执行原 RESET。 [T820 证据与修订](architecture-T820-v1.md)

> T813 补充。先读 T811 原 RESET 的逐调用进度。能定位时直接按原调用分流。仍无法区分 `Open` 与角色准备时，按 T813 选择同签包、同逻辑 owner 的 EGL-only 或单角色对照。 [T813 触发与对照合同](architecture-T813-v1.md)

**已证 RESET 的 owner 在 CPU 上持续活跃，能读到的快照始终为 `CREATED`。现有发布边无法定位精确调用。** 签包源码在 `CREATED` 后把 `CONTEXT` 和部分错误先留在线程内，RESET 直到角色都准备好才发布 `ROLES_LINKED`。因此下一步先补逐调用进度和取消收据。凭报告无法裁定为 EGL 阻塞、着色器编译/link 停滞或应用忙循环。 [T770 v7 原报告](../test/reports/t491/t770b/v7-probe-exec-blocked.json) · [逐项证据](architecture-T808-evidence-v1.json)

## 实际签包和 CPU 原证

v7 运行的是 T770-v6 签名交接中的主 bundle。未签 `ecc5c815…`，已签 `141773f5…`，13 个 payload 逐字匹配。ABC 含实际探针页路由，包内 `libgrs_g13_visibility.so` SHA `2369bba0…`，GNU BuildId `30a1c65fa27c45eeebef73aa0542bdc42d0a4a08` 与真机快照相同。ELF 含 `DEPENDENCY_SPLIT` 及本轮 RESET `OP_00` 实际 shader 字节。它提供本轮诊断入口证据，T782 的可信工件合同仍要求指定角色、实际安装包与 `G13_VISIBILITY_TRUSTED_MAIN`。 [签名回执](../test/reports/t257/T770-v6.json) · [原 snapshot](../test/reports/t491/t770b/snapshot-1.json) · [T790 工件路由](architecture-T790-v1.md)

快照为 649 B。PID 12312/start 7904132、owner TID 13692、attempt/generation 1/1、nonce `[1,1413953334]`，64×64。唯一事件为 `CREATED ns=79139164043750`，无首错、零 rawBytes。多次读 `probe-reset*`/`probe-compare` 均逐字等于这份快照，`snapshot-2` 为空。v7 报告记 30 s 内 `utime 55780→58489`、R 态及 `wchan=0`，两个原 hilog 持续独立报该 TID 的 HighLoad。归档缺这两次完整线程 stat、采样 HZ、原栈/PC 和完整干预时间点。CPU 活跃成立，持续 CPU 百分比、具体循环、单个 API 内停留时间保待采证。 [原 hilog 7](../test/reports/t491/t770b/hl7.txt#L4362) · [原 hilog 8](../test/reports/t491/t770b/hl8.txt#L434)

## 真实代码路径

1. `Phase("CREATED")` 写状态并发布完整 Report。`CheckCancel` 读取原子取消位并检查 overflow，函数自身无循环。之后 `Open()` 依次取得 EGL display、初始化、绑定 API、选配置、建 pbuffer 与私有 context、make current，查询 GL 版本和 vendor/renderer/version，最后检查 `CONTEXT_READY`。
2. `Event("CONTEXT")` 写线程内暂存串，失败的 `Fail()/Event()` 也可先留本地。即使已经通过 EGL，外部仍可能看见 `CREATED`。第二次取消检查之后，RESET 的 `Prepare(5/6)` 对应 opcode 12/13，共用同一 **48,695 B `OP_00`**。首次准备走源字节 SHA、compile、compile status、link、link status、反射及 program 绑定。第二次复用该 role 成功后，下一个正常发布才是 `ROLES_LINKED`。可返回错误后还需要到退役阶段发布。已独立证明的 G13 BOOT 编译时不外推为 OP_00 的耗时。
3. `Start` 对同 attempt 回 `READ_EXISTING_REPORT`，新 attempt 在旧 lease 忙时回 `OWNER_BUSY`。重新选择 COMPARE 无法推进旧 worker。Cancel 设置指定 attempt 的标记，实际由 owner 在检查点消费。`Run` 返回且退役成功后才允许新 attempt，退役不安全仍持 lease。不得用 UI 超时释放 lease、另线程 GL 清理或启动第二个 owner。 [该签包 probe 源码](../.codex/local-test/t767-visibility-final/entry/src/main/cpp/gpu_resident/g13_visibility/probe.cpp#L127) · [owner 桥接](../.codex/local-test/t767-visibility-final/entry/src/main/cpp/gpu_resident/g13_visibility/bridge.cpp#L14)

## 修订合同与回归

- **Developer 先修证据发布。** 增一个独立、定长、有序且读者可校验一致性的进度快照。每条包括 attempt/generation、PID/start/ownerTID、单调 wall 与 thread CPU、递增序号、stage、ENTER/EXIT、结果和首次原错。开始后即能读到 `CREATED_PUBLISHED`，给 `CheckCancel`、`Open` 和每个 EGL/GL 入口保立即可读的 ENTER/EXIT，覆盖 `eglGetDisplay` 到 GL 版本/字符串检查，及角色 SHA、`glCompileShader`、compile status、`glLinkProgram`、link status、反射/绑定和退役关键调用。记录失败边原返回与按原时点采样的 EGL/GL error。原 semantic Report 的事件、nonce、raw 和旧 reader 语义继续独立；进度读取不持 lifecycle 锁等待 GPU/driver，也不在每个观测点重新格式化完整 Report。溢出明确报告。
- **取消与 lease。** 写清 `CANCEL_REQUESTED`、owner 检查点的 `CANCEL_ACK`、`RETIRING`、`RETIRED` 或隔离失败。确认指定 attempt 才消费取消，长调用未返回只报告该 ENTER 未闭合，禁止伪报取消完成。沿原独立离屏 context、原可见性及实际同队列 fence 做退役；确认失去所有旧发布权后才清 lease。首边若落在 EGL，单独检查该调用及初始化；若落在角色 compile/link，先核该 role 字节和真实 driver，再选编译优化。只有栈或连续 PC 采样指向源码循环，才按实际循环修复。若新的逐调用快照仍缺原边，再用同工件同 owner 的 EGL-only 对照分界。
- **Tester 离线验边，effector 先跑 RESET。** 离线模拟单次长调用、返回错误、缺 EXIT/截尾、错 attempt 和取消未确认，读者仍准确保持最后实际边。真机先保存新工件签前/签后 SHA、映射 BuildId、设备/进程/attempt 与首次 ENTER/EXIT，同时保存两点完整 `/proc/PID/task/TID/stat`、实际 tick 频率、`wchan`、可用栈/PC 或原采样拒绝。确认正常 RESET、取消期间 `OWNER_BUSY`、真实同代退役后新 attempt 才开放。再由 T769/T782 读各用例原报告及正式可信包证据。产品 OP65 真数值资格及 XComponent+shader 首 Swap 保原独立证据。 [原探针独立性合同](architecture-T777-v1.md) · [G20 计分口径](architecture-T741-v1.md)

本卡离线完成归因与工程合同。设备上具体未闭合的调用由修订工件取原证。
