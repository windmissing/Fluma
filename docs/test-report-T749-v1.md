# T749 v1 · 目标 20 RX BOOT 进程身份最短补证

日期：2026-11-21，effector（tester 兼），真机（011124264R000337）。
被测：同签包 T738-rx（signed 73027d13，映射 f23b8bd6）——单次 Configure
（合同：不重复冷暖轮）。证据：`test/reports/t491/t738/v6/`（g9boot-53051
.jsonl 19 事件、h-t749.txt 全 hilog、v6-t749-identity.json）。

## 前置检查

T738 RX 原件（v3 冷/暖 hilog）**无 processStartTick**——按合同触发最短
补证：同签包单次 Configure，进程存活时采 /proc stat 第 22 字段。

## 身份五元组（同次运行同票采集）

| 项 | 值 | 来源 |
|---|---|---|
| pid | 53051 | pidof |
| **stat 第 22 字段（starttime）** | **5620187** | 进程存活时 /proc 直采 |
| ticket | **1**（11 事件全一致） | G9RxConfigure 侧 |
| 映射 BuildId | **f23b8bd61d2c…**（19 事件全一致） | G9BootV1 |
| run/owner/context | 1/1/1 | G9BootV1 |

阶段链（单次运行完整 11 相）：ENQUEUED→OWNER_SEEN→CACHE_PRELOAD→APPLY
→SHADER_COMPILE→LINK→CONFIGURE_RESEARCH_EXIT→**BOOT_AUDIT_OK**（结果
状态行同步显示）。

fence：FINISH_EXIT raw=37146（与 DM 同型）、gl=0/egl=12288；seal=-1
本轮未产独立尾文件（RX 尾走 G9BootV1 内嵌字段）。

## 备注

RX G9BootV1 事件内无 ticket 字段（DM 版有）——身份四元组由 Configure 侧
（ticket=1）+Boot 侧（pid/映射/run/owner/context）跨源拼合，同票一致。

## 判定

RX BOOT 进程身份**最短补证完成**：pid+startTick+ticket+映射+fence 全部
同次运行捕获、采样期间进程存活。原 G9BootV1 19 事件落盘可追溯。
