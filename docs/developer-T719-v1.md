# T719 RX 配置票据与首帧停点

## T594-v5 原件

签包 `b4b4e7a376fc96a808a2a596170cafbc22f5867dea4d73e492b4962fde5d51ab` 的内嵌 RX ELF 与 T716 编译回执吻合，BuildId 为 `317645e31a3cd07e11e125f28b91b34601a0c820`，GLSL SHA256 为 `2cb9a84813833027f68ee7434f98f833dae01e310d9ea053e57e2870d133778e`。原工作路径的未签包和未剥离 ELF 的现存 SHA 已偏离旧回执，此次以校验通过的原签包为准。

`test/reports/t491/t594c/r1.json` 至 `r4.json` 只有 NAPI 同步返回的 `CONFIG_QUEUED`。报告描述重复点击后的 55s+ 窗口，布局无票号和 owner 状态，无法确定同一请求是否始终挂起。同期 hilog 的 7768 行中 S0001/S0022 均为零；当次实际加载身份、owner 是否取单及首错均缺原件，驱动编译结果未观测。Configure 路径由 UI 的 NAPI 调用进入 RX 原生 GL owner，冷档 Worker 仅由存档操作创建。

## 修复与分段

- 入队成功生成票号。票据在窗口/EGL 就绪和旧运行资源可安全释放后才出队，直接从原有 GL owner 应用。修复了上下文未创建时取单丢弃，以及等待资源期间旧配置覆盖新请求的窗口。已有待应用或应用中的票据时返回 BUSY，只有入队成功会停旧运行。页面离开时按票取消，避免遗留等待请求。
- EGL 创建失败保存原调用和原始错误并清理部分创建资源。对同一票据记录 enqueue、owner、apply、terminal 的单调时戳，以及实际 run/context、进程映射 BuildId、原始配置结果和 GL/EGL 错误。现有编译观察回调提供 shader compile/link 的进入、退出与原始状态；程序缓存命中可没有这两次调用。配置回执与后续渲染状态独立。
- 页面等待本票 `APPLIED` 后才激活 session 和提交 pulse，显示当前票据的等待或失败阶段。现场分别用 `G9RxConfigure` hilog 和 `g9RxStatus().configure` 取回同票记录。底层 GLSL、协议 ABI、预算门和显示后端沿用 T716。

## T713 交接

取 `test/reports/t719/compile.json` 指定的未签包，在冷启动 RX 页只点击一次启用。保存签包 SHA、同一 PID 下 `G9RxConfigure` 的票号与映射 BuildId。记录从 ENQUEUED 到 APPLIED/FAILED 的阶段、run/context 与原始码；若到原观察窗口仍未完成，保存 55s 截点及最后一条同票记录。若 APP 已 APPLIED，分别记录实际 READY 和首个成功 swap；若停在编译/链接，保留该票原始驱动日志（若设备返回）。开发侧完成 Hvigor、ARM64 链接与静态 GLSL 检查，历史 55s 的首错与首帧由 T713 真机补证。
