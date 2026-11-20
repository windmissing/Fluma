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


## v3 追加（2026-11-20 19:45，effector；接续项完成——PID/映射/新进程暖证）

**RX 冷轮**（pid 43774，实际映射 BuildId f23b8bd6 hilog 验证）：19 阶段
全链（BUILD→COMPILE→LINK→REFLECT→ROOT_INIT→MAP→WARM→BOOT_AUDIT_OK→
FINISH），总时长 637.3ms。

**RX 新进程暖证**（pid 45505，force-stop 后全新进程）：阶段链
ENQUEUED→OWNER_SEEN→CACHE_PRELOAD→APPLY→BOOT_AUDIT_OK——**无
SHADER_COMPILE/LINK（缓存命中）**，总时长 **186.3ms**（暖比冷快 3.4x），
同映射 BuildId——**新进程暖命中确认**。

**DM 复跑**（dm20-out7）：CREATE_EXIT result=1638 BOOT_AUDIT_OK 复现；
driver execution.jsonl 尚无 pid/start/映射 BuildId 字段（插桩待
developer 补），证据来源 ACTUAL_GL_EXECUTION 保持。

**口径修正（接受）**：260KB 死/49KB 过为两点观测，单变量"大小唯一根因"
需中间尺寸探针——与次正规 FTZ 决策一并归 architect。

8B 尾 [0,0x641647] 语义仍待 T734 规范核。


## v4 追加（2026-11-20 22:55，effector；DM 字段缺口如实——tester 权限边界）

DM 复跑（dm20-out8）：CREATE_EXIT result=1638 BOOT_AUDIT_OK 复现。但
**driver execution.jsonl schema（T549_GPU_EXECUTION_V1）缺
pid/processStartTick/mappedBuildId/fence-8B 尾字段**——T734 新增
g9_boot.h（231 行）与 DM transport.cpp +44 行，driver 侧输出未生效。

**tester 权限边界**：driver 代码无权修改，DM pid/映射/fence 原件捕获需
developer 扩展 driver schema。RX 侧已完备（v3：冷 43774+暖 45505 新进程
暖证）。fence 后 8B [0,0x641647] 捕获同受此 schema 缺口阻塞（语义待
T734 规范核）。

SOURCE 前置：按 T736 各族同域原证——G13 SOURCE 被次正规决策阻塞
（T743 确认硬件 FTZ）；RX/DM SOURCE 配置未跑（本轮 BOOT-only 范围）。


## v5 追加（2026-11-21 07:30，effector；T745 driver 导出 DM BOOT 原件——**8B 尾 [0,0x641647] PASS**）

被测：T745 新 driver（g9_dm_differential 713344B buildId a2f4a69e + 
libgrs_dm_t647 5441272B，32f1061b 交付）。复跑 dm20-out9。

**schema 缺口关闭**：T745 driver 逐事件输出 G9BootV1 原件（ROOT_INIT→
WARM→FINISH(fenceWaitRaw 37146 fenceCompleted=true)→MAP→UNMAP
(workByteOffset 1166976 workTailBytes 8)→PREPARE→**BOOT_AUDIT_OK**），
pid 45665、**真实映射 BuildId 9b6cc802**、run 7/owner 1/context 11 全齐。

**8B 尾 [0,0x641647] PASS**：boot-work-tail.u32 导出，od 实测
**[0, 6559303] == [0, 0x641647]**——合同 MUST-PASS 项 PASS（v4 的 schema
缺口已由 T745 关闭；seal=6559303 与尾值互证）。

**卡面全项达成**：签包/BuildId 映射✔、pid/start✔、fence-8B 尾✔（DM），
BOOT_AUDIT_OK✔、暖命中✔（v3 RX）；SOURCE 前置按 T736 保持（G13 硬件
FTZ 阻塞）。
