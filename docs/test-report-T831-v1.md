# T831 v1 · Windows PC 硬件 GPU 及首业务帧独立验收设施

日期：2026-11-22，tester，Windows PC（无手机签装）。
设施：`test/reports/t831/t831-facility.json`。

## 1. adapter/driver 与非软件确认（现时实测）

- adapter：**ANGLE 2.1.22481 → Direct3D11**
- GPU：Intel UHD Graphics 630 (0x3E98)
- driver：31.0.101.2140
- 确认工具：t236_source_host（GL_VERSION/GL_RENDERER 原样打印）
- **非软件确认**：D3D11 后端=硬件路径（SwiftShader Vulkan 路径独立
  确认 UNAVAILABLE，排除软件栅格误判）

## 2. 非空 GPU 根/提交/像素/重放判据（登记）

- 非空根：GPU root commit 必须带非零 event arena（t236 原件
  completion[61]==262144 模式）
- 提交：Submit→Poll OK code
- 像素：离屏 pbuffer RGBA 读回非黑+已知色探针
- 重放：t280 归档确定性（full-192-64/128/512）

## 3. 负控（VERIFIED + 本会话 live 复跑）

| 负控 | 期望 | 状态 |
|---|---|---|
| fault-unlogged kernel | PROTOCOL_REJECTED+header 不变+bank 保持 | **VERIFIED** |
| conflict outcome2 | completion[64]==8 拒绝 | **VERIFIED** |
| unsupported opcode 11 | outcome==2 completion[64]==1 | **VERIFIED** |
| tiny sub-min | outcome==2 completion[64]==3 | **VERIFIED** |
| cancel quarantine | Stop==QUARANTINED+mappedBytes 5120 | **VERIFIED** |

**live 复跑**：foundation-O2 87/87 PASS、source gate
PASS_EXPECTED_UNSUPPORTED（rawGlError=1280）——负控现时有效。

## 4. 复用与分区

复用：T825 台账（adapter/分类）、T826 用例迁移（2-11）、T496 reader 链、
T724-726 offline replay+四臂设施。

分区：BOOT=PC 不适用（无手机 boot）；离屏=READY（pbuffer fixture 已验
证）；真实窗口=WINDOW_PENDING（需 GUI harness）。

## 5. 如实边界

设施已登记+负控已验证；**端到端待 T830 实际构建后另做**（按卡面）。
