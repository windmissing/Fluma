# T366 developer 交付

`test/t366/generate.cjs` 生成隔离 V2 入口，并在帧预算冷出口追加 `hotApply`。细计时默认关闭。诊断页每 16 帧采样一帧，最近 64 个 apply 以帧、物理子步、attempt 和角色区分。六段互斥时间与 apply wall 闭合。T311 仍保留其原有 inclusive 计时。时钟粒度为整数毫秒，host 数值仅供验证分账。

四个固定工作快照在冷绑定时各做一次完整 alias 检查。热路仍先查 busy、尺寸、当前 canonical 绑定和长度，再确认已登记的视图。任一身份变化回原完整检查。K16、fallback、public exact70、审计顺序和回滚保留。工作快照仍承担各自的回滚、审计、事务证据或冻结职责，本轮保留 payload 复制。

离线与 `798d5d4` 对照 8/8，核 checkpoint F64、事件、返回与发布可读性，含空场、低载、完整 LAND、受限沉积、跨列、故障回滚、别名替换及 detach。N512 / L101 / Q2 / A0 每帧 R404。N512 / L134 / Q2 / A0 每帧 R536，覆盖原档请求量下沿。离线无法同时复刻原档的 L101 与 R534–606，两个夹具分别覆盖其存活量和请求量。已登记快照在接触样本中实测每次 apply 350–534 次快检，完整 alias 交叉比较为 0。诊断成本及真机收益交 T368。

Hvigor 隔离构建成功。HAP 签名和校验通过，SHA256 `b7b71466283b5d32a0a18f1a69543bcc3f587db78e24643e2131167d4a2d37fd`。包和签名回执分别位于 `.codex/local-test/t257-sign/T366-delivery/T366-delivery-signed.hap`、`test/reports/t257/T366-delivery.json`。

附加旧套件 field、hot、hot_patch、hot_sink、m2 通过。refund 的历史源码哈希断言与 hot_s4 的 full-lift-degrade（13 vs 0）在当前树和基线 798d5d4 各自复现同一错误，无新增差异。
