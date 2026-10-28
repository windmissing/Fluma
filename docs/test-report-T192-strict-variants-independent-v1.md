# T192 GPU 严格优化独立验证报告

日期：2026-10-28。任务：6ae1451be4b0c1a6bd55a65a（tester，离线）。消费：T191 049300e（`developer-T191-strict-build-v1.md` + `test/reports/t191/` 双 ZIP/delivery-manifest），按 T183 §7C。

## 执行器与独立点

`test/p192_strict_variants_independent.cjs`（6 用例）。独立性：

1. **独立标准 SHA-256**：本执行器内嵌 FIPS 180-4 直接实现（消息调度/轮函数/大端位长编码手写），与 node:crypto 跨 12 长度对拍（0/1/54/**55/56/57/63/64/65**/127/128/1000 + canonical 空参考 e3b0c442…）——不调用被测源做参考基准。
2. **双 variant 物理等价**：独立 runner 子进程（隔离模块缓存）对 STRICT_O0/STRICT_O2 两生成工程各跑三路（LEGACY_SCAN/T123/GPU 臂独立 min-id 参考），poolBytes hex 拼接 SHA 全部等于已知基线 **0ee7cf0ba5664c26…**（与 T172/T173 跨机基线一致）。
3. 数据源独立读取：flags 从实际 `compile_commands.json`（clang++ pair_binding_v1.cpp 命令行）核验，非引用开发声明。

## 结果：6/6 PASS（原始 JSON：test/reports/t192/independent.json）

| # | 场景 | 结果 | 关键证据 |
|---|---|---|---|
| 1 | 独立 SHA-256 边界 | PASS | 12 长度全 match（含分块跨界 55/56/63/64/65 与 canonical 空） |
| 2 | O0/O2 字节等价 | PASS | 双 variant 三路 pool SHA = 0ee7cf0b…（GPU 臂 64 calls 确认 stand-in 执行）；host doubles 证据，非 ARM/GPU 执行 |
| 3 | variant 身份 | PASS | BuildIdentity 导出 `T191_BUILD_VARIANT='STRICT_O0'/'STRICT_O2'`；交付 unsigned HAP SHA 异（df833640… vs 7a574b52…），source SHA 亦异（声明一致） |
| 4 | 实际编译 flags | PASS | compile_commands：O0 得 `-O0`、O2 得 `-O2`，两者均保留 `-fno-fast-math -ffp-contract=off`；无 -Ofast/-flto/-DNDEBUG 差异（宏仅 source literal 与编译器内建） |
| 5 | 物理树白名单 | PASS | 65 个 .ets 中 O0/O2 与 V3 参考逐字节相同；差异仅白名单身份文件（T159BuildIdentity/T165PageLauncher/T171Protocol/T171Micro——variant 配置身份） |
| 6 | 边界声明 | PASS | 交付文档明确：设备运行/GPU timer/性能收益/热状态 UI 竞争 NOT_RUN/NOT_MEASURED；NAPI/executor host doubles 非 ARM/GPU 证据；signed=NOT_BUILT |

## 消费拒绝相关边界（§7C 清单覆盖情况）

- **stale/owner 耗尽/overflow**：V3 协议与 T159/T171 binding 层逻辑字节未改（物理树白名单核），T172/T175 已独立验证的拒绝/恢复语义在双 variant 下结构不变；host doubles（t171_binding_test MSVC /Od /fp:strict 与 /O2 /fp:strict 双档）T191 交付内含，本卡复跑确认基线。
- **RN64 半径/signed-zero/subnormal/tie/hi-word/detached/尺寸**：属 prepare/consume 层，被测源字节未变（白名单核）+ T172 15/15 独立反例已覆盖同源逻辑；本卡不重复宽矩阵（窄回归原则）。
- **无法在离线覆盖**：设备 GL 执行、真实 GPU timer、收益——全部归 T193 真机。

## 结论

双 STRICT variant 构建（O0/O2 严格 FP）在独立 SHA 实现、独立物理运行、独立 flags 读取下验证**源级/字节级等价且身份正确隔离**；T191 交付边界声明与实际一致。**无阻断，T193 真机对照可执行**。失败上报：无。O2 仅隔离常数项对照，不替代整体复杂度方案，不宣产品收益。
