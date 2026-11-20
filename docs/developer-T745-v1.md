# T745 · DM BOOT 同次原件导出

2026-11-21 · 目标20 · developer · 离线交付。

`g9_dm_differential` 的 BOOT-only 构建连接实际 `libgrs_dm_t647.so`。驱动在独立 EGL pbuffer 上调用原 `Transport::Create`，由 DSO 在同一 owner 线程将 T734 原有事件直接写入 `execution.jsonl`。原 `T549_GPU_EXECUTION_V1` 首行、Create 返回码及 `rootsAuthorized=false` 仍供既有消费者读取。诊断接线只在此独立 BOOT 构建启用。原 shader、运算顺序、资格门、ABI 和 DM 实现指纹保持原值。

| 原件 | 同次定位方式 |
| --- | --- |
| 进程与独立 driver | `ATTEMPT_IDENTITY` 带 PID、`/proc/self/stat` 第 22 字段、driver 实际内存映射 BuildId、`/proc/self/exe` 以及 run/owner/context。每次启动仅执行一次 Create，ticket=1 与 PID/start 共同关联，冷暖各用新进程。 |
| 实际 DM DSO | `TRANSPORT_CREATE_ENTER` 从 DSO 内的地址读取已映射 ELF 的 GNU BuildId，和后续 `G9BootV1` 记录同 ticket、PID、scope。与编译收据中的 driver、DSO BuildId 分别比对。 |
| 等待与原值 | `FINISH_EXIT.fenceWaitRaw` 保留真实 `glClientWaitSync` 返回。未创建 fence 时该字段为 `null`。沿现有等待门，只有已信号且原 GL/EGL 检查成功才尝试 map。 |
| WORK 原尾 | `MAP_EXIT` / `UNMAP_EXIT` 带实际 WORK GLuint、byte offset、原返回和原 error/seal。K192 的 `WORK work[7].begin` 为 1,166,976；成功 unmap 且原 GL/EGL 检查通过时才写 `boot-work-tail.u32`，恰为两字共 8 B，小端。即使读到的 error/seal 不合格，仍保留有效映射取得的原字。 |
| GL/EGL | `gl`/`egl` 沿 T734 保留累计原值。`glSample`/`eglSample` 是既有错误检查在 fence、map、unmap 后分别取得的当次原值。 |

T738 使用[编译收据](../test/reports/t745/compile.json)中同一对 driver 和 DSO，按原 K192 fixture 在冷、暖两次启动各给一个新输出目录。保存完整 `execution.jsonl`、stderr、`boot-work-tail.u32` 及 T475 真正的缓存事件。原资格是 fence 完成、map/unmap 成功、GL 0、EGL 12288，以及实际读取 `[0, 0x641647]` 后通过原 Prepare。`BOOT_AUDIT_OK` 仍是 BOOT 诊断终态。开发阶段未签装或使用真机。

从仓库根目录准备隔离项目，在输出目录运行项目原 `hvigorw --mode module -p product=default -p module=entry@default assembleHap --no-daemon`。

```powershell
python test/t670/prepare.py dm .codex/local-test/t745-dm-rebuild --boot-only
```
