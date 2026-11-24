# T926 · Demo 退役恢复与吞吐候选

## 开发交付

- 普通 PC Demo 的已提交拒根在首次同 fence 等待超时后，由原 owner 保留设备、队列、fence、geometry、nonce 和原在途资源。后续按最多 20 ms 一批等待，诊断窗口 2 s。旧根仍受原 250 ms 业务期限和 freshness 约束。只在同 fence 成功、墓碑 Poll 回 STALE、owner Idle 且无冲突租约后，才允许进入新会话。设备丢失、对象失配、窗口撤销和持续超时均有原因码。
- 收据记录拒根、待退役、原 VkResult 和 QPC、墓碑退役、最后 accepted BASE 及最后呈现的 publication。握持状态由 Win32 UI 重验。Up、失捕、换料或换代后须新 Down。窗口标题提示退役中或退役故障。
- 紧凑档新增候选。每列库存改为一次扫池加一次扫列，保原各列容量、域外零库存、重复 ID 和来源审核。三套 bitonic 网络各在一个 workgroup 内运行，保原活槽、列、y、ID、slot 顺序。每 QUANTUM 派发计数从 127 降至 35，SOURCE 与独立 COMMIT/replay 的 SPIR-V 哈希与 T918 相同。
- 优化档可在一次呈现前推进最多四个已经发放额度的 QUANTUM。任何时刻至多一个计算根在飞。继续按每 Q 实际 1/120 s 记账。30 Hz 呈现节奏与真实 Q 计数分别记录，中间 accepted Q 有单独收据。逐根观测保一根一帧。
- `--m1-stage-timing` 同时记录 SOURCE、QUANTUM 与 graphics 的设备时标，关联 Q 的起止活沙/活水、事件计数、派发、拒根迟退役。设备没有可用 query 时使用已有 host QPC 证据。

## PC 交接

用 MSVC x64 开发环境执行以下命令重建工件。安装目录沿用本机 SDK。

```powershell
python pc/vulkan/build_t918_compact.py --assembled .codex/local-test/t280 --compiler .codex/local-test/g13-glslang/bin/glslang.exe --out .codex/local-test/t926-shaders --fast-candidate
cmake -S pc/vulkan -B .codex/local-test/t926-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release "-DG13_GLSLANG=$PWD/.codex/local-test/g13-glslang/bin/glslang.exe" "-DVULKAN_HEADERS=C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/native/sysroot/usr/include"
cmake --build .codex/local-test/t926-ninja --config Release
```

从仓库根目录启动。每次跑单独关闭窗口，启动脚本会记录 argv、工件 SHA-256、日志路径。将实际 GPU/驱动、显示模式、输入位置及种子/初态随运行记录。

```powershell
# T925 基线窗口和模块
./tools/start-pc-compact-demo.ps1 -Baseline
# 同一紧凑模块上单独对比退役修复
./tools/start-pc-compact-demo.ps1
./tools/start-pc-compact-demo.ps1 -T926 -StageTiming
# 比较候选；先固定每根一帧，再打开呈现分频
./tools/start-pc-compact-demo.ps1 -FastCandidate -MatchSingleQuantum -StageTiming
./tools/start-pc-compact-demo.ps1 -FastCandidate -StageTiming
```

使用 T920 的固定喷口、物料和长按序列。分轻重载记录首个可见堆、见堆后 60 s 持按、各 20 s 窗实际 Q/s、实收 SOURCE、活槽及同根画面。无源重床、活沙 0/32/64/96/120 可由测试侧用同种子准备。针对退役，区分首次等待即成功与首次超时随后成功。仅后一类可验证异步恢复，核对同 fence/nonce/geometry、前态 BASE、新会话、同根 SOURCE 与着色输出。待退役窗口的 Up、失捕、换料、窗口重建走负控。独立 PC 跑候选与旧版同输入，并对照实际事件、库存和 replay 原字，派发计数和上传量按各自合同核对。

Windows x64 Release 编译、三个紧凑档模块的 SPIR-V 静态验证、T894/T899 既有 host 对照通过。真实迟退役恢复、候选的同输入 GPU 等价、高负载各段设备时间、2–4 秒见堆及长时继续出沙由 PC 验证确认。当前 replay 仍用原实现，后续优化需先用新同根时标和实际事件量确定热点，再保持独立原字核验。没有启动 Demo 做端到端 PC 或手机测试。
