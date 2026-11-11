# T482/T483 · tester 验收复核（v1）

- 日期：2026-11-11
- 角色：tester（离线复核，不使用真机）
- 结论：**两卡验收通过**。全部自测独立复现，核心断言逐项核过原件；T482 修复边界经 GLSL 规范语义核对成立；Maleoon 真机终判如实 NOT_RUN。

## T482（coherent 值传参修复，commit f9e28ce）

| 复核项 | 结果 |
| --- | --- |
| check_source.py 复现 | 旧源违规 16 处（含 1138 m2KeyLess）、新源 0 处、barrier/memoryBarrier/coherent·volatile·restrict 声明逐一保留、finalQuantum 字节不变——全部 PASS |
| 判据语义核对 | `fullmatch(r'[CWE]\[.*\]')` 只计"整体下标引用作实参"——与 GLSL ES 3.1 规范一致（l-value→r-value 转换后值表达式如 `W[x]+1u` 已脱限定符，不属于可拒绝类）。tester 独立逐函数核：m2KeyLess/m2ReadSourceRow/seth/grsColumn*FloatBits/grsQ20FloatBits 全部 0 处 buffer 实参；mismatch 两处调用实参为已加载局部变量（expectedWord/actualWord），正则初判误报已排除 |
| 离线 GL 实证 | 新 SOURCE 在真实 ANGLE ES3.1 编译通过，三 program cold=PERSISTED / warm=BINARY_LOAD / warmCompileCalls=0 |
| 数学语义保留 | T452 四配置（192/64、192/128、512/64、512/128）SOURCE/replay/QUANTUM/负控 root 断言复用通过 |
| 缓存 key 失效 | old key d35cc73b…→new 4b181da7…，同环境/layout（SOURCE 字节变更即 key 变更，旧 artifact 无法命中）；HAP 内 ELF 与实际源码字节一致 |
| 签包 | T482-final-signed.hap SHA256=300bd52a…381e，signVerify=PASS |
| Maleoon 终判 | NOT_RUN 如实（真机重跑归 T461/T477 续测） |

## T483（来源一致性与恢复证据核对，commit 7d22485）

| 复核项 | 结果 |
| --- | --- |
| t467-unit vs t467-dev 差异定性 | unit 含 `replace?"WINDOW_OR_RUN_CHANGED":"RUN_RESET"` 三元分支；dev 无条件 WINDOW_OR_RUN_CHANGED——**T481 消费路径确为 t467-unit**（include 原件核对）；四签包输入含三元分支，dev 为旧中间目录不进签包。T481 报告的"差异观察项"据此定性为中间产物，非产品缺陷 |
| 生成器重生成一致性 | 四来源重生成与冻结目录逐文件规范化相同（bridge 原/规范化 SHA 双存） |
| 恢复差分回归 | 31/31 在本轮新生成 experience bridge 上重编译运行全过（recovery-diff.json PASS） |
| T473 READY 后 GAP 与 T477 coherent 分流 | T467 四包引用 m1，不能从 M2 SOURCE 编译错误推 M1 GAP 成因——**撤去"共同前置"表述正确**（T410/T437 可用 M1 来源继续，M2 修复非其前置） |
| T473 原件核对 | epoch4 readyEpoch=4 后首错 V2_PULSE_CLOCK_GAP 间隔 33,464,218ns>33,333,333 门限；事件环 incomplete+dropped，不推断单次调用因果——与 T473 报告一致 |
| T410 v4 声明修正 | SHA_DECLARED=旧 T450-t410 SHA，仅证输入通道不证安装 HAP；epoch4 对接 T473 READY 缺同 PID/run/context 关联——**修正成立**（tester 报告该段按 T483 修正采信） |
| install_input.py 修复 | 新 `--receipt` 用法：T467-t410+own receipt 通过（输出 Want SHA=8f1c45f7…签包自身 SHA，同时修掉 T410 v4 声明取旧 SHA 的问题）；旧 hap+新收据 → INSTALL_HAP_IDENTITY_MISMATCH 拒绝；混用 target → SIGNING_RECEIPT_TARGET_MISMATCH 拒绝；T450 旧默认用法兼容。6/6 |

## 给后续真机卡的精确输入（采信 T483 表）

- **T461/T477**：T482-final 签包（300bd52a…）先核 M2 SOURCE 创建，再续整根与三 program 冷暖主链。
- **T410**：`install-t410.json` 的 T467-t410 HAP + 完整 Want SHA（8f1c45f7…=签包自身），显式新空场，采同 PID/epoch/run/session 的 READY 与首错。
- **T437**：`install-t437.json` 的 T467-t437 双路 HAP；有效 READY 窗口内比较完整/前缀路由。
- M2 coherent 修复不再是 T410/T437 前置（M1 来源独立）。
