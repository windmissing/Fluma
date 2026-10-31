# T336 · T324 Amendment 2 实施交付（fence 停顿修复：分批 glFlush + 环扩容）

2026-10-31，developer；任务卡 `6ae55673e4b0f69bc5818507`。唯一合同依据 `architecture-T324-gpu-partial-submit-contract-v1.md` §7 Amendment 2（2026-10-31）+ 归因稿 `architecture-T335-fence-stall-attribution-v1.md`（MECHANISM_ATTRIBUTED / SINGLE_FIX_RECOMMENDED / AMENDMENT_2_READY）。纯代码+非机回归；真机验证归 T337（K1-K5）。不使用真机。

## 四件实施

1. **分批 flush（§2.2 修订）**：`full_transport.cpp` `ExecuteBatch()` 非末批路径（passCursor<total、lastBatchWallNs 采样之后）追加 `glFlush(); flushes++;`——末批（封尾路径）的 `glFenceSync→glFlush` 序原样保留（Amendment 2 明确不动）。机理对位：T335 归因机理 (a)（驱动在 sync 创建时实现累计未 flush 命令缓冲，296/325µs/dispatch 线性指纹）；flush 后 fence 面前残余降至单批 ~10 dispatch，预期停顿 37.65/41.28ms→~3ms、单 Pump ≈6.7ms<8ms 预算。flush 成本自然计入批 wall（INV-9，无 timeline 事件、不隐藏）。
2. **环扩容 1024→4096（T335 §4 裁定）**：`bridge_timeline.h` `capacity` 一处常数 1024→4096。依据：Amendment 1 使单 root 事件密度 50→376 行（7.5 倍，结构性后果非异常），多 run 会话在 1024 下必然溢出（T330 三 run 导出全 INCOMPLETE）；4096≈10.8 root 余量，+330KB 诊断构建内存。分页导出（4 行/页）与 fail-closed 语义零变化；不做自动导出清环。
3. **flush 计数新用例（替身计数）**：transport 新增诊断字段 `flushes`（`SubmitBatchFlushes()` getter；Create 与每 root SubmitQuantum 重置）——真实 GL 套件 partial_host 新增 **CASE_F**：断言 `flushes == batchesIssued-1`（192×64：13 批恰 12 flush；512×128：15 批恰 14 flush）、`flushes≥1`（flush 路径存在）、封尾后 Poll 接受（flush 不破坏完成链）。PC ANGLE 真实 GL 执行。计数落在 transport 内部（与 CASE2 的 Cancelled==1 先例同模式），真机侧零开销，同时充当 K1 诊断代理。
4. **INV-8 语义注释**：`CancelSubmit()` 头注释同步合同 INV-8——FENCE_PENDING×GAP 的 code=2 取消（删 fence+owner.Stop+unpin+IDLE）是完整清理；GPU 侧已提交工作随新 run 作废（anchor 窗统辖 run 存活，既设行为，T330 真机已验无泄漏无崩溃）；Amendment 2 落地后该路径转罕见，不删除——GAP 判定不豁免不放宽。

## 证据（设备 NOT_RUN；归档 test/reports/t336/）

| 套件 | 结果 |
|---|---|
| partial_host ×{192×64, 512×128}（真实 GL PC ANGLE） | **32/32 ×2**（T329 原 26 + CASE_F 新 6；CASE1 分片/trace 逐位/零 GL 错、CASE2/3 取消、CASE_N1 循环全保持） |
| run_contract（替身，单编译单元） | **12/12**（Amendment-1 六例全保持） |
| bridge（t324 版，STAGE41 断言） | **11/11**（M0 路径零回归） |
| app 全链（生产 App+transport+Run+shader+scene，真实 GL） | **6/6**（accepted=2/sourceCommit=1/physicsCommit=2/CURRENT/token 相等/rawGL=rawEGL=0） |
| cold units | **23/23**（含 T324 41 阶段字典三用例） |
| 生产 hvigor | BuildNativeWithNinja 实跑绿（libgrsm1.so 重编入 flush 修改，.so 11:05 > 源 10:59）；BuildJS/PackageHap 绿；SignHap 00303116 已知环境限制——**无签包**，T337 的包由执行 tester 按 T151 流程补签 |

CASE_F 实测（替身计数核心数据）：

```
192×64:  CASE_F batches=13 flushes=12 (non-final batches)   passed=32
512×128: CASE_F batches=15 flushes=14 (non-final batches)   passed=32
```

flush 数恒等于非末批批数（batches-1），末批零额外 flush（fence→flush 原序）——Amendment 2 §2.2 的结构判据在两配置一致成立。CASE_N1 循环墙钟 192×64：0.32ms/14 批（较 T329 的 1.97ms 更低，flush 使 SwiftShader 批尾更紧凑，无异常放大）；Poll 接受 physics=2 全保持。

## 修改文件

| 文件 | 改动 |
|---|---|
| `entry/src/main/cpp/gpu_resident/m1/full_transport.cpp` | ExecuteBatch 非末批尾 `glFlush(); flushes++;`（含 Amendment 2 机理注释）；CancelSubmit 头注释 INV-8 语义同步；Create/SubmitQuantum 的 flushes 重置 |
| `entry/src/main/cpp/gpu_resident/m1/full_transport.h` | `flushes` 字段 + `SubmitBatchFlushes()` getter（T335 注释：K1 诊断代理，非末批 flush 计数） |
| `entry/src/main/cpp/gpu_resident/bridge_timeline.h` | `capacity` 1024→4096（T335 §4 注释：7.5 倍密度重标定，+330KB 诊断内存，fail-closed/分页不变） |
| `test/t324/partial_host.cpp` | CASE_F 新用例（flushes==batches-1 / ≥1 / 封尾 Poll 接受，5 断言） |
| `test/t336/build.cmd`（新） | T336 五套重建脚本（无 vcvars——沙箱 reg.exe 黑名单；调用方须先 export INCLUDE/LIB/PATH，命令照抄本回执 bash 块） |
| `test/t336/fwd_extra.cpp`（新） | glUniform1f/4fv 转发补充（partial_host 链接单元需要，app_gl 自带故不参与该单元） |
| `test/reports/t336/`（新） | 五套原始输出归档 |

## 实施注记（源码现状对位）

- 归因稿 §3 规格写的实施点为「非末批（passCursor < total 返回 OK_SUBMITTING 的路径）在 lastBatchWallNs 采样后追加 glFlush()」——与现行 ExecuteBatch 结构逐字对位（return OK_SUBMITTING 前一行），零语义偏差。
- 环扩容后 bridge_test（t324 版）断言的 STAGE41 字典与 cold_units 的「1024 rows」用例均不受影响（cold 的 1024 是行数参数非容量常数）。
- 8ms 预算与 33ms GAP 窗零触碰；Amendment 1 六项已验证条款（nonce 一次消耗/carry 单调/取消无崩溃/submitting≠inflight/41 字典/逐批 ≤8ms）经五套全绿复证。

## 签名状态

**无签包**。生产构建止于 SignHap（00303116，keyPassword<32 环境限制，非代码问题）。T337 的设备包由执行 tester 按 T151 流程补签（Python 重打包绕 sign-tool 11017001 + localSign + verify-app，T328/T330 先例）。

## 路由

交付 → T337（tester 真机 K1-K5：fence 停顿 ≤5ms / submitPumpWallNs ≤8ms / accepted≥1+双源链 / 六项无回归 / 环 4096 三 run 导出 COMPLETE）；K1 若仍 >5ms 回流 T335 重估机理 (a) 摊销模型。
