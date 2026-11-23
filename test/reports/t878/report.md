# T878 目标13 PC 独显同工件对照准备与取证

- 任务：T878 目标13 PC 独显同工件对照准备与取证
- 角色：tester（PC 实测）
- 日期：2026-11-23
- 结论：**无可用合格独显工位** → 按卡面交确切前置，数值门不放宽，未等待 T877

## 独显确认（四路枚举）

| 枚举途径 | 结果 |
|---|---|
| `Win32_VideoController`（wmic/CIM） | GameViewer Virtual Display Adapter（虚拟）+ Intel UHD Graphics 630（集显，1 GiB，驱动 31.0.101.2140） |
| PnP Display 设备（含禁用态） | 仅上述两件，均 Status=OK / CM_PROB_NONE |
| PCI 厂商过滤（VEN_10DE/1002/1022） | **零 NVIDIA/AMD 设备** |
| `vulkaninfo --summary` | GPU0/GPU1 均为 vendorID=0x8086 deviceID=0x3e98 `PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU`，deviceUUID `8680983e-0200-0000-0000-000000000000`——同一物理 Intel iGPU 经物理+虚拟显示路径的双重枚举 |

Thunderbolt/eGPU 外接：无。

**结论**：本工作站不存在合格独显。T868 全部 QUANTUM 850 派发 250ms 超时均发生在这块 Intel iGPU 上；T873 预期的"目标独显环境"要求在本机无法满足，F02/F03/F05 数值级与撤权对照无法在本工位执行。

## 交付的接手前置（确切）

- **硬件**：任一合格独显 Windows PC（NVIDIA/AMD discrete，Vulkan 1.2+，timestamp query 可用优先）
- **PE**：`.codex/local-test/t833-candidate-release.exe`（SHA256 `78ee42e9` 系，与 T833 交接 `924cea58` 匹配）
- **冻结输入**：`test/reports/t868/fixtures/{F02,F03,F05}-config128.bin` 与 `-state.bin`（SHA 见 fixtures.json）
- **对照基线**：T868 八轮原日志（commit `0dc3761f`）
- **argv（成对对照同口径）**：
  - F02：`--g13-candidate <capture.bin> --g13-fixture F02 <F02-config128.bin> <F02-state.bin>`
  - F03：`--g13-candidate <capture.bin> --g13-fixture F03 <F03-config128.bin> <F03-state.bin> --g13-trial 1`
  - F05：`--g13-candidate <capture.bin> --g13-fixture F05 <F05-config128.bin> <F05-state.bin>`
  - 撤权正控：`--g13-revoke-before` / `--g13-revoke-after`
- **必记**：完整 argv、GPU/driver/device UUID、Vulkan 能力与 FloatControls、实际角色 SHA、QPC、真实原件及退出；首根 250ms 内通过与否只证该工件在该适配域可运行
- **数值门**：250ms deadline、33,333,333 ns pulse、Completion256/槽 latch 判据**全部原值不放宽**
- **T877 关系**：卡面明确不等待 T877；其 PE（`ce824652…`，`--g13-diagnostics full/hot` 两入口）在仓可供接手工位一并使用

## 证据

- `test/reports/t878/summary.json`（四路枚举+前置全档）
- `test/reports/t878/vulkaninfo-summary.txt`（原始 vulkaninfo 输出）
