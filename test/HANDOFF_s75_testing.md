# s75 测试交接单（本会话=开发，测试由另一智能体执行）

日期：2026-10-12
交接方：开发会话（WorkBuddy 主会话）
接收方：测试智能体（另一会话）
滴答任务：6acc45f8e4b0fa0cce2b0912（标题「【待测试】renderFluid 体积守恒渲染修复 s75」）——完整测试执行包已写入该任务正文

## 一、被测对象
- entry-default-s75-signed.hap（已装真机 VYG-AL00，用户已关锁屏密码，hdc 直接可用）
- 修复点：SandCanvas.ets renderFluid() 分相计数渲染（修 VV-01 蓝相丢失）
- 修改文件：仅 entry/src/main/ets/pages/SandCanvas.ets 的 renderFluid()（305-445 行区域）
- 未动：CA 引擎 SandGrid、FluidField、传感器、手势、帧循环

## 二、测试数据（开发者自验产物，供对照，非测试轮数据）
| 数据 | 路径 | 说明 |
|---|---|---|
| 自验报告 | test/reports/TC-01..08_s75_verify_report.md | 开发者四项验收全过程 |
| 自验帧 TC-01..08 | selfcheck/tc_frames_s75_dev/（219 帧） | 2026-10-12 录制 |
| 自验帧 TC-05 60s | selfcheck/tc05v2_frames_s75_dev/（78 帧） | tc05_retest.py 产物 |
| 自验视频 | selfcheck/tc_videos_s75/（8 mp4） | 6fps 慤放 |
| 自验关键帧 | selfcheck/tc07_mid_s75.jpeg 等 3 张 | 已人工审看通过 |
| 判定脚本 | test/vv_s75_analysis.py | BGR 阈值已标定 |
| 构建日志 | D:/hmprojects/build_s75.log | SignHap 预期失败属正常 |

开发者自验核心数据：蓝漂移八用例全部 ≤±0.07%（阈值 ±3%）；TC-05 60s Δ=-0.13%；FPS 60。

## 三、历史基线（s74 FAIL 轮，修复前）
| 数据 | 路径 |
|---|---|
| s74 帧 TC-01..08 | selfcheck/tc_frames_s74_old/（219 帧） |
| s74 帧 TC-05 60s | selfcheck/tc05v2_frames_s74_old/（78 帧） |
| s74 报告 | test/reports/TC-01..08_s74_test_report.md |
| s74 判定脚本 | test/reports/tc_official_test.py |
| s74 蓝相时序 | test/reports/tc_blue_series.json |

s74 核心数据：TC-02 蓝 ±6.5% 丢 55.8%｜TC-03 ±7.1% 丢 52.9%｜TC-04 ±4.8% 丢 30.4%｜TC-05 丢 77.3%｜TC-06 ±4.6% 丢 36.8%｜TC-07 ±3.8% 丢 25.9%｜TC-08 ±2.3%。

## 四、测试执行指引
详见滴答任务正文【附：测试执行包】段（用例定义/工具链/已知坑/验收标准/报告要求）。要点：
1. 录制前将 selfcheck/tc_frames 与 tc05v2_frames 备份改名（上一测试会话已建 tc_frames_s75_dev 等备份，**注意区分：_s75_dev=开发者自验帧，_s74_old=s74 基线帧，均不得覆盖**）
2. 真机 /data/local/tmp 旧快照已清理（本交接单落盘前已删 tc01_*、a0* 残留）
3. 工具链与环境坑清单见任务正文，判定阈值可直接用 test/vv_s75_analysis.py
4. 结果写回滴答任务正文，报告文件放 test/reports/

## 五、本会话职责边界
- 开发会话不再执行录制/判定/报告（交接收方完成）
- 如测试轮发现问题需返工，回本会话修代码并出新版本（s76 起）
