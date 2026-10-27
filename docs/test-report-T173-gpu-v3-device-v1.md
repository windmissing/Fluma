# T173 GPU 隔离 V3 真机收益与注错报告

日期：2026-10-27。任务：6ae06c77e4b0e6ce82e08214（tester，真机 VYG-AL00 62T0225B10010020，Mali B318）。被测：T171 961d837 生成工程（V3），T172 13/13 无阻断前置已关。

## 签包与身份

| 项 | SHA256 |
|---|---|
| unsigned（T171 生成器输出，字节级验证） | 0092232bfbfda83565f2318a050b7b9e10b053573c940b392746b77e620b1191 |
| **signed（本卡）** | **ebd7cdfe269c72469314a231bb5fff59c7a9fd5608fbeb8b0e2365100bfdd4c2** |
| buildManifest 参数 | 0092232b…（与 unsigned 一致） |
| sourceManifest | ed26af35… |
| 三 shader | da4feecc / ca0ac154 / 3e5520d1（与 T159_SHADER_SHA 期望一致） |

无权限/SELinux 修改；同 bundle 覆盖安装，**测试完成后已重装主工程 entry-default-s139h-signed.hap**。

## ① 正确性先行：PASS

- WET_192_64：GPU/T123/LEGACY 三路 TRANSACTION_RETURNED，selectorCalls=mergeCommitted=64，poolBytes SHA 全部 **0ee7cf0ba5664c26**——与 T172 离线结果逐字节一致（跨机物理等价）。
- WET_512_128：三路一致 **13f4cf38b4694027**（同上），128 merge，物理 attempt 2754–4109ms。

## ② 高 merge 交错三路收益（PERFORMANCE，99 run/fixture，全 TRANSACTION_RETURNED）

**WET_192_64（33 run/route）**：

| 路由 | physicalAttemptMs p50 | p95 |
|---|---|---|
| GPU | 1747 | 1785 |
| T123 | 1726 | 1760 |
| LEGACY_SCAN | **1427** | 1451 |

**WET_512_128（33 run/route）**：

| 路由 | physicalAttemptMs p50 | p95 |
|---|---|---|
| GPU | 4852 | 4885 |
| T123 | 5917 | 5951 |
| LEGACY_SCAN | **3959** | 3983 |

两 fixture 三路最终 poolSha 各自一致（0ee7cf0b / 13f4cf38）——交错执行下三路物理输出仍逐字节相同。

**GPU 请求级分项（完整计时分母，nativeTimings 32 槽）**：
- 192：2112 请求（64×33），requestCpu p50=1.581/p95=1.897ms，fence p50=1.353，prepare p50=3.112，bridge p50=5.0ms；缺测 null=0。
- 512：4224 请求（128×33），requestCpu p50=1.649/p95=1.889，fence p50=1.405，prepare p50=5.367，bridge p50=7.0ms。

**收益结论（如实）**：本高 merge 负载下 GPU 路由未显现收益——192 时 LEGACY 最快（-18% vs GPU），512 时 LEGACY 仍最快（GPU +23%）；GPU 每请求 requestCpu ~1.6ms 与 prepare ~3–5ms 是主要成本，fence 仅 1.4ms 上下。**这是本实现链（含 NAPI 序列化/bridge ~5–7ms/请求）的条件结论，不是"GPU 无收益"的普适判定**；分项归因（prepare hash-pack 与 bridge 序列化占大头）已可定位优化层。gpuTimer 仍 NOT_MEASURED（无 timer 通道，fence 只称 CPU 等待）；UI 竞争 NOT_RUN（未实现，不冒结论）。

## ③ 四类真实注错与恢复：全部 PASS

| 类别 | 观察 | 恢复 |
|---|---|---|
| QUARANTINE_GL | observed=true：exec=GL_ERROR、rawGlError=1280（真实原码非合成）、quarantined=true、errorCode=TEST_ILLEGAL_UPLOAD_TARGET | 第二请求 BINDING_UNAVAILABLE（无消费）；reset live=0/RESET；COMPLETED |
| STALE_REQUEST_HI | observed=true：identity 校验前拒绝 REQUEST_CONTEXT_OR_INPUT（consume NOT_RUN，不冒"消费期拒绝"） | reset 确认；COMPLETED |
| STALE_CONSUME_HI | observed=true：真实执行后 verifyAndConsume STALE（exec OK） | 第二请求 BINDING_UNAVAILABLE；reset；COMPLETED |
| SHADER_SHA_MISMATCH | 本 run 私有副本 +1 字节：program0 expectedSha=da4feecc… actualSha=bd3a7e3d…，compile/link NOT_RUN（SHA 早退，未调 GL） | recovery 目录原副本重 configure，ordinal1 TRANSACTION_RETURNED；COMPLETED |
| OWNER_EXHAUSTION | 专用 allocator DEDICATED_EXHAUSTION_INSTANCE 冷置 UINT64_MAX，倒数代成功；reset 后 configure 真实 OWNER_GENERATION_EXHAUSTED，MAX 不清零 | 输出标 NEW_OS_PROCESS_REQUIRED_NOT_ENV_RESET；COMPLETED |

## ④ 受控规模微测矩阵（MICRO，6 格 24 稳态）

162 请求（18 warmup + 144 稳态），6 格全 24 样本、status 全部符合族：

| N | 族 | fence p50 | fence p95 | request p50 |
|---|---|---|---|---|
| 192 | PAIR / NO_PAIR | 1.092 / 1.113 | 1.555 / 1.317 | 1.246 / 1.299 |
| 256 | PAIR / NO_PAIR | 1.086 / 1.042 | 1.349 / 1.172 | 1.250 / 1.237 |
| 512 | PAIR / NO_PAIR | 1.187 / 1.237 | 1.406 / 1.461 | 1.348 / 1.475 |

192/256 差异微小（同 K256/h8），512 略升（K512/h9）——与 T164 两点观察方向一致但**不据此定线性**（合同 §4：只答条件差异，弱相关不排除漂移）。微测分母与完整事务分开记录，不冒三路收益。

## 采集合规

全部 stdout 经 hilog 归档（本报告引 JSONL 为设备原文件 recv）；10 份设备 JSONL 全量归档 test/reports/t173/；退出码全 0（COMPLETE）；签包/source/so/shader 身份分列 archive.json。

## 边界与结论

- 本卡为隔离测试包结论，**生产迁移未授权**；GPU 路由在本实现链的条件性成本已定位（prepare 3–5ms + bridge 5–7ms/请求），供后续优化决策；不据此宣称产品 GPU 收益或无收益。
- 真实注错四类首次真机实测通过（T164 时 NOT_INJECTABLE 的通道缺口已由 T171 注错接口补齐）。
- 失败上报：无。
