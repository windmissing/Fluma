# T139 compute 隔离 harness 修订：开发回执

日期：2026-10-26。角色：developer。不使用真机。

新建测试专用 `test/p013_native/t139_compute_harness.cpp`，保留 T131 原码和历史数据。每次 dispatch 后使用 `GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT`；fence 创建、每次 clientWait 原码/错误、1 秒 deadline、map/unmap、结果匹配逐阶段失败即拒读。原始 JSON 为每轮记录 run/build/driver/contextEpoch、输入 FNV64、资源阶段、CPU 提交/等待/映射时间；GPU timer 明确 NOT_MEASURED。首次 context/compile、每尺寸 buffer 首次和复用同一 buffer 的 warm 分列；第二个 context 重建资源并以 epoch 2 采样，旧 epoch 的资源不能被采样。64 位 SSBO 最大块、compute storage block 数与全局 binding 数分列。统计脚本只对有效样本求 nearest-rank p50/p95，失败样本及 outlier 候选保留原始身份，outlier 不删除。

离线验证：`build_t139.cmd` 编译通过；`run_t139_compute_units.cjs` 合同与失败样本过滤通过。本机 ANGLE 执行返回 `GLES_31_REQUIRED`，因此没有生成或声称 GPU 成本/同步实际测量；后续 tester 按设备队列采样。此卡未改生产 renderer/物理或任何 Canvas 路径。
