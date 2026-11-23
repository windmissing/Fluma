# T893 · Windows PC 离线测试交接

- A 路使用 `empty`、`gravity`、`roi`、`land` 判定空箱的 null root、跨几何代重力输入及约束角、同根 ROI 像素与 GPU 原始缓冲区、同根 LAND 事件与 Up 后质量账本。复用 T836、T883、T888 回归。T884 原记录的空箱帧计数为 0，ROI 和 LAND 仍缺证据。
- B 路复核 T886 提交 0e5e2f94 生成的两份独立静态库、角色清单、SPIR-V 和调度表。DM2 有 45 个计算角色，R1 有 49 个计算角色及 6 个图形模块。两份清单均记录了模块摘要。ROOT 和 FULL 分别核验两族 K192 的范围、分段长度、作用域、复制 fence、invalidate 及 FULL 预留，R1 另核验 phase witness 与实际呈现租约的连接。
- Python 编译通过。离线自测覆盖 A 路正例、两族角色创建夹具、四种冷读范围、R1 witness/lease 及 17 项定向负控。静态清单来自实际编译产物，角色创建及场景正例来自合成夹具。

从仓库根目录运行 `python test/t893/check.py selftest`。默认读取本地 `.codex/local-test/t886-dm2-release` 和 `.codex/local-test/t886-r1-release`，使用 `static --family dm2 --build-dir <目录>` 或 `static --family r1 --build-dir <目录>` 可改用其他位置的同一产物。

T890 采到真实证据后，按场景拆分 JSON 并调用 `python test/t893/check.py <empty|gravity|roi|land|cold|roles|r1-lease> <证据.json>`。其中 `roles` 需指定 `--family`。`a_cases()` 和 `b_cases()` 给出输入结构示例，其中 Vulkan 成功码、句柄、时钟和二进制数据均应来自真实记录。两族实际角色运行及真实场景取证状态为 `PENDING_T890`。
