# T131：真机 compute 能力探针（GLES 计算着色器、SSBO 与同步成本）v1

日期：2026-10-26。角色：tester。任务卡 `6adf42e9e4b03273ad8646a7`（真机标签，同机串行）。执行器：`test/p013_native/t131_compute_probe.cpp`（测试专用独立 probe，不切生产物理、不覆盖其它 hunk）；交叉编译 `aarch64-unknown-linux-ohos-clang++`（OHOS SDK 26.0.0 native/llvm）；真机部署 `/data/local/tmp/t131/probe`；host 编译验证 `build_t131.cmd`。证据：`test/reports/t131/`（t131.json、t131_run5.json、t131_stats_summary.json、environment.txt、compute_shader.glsl）。

## 1. 设备与 GL 身份

| 项 | 值 |
|---|---|
| 设备 | VYG-AL00 `62T0225B10010020`（HUAWEI Mate 80，OpenHarmony 7.x） |
| EGL | `1.4 OpenHarmony EGL` |
| **GL** | **OpenGL ES 3.2**（B318.bd51c15da8175cd9bc398a845491808e549b4664） |
| GLSL | OpenGL ES GLSL ES 3.20 |
| ES3.1 兼容 | **是**（GL 3.2 ⊇ 3.1；major=3/minor=2 实测查询） |

**T116 §4 的疑问已闭合**：现有代码 `gl3.h`+major3 请求不能证明设备 ES3.1 计算能力——本探针直接证明设备真实支持 **GLES 3.2（含 compute shader/SSBO 完整能力）**。

## 2. compute shader 编译/链接/派发（真实执行）

- `#version 310 es` compute shader（`out[i]=in[i]*3+1 mod 2^32`）编译+链接 **OK**。
- 首版 `i < inBuf.data.length()` 在设备驱动（Mali B318）报 `S0001: Cannot compare 'uint' with 'int'`——真实驱动比桌面 ANGLE 更严格，需 `uint(length())`；已修正并归档 shader 源与 log。
- 结果与 CPU 精确期望**逐元素一致**（N=192/512/4096 全部 match=1），同 buffer 二次派发结果**可重复**（repeat=1）。

## 3. 上限（实测查询，非推断）

| 项 | 值 |
|---|---|
| MAX_COMPUTE_WORK_GROUP_COUNT | 65535 × 3 轴 |
| MAX_COMPUTE_WORK_GROUP_SIZE | 1024 × 3 轴 |
| MAX_COMPUTE_WORK_GROUP_INVOCATIONS | 1024 |
| MAX_SHADER_STORAGE_BLOCK_SIZE | 1,073,741,824 (1 GiB) |
| MAX_COMPUTE_SHARED_MEMORY_SIZE | 32,768 B |
| MAX_COMPUTE_SHADER_STORAGE_BLOCKS | 16 |

## 4. 微基准（整数 map，分列计时；5 轮 × 6 组，p50/p95 毫秒）

计时分列：CPU 上传（glBufferData）/ dispatch（CPU 提交侧）/ **GPU 完成（glFenceSync+glClientWaitSync，不用 glFinish 冒充）** / 读回（glMapBufferRange READ）。

| 组（N,冷/热） | upload p50/p95 | dispatch p50/p95 | **gpuFenceWait p50/p95** | readback p50/p95 |
|---|---|---|---|---|
| 192 冷 | 0.138 / 0.138 | 0.370 / 0.380 | **1.070 / 1.599** | 0.005 / 0.005 |
| 192 热 | 0.005 / 0.006 | 0.008 / 0.014 | **0.295 / 0.722** | 0.002 / 0.004 |
| 512 冷 | 0.135 / 0.147 | 0.008 / 0.008 | **0.297 / 0.690** | 0.002 / 0.003 |
| 512 热 | 0.004 / 0.005 | 0.005 / 0.007 | **0.271 / 0.322** | 0.002 / 0.003 |
| 4096 热 | 0.006 / 0.009 | 0.007 / 0.009 | **0.318 / 0.892** | 0.020 / 0.033 |

**关键观察**：
1. **同步读回成本约 0.24–1.6 ms/次**（含 GPU 完成等待），与 N 无关（192→4096 基本持平）——固定开销主导，**每子步一次 dispatch+readback 的模式会被同步成本吃满**（60fps 帧预算 16.7ms，若每帧 2 子步×读回即 ≈1ms，可承受；若 merge 每次事务同步读回则不可行）。
2. 冷启动首个 dispatch p95 ≈1.6ms（驱动首次提交路径），热路径稳定。
3. 计时 API：`steady_clock` 侧 CPU 计时 + `glFenceSync` GPU 完成侧有效（`glClientWaitSync` 返回非超时）；未以 submit 耗时冒 GPU 完成。
4. N=192/512 与 T116/T96 的 pool 容量同数量级，但本探针只测 map/reduce 微内核——**不能据此推完整粒子事务（sequential merge/预算/回滚）的 GPU 收益**，该判定归 T130。

## 5. 边界与移交

- 微内核通过 ≠ 完整粒子事务加速；生产 GPU 原型须按 T130 合同另派。
- 未动生产代码；probe 与 shader 源已归档（`compute_shader.glsl` + 编译 log）；设备临时文件已清理（保留原始 manifest 副本于报告目录）。
- barrier/共享内存仅查询上限未做压力测量；fence/readback 路径真实执行。
- 报告数字均可追溯到 `test/reports/t131/` 下 manifest 与 stats_summary。
