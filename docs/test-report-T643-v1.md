# T643 v1 · 目标 13 正式新包身份与配置首失败独立补证（T638 交付消费）

日期：2026-11-19，tester，真机（011124264R000337），设备串行。
被测：T638 f1145cf 正式交付包——`t638-delivery` unsigned
（SHA 2008c704f35b709e…与 development.json artifacts 逐字一致）→
T643-g13-signed.hap（8e5cfac9f6453e1b…，sign=PASS verify=PASS）。
证据：`test/reports/t491/t643a/`（init-diagnostic.json / after-configure.json /
maps-g13.txt）。

## 身份链（unsigned→签名→ELF→加载映射）

| 环节 | 值 |
|---|---|
| unsigned HAP | 2008c704f35b709e74fcd61f8a9141f0e355b0611f9cfe5270bac1a2d35b7eaa（=development.json 记录） |
| 签名 HAP | 8e5cfac9f6453e1b7713ea6d0c53af4a44aec53238ec8d873af451d970997ed7 |
| 包内 libgrs_g13.so | 403bb9bb1ae3025d…（1,349,464B） |
| 设备加载映射 | /proc/25272/maps：**5 段装载，inode 57371，vaddr 5a30640000** ✓ |
| 关键导出符号 | InitDecode / InitDiagnostic::Decode(Ticket) / InitDiagnostic::Stage /
  InitStageEvent（**T638 新阶段 seam 与 ConfigTicket 已在正式包内**）+ Controller 全族 |

## 一次真实 g13Configure（首解码/首配置）

页面 T638 新诊断 payload **G13InitDiagnosticV1**（32768B raw）实测：

| 字段 | 值 |
|---|---|
| currentLifecycle | UNCONFIGURED →（配置 REST 后）**CONFIG_FAILED** |
| firstConfigFailure | **CONFIGURE_ERROR_29**（attempt 232，首失败锁存 ✓） |
| lastProgress | **CAPABILITY:EXIT**（capabilities 探针已完成） |
| queuedTicket/handledTicket | 232 / 0（ticket 防跨采样拼装机制在档 ✓） |
| dropped/truncated | 0 / 0 |

error 29 语义（transport.cpp:87/97/109/166）：capabilities 硬限
（blocks<4‖bindings<4‖invocations<64‖precision<23‖group<64‖shared<4096‖
maxBlock<ww*4）不满足、或 program Build/Reflect 失败——lastProgress 显示已过
CAPABILITY:EXIT，即卡点在 PROGRAM_BUILD 域（GL 硬限或 shader Build/Reflect）。

## 判定

- **配置首失败原件已固定**（按卡面"首解码未过立即保原错不空等 60s"——本卡未空等，
  原错 CONFIGURE_ERROR_29/attempt 232 在档）——**回 architect 精确归因**：
  PROGRAM_BUILD 域失败（GL 硬限或 shader 编译），非 T609 v1 的
  CONFIG_PENDING 恒挂死（T638 诊断改进后错误显式可见，生命周期推进到
  CONFIG_FAILED）
- 未假 driver 因（卡面禁止）；未向 Ready 推进（首失败即停，15s 窗口不适用）
- 本卡与 T609 完整物理/T610 观测共享原件，不重复另起同证
- 激活/pulse 安全门未单测（首失败域，激活不改变首失败锁存）
