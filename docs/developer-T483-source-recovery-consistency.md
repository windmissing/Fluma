# T483 来源校验与恢复证据修正

2026-11-11，developer，不使用真机。按用户本轮指令直接处理。完整机读记录为 `test/reports/t483/delivery.json`。

T467 四个最终签包的 HAP SHA、签包收据、原构建 stripped ELF 与 HAP 内 ELF 均核对通过。experience 的实际签包输入目录为 `t467-unit`。该目录和 regular/t410/t437 最终源都有 `replace ? WINDOW_OR_RUN_CHANGED : RUN_RESET` 分支。T481 消费的 experience bridge 对应已交付来源。`t467-dev` 为旧中间目录，保留原件，其无条件 WINDOW_OR_RUN_CHANGED 分支未进入这四个签包输入。

当前生成器重新生成四来源，原冻结目录中全部 .ets/.cpp/.h/.inc 内容经行尾规范化逐文件相同。报告同时保存 bridge 原字节 SHA 与规范化 SHA。基础生成器新增复制四个共享诊断/缓存头文件，经 include 核对未被 M1 冻结源码引用；无新增编译单元。31 项恢复差分在本轮实际新生成的 experience bridge 上重新编译运行全过，结果覆盖显式 reset、退休、新 session、首 pulse、GAP、旧 token、RUN_RESET、期限及代际隔离。T481 原报告和原目录保留。

T475-handoff 的实际 bridge 引用 m2route，SOURCE SHA 为 eb479a7b…，与其 HAP ELF 中源码及 T477 收据吻合。T475 恢复 overlay 的 bridge 与 T482 新签包生成 bridge 内容一致；本轮 T482 修改的是 M2 SOURCE 的值传参边界。T467 四包引用 m1，不能从 M2 SOURCE 编译错误推出 M1 READY 后 GAP 的成因。

T473 的 boot-after-ready 原件中 epoch4 的 readyEpoch=4，随后原始首错为 V2_PULSE_CLOCK_GAP，观测间隔 33,464,218ns 大于门限 33,333,333ns。原件 recovery 事件环 incomplete 且已有 dropped，保留旧代事件，不能据此唯一归到某次 shader、flush 或 driver 调用。此前 epoch3 另有 V2_GPU_PROGRESS_TIMEOUT，必须按各自 attempt 和 root 记录。现有恢复协议按 fail-closed 执行；本卡不改变 GAP 或 root 期限。

T410 v4 的 layout 原件只读声明为 `63b0fd25e8476fd2bf792b142f63caf52b60c3bdf0f0a1363e74fa806c4fa706`，等于旧 T450-t410 SHA。T467-t410 最终签包 SHA 为 `8f1c45f711f1aaf9e07e6b68d65a8692ed5eaaecebffc54c5f7e60266bda39f6`。故该声明能证明输入通道，不足以证明安装了哪一个 HAP。T410 报告把同数字 epoch4 对接 T473 的 READY，也缺少同 PID/run/context 的关联证据。T437 历史报告明确是 T450-t437，其历史结论保持在该来源范围。

修复 `test/t450/install_input.py`，新增显式 `--receipt` 参数，按所选代际签包收据验证 HAP、目标变体及冷启动声明，保留旧 T450 默认用法。测试确认新 T467 包通过、旧声明拒绝、目标混用拒绝、T450 原调用兼容，共 6/0。脚本只输出安装/启动命令，未连接设备。

| 续测卡 | 精确输入与条件 |
| --- | --- |
| T461/T477 | T482-final 签包收据，先验证 M2 SOURCE 创建，再按原卡验证整根与三 program 冷暖缓存主链 |
| T410 | `test/reports/t483/install-t410.json` 中的 T467-t410 HAP 与完整 Want SHA；显式新空场，采同 PID/epoch/run/session/真实时钟的 READY 与首错，再执行双源/负控/30fps |
| T437 | `test/reports/t483/install-t437.json` 中的 T467-t437 双路 HAP 与 Want SHA；显式新空场，在有效 READY 窗口比较完整/前缀路由 |
| T473 后续 GAP | 分别保留 V2 UI/native GAP 与 job 期限原域，带首次 driver enter/exit；事件环缺失处不推断调用因果 |

T410/T437 可使用已交付 M1 来源继续其当前取证范围，M2 coherent 修复不作为二者共同前置。二者产品双源、收益及 fps 仍是 NOT_MEASURED。本轮新生成 experience 工程 Hvigor 30 任务执行、零错误，BUILD SUCCESSFUL。本轮完成来源工具修复、签包冻结源核对及离线恢复回归，设备安装身份与后续运行结果由独立 tester 接续。
