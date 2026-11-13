# T492 稳定快照、首因幂等与首帧诊断

2026-11-13，developer，不使用真机。按 T489 §4–5、T490 §2–5 实现，源文件及实际包/ELF 的机器证据见 `test/reports/t492/delivery.json`。

## 实现入口

`test/t492/generate.cjs` 是单一 bridge/页面集成 overlay，分别消费 T485 的 M1/M2 生成链。复用已交 66/67 字典、停止旧 frame 的页面门、恢复与场景缓存/冷准备。M2 保留原 V3 source 命令及 7/3 profile，M1 保留 2 和 3/2。未修改 shader、物理 pass、批边界、Flush/fence、250,000,000ns root deadline 或 33,333,333ns GAP。

`timeline_snapshot.h` 实现独立的 `timelineSnapshotBegin/Page/End`：

- 单份固定 payload：4096×168B 行＋32×2048B token 对＝753,664B，另有固定 metadata。一份随 App 分配的储存区，Page 只复制最多四行及其 token；同 App 短锁内复制，JSON/JS 字符串和文件 I/O 均在锁外。
- 独立、不复用的 U64 ID；BUSY、ID 耗尽、错 ID、越界 cursor 均明确拒绝。60 秒单调 TTL，读不续期；下一次访问回收过期所有权。End 释放快照所有权，不清旧 live timeline，也不改变业务身份、许可或唤醒 owner。
- 捕获 epoch/run/context/surface/PID、source 声明、字典、count/serial/dropped 与 token 表。新 run 后旧快照仍带旧 metadata。声明不是安装证明，实际包身份仍沿 T483 的 HAP/ELF 收据。
- 旧 `timeline(cursor)` 接口和固定前缀保持。`GpuTimelineSnapshotCold` 复用原 reader 严格校验并逐页核冻结 identity；4 行/16KiB 页、30 秒、总 trace＋diagnostic 4MiB，单 diagnostic 2MiB。取消、页面隐藏、超时、I/O 或校验失败均 finally 释放。未知 stage、原 dropped 或缺页仍 INCOMPLETE。
- 拒页保留完整 ≤16KiB 原始字节，fsync、SHA 后再写错误 manifest；错误含 field/row index。修正原 reader 的 pendingRaw 过早清除，使 serial 等后续校验失败也能保存原页。未跳行、映射未知字典或补造历史坏页。

`execution_diagnostics.h` 提供独立 `T492_EXECUTION_DIAGNOSTICS_V1`，不往旧字典追加未知事件：128 个定长 stage 汇总、最多四个 scope、每 scope 首 root 768 槽＋首 QUANTUM 768 槽（相同时不重复写）、首错 root 和 64 槽尾部；96 个 UI 记录、64 个原命令记录、32 个独立 reset 回执。溢出各自计数，不覆盖旧首因。hot 计数/point/packet 写入均固定存储，无热文件或 JSON；查询复制后在锁外序列化。

首 root/QUANTUM 保存完整 identity/input、nonce/kind/plan、session/reset/revision、started/deadline。Submit、批次 from/to、Flush、fence/poll、接受、Scene Build/load/compile/link、VAO/FBO、八个 draw、Swap 前门/实际调用/返回/事后许可均保 enter/exit 和线程/时间域。调用未返回只有 enter；FBO 原始错误在 Reset 前保存，未增加 GL error 查询或改变原短路顺序。首错后记录同首错 root 的有限尾部，独立于 boot terminal 和 legacy timeline 满容量；晚旧 pulse 只增独立统计，不占满 Swap 尾部槽。

native `PagePulseLocked` 对错误 session 或已经关闭的 session 早返回兼容的 `STALE_PAGE_SESSION`（V1 保原兼容回复），不调用 ClockGapLocked、不再撤权或改变 permissionRevision。原原因继续从 recovery/pageFault 读取，新增首因记录不可被尾部覆盖。只有本次进入时 open、当前 session 的真实 GAP/倒序才能触发原故障路径；页面现有停止逻辑保留。

## 操作链

公共 controller 记录 source 选择、pointerDown/READY/坐标转换、实际 128B/32U32 命令的 `FNV1A32_LE_128B` hash、Offer 回复和 SOURCE receipt；native 固定存下同一命令，冷导出补 SHA256。所有 native 记录带 reset/session/run/permissionRevision，拒绝也留记录；不新增 SOURCE 路径或改数学。

两页面保按钮 handler→restart→旧 frame cancel→newRun 发起/返回/异常→新 frame bind。`newRunAction(serial)` 与原 newRun 共用 invalidate；独立 32 槽 native reset 回执保存 actionSerial、旧/新 epoch 和 reset scope。自动恢复仍用 serial=0，不冒充用户第二次按键。

## 必要检查与交接

两路线各有原恢复 native 23 项＋新增 native 20 项；页面/reader 20 项；原 T477 reader 9 项及原 frame 12 项回归。原 T306/T404 live reader 分别 20/22 项通过，只将已授权迁移的 M1 页面调用断言改为 collectSnapshot，其余判据保持。覆盖超期后连续旧 pulse、首因/revision 不变、真实 GAP 和 deadline 等号、快照活跃增长/跨代/TTL/BUSY/溢出、拒页落盘、取消/超时/I/O、两次 reset、SOURCE 合法/未 READY/错坐标、首 SOURCE 后首 QUANTUM 以及后续 root 故障尾部。

实际生成 App 在 cold/warm 检查三 graphics 准备、uniform/VAO 及首 pulse 零冷 quota；实际 SOURCE GL fixture 行使 Offer、compute 接受、J→W、Scene 与 native Swap。该 fixture 使用受控 host 时钟，不证明设备 root 成本、系统像素或持续帧率。所有仿真/图形仍 GPU/XComponent+shader；ANGLE 仅离线设施。

两条最终生成目录为 `.codex/local-test/t492-m1-delivery`、`t492-m2-delivery`；最终签包收据为 `test/reports/t257/T492-m1-delivery.json` 和 M2 对应文件。生成 manifest 冻结 native/页面 hash；collector 核包内 ELF 与构建 ELF、T485 物理源码/compute cache 字节兼容，归档原页、拒页、SHA 与原日志。中途打包一次 spawn EPERM 已重试成功；最终两路线均要求完整 hvigor 构建及 verifyApp PASS。

T493 独立差分可直接消费 `test/t492/README.md` 和 `test/reports/t492/`；T495 使用最终 delivery 签包串行补真机首 root、显式两次 reset 和真实源动作。没有把缺 SWAP 行补成成功，也未将历史缺证记为产品 PASS。
