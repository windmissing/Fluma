# T786 v1 · 目标 20 G13 BOOT 缓存新进程暖命中真机取证

日期：2026-11-21，effector，真机（011124264R000337）。
前置：T783 定位——T760 同产品签包（com.example.helloworld，installTime
1795266764668）+ 同机同 key cache 在位（7 条目）。证据：
`test/reports/t491/t786/`（t786-cache.tar+cache/ 逐 key 审、warm-chain
.txt、warm-g9boot.jsonl 10 事件、diag 布局、v1 判定）。

## cache 原件审计（逐 key）

| key | role | persistence | binaryBytes | shaderSHA |
|---|---|---|---|---|
| 760d1593 | **G9_BOOT** | **PERSISTED** | 37965 | 8e0a89e6（T760 时代 BOOT） |
| 893544ba | SCENE_PARTICLES | PERSISTED | 4845 | — |
| 8f8f4ee9 | SCENE_FULL | PERSISTED | 4693 | — |
| e4ab06f6 | G9_SOURCE | CACHE_NOT_STORED | 0 | ee66f5f2 |

SOURCE 未持久化属预期（pump 从未达 commit）。头与 SHA 经 T475 jsonl 末
记录核验。

## 新进程暖命中（本轮核心证据）

force-stop → 全新进程 **pid 43232**（startTick 406010 存活时直采）：
```
ENQUEUED → OWNER_SEEN → CACHE_PRELOAD_ENTER → CACHE_PRELOAD_READY
→ APPLY_ENTER（READY 后 0.0ms = compile/link 0 已验证）
→ BOOT_AUDIT_OK（G9BootV1 10 事件，seal 6559303）
```
- **BOOT 角色 glProgramBinary/状态/反射走缓存**——无 BOOT 编译/链接相
  位，**暖命中确认**
- APPLY 后的 SHADER_COMPILE_ENTER 属 SOURCE/scene 域，非 BOOT
- miss/fallback：BOOT 角色**零**（干净命中）
- 原 FP32 终态：PUMP_FAILED code=5（scene/SOURCE 域，RX STALE 族——
  数值门域未变，与 T755 一致）

## 与 T770 共存

probe bundle（g13visibility）未装在设备（profile CA 门阻）——产品数据
与原件无覆盖冲突。

## 回执

BOOT 角色同 key 新进程暖命中证据完整（0 编译/链接+0.0ms APPLY 间隔+
全新 PID）；cache 原件审计 4 key 全记录。
