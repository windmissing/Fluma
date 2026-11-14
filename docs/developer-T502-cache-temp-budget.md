# T502 缓存故障临时文件预算

2026-11-14，developer，不使用真机。按 T500 R5 修改公共 `program_cache/artifact_store.h`，独立 `test/t502/generate.cjs` 消费 T501/T492/T485 完整 M1/M2 生成链。32 条、单条32MiB、总128MiB、T475B001/144B header、compute/graphics key 和 receipt 限额均保持。

文件 worker 在每次 DiskRoom 前重试清理普通 .tmp，并重新统计仍存在的实际 bytes/files。删除失败保存错误及累计计数，残留 bytes 与合法临时 key 一并参与同一个磁盘预算；同 key 的 .bin/.jsonl/.tmp 只占一个 key，临时文件仍占全部真实字节。未清掉的临时文件不可被原位 truncate 以逃账，写前拒绝；目录/非普通临时对象保持拒写，未放宽旧文件故障合同。

启动也走相同清理/预算：清不掉但总量仍守界可继续读旧有效缓存；超预算或扫描失败拒预载，保留占用与错误，恢复权限后可重新 Configure 收敛。清理失败保留实际占用；扫描失败显式标记 diskScanFailed 并拒预载/拒存，不将不完整采样当作空闲容量。worker 仍单线程，不增加并行临时份数；统计使用固定 atomic 计数，临时文件清单不常驻累积。正常写保旧 destination 直到原子替换成功。

`Diagnostics502()` / 新冷 endpoint `cacheStorageState()` 输出 T502_CACHE_STORAGE_V1：payload size/capacity、entries/pin、queue、temp bytes/files、最后磁盘 reservation、预算/queue/pin 拒绝次数、清理原错。T501 的 resourceState.cache 串行接到相同实账。diskPeakObserved 是扫描/预留请求的高水位，不是系统物理磁盘峰值；外来文件、journal、driver 均未冒实测。

Artifact/pin/receipt 与 context 有效句柄明确分账：报告 glHandleValidity=NOT_INFERRED_FROM_ARTIFACT_OR_RECEIPT；历史 compute metadata 保原有界行为，当前 GL 活跃/retiring 仍由 T501 owner ledger 判定，不凭历史收据宣布旧 context 可用。满pin/满队列/写失败不改变合法 GPU program、物理数据或业务成功条件。

必要非机：新增12组 store 检查覆盖 remove失败、重复失败不增长、128MiB/32key残留准入、启动拒绝后收敛、实例与独立新进程重启、满队列、4×32MiB pin及并发 Get；旧 T475 持久化套件和 T476 26/26 差分。实际 graphics 原13组加本卡3组（remove失败、queue满、pin满）验证合格 program 仍可 use；真实旧 compute binary 保原key暖命中。两路线生成 App 继续做 cold/warm Scene准备与账目检查，完整hvigor构建及签包verifyApp；证据见 `test/reports/t502/delivery.json`，复跑见 `test/t502/README.md`。

未使用真机，未修改 SOURCE/QUANTUM、渲染数学、GAP、deadline 或容量；产品前景背景仍为 XComponent+shader。ANGLE 只作离线设施。设备/driver长期证据按独立 tester 卡接续，不扩本卡为R2/R3冷协议改造。
