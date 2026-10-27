# T161 GPU 修复后完整事务真机复测（tester 报告 v1）

日期：2026-10-27。角色：tester。真机 VYG-AL00（62T0225B10010020，Mali B318，GLES 3.2）。同机串行，未打断在途 run。
被测：T157 b21d0e5（wire echo R6/R7 修复 + half 避名 + 首次 GL 原码保留 + exec/consume/oracle 三分类）× T159 c8dff53（完整事务测试包）。消费 T158 修订导出器约定（L4/L5）。

## 结论总览

| 项 | 判定 |
|---|---|
| ① 信封消费（T157 修复闭环） | **PASS** |
| ② 三路完整事务收益 | **BLOCKED**（T159 生成项目 worker abc 打包缺口；stand-in 不冒 GPU） |
| ③ 故障恢复实测 | **部分 PASS**（首次 GL 原码/wait 数值/分类验证；quarantine/owner 耗尽/stale hi-word 注错 NOT_RUN，CLI harness 无对应注入点） |
| ④ 采集合规（T158 L5） | **PASS**（stdout 全部重定向归档） |

## ① 信封消费：PASS（决定性闭环）

T157 修复后的 reduce shader（R[6]=h50..53、R[7]=h54..55+DONE/0）真机执行，**T158 修订导出器**记录 10/10 样本：

- **T140-192 fixture（冻结无匹配）**：5/5 样本 `exec=OK, dispatchCount=11, pair=w[4..7]=[0,0,0,0], consume=NO_PAIR, agree=true, classification=EXEC_CONSUME_NO_PAIR, protocolPass=true`
- **T140-512 fixture（冻结 pair 16/342）**：5/5 样本 `exec=OK, dispatchCount=12, pair=[16,342,15,341], consume=OK, agree=true, classification=EXEC_CONSUME_OK`
- **合成池（有 pair）**：192→pair (4,132)/(3,131)、512→pair (2,194)/(1,193)，全部 `consume=OK, agree=true`
- **driver 端到端**：两池 `pairOracle=AGREE`（T141 三分类：`execStatus=OK / consumeStatus=OK|NO_PAIR / oracleVerdict=AGREE`），`rawGlError=0`、`waitReturn=CONDITION_SATISFIED`、`rawWaitReturn=37148`（首次原码保留）
- T142 的 F-T142-1（echo 错位导致 AGREE 永不可达）在真机上**正式闭环修复**；owner 严格校验（含 identity SHA 回显）在正确 shader 下通过。

## ② 三路完整事务收益：BLOCKED

T159 隔离项目（bundleName=com.example.helloworld，unsigned HAP SHA `f7b1baf3…` 与 T159 post-build manifest 一致）沿 T151 流程取得有效签包 `entry-default-t161-signed.hap`（SHA256 `92ef3d913144ca0d4a7a4e8ddf8b343faf7048ecb721aef1afb43c95ffc1c730`，未改密码、未用 unsigned/旧包冒新管线），安装成功。

**发现部署缺陷（交 developer/architect）**：ArkTS Worker 加载失败——设备日志 `zip_file.cpp GetEntry failed for ets/workers/T159PhysicsWorker.abc`。HAP 内 worker 源码已编入 `modules.abc`（`T159PhysicsWorker in abc: True`），但运行时按独立 `ets/workers/T159PhysicsWorker.abc` 路径查找且 HAP 无该条目；`build-profile.json5` 的 `sourceOption.workers` 注册了两个 worker 但 hvigor 打包未生成独立 worker .abc。Want 参数直传（`--ps/--pi t159nmax 192 …`）已按 EntryAbility 校验字段全量传递。**该缺口属 T159 生成项目打包层，未改生成器合同，登记交回**。

因此 LEGACY_SCAN/T123/GPU 三路完整 attempt 收益维持 **NOT_MEASURED**（T142 遗留项）；stand-in CPU 路由（T159 离线 9/9）单列，不冒 GPU 收益。

## ③ 故障恢复：部分 PASS

实测：首次 GL 原码保留（`rawGlError=0`、`rawWaitReturn=37148` 数值与字符串双录）、wait 1 次 `CONDITION_SATISFIED`、slot/epoch/identity 协议在 10 正常样本全过。
NOT_RUN（CLI harness 无注入点，如实登记）：quarantine→reset 恢复、owner 代耗尽、stale hi-word 注错、SHADER_SHA_MISMATCH（executor 有源码 SHA 校验但本卡未构造错 SHA shader 触发）。归 T162 或下轮设备卡补测。

## ④ 采集合规：PASS

全部设备输出经 `hdc shell "… > file 2>&1"` 重定向后 `hdc file recv` 归档（`diag_stdout.txt`、`diag2_stdout.txt`、`driver_stdout.txt`），非仅 hdc shell stdout；JSON 为 T158 修订导出器产物（首项无逗号、原始 JSON.parse 直接可解析）；**compile/link log：本轮 shader 编译一次成功无拒编**，成功路径的 compile info log 随导出器下一版本补常驻字段（本卡 stdout 未含 info log 文本，登记给 T162 采集项）。

## 同步成本（5 轮/池，T140 fixture，GPU pair 单链）

| 分项 ms | 192 p50 | 192 p95 | 512 p50 | 512 p95 |
|---|---|---|---|---|
| uploadCpuMs | 0.0055 | 0.053 | 0.0069 | 0.069 |
| dispatchCpuMs | 0.036 | 1.190 | 0.044 | 1.105 |
| fenceWaitCpuMs | **0.733** | 1.750 | **1.557** | 2.004 |
| mapCpuMs | 0.0014 | 0.003 | 0.0017 | 0.012 |
| requestCpuMs（单请求墙钟） | **0.810** | 3.144 | **1.635** | 3.307 |

GPU timer NOT_MEASURED（无 timer query 通道）；512 fenceWait p50 较 192 上升（0.73→1.56ms）——首次出现 N 相关性迹象，样本量 5 不足以定级，登记观察。

## 真实包源 SHA（归档 t161_source_sha.json）

driver `7f7c6e6b…`、diag `aca147e6…`、signedHap `92ef3d91…`、unsignedHap `f7b1baf3…`、三 shader（与 T157/T142 改名副本一致的正式版）、T140 fixtures 两份。

## 移交

- developer：T159 生成项目 worker abc 打包修复（hvigor workers 产物链）。
- T162：quarantine/owner/stale hi-word/SHADER_SHA_MISMATCH 注错实测 + compile info log 常驻采集 + 三路收益（待 worker 修复后的 HAP 路径）。
- 采用裁决回 manager：GPU pair 单链 0.81/1.64ms 与 CPU 权威单次对比需完整 attempt 数据支持，本卡不提出采用建议。
