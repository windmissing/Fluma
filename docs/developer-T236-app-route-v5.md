# T236 产品 M0 实验路由接线

2026-10-29，developer；不使用真机。

新增 `grsm0=true` 启动 Want 路由 `pages/GpuResidentM0`，单独 `libgrsm0.so`，默认路由不变。页面触摸/按钮发送完整 sourceCommand32，保留 run/sequence/input/geometry/time/seed；滑杆提交角输入。UI 只入有界 FIFO16 或读状态，`ENQUEUED_NOT_ACK` 明示不是准入成功。原 `ResidentRun` 再按 FIFO16 每 tick 最多一个 source、最多四个 physics root，`Pump` 一次只提交或零等待 poll 一个 root。

独立 native GL owner 线程创建真实 window EGL context，调用原 `SourceGpuTransport` 与产品 completion256 校验门，接受后才将 publication lease 交 `ResidentScene`；前背景/床/粒子全部同 token shader，实际 swap 成功才 CURRENT。输出 source/physics commit、requested/rendered token、raw GL/EGL，displayPresentVerified 始终 false。无 CPU 物理补跑和 Canvas。

窗口 create/change/destroy、page hide/show、Ability background/foreground 均递增 epoch 并清输入；旧 epoch 不能发布 CURRENT，swap 前再次核 epoch。callback 只保留 native window 引用并递交状态，不调用 GL。owner 退役整个旧 context，使未提交候选/lease/GL 名失效，重新冷开空 run；不从摘要恢复旧粒子。失败停在 FAULT，用户可 New empty run。当前 K192，M1 水/旧 channel 不启用。运行时线程以 steady clock 计时，不以后台墙钟补跑。

native+ArkTS+PackageHap BUILD SUCCESSFUL。`check_app_package.py` 核 HAP 内真实 libgrsm0、M0 产品代码、路由与 ABC，证据 `test/reports/t236/resume/app-route-package.json`；这是离线打包检查，不是窗口生命周期执行测试。沿授权签名流程 verify-app/profile PASS，签包 `test/reports/t257/T236-app-v1.json`，unsigned SHA `83dec7c2a3e1ddd5dc5fe9ec102d928ac509e23ad505466893bdf5cb6133064b`，signed SHA `18e4895bd46de6974242df588b36789cb9cc2cb2d5ad7483a37111755248b1cc`。

本切片交付真实产品接线源码/编译包。完整 T236 仍部分交付：末 ID 实际 shader fixture、全部故障矩阵/逐阶段完整计量尚缺。新桥窗口、背景/resize/迟到/lease 设备执行未做，须 tester 对该包取证，不将编译或旧 SwiftShader 证据冒窗口 PASS。
