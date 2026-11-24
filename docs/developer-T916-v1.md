# T916 · Demo 续沙与 G13 建管线对照

2026-11-24 · Windows x64 Release。工件 `.codex/local-test/t916-ninja/hello_world_pc.exe`，SHA-256 `6dd3378f5aa672234823b4297acf3d4fb4fba148adc4b693ab464df30306a002`。OP_48 的 SPIR-V SHA-256 为 `f01959cc0f2fdecff46c953cf6c6073498f53d91bec0ae7d900b35c4c18aceb`，同 T890/T909。

## Demo 同一次按压跨 CLOCK_GAP

普通 FullM1 已有接受根时，CLOCK_GAP 首先暂停 SOURCE 授权、撤销在途 offer 和清相邻拍锚。Win32 的真实 Down 身份和静止坐标留给新会话核对。旧根提交过时须同 fence 真退役，未提交时核 owner idle 且无 lease。新物理 baseline 后，UI 线程复核捕获 owner、Down、运行/几何、材料和坐标，分配新授权 epoch。复核所用拍及此前拍不得建锚。第一笔可用拍只建锚，后续同会话实际相邻拍沿原间隔、时效、配额和接受门。认领、提交和接受在窗口锁内复核授权。没有旧 offer 或暂停时长补发。

Up、越界、失捕、焦点丢失、换料、窗口换代和关闭均清除实按及恢复身份。D1 和不可恢复路径沿原撤权。用此前 T912 同窗口、同一 Down 持续 8 秒与原观察参数，将程序换成该工件并启用 `--m1-source-trace`。结合 `M1_HOLD_RECOVERY` 的 SUSPEND、RETIRED、NEW_BASELINE、HOLD_REVALIDATED，`M1_RESERVATION_*` 的授权 epoch、两端原始时标及 terminal root，以及 SOURCE 同根实际准入质量核对。复核期 Up、失捕和换料用实际输入触发。真实时钟间隙、同一次 Down 续沙和首堆验证由 tester/effector 采证。

## G13 OP_48 对照

在仓库根执行。每次给脚本新的结果目录。先做关 snapshot 的两旧 PE 同路由比较，再用同一份冻结的 T909 PE 做 snapshot 开关对照。两组均为 ABBA，新进程、同组内相同镜像路径及 CWD、同一 F02 输入、关闭 tail/seal。续用新 PE 的 `--g13-compile-trace` 记录实际 create-info、WORK 范围、模块及管线对象代、保留资格对象数量。

```powershell
$cfg = "test/reports/t868/fixtures/F02-config128.bin"
$state = "test/reports/t868/fixtures/F02-state.bin"
$t890 = ".codex/local-test/t890-observation-timing-release.exe"
$t909 = ".codex/local-test/t909-pc-ninja/hello_world_pc.exe"
$t916 = ".codex/local-test/t916-ninja/hello_world_pc.exe"
python pc/vulkan/t916_g13_compare.py pe $t890 $cfg $state OUT_PE --other $t909
python pc/vulkan/t916_g13_compare.py snapshot $t909 $cfg $state OUT_SNAPSHOT
python pc/vulkan/t916_g13_compare.py snapshot $t916 $cfg $state OUT_EXPANDED --compile-trace
python pc/vulkan/t916_g13_compare.py scope $t916 $cfg $state OUT_SCOPE --compile-trace
```

监督器记录经选择的 Vulkan 环境、实际 argv、PID/owner TID、QPC、CPU 工时、已加载映像及可识别的 loader/ICD/compiler 路径、SHA 和文件版本。沿 T854 在待返回调用的 20/45 秒取子进程栈，60 秒调用观察窗或 120 秒进程观察窗耗尽记 `CENSORED` 并保最后 ENTER。驱动缓存隔离标未知。业务根的 250 ms 和 33.333333 ms 时钟边界沿原值。仅在实测显示 snapshot 分化后再拆 WORK 扩容与 ledger；仅在关 snapshot 的旧 PE 分化后才开始用真实前缀二分。

离线检查通过。已跑实际 ingress 和拍门的主机断言，覆盖旧 claimed 撤销、迟到旧 epoch、跨会话、baseline 边界与新拍配对。监督器用模拟子进程验证 ENTER/EXIT、已加载模块清单和原件收据。实测 GPU A/B 及 Win32 端到端续沙另交 tester/effector。
