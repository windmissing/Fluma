# T825 v1 · PC GPU 执行与原 oracle 离线复用验证

日期：2026-11-22，tester，Windows PC（无手机签装）。
台账：`test/reports/t825/pc-gpu-ledger.json`（5 行）。

## 工具复跑与分类（mock / 软件 / 硬件 GPU）

| 工具 | 模式 | 结果 | GPU 类型 |
|---|---|---|---|
| t236_source_host.exe | PC_ANGLE 硬件 | **GATE PASS_EXPECTED_UNSUPPORTED**（sourceExecution=NOT_RUN） | **硬件**（ANGLE → Intel UHD 630 D3D11） |
| t236_source_host --swiftshader | 显式软件 | **EGL_INIT FAIL**（Vulkan 软件后端本机不可用） | 软件 oracle NOT_RUN |
| foundation-O2.exe | 单元 | **87/87 PASS**（FOUNDATION_ONLY_NOT_M0_PRODUCT，GPU NOT_EXECUTED） | CPU-only foundation |
| prepare_t279_quantum | 编译 | semanticCompile PASS（sha 54946590，CANDIDATE_ONLY device NOT_RUN） | N/A（仅编译） |
| t280 replay 归档 | 离线 | test/reports/t280/full-*.txt（历史 host 产物） | 离线 fixture |

## 已验证范围（真实接口未形成，如实）

- **PC 硬件 GPU**：ANGLE→D3D11（Intel UHD 630）可达，但 SourceGpuTransport
  在此配置下能力门判 UNSUPPORTED——SOURCE 真实 GPU 执行 NOT_RUN
- **软件 oracle**：SwiftShader 路径本机不可用（Vulkan 初始化失败）
- **CPU foundation**：87 断言全过（非 M0 产品仿真）
- **可复用 fixture**：t280 replay 归档（full-192-64/128/512）离线可用

## 台账登记

PC 驱动：ANGLE 2.1.22481 → Direct3D11（Intel UHD 630 0x3E98，驱动
31.0.101.2140）。台账 `pc-gpu-ledger.json` 5 行，区分 mock/软件/硬件。

## 回执

覆盖域：高度场/粒子/SOURCE/LAND/守恒/ROOT/FULL 中，仅 CPU foundation
单元域+离线 fixture 复用已验证；**真实 GPU 接口在 PC 未形成**（能力门
UNSUPPORTED）。与 PC 基线并行推进，不阻塞手机线。
