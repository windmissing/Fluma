# T833 Windows Vulkan G13 稳态程序资格阶段交付

x64 PE 新增 --qualify-steady 独立诊断入口，复用 T824 原 BOOT。五个 canonical 稳态角色生成 SPIR-V，静态校验通过。每个角色保 64 lane、四 storage binding、layout UBO、六个 push offset 和 FP32 的三项执行模式。完整输入检查 49,152 B 门，精确表达式包含原双轮 OP66 要求的 NoContraction。编译期用原 T753 映射和 T607 操作表检查 16 个 SOURCE、820 个 QUANTUM 槽及其非别名描述符选取。后者是计划校验，实际业务提交尚未执行。

同一 Intel UHD 630 上原 BOOT 通过，OP65 原 +53 为 1，OP66 掩码 [0,0]。首个稳态角色 OP_00 到达 vkCreateComputePipelines。首轮进程龄 271.56 s 和 305.90 s 取样，owner CPU 从 261.94 s 增至 294.53 s。复跑以实际 ENTER 细粒度采证，保 module SHA、PID、owner TID 和 QPC，窗口观察内没有原 EXIT 或 VkResult。原两个诊断进程均已清理，首轮没有 owner 退役回执。不能用这两次采样判定驱动调用内部具体原因。 [原进度](../test/reports/t833/steady-progress.jsonl) · [原始工件与取证](../test/reports/t833/receipt.json)

按 T830 编译流程构建，执行如下命令。

~~~powershell
.codex/local-test/t830-ninja/hello_world_pc.exe --qualify-steady
~~~

尚缺稳态角色完成原 OP66、Windows 同 scope 的真实 pulse/permit 接线、非零 SOURCE/QUANTUM 执行与 Completion256、ROOT/FULL 和 GridView。原 250 ms、33,333,333 ns freshness 和 T721 数值门保持。architect 先复核未闭合的驱动调用，再裁定角色编译取证或新的合规切片。目标14的可信速度、压力和黏性输入前置仍缺。
