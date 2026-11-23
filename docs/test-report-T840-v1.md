# T840 v1 · PC 非空 dry SOURCE 与同根窗口呈现独立验证

日期：2026-11-23，effector（tester 兼），Windows PC（无手机）。
被测：T830 m1-source-window 原件（24 条）+ PE t830-window-final.exe
（9699313c 回执核验一致）。证据：`test/reports/t840/`（run1–3.jsonl
独立复跑、v1-checks.json 11 项、v1 判定）。

## 原件判据核对（11/11 PASS）

对 T830 m1-source-window.jsonl（M1_DRY_SOURCE_PRESENT_RETIRED 会话）
逐项独立核验：

| 判据 | 结果 |
|---|---|
| PE SHA 9699313c 回执一致 | ✅ |
| 适配器 Intel UHD 630（devices 枚举） | ✅ |
| **m1AdmittedQ = 131072** | ✅ |
| **m1Completion = 256 words** | ✅ |
| 同代 lease（generation 1） | ✅ |
| 中心像素 **0xff00ff00** | ✅ |
| Present 链零错误（acquire/submit/fence/present/imagePresent 全 0） | ✅ |
| acceptedRoot 非空（匹配 Completion nonce） | ✅ |
| 状态 **RETIRED** | ✅ |
| SPIR-V m1SourceModuleSHA **1912af9e** | ✅ |
| fence 74.596ms（冻结 250ms 内） | ✅ |

## 独立复跑（3 次，40–150s 窗口）

- **BOOT 窗口路径 3/3 复现**：16 阶段 + 精确像素 0xff00ff00 每次
- **M1 SOURCE 阶段未触发**：M1 需 `--m1-window <module> <candidate>
  <commit>` 三个 SPIR-V 组件文件——**本地缺失**（T830 生成后清理；
  1912af9e/b8f0e327/caf0614c 三个 spv 不在盘上）

## 诊断范围（按卡面）

指针、QUANTUM 与产品帧不在本卡范围（待 T830 实际产物）。

## 回执

- 原件证据链 **11/11 验证通过**
- 独立复跑：BOOT 窗口路径 PASS；**M1 SOURCE 独立复跑阻塞于 spv 组件
  缺失**——需 developer 重新提供三个 .spv（或保留生成产物）
