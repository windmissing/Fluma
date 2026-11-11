# T476 · GPU 编译缓存独立持久化与失效差分（v1）

- 日期：2026-11-11
- 角色：tester（离线卡，不使用真机）
- 结论：**PASS**。文件层 26/26 + 真实 ANGLE ES3.1 GL 层 cold 3/3、warm 11/11、disk-corrupt 3/3、disk-truncate 3/3，合计 46/46。
- 代码：`test/t476/store_diff.cpp`（文件层）、`test/t476/gl_diff.cpp`（GL 层）、`test/t476/build_store.cmd`、`test/t476/build_gl.cmd`
- 证据：`test/reports/t476/`（receipt 原件 jsonl）、`.codex/local-test/t476-data/`、`.codex/local-test/t476-gl-data*/`
- 消费前置：T475 developer 交付（`docs/developer-T475-program-cache.md`），被测对象为生产源 `entry/src/main/cpp/gpu_resident/program_cache/{artifact_store.h, program_factory.h}`，未改动生产代码。

## 差分范围与判据来源

判据全部从本卡合同（T465 路由 + T475 卡面）独立推导，不 import 开发 `store_test.cpp` / `gl_test.cpp` 的断言；GL 侧直接挂生产 `program_factory.h` 于真实 ANGLE ES3.1（binary formats=1），无查询注入桩。

## 文件层（`t476store.exe`，26/26）

| 项 | 判据 |
| --- | --- |
| F1 | key 派生分离、64hex 校验、非法 key 拒绝 |
| F2–F5 | Put→WRITE_PENDING→Drain→PERSISTED；磁盘 wire=144B header（magic `T475B001`）+payload；8 线程同 key 并发提交共享同一 artifact 且字节不变 |
| F6–F7 | 空 payload、超 32MiB 单条拒绝 |
| F8–F9 | 重启实例读回 PERSISTED 同字节；启动清理 `.tmp` 残留 |
| F10–F14 | 坏 hash（DISK_ 前缀拒绝）、截断、坏 magic、header key≠文件名（copy+remove 等价实现）逐一拒绝；原字节还原后可载入 |
| F15–F19 | 故障注入宏：ENOSPC、部分临时写、sync 失败、`.tmp` 目录占位 → 均 WRITE_FAILED 且无半成品 `.bin`，既有 key 不受影响 |
| F20A–B | 同 key 有效 entry Put 共享不可变旧版本（PERSISTED 不触盘）；替换失败路径磁盘旧档逐字节完整（147/147） |
| F21–F23 | 条数 32 上限（含既有条目动态计数）；被 pin artifact 驱逐时保留；超 128MiB 总 payload 拒绝 |
| F24–F25 | receipt 历史滚动 ≤128KiB（1500 条写入后 87390B）；非法 key 收据计入 receiptFailures |

## GL 层（真实 ANGLE，`t476gl.exe`）

- **cold 3/3**：真实 binary formats=1；route=SOURCE_COMPILE；导出落盘 PERSISTED（3109B）。
- **warm 11/11**（跨进程，同一目录重启实例）：
  - route=BINARY_LOAD；**compile 时间戳未记录 = 暖阶段零源码编译**；环境指纹一致同 key 命中；
  - key 分离：代码 / layout / 环境 / link 任一变更 → 不同 key；
  - 真实驱动拒绝坏 binary（payload {1,2,3}）→ `REJECT_THEN_SOURCE_ONCE` + 真实源码编译一次（compileBegin>0）；
  - 未知 format 0xdeadbeef → 拒绝 + 源码回退；
  - 无效 shader：诊断收据、无可执行产物；
  - 既有产品 GL 错误以原码 GL_INVALID_ENUM 传播（不当 cache miss 吞掉）；
  - 原 context 存活时跨 context 导出拒绝（key 无产物）。
- **disk-corrupt / disk-truncate 3/3 ×2**（跨进程：python 改盘 → 新进程 Configure 读坏档）：坏 hash / 截断（50B）→ `DISK_*` 拒绝 + 一次源码编译（`REJECT_THEN_SOURCE_ONCE`）。

## 过程发现（差分器自身，非产品缺陷）

1. **同 key 共享语义**：对有效 entry 重复 Put 不触发写盘（返回旧 artifact，state=PERSISTED）——生产头设计行为，初次差分误判为"替换失败未生效"，按合同修正断言为共享语义；"替换失败不破坏旧档"由 `.tmp` 目录占位场景（F19/F20B）覆盖。
2. **磁盘损坏场景必须跨进程**：进程内改盘文件后内存 entry 仍有效（内存优先），单进程内不构成 DISK_ 拒绝场景——改为外部 python 改盘 + 新进程校验（与 T475 交付的跨进程语义一致）。
3. **receipt 队列上限**：一次循环 enqueue 400 job 超 128 队列上限会抛 `CACHE_QUEUE_FULL`（被 Receipt 计入失败，不崩）；差分器改每 100 条 Drain。
4. `std::filesystem::rename` 在此环境对刚关闭句柄的文件持续报占用——差分器用 copy+remove 等价实现（不影响生产头，生产写路径用 tmp+replace 原子替换，已在 F19 验证）。

## 缺口如实

- 真机侧冷暖启动（binary load 墙时、驱动真实格式）归 **T477**，本卡无设备结论。
- 生成应用六 program 全链（M1/M2route 实际入口接入工厂）已由 T475 developer 交付覆盖，本卡复用其生产头独立验证存储/工厂层语义，未重复执行六 program 端到端。
