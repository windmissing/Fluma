# T717 首帧会话与创建停点

## M1 会话

T714 签包、unsigned HAP 与包内 ELF 已对齐。设备布局从 epoch 2 / run 0:0 变到 epoch 2 / run 0:1，均显示 native session 1，后者物理提交为 0:1 并显示 `STALE_PAGE_SESSION`。这能证明物理提交推进。现有目录缺回调实参 token、native open、同 scope 首根/Swap 回执和进程 loaded ELF，具体撤权原因与是否曾上屏均未确定。

源码确认 `ProgressClockV2::Pulse` 对 token 不等和 native 会话已关闭返回相同字符串。旧页收到终态仍重排帧回调，后续 `STALE_PAGE_SESSION` 会覆盖最初的时钟故障，native 也会误把已关闭会话上的 stale 调用送入时钟故障处理。T477 旧 manifest 有明确 `CLOCK_GAP`，实际差值 34,329,218 ns，且此前已登记 2,446 个 native 物理帧。T717 与它共享错误遮蔽路径，首因仍需新原件。

M1 对照包保 T714 CLOCK_V2，在已失效回调上终止调度，保留同 epoch、run、session 的撤权原因。只有真实时钟拒绝进入 clock fault 分支。显式新空场、档位或光学新场重绑实际 frame，旧 callback 保持失效。新增有界首帧原件记录同 scope 的 callback token、open 前后、前一接受 pulse、首个接受 root 和首个 Swap 尝试。根期限、GAP、原发布许可和渲染路径沿用原合同。

## M2 创建

T714 TWO_ON 原件同一 boot 中，SOURCE object 0 的 LINK 阶段实际成功返回，耗时 72.165237031 s、rawGL 0。object 1 标为 `NOT_USED_BY_VARIANT`。之后 FINAL_Q object 2 的游标为 API 1 `glCompileShader` ENTER，sequence 29，开放 10.323132969 s。endNs 为 0、returned/rawGl 为 -1、未 sealed，属于采样截尾。SOURCE 与 FINAL_Q 的 SHA 均与 T714 包内角色匹配。这里的最终 Q 编译终态未见，沿用 T714 实际 M2 工件与已有 T663 游标。

## T711 最短复测

- M1 使用新 THREE_OFF 一次冷启，不手动重开 session。首次稳定出现终态或 CURRENT 时点击“导出首帧原件”，最多保两份，每份上限 16 KiB，同时留签包和进程 loaded ELF 身份及当时 XComponent 布局。按 epoch、run、owner、window、context、surface、session、permissionRevision 绑定 token 和首根。Swap 另按 nonce 比对其进入、实际调用、原返回及许可结果，缺字段保未观测。
- M2 复用 T714 TWO_ON 已签工件，一次新进程直达原 M2 V3。沿原 60 s / 64 query 预算保存原 native JSON、对象日志与加载身份。进入后若游标推进，按相同 boot/owner/context 绑定 API 退出和终态。窗口截止仍开放则保截尾，不用另一轮 LINK 补写 FINAL_Q。

M1 生成参数为 `node tools/t701/prepare.cjs M1 <隔离目录> --session-control`，本次产物在 `.codex/local-test/t717/scoped/THREE_OFF`。原件分析见 [attribution.json](../test/reports/t717/attribution.json)。M1 编译回执见 [compile-m1.json](../test/reports/t717/compile-m1.json)。未签包、运行产品或使用真机。
