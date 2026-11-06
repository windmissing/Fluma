# T387 · GPU 第二批 flush 超窗解法裁定

2026-11-06，architect，P5。不使用真机。修订 T341 §3–4 的预热选型与失败分流。T324/T328 的热路径时钟和提交事务继续适用。

## 1. 续测原档能裁定到哪里

核对 `test/reports/t376b/t376b_verdicts.json` 的 21 个 SHA256 前缀，运行 `test/t384/read.cjs` 复算 stage43/44/45/49。探针关闭档的三个 run 独立复现第二批尾 43.842/48.405/61.338ms。探针开启档 r2 的 446 行与 r3 前缀逐行相同，第一笔长调用只计一次。下面两个样本同 TID 48777，跨两个 epoch，root nonce 均为各自 run 的 1。

| epoch/run | 批1 flush ns | 批2 flush ns，pass cursor [10,20) | 批2调用前/后 ns（stage43 边界） | GAP 撤销 |
|---|---:|---:|---:|---|
| 5/4 | 22,812 | 62,078,907（serial 437） | 252,968 / 1,718 | 444，距 anchor 66,349,375ns，accepted=0 |
| 6/5 | 25,937 | 38,492,969（serial 551） | 174,531 / 2,031 | 558，距 anchor 47,453,782ns，accepted=0 |

两次都是非末批 `FullM1Transport::FlushBatch` 的 **GL 调用墙时**，随后 stage41 以 `BATCH_ACTIVE` 取消。该两组无 46/47/48/50，证实未封尾、未实际 Poll、未接受或呈现。W2 首批仍通过，W3 第二批失败，W4 被提交路径阻断，K5 归档保持。由两点墙钟无法分开调用内 CPU 执行、驱动/内核等待、GPU 队列背压或同线程失调度。62→38ms 与不同 run 历史和注入条件同时变化，当前只能描述差异。若设备可取线程调度轨迹，把同 TID 的 running/runnable/blocked 切片按可校准时钟映射到 stage44 原始 ns；拿不到就分项标缺。计数一并保留 probe-off 与 probe-on。

## 2. 选型

| 方案 | 裁定 |
|---|---|
| READY 前复用真实前两批形状 | **先做一个隔离对照包。** T375 的单工作组 pass0 加一次 flush 已覆盖一种冷工作；下一步验证更接近实际命令/资源/排序负载的两批准备是否改善真实热路径。控制只切换冷策略，同包热路径相同。 |
| 重排/删减非末批 flush，缩批或加同 Pump 循环 | 留为有条件的第二实验。T330 末批集中封尾曾在 `glFenceSync` 花 37.65/41.28ms。当前 `QuantumCandidate::Enqueue` 也有独立 `glFlush`，stage44 的 batch2 是批号而非上下文第二次 flush。直接删除或延期可能把墙时移到 dispatch/fence/末批 flush；把同一长调用挪到下一 Pump 也跨不过原 anchor。只在受控新证据指出负载或调用布局可优化时，另审各触点和整 root 时钟。 |
| 改 GAP 口径、重设 anchor、暂停或钳 delta | 现合同下拒绝。调用本身一旦越过 33,333,333ns，批循环 guard 无法在调用中抢占，产品 run 必须按现合同撤销。若代表性冷准备后热路径仍反复越窗，回 architect 按观测分流。若设备能力与现时钟确实冲突，交 manager/design 显式裁决能力范围或另版时钟合同。 |

## 3. Developer 最小实施合同

1. 用现有冷初始化 owner、EGL context、已链接 `QuantumCommitKernel`、布局、uniform location 缓存与 pass table，READY 和首 anchor 前执行**一次**实验冷前缀。与现有 T375 冷策略做互斥 A/B，冷配置固定到整个 run。实验使用受控有效初态，记录实际 K/Kw、角度、前缀与输入 digest。复用 Enqueue 的底层 staging 与 pass 执行语义，按 `[0,10)`、`[10,20)` 各发一次并保留各自屏障/flush，含前置 staging flush 的形状。该前缀的第二批包括真实 stage7 bitonic 轮次。准备需要合法的 kernel 参数（含 quantumOrdinal），冷标识与产品 nonce 空间隔离。无产品 `RootRequest` 入队、`BeginQuantum`、封尾、Poll、接受或发布。
2. Base 只读，写入限 candidate/workspace 和隔离 event scratch。前缀内水边路径能写 event，须用明确隔离的写目标，或证明被选空夹具所有 event 写不可达；不能只凭 pass0 只写两块就扩到二十个 op。优先复用已有 GPU scratch，核角表和实际运行重写范围。热首根仍完整初始化 candidate、workspace、计数/错误锁存和 event 逻辑长度，按原语义重拷有效 ledger/prefix。证明从初态进入首个正式 root 的有效输出逐位等价，含 bank、event、publication、completion、状态和统计，列实际冷临时与峰值字节。无 active bank、源、计费/提交时钟或已发布资源被写。
3. 两批后用冷准备专用 fence 验证工作完成并回收，owner 上有界非阻塞轮询，完成后在隔离冷诊断核 shader 首错与前缀确实执行，失败保原 GL/EGL 码、阶段、冷耗时及释放路径。预算是离开驱动调用后检查的墙钟界，阻塞中的 GL 调用不可抢占。READY 前再次核 epoch/window/owner 有效性；失败或失效清理后保 FAULT，不延后到首热根继续冷前缀。可复用 T375 冷前缀等价、失败 Create 后重试和 T384 host 取消回归。补两策略首正式 root 位型等价、冷准备身份撤销、冷 fence/GL 失败及重建。离线编译、自测、签包后交设备卡。
4. 保留 stage42 成对、root=0 和原始预热总耗时，新增版本化冷分段回执。热路径给 Enqueue staging flush 单独标阶段和同 nonce 身份，沿 T384 采真实逐批/final flush、fence、Poll、accepted/GAP、交换，保持旧档可读和 4096 有界冷归档。所有 W2/W3 数字用实际包含 flush 的 wall，守住 8ms Pump guard 和 33,333,333ns GAP；120Hz、carry、nonce、owner、事件封签、预算、守恒、撤销及事务次序照原规则，GAP 撤销样本不计成功。仅 XComponent+shader，包括背景/前景；失败显示 FAULT 或有完整旧 token 的 STALE。

## 4. 定点真机证伪

用新签包同设备对照两种冷策略，预先固定 K/Kw、角度、进程冷启与同进程新 run、首帧前空闲和沙水脚本，交错每策略至少三次独立 run。基线对照是同包的 T375 策略。分别记录 init→READY、READY→首有效 swap、热各批墙时、长调用是否移到 staging/其它批/final/fence/Poll，以及各 root 的 anchor→accept/swap/GAP。以 T341 的 W2/W3 和原时钟判定，不能用较短的第二批数字替代整条链。先观察真实空场量子封尾，再核沙/水分别实际准入、成功量子和同 token XComponent Draw/Swap；做一次真实/注入 GAP 负控，保持 fail-closed 与 K5。热路径若再次越窗则保留原档回 architect，收益仍按 T380 等完整前置。
