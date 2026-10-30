# T313 · GPU 首 root 超窗首次归因与工程修订 v1

2026-10-30，architect；不使用真机。依据 T307 报告及 `test/reports/t307/part0_m0.ndjson`、`part0_m1_v2.ndjson`、两份 manifest，T295/T306 开发报告与 `entry/src/main/cpp/gpu_resident/{app_bridge.cpp,run.cpp,m1/run.cpp,control.cpp,bridge_timeline.h}`。M0/M1 冷出口 COMPLETE 只证明本次时间线连续可解，不等于产品链通过。

## 原 ns 相位

M0 epoch4 READY `2733085221216266`，FirstTick +2.5µs，首次 AnchorSet `2733085221222828`；其后两个约 8.1ms 的 Advance/Pump 无 root。真正提交前一轮的 AnchorSet 是 `2733085237433610`，PumpBegin `2733085237434391`，PumpEnd `2733085425237672`，RootSubmit `2733085425242203`：**Pump 调用内 187.794ms**，前一 anchor 至 RootSubmit **187.909ms**。报告的 204.0ms 用首次 anchor 作端点，包含两轮正常 Wait/Advance，并非单次提交耗时。PauseBegin `2733085433339703`，其 `delta=195907500ns` 从前一 AnchorSet 对应的取时算起，RootSubmit 后约 8.094ms；不是“提交后又挂起 195.9ms”。

M1 epoch4 READY `2733453380748922`，FirstTick +47.5µs、首次 AnchorSet `2733453380800953`；真正提交前一轮 AnchorSet `2733453390618453`，PumpBegin `2733453390618766`，PumpEnd `2733453468688297`，RootSubmit `2733453468694391`：**Pump 内 78.069ms**，前一 anchor 至 RootSubmit **78.076ms**。报告 87.9ms 同样从首次 anchor 起算。PauseBegin `2733453469306110` 的原 `delta=78688906ns`，RootSubmit 后仅约 0.612ms。

`app_bridge.cpp` 先在 `Advance` 成功后把 `last=time` 设为进入 Advance 前的 Stamp，记 AnchorSet，然后同步调用 `run->Pump()`；`run.cpp`/`m1/run.cpp` 在未 inflight 时调用 transport Submit/SubmitQuantum，返回后 `submitted++`，bridge **在 PumpEnd 后**才写 RootSubmit。故 RootSubmit 是“提交调用已返回”的观察点，当前 23 阶段字典没有 transport SubmitEnter/Exit、内部 GL/driver/初始化段，也没有系统调度/JIT 栈。两轮 WaitEnter→Wake 约 8ms 只证明这些已记录等待正常，不能证明 Pump 内全程无调度、无 JIT，亦不能宣系统挂起/切换为唯一根因。两份原件均无 accepted 后的 Draw/Swap，也无 source Offer/Consume→quantum→lease→同 token 上屏；accepted=0，完整链 `NOT_RUN`。

## 裁定与 developer 工程合同

确定的工程问题是**首 root 同步 Pump 工作跨过 33,333,333ns 时钟窗口，且现有阶段粒度将实际耗时混入“首次 anchor→RootSubmit”**；具体是 shader/driver 初始化、提交、CPU 计算还是调度停顿尚不明。GPU 优先派 developer 在 `app_bridge.cpp`、`run.cpp`/`m1/run.cpp`、M0/M1 transport 增固定容量、原 steady ns/TID/epoch/root 身份的 SubmitEnter/Exit，以及首次 shader/program/resource/fence/dispatch 必经边界和调用返回码；分清一次性初始化与每 root 工作、CPU wall 与 GPU timer，保持 T306 有界冷导出/原始字典版本化。核是否可把合法一次性准备放在 READY/首个 anchor 之前；运行中任何新 root 仍需完成预算内的实际提交，不能靠重置 anchor 隐藏长工作。若同步提交本身超窗，给出有界拆分或异步飞行的 owner/lease 时序方案并核不改变提交顺序、源接受→量子→审计→租约→同 token shader Draw/Swap 的原子性；无法在既有时钟合同内做到时回 architect 判设计分流。

不得钳 delta、补跑 quantum、跳审核/守恒、把长提交计为正常 tick 或用旧 token 伪装 CURRENT；真实超窗仍原值 CLOCK_GAP/撤权/新 run，失败 FAULT 或完整旧 token 显式 STALE。全部前背景仅新 XComponent+shader，禁止 Canvas。非机回归用同一 App loop/transport 替身覆盖 8ms Wait、延迟 Submit、延迟 Poll、初始化、旧 epoch/撤权和已提交未接受的 Pause drain；编译签包后交 tester 真机核阶段与完整产品链，不把真机加作开发关闭门。若新增最小探针仍不能区分 CPU 与系统停顿，tester 只需同 run worker TID 的有界调度/栈窗口，不重复相同冷导出。

本卡工程修订转 developer；尚无物理设计冲突证据，不先转 designer。T307 的冷导出 PASS 保留，产品 source→quantum→lease→同 token 上屏未通过。GPU 主线保持最高调度优先。
