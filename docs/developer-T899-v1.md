# T899 Windows PC 开发交付

## A · M1 持续落沙

提交 `1bc13152`。普通交互已有接受根时，恢复经原退役门、窗口与 bank 检查，再用新时钟会话的真实拍从最后接受的 bank 开始 QUANTUM。SOURCE 仍走原输入资格；恢复时清除旧输入授权及旧 grant。初次入料沿用原路径。

普通交互使用 `--m1-one-quantum-grant` 开启对照，默认上限仍为 4，单份 grant 的 250 ms 截止不变。对照记录同份 grant 已完成的量子、首次等待预算、提交和退役 QPC。用这些字段区分共用 grant 余额不足与独立根本身耗时过长。

## B · G13 OP17 取证复制

同一 B 构建以 `--g13-candidate FILE --g13-diagnostics hot` 为对照，再加 `--g13-slot-snapshot`。亦支持 `full`。OP17 的 ledger 原写集和首错记录照旧，新模块在原 ledger 末尾把同一时点的 32 个 latch 字和 10 个 Control 字写入专用 Work 尾区。前序 latch 验证失败时也捕获原槽内容。14 槽的归档偏移和冷档 Work 区边界保持原格式。

按 QUANTUM 命令录制计算，hot 路径的业务复制从 29 次降为 2 次，含 Completion 和合并的 2,352 字节。SOURCE 无 OP17 槽。业务截止、原验收门和退休诊断独立。超时根若在析构时确认同一 fence 退役，其槽证据使用 `T899_SLOT_EVIDENCE` 输出，且 `acceptedRoot=null`。常规槽证据仍走原热档/冷档。

## 构建和自检

| 构建 | 本地产物 | SHA-256 |
|---|---|---|
| A · Windows x64 Release | `.codex/local-test/t899-a-release.exe` | `c09e87faae0e432a9cf67e717a24d229fce2bc72a075e5d9beb77c7dd777da79` |
| B · Windows x64 Release | `.codex/local-test/t899-b-release.exe` | `d5f0b8bf8b850dc62d6e6992d5474fcee52312f045ad4a7ca63bc0ed5c34d24c` |

两个构建复用 `.codex/local-test/t882-shared-build-inputs.zip` 中的 8 个共享输入，逐项核对一致（忽略换行格式）。构建日志为相同目录下的 `t899-a-build.txt`、`t899-b-build.txt`。

- A · T899 grant 与 T894 退役、配额自检通过。
- B · T877 时间、真实拍时钟与绑定计划自检通过。T899 静态自检确认 14 个槽位、拒绝与完成两个采样分支、原有 SPIR-V 角色哈希一致。新增快照模块完成 SPIR-V 验证。
- 独立 PC 验证待测无新 Down 恢复、逐根配额、14 槽共 588 个原字的等价性、拒绝证据及 GPU 时延。复制次数来自录制路径计数。
