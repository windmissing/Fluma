# T693 Host trace 与双臂设施

2026-11-20，tester，不使用真机。沿 T684 的冻结 T492 host 夹具，保留原 52 项动作与外层 420s、业务 120s、排空 30s。二进制模式由 T693_BINARY_TRACE=1 显式启用。每臂独立进程，沿原 parent PID/start 绑定和 ActivePhase/watchdog。用 test/t693/run_pair.py <新attempt前缀> 串行运行，单臂未封口或动作检查失败就拒绝整对。

test/t693/trace.h 以 32B LE 行保存调用序、steady、OS tid、函数编号、edge、字典编号。函数原号、原 detail 字节按 hash 加原值匹配，同一字节串只存一份。预分配 append-only 行与字典，独立写线程保存带 SHA 的已提交块，不在每次调用中同步文件 I/O。容量耗尽不覆盖；尾缺时 reader 只读取有校验块的前缀。现有 wrapper 仍创建 detail 文本，live 二进制行中的空 stamp 表示使用行 steady 时间；实际声明动作时钟另保在业务记录中。API trace 分区 12MiB，后续完整 root 及业务分区仍按 T684 约束。

历史 t657v4off 原件 102733 行按实际 C++ writer 编码后逐条还原调度序、两个时刻、tid、函数号、edge及逐字节 detail。首次固定分区字典约 3.88MB 过早耗尽，重配行/字典配额保持同 12MiB。最终数据在 test/reports/t693/prefix.json。篡改行原字节后 SHA 验证拒绝。python test/t693/verify.py --decode <stem> 可逐行导出。

live OFF 首轮 81.922s、9/52 项动作，外部守护 TRACE_EVIDENCE_BUDGET。实际索引为 480 项，恢复 12930 行，原 trace 总 1400149B。根因是每 25ms 的稀疏提交导致保守索引 credit 先耗尽，改为最长约两秒的块写。旧失败原件与最新复跑分开保留。

业务协调器释放 scheduler 锁后再读 App，避免排空阶段嵌套两把锁。原动作表不改，SOURCE/RESERVE/END 跳过此前的统一全 idle 等待，并保存 checkpoint。第一笔实际外线程 SOURCE 在 ownerPending=1 后入队并出现接受观察。尚不能把 ownerPending 等同于确切的 in-flight bank/event 切点。

T692 在本轮交付（2351cf3d）。独立重放编译消费其生成 overlay 与真实 AcceptedRoot sink，预分配并保 16MiB root 上限。两臂均从新进程运行。可使用 test/t692/prepare_host.py 生成安全隔离树，执行生成 build.cmd 后以 T693_REPLAY=1、T693_BINARY_TRACE=1、T693_REPLAY_EXE=<该重放PE> 调用 run_bounded.py。runtime 明确将重放保存在独立目录，输出 metadata/原始 bank+event 和独立 1024B completion。拒绝项及首次失配保真实 result。

主 OFF/ON 均送达 52/52，独立正常退出，分别约 144.328s、139.813s，API trace 9,911,001B、9,893,138B。两臂同 PE/模块和缓存输入，动作字节一致，92 个接受根（2 SOURCE + 90 Q）逐根账本及 token 观察完全相同；排空终态除独立进程绝对 stamp 外一致。ON 留 55 原始完整 FROZEN 页。两个进程启动时的源码快照曾跨短暂编辑，PE 字节没有重编或变化，以相同 PE SHA 绑定本次比较。

重放 OFF/ON 均真实 92 根，raw bank/event/completion 完全等值，逐根按 nonce/来源与物理提交计数配回主臂；对照了实际原 map 的调用 ID、非空返回与同线程后续 unmap 成功，以及 T692 AcceptedRoot 1024B 完整原件。动作、接受账、token 和排空终态均对应。Python test/t693/compare_replay.py <replay-off> <replay-on> <main-off> <main-on> 复现。重放真实时间分别为 150.000/126.828s，保每根读取耗时。

仍保「看评论」。T684 强完整证据还缺生产原 map 现场的 GL buffer generation 及 offset/access 原参数同 accepted 时点完整自证。历史 wrapper 只记录 target/length，且以读取前 CallSeq.load 取号。现有逐 nonce 与字节配对保可验证的正结果，不能回填缺的调用边界证据。另 live 格式沿旧 detail 文本及业务单独时钟记录，未把三时域都绑定到每条 API row。需 architect 明确哪些身份仍需重采，以测试 wrapper 精确分配 callId/绑定代并同一新版主臂与重放复核。32cycle 另矩阵仍未测，设备未用。
