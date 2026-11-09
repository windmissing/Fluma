# T409 开发交付：同窗采集与 V2 绘制节流

2026-11-09，developer；依据已验收 T407 工程稿 §4。完成本卡实现、离线自验、Hvigor 编译及签包；不使用真机。

## 实现

- 页面实际 FrameCallback 先 pulse，再取一次 native snapshot；控制与文字消费同一份值。文字最多 30Hz，展开诊断 4Hz。计数另带自身 run/epoch/session/revision，失效后标 STALE，避免把旧 prefix 归给新 session。固定文字高度，采集按钮常驻诊断面板之外，避免操作改变 XComponent 几何。
- 页面 stop 仅跳过同身份且已确认关闭的重复请求；native 对重复关闭幂等，首次真实撤销仍执行。DOWN 记录有界坐标、页面与 native viewport、样本身份和拒绝原因，UI 接受不冒充物理准入。
- 仅 V2 在接受新 root 后按锚定 30Hz 节奏决定是否 Draw/Swap；首个立即尝试。跳过不 Acquire scene、不占新 lease、不形成延迟队列，也不跳物理接受。V1 保留逐接受 root 绘制路径。
- 原 permit/nonce/slot/generation/绝对 deadline 仍逐关核对；原 scene 在 Draw 前提供真实 requested token。Swap 原生成功但返回越过 deadline 时，保留原调用证据、拒绝 CURRENT。SOURCE BUSY 未消费 nonce 后切换 QUANTUM，必须按新 kind 建立自身意图；同 kind BUSY 不续期，旧 SOURCE 不能重用新 QUANTUM 许可。
- 三个有界采集入口及冷导出已落地，详见 [接口说明](developer-interface-T409-evidence-capture-v1.md)。只在无在途工作边界显式 arm；首错、到时、end 或换代冻结，frozen 后分页导出，不拼实时 status。采集中无 JSON/磁盘写入，GL 不等待采集锁。
- 每个采集 storage 定容 2,818,864 字节，仅显式 arm 冷分配；替换时新旧块可能短暂并存。4096 行和 1MiB 是分别作用于采集行数与整个冷档的上限，不能把冷档预算当 native RAM。关键事件及 END 有预留；不足即 INCOMPLETE。其它 root 只聚合，第一个 root 保声明的 selected-call 明细，至多 8 组完整 root/呈现身份。
- U64 输入先查询完整 UTF8 长度，拒绝超过 20 位及非严格十进制，避免 NAPI 固定缓冲截断误接受；配置 JSON 总长度先限 2048 字节再解析。

未修改物理 120Hz、真实回调时钟、GAP=33,333,333ns、GPU deadline=250ms、准入/守恒/容量阈值。全部渲染仍为 XComponent + shader，无 Canvas 或失败回退。

## 检查及证据

原始输出位于 `test/reports/t409/`；可复跑入口见 `test/t409/README.md`。

| 检查 | 结果 |
| --- | --- |
| 定容池、关键事件预留、预算不足、冻结分页及幂等 end | PASS；理想 120Hz 的 1200 次接受产生 300 次绘制候选 |
| 实际控制器 Node 单测 | 25/0，包括单 snapshot、首次 stop 与重复 stop、DOWN 身份 |
| 实际 App 控制/撤销及许可 kind 回归 | PASS，包括 10000 次重复关闭、首次撤销、busy/stale arm、冻结账本与一次性 fault |
| 实际 ANGLE App 混合沙水窗口 | PASS；2 个正准入 SOURCE、10 个 QUANTUM，接受总数 12、跳绘 9、呈现 3；原子整数账闭合，首 root 明细无丢失 |
| 实际 GLES Swap 成功、原 deadline+1ns 反例 | PASS；接受 prefix 保留，CURRENT 拒绝，原 requested token/lease/Swap 调用与首错证据保留 |
| 实际 ETS 冷导出及开发校验器 | 合成、混合、晚返回各正例通过，并各拒绝 9 类缺失/混身份/预算/游标/SHA 等反例 |
| 严格 U64（提取生产函数体，含真实截断语义桩） | PASS；最大值、溢出、超长、尾随和非十进制拒绝 |
| V1/GLES 原回归 | 两容量分别 53/0、55/0；run contract 12/0；App loop 11/0；生产 GLES App 6/0；bank/event/publication/completion parity 通过 |
| 原 V2 permit/clock 回归 | 5/40/65/200/250ms/+1ns、BUSY 原 deadline、UP 与独立 QUANTUM 全通过；100000 pulse 整数 oracle 通过 |
| Hvigor | clean 全部 30 task 执行成功；最终代码增量 PackageHap 成功，零错误 |
| 签名/验签 | PASS，最终签名报告 `test/reports/t257/T409-finalized.json` |

封存完整档：`test/reports/t409/mixed/` 共 198 行、7 组 token、251012 字节；`late/` 共 197 行、1 组 token、237745 字节。两档按 manifest 精确复制全部引用文件，并再次校验 SHA/字节/游标/首末账。原始同窗热路径日志与末尾显式 END 为不同时间点，不能要求热日志打印的行数包含之后 END。

这些 host 档标记 `HOST_SCRIPTED_OFFLINE`，场景经真实 App/ANGLE 执行，脉冲时间为脚本驱动；SOURCE 来自直接 Bridge 指令，不是实际触摸。水点 y=128，避免沙床增长后 y=0 已入床的合法拒绝。晚返回反例由 hook 将脚本时钟推进到原 deadline+1ns，期间注入健康间距 pulse。以上不能当作设备帧率、实际 250ms 延迟、用户掉沙或系统呈现证据；`systemPresentVerified=false` 保留。独立档检查器及额度 oracle 由 T411/T412 负责。

## 构建交接

最终签包：`D:\hmprojects\HelloWorld\.codex\local-test\t257-sign\T409-finalized\T409-finalized-signed.hap`，8541517 字节。

SHA256：`b27eb2fab5fd89a65cdc6cd9e4bf3873d0c8c0bfe47b71c6e0e0b9127024067d`。

构建项目 `.codex/local-test/t409-handoff` 中本卡全部生产文件与当前工作区逐文件核对一致。签包未安装设备。正式测试需使用该包实际 SHA，并按接口要求在稳定 viewport、正确实际 revision/run/session 下 arm；不使用本文 host SHA 或脚本时间冒充设备采样。
