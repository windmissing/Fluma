# T164 GPU 三路收益真机复测报告

日期：2026-10-27。任务：6ae017b3e4b0127832dccef8（tester，真机 VYG-AL00 62T0225B10010020）。消费 T165 e2163d4（`docs/developer-T165-worker-page-launch-v1.md`）。

## 签包身份

| 项 | SHA256 |
|---|---|
| unsigned HAP（T165 归档，本地字节级一致验证） | 49b45c024ea5935e96a306e16faf4572a654542cf72fcada95312e028c0c8db0 |
| **signed HAP（本卡产出）** | **3f5636cb357d2e44d7195021dacdfeb1b1c42b0d14e12c8b8fd0d756c8f1aea8** |
| sourceManifest | ed26af35c31751b057b5dcc9bb5128809ecd83801c9791b011bf4e3f2b2e174c |
| so | b53b415a3b9f84ce3801f976f41650f54d33b19abe04b67ac6f2c38797536524 |

签包流程：dev-cert-chain.pem + entry-default.p7b + OpenHarmony.p12 localSign（T151/T161 已验证流程）。安装成功，bundle com.example.helloworld（生成项目与主工程同名，安装覆盖主工程）。

## ① T165 候选修复设备验证：**PASS（worker 真机执行成功，F-T163-1 假设证实）**

T161 时同协议启动即 `WORKER_FAULT`（GetEntry failed）且无任何事务输出。本轮（页面+loadContent 双条件就绪后建 worker）：

- 日志零 `WORKER_FAULT`、零 `GetEntry failed`（worker 链）。
- GPU 路由冷启动 → `TRANSACTION_RETURNED`，JSONL 12 行（READY/BEFORE/2×SEED/32×STEP/AFTER/REQUESTS/CLEANUP/尾部元数据）。
- GPU 真实执行：3 请求全 `exec=OK, consume=OK, oracleVerdict=AGREE`，dispatchCount=11，rawWaitReturn=37148（首次原码保留），nativeSourceBuildSha256 与 T165 source SHA 一致。

## ② 三路完整 attempt 收益（nmax=192, steps=32, 同输入）

三路 AFTER poolBytes SHA 完全一致（8cbd96b7…，物理结果逐字节相同），对比有效：

| 路由 | attemptWallMs | serviceMs | queueMs | physicsTime p50 | 请求 |
|---|---|---|---|---|---|
| GPU | 825 | 808 | 17 | 21.0ms | 3 req 全 PAIR/AGREE，requestCpu 合计 3.45ms |
| T123 | 791 | 775 | 16 | 21.0ms | CPU 索引（无 REQUESTS 归档） |
| LEGACY_SCAN | 773 | 755 | 18 | 21.0ms | CPU 扫描 |

**结论：本负载（steps=32 仅 3 次 merge 调用）三路 attempt 墙钟差异 ~6%（791 vs 825 vs 773），GPU 请求级成本 3.45ms/32步 可忽略；无 GPU 收益信号也无退化。** GPU 收益要在大 merge 频次负载下才可能显现——当前 T159 事务模板 merge 触发稀疏（2 seed + 32 步仅 3 对），登记为模板局限（不改模板，属 worker 协议冻结）。512 池 attemptWall 2711ms（物理更重，与路由无关）。GPU timer 仍 NOT_MEASURED（无 timer query 通道）。

## ③ 512 fence N 相关性复核：**定级成立（24 样本/池）**

t164_diag（t142_device_diag.cpp 唯一改动 rep2<5→rep2<24，源 SHA 归档）OHOS NDK 交叉编译，设备直跑：

| 池 | n | execOK/agree/protocolPass | fenceWait p50 | p95 | min–max | requestCpu p50 |
|---|---|---|---|---|---|---|
| 192 | 24 | 24/24/24（EXEC_CONSUME_NO_PAIR） | **0.771ms** | 1.228 | 0.550–1.819 | 0.890 |
| 512 | 24 | 24/24/24（EXEC_CONSUME_OK） | **1.527ms** | 1.804 | 1.286–2.005 | 1.698 |

- 两池分布不重叠（192 max 1.819 与 512 min 1.286 仅个别交叠，p50 间距 2 倍）；时间序 Pearson r=-0.131/-0.306（弱负，排除会话内漂移）。
- **T161 登记的"N 相关性迹象"升级为成立**：fenceWait p50 随 N 近似线性（192→512 约两倍），与 upload/map 无关（均在 µs 级），主因是 fence 等待 GPU 完成时间随输入规模增长。
- T159BuildIdentity 期望 SHA（da4feecc/ca0ac154/3e5520d1）与 T157 修复版 shader 逐一匹配，diag AGREE 前置确认（设备 /data/local/tmp/t142/ 曾是旧 reduce cfff927a，已刷新后重跑）。

## ④ 注错四类：NOT_INJECTABLE（无外部注入通道，与 T161 结论一致）

逐项核过注入面：quarantine→reset 与 owner 代耗尽需 GL 错误/分配失败在 native 内部触发（无 CLI 注点）；stale hi-word 需改写内存中 request identity（worker 内部状态）；SHADER_SHA_MISMATCH 的期望 SHA 与被校验文件同源于 HAP rawfile（launcher 每次覆写 cache，外部篡改在 configure 之前即被覆盖）。**四类均需 native 接口扩展注错参数才可实测——属测试通道缺口，不是产品缺陷**；按红线不改业务，维持 NOT_RUN→NOT_INJECTABLE 登记，交后续卡评估 native 诊断注错参数。compile info log：本轮无拒编（shader 一次编译成功），无 info log 文本可采；T162 常驻字段建议仍待导出器下一版。

## 环境注记（不冒产品结论）

- 设备 /data/local/tmp 二进制执行需 selinux Permissive（hdc shell root 上下文 setenforce 0）；HAP 内 worker 运行不受影响，未改产品任何行为。
- 生成项目与主工程同 bundleName，安装覆盖主工程；测试后主工程 HAP 需要时可重装（本次未重装）。
- steps>32 会被 worker 协议拒绝（COMMAND_INVALID），有效上限 32。

## 结论

①T165 页面时机候选修复真机 PASS——worker 加载缺口（F-T163-1）证实为创建时机问题，GPU 链真机端到端打通；②三路完整 attempt 收益首次真机实测（此前 NOT_MEASURED）：本负载无显著差异，物理结果三路一致，收益评估受 merge 触发稀疏限制；③512 fence N 相关性以 24 样本/池定级成立（p50 翻倍）；④注错四类 NOT_INJECTABLE 登记通道缺口。**不宣称产品 PASS；GPU 三路收益的产品级结论留给采用裁决，本卡不提建议。**

原始证据：test/reports/t164/（archive.json 签包与源身份、5 份设备 JSONL、24 样本 diag JSON、diag 源码）。
