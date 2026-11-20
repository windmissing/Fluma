# T738 v2 · 目标 20 RX/DM BOOT-only 冷暖验证——**双族 BOOT_AUDIT_OK 首达**

日期：2026-11-20，effector（tester 兼，含出包），真机（011124264R000337）。
被测：T738-rx（inputSha b9b64ff9→signed 73027d13，buildId f23b8bd6）、
T738-dm（c88c3711→d6c4b2ca，driver ELF 637128B buildId a64de80e）——
T734 compile 回据两族，sign 均 PASS。shader 上限 49152B。
证据：`test/reports/t491/t738/`（l1/l2/d1 布局、h738rx2.txt G9BootV1 原件、
rx-cache/97ef 8 条 T475 记录+bin、dm20-boot.jsonl、v1/v2 判定）。

## RX 族：全链通 ✅

CACHE_PRELOAD→APPLY→**SHADER_COMPILE 0.2s**→LINK→**BOOT_AUDIT_OK**
（G9BootV1 原件：UNMAP_EXIT→PREPARE_EXIT→BOOT_AUDIT_OK，pid 1256/tid
1942/run 1/owner 1/context 1）→ PERSISTED binaryBytes=37965 + WARMUP_OK
（8 条 T475 记录）。

## DM 族：CREATE 首次返回 ✅

driver 独立进程（T734 ELF）：CREATE_ENTER → **CREATE_EXIT result=1638
state=BOOT_AUDIT_OK**（variant=BOOT_ONLY，sourceSHA 486538f1，
roleSetSHA f0f968d7，ACTUAL_GL_EXECUTION，Maleoon 935F/k192）——**DM
CREATE 历史上第一次返回**（此前所有轮次编译内静默死亡从未出 CREATE）。

## 根因实验完成（对照闭环）

| shader | 编译结果 |
|---|---|
| 260KB monolith（RX/DM/G13 全族） | 静默死亡，永不返回 |
| 49KB BOOT-only | RX 0.2s / DM CREATE 返回——**全过** |

**shader 大小=驱动中止根因**，受控实验闭环确认。

## 8B 尾 [0,0x641647]

RX PERSISTED bin（38109B）footer="spirv-binary"+pad；0x641647 于偏移
37916 出现但该处 8B=(6559303,787471)≠[0,6608199]——合同语义待 T734
规范核（可能为其它 fence 位置或大端），按原样记录。

## 合同边界

BOOT-only **不宣称产品 READY/首根/有效帧**（卡面明示）；PERSISTED 后
新进程暖载入与资源退役已具备条件（RX T475 已落盘）。
