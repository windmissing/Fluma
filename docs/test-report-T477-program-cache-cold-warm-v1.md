# T477 · GPU 编译缓存冷暖启动真机验证（v1）

- 日期：2026-11-11
- 角色：tester（真机 62T0225B10010020，设备串行）
- 结论：**阶段交付（部分覆盖）**。T475 签包 SHA 核验一致并冷启动成功；编译缓存文件层真机行为按合同取证（失败诊断收据/无 artifact/重启重试源码）；**三 program PERSISTED→BINARY_LOAD→compile0 主链被真机 shader 编译失败阻断**，且该失败产出了 T461/T468 一直缺失的关键根因证据。
- 证据：`test/reports/t477/device-cold/`（8 件：全部收据 jsonl、t468 创建原始 log、截图×3、布局×2）

## 已证

| 项 | 证据 |
| --- | --- |
| 签包身份 | T475-handoff-signed.hap SHA256=29ecdbde…785ec 与卡面一致；install 成功 |
| 冷缓存条件 | `bm clean -d` 清数据后首次启动（APPSPAWN 日志证实 shader_cache/cloud 目录缺失=真冷） |
| 缓存目录建立 | `files/gpu-program-cache/v1/` 自动创建 ✓ |
| **真机 shader 失败根因（关键移交）** | 收据 shaderLog 原文：`0:1138: S0001: Function call discards 'coherent' access qualifier.` ——Maleoon 驱动拒绝 coherent 限定符函数调用（GLSL 1138 行）。T461 SHADER_FAILED / T448 体验失败的根因族首次拿到原始日志（T461 时失败路径 log 为空，T475 ProgramFactory 补齐了 shaderLog 捕获） |
| 失败不缓存（合同） | 缓存目录仅 jsonl 无 .bin；`COMPILE_OR_LINK_FAILED` 收据含原日志 hex、reported/copied=67/68、compileBegin/End；无 PERSISTED |
| 失败重试源码 | 保留数据强停重启 → 新进程再次 COMPILE_ENTER（compile 总数 2，两进程各一次），不伪造热缓存 |
| 编译时长 | 进程1 20.6ms、进程2 17.1ms（单 program compile；环境指纹 env=5ef5dd69… 两进程一致） |
| T468 诊断通道 | "导出 M2 创建原始诊断"落盘 `t468-create-1-*.log`（shader 原日志原件）；"上一创建诊断" NO_PREVIOUS_CREATE（fail-closed 正确） |

## 阻断如实（NOT_MEASURED）

- **binary load compile0 / 冷暖墙时差（load/reflection/warmup/READY/首root/Draw）**：三 program（SOURCE/INTERMEDIATE_QUANTUM/FINAL_QUANTUM）的 PERSISTED 前提=Create 成功；真机 SOURCE 即 COMPILE_OR_LINK_FAILED（coherent 问题），Build 链在首 program 停止，后续 program 未尝试。无冷缓冲成功档，无法对比暖启动收益。
- **重签来源 key 分离/清数据二冷**：依赖成功 PERSISTED 档，同上阻断。
- M1 路径真机验证：T475 生成应用真机包仅暴露 M2 V3 入口（M1 检查为离线 fixture），无 M1 真机入口。

## 判定

- 不因首次 shader 失败伪造热缓存（卡面红线遵守）：所有失败如实记 COMPILE_OR_LINK_FAILED，无任何 PASS 冒认。
- shader 失败本身不是缓存功能缺陷：缓存对失败的行为（诊断收据+无 artifact+重启重试）完全符合 T475 合同。
- **接手角色：developer**——真机 Maleoon 驱动 shader 修复（移除/条件化 coherent 限定符）是全部后续真机验证（T477 主链/T472/T461 功能/T410/T437 体验链）的共同前置。修复后 T477 复跑冷暖主链。
- 根因证据同时回 **T468/T472**（T472 卡面的"确认错误 6 实际来源"已由本卡收据+原始 log 直接回答：错误 6=SHADER_FAILED=coherent 限定符驱动拒绝）。
