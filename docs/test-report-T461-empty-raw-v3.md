# T461 v3 · M2 空日志导出修复域真机验证（T579-m2fix 包）

日期：2026-11-18，tester，真机（011124264R000337）。
被测：T579-m2fix-signed.hap（t562-m2-fix，T551/T562 空日志导出兼容修复）。
依据：T547 §5（唯一空输入处理合同：0B raw 不调 md.update、返回数学常量
SHA256(empty)=e3b0c4…）。
证据：`test/reports/t491/t461b/`。

## 修复域真机 PASS（T521 缺口闭合）

操作链：冷启（bm clean -d）→ M2 页 → 沙 → **导出 M2 创建原始诊断** → 状态行
`CREATE_DIAGNOSTIC_EXPORTED`。

导出原件 5 份全部取回并逐文件 SHA 核验：

| 文件 | bytes | SHA 核验 |
|---|---|---|
| t468-create-1-…-0.log | **0** | ✅ = e3b0c442…（数学常量，未调 md.update） |
| t468-create-1-…-1.log | **0** | ✅ 同上 |
| t468-create-1-…-2.log | **0** | ✅ 同上 |
| …-native.json | 4778 | ✅ 与 manifest 记录一致 |
| …-manifest.json | 719 | ✅（native sha 记录一致） |

- 0B raw 不再触发 `update failed`（attempt 现场 CREATE_EXPORT_FAILED 缺陷消除）
- manifest 声明域 `status=DECLARATION_MISSING, source=COLD_WANT_EXTERNAL_RECEIPT`
  （packageSha/libElfSha 空）——按原合同如实，非本次修复域
- WS_EVENT t468_create 全链 hilog 在档（manifest sha=e2283ef3…）

## native.json create 域

bootSerial=1/previousSerial=0/epoch=3/window/ownerTid/eglContext/nativeBeginNs/
candidateRun=1 等域完整（nativeEndNs=0 与 NO_TRACE 口径一致，未完成 root 域维持）。

## 剩余域（维持）

- native.json 与 ROOT 断链（NO_CREATE_ATTEMPT→导出链本身已通；Create 真实完成域）
  ——维持 T521/T547 口径：不猜缺失 code/firstFailure，等 Create 后段证据（T561/T571）
- 设备 SHA 声明（DECLARATION_MISSING）→ 外部收据通道，归原卡后续轮

## 判定

- 空日志导出兼容修复：**真机 PASS**（修复域完整验证，无改写 PASS 到其它域）
- 本轮为 T579-m2fix 包首个消费验证；T545/T542 续测取 T579-m3diag/g9diag 包
