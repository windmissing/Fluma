# T389 · GPU 冷准备 A/B

沿原 `grsm1` 入口进入「光学/诊断」，选「下轮冷准备：T375 / 两批」后点「新空场」。默认 T375。冷策略在创建 run 时锁定。「下轮提交探针」独立控制 T384 和新增热 staging flush。两种冷策略共用原热提交代码、120 Hz、8 ms guard 和 33,333,333 ns GAP。签包路径和 SHA256 见[签包回执](../t257/T389.json)。

两批策略使用同一 EGL owner、program、uniform 缓存和 pass table。有效空场 bank 只读，按正式 Enqueue 拷入 ledger、绑定并 flush，然后执行 `[0,10)` 和 `[10,20)`，各自保持屏障和 flush。第二段的十个实际 stage7 bitonic 轮次产生独立的降序块，冷 fence 完成后读工作区验证该块及 shader 错误锁存。产品 nonce、run、接受和发布对象不参与。candidate 和 workspace 复用，事件使用独立 GL buffer。初态 header、零库存、零 ledger 和 uniform 输入以 FNV-1a 记录。Header 含各次 run 的身份，所以不同 run 的输入 digest 可以不同。

新增 GL 临时峰值在 K192/W64 为 10,795,008 字节，在 K512/W128 为 27,654,144 字节。验证映射 256 字节。临时 event buffer 和 fence 在 READY 前销毁。500 ms 是调用返回后检查的冷预算，owner 上以零等待 fence poll 并在 BUSY 时短暂让出。冷阶段失败或 epoch/window 作废后清理，不发 READY。显式记录 GL、EGL、shader 和失败阶段。

原 4096 环升级为 `T389_STAGE57_V1`，旧 1–50 含成对 stage42 保留。51 是热 Enqueue staging flush，root 为实际 nonce。52 是冷夹具（A/B 为 carry 0/1，A/B/C 字段为 K/Kw/digest，code 为角度定点位型）。53 是冷 staging flush。54 是冷批次整段墙时（A/B 为 pass 游标，C 为累计 dispatch）。55 是冷 fence（A/B/C 为原 wait/GL/EGL，code 为 transport）。56 是 cleanup（A 为临时字节，B 为 shader 首错，C 高低位为 EGL/GL，code 为 transport，carry 为 cut）。57 是冷批次 flush。52–57 的 root 恒为 0。时间戳为原 steady ns。51/53/57 区间只含实际 GL flush，54 包括整批命令及 flush。

PC GLES K192/W64 52 项、K512/W128 54 项、Run 12 项、App 11 项、冷导出 21 项均通过。两策略首正式 root 的 bank、有效 event、publication、completion、owner header 和结构统计逐位一致。真实 GL 故障注入覆盖冷 fence 路段，另有 epoch 撤销与重建。Hvigor ArkTS/native 构建及签名验证通过。T390 负责设备端 A/B、双源和同 token 的后续证据。
