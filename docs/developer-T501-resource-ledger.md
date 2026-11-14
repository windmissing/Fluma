# T501 GPU owner 资源账与退出首错回执

2026-11-14，developer，不使用真机。按 T500 §6 R1/R4，独立 overlay `test/t501/generate.cjs` 消费 T492 完整 M1/M2 生成链；不改变 shader、池容量、物理 passes、Flush/fence、GAP 或 root deadline。

`resource_ledger.h` 是有界线程安全观测器：256 活跃对象槽、256 最近事件、64 host ownership 槽、32 host 类别高水位及 16 context 退出回执。对象由 context 指针＋scope generation＋kind＋name 识别，每次 create 有独立 generation；create/replace/delete/tombstone/retired、失败原码、live/retiring/peak 均在实际切点更新。满容量/历史覆盖显式 incomplete/dropped，不能冒无限完整。

`resource_gl.h` 只包装资源创建、存储、删除、sync 和原 glGetError，不插入 dispatch/Flush/fence 等物理操作。分配前后读取 GL error，保存原错误 flags 给应用下一次 glGetError；不会把之前的错误错归本次成功分配。BufferData 替换只记一份当前 store；texture 按实际单层格式尺寸计请求 bytes。Program/shader/VAO/FBO/sync 元数据大小未知，只计对象。glDelete 只叫 `DELETE_NAME_REQUEST`，不冒驱动物理页立即回收。

生成 App 先退旧 context 才创建新 context 的条件保持。ContextLost 前将该代对象转 retiring，host Run/transport 销毁不减旧 context 的 GPU 请求账；仅 EGL destroy context 成功后转为逻辑 retired。UNBIND、DESTROY_SURFACE、DESTROY_CONTEXT、TERMINATE 各有 enter/exit/failed 回执，保存实际 owner、scope、未退句柄/window、最后 stage/error 和首错。reset 失败继续原 owner 循环重试，shutdown 仅做原有限一次退出；失败保不完整回执，不在 UI/control 强毁、无限 join 或强行归零。旧 context 成功后驱动内部退役仍未知。

host 分类观测 App 内嵌块、独立 snapshot、ledger、source cold/work vector 的 size/capacity、passTable 容量、Run/transport、EvidenceCapture 及 worker 移交后实际销毁、执行诊断临时副本、M2 trace 实际 vector 容量及其 shared ownership。EvidenceCapture/trace 持有 ledger，旧 reader pin 的对象直到析构才退出账，未更改旧 trace 生命周期合同。字符串/allocator/driver 不是全进程硬上限；不将 reader 预算冒为 native 预分配限制。

冷查询 `resourceState()` 输出 `T501_OWNER_RESOURCE_LEDGER_V1`，明确 `LOGICAL_REQUEST`、`HOST_ALLOCATED_CAPACITY`、`DRIVER_NOT_MEASURED`，附 cache entries/pins/queue/capacity、snapshot active/保留 payload、M2 trace 当前 id/readPins。T501 的 cache tempBytes 明确未由旧 T475 观测，T502 串行接线后补真实故障临时账；不伪填 0。冷热记录不写 hot JSON/文件，无粒子读回。

必要检查：两路线原 native 23＋20 项及新增 ledger 7 项；实际 App 冷/暖准备检查17/20 buffer、五稳态 program，请求容量分别37,728,720/38,388,176B（K192/Kw128）；实际 Scene resize/拒绝与失败 BufferData 原错保存；每路线九退出进程覆盖正常、四 shutdown 失败和四 reset 失败恢复；实际 SOURCE/J→W/Draw/Swap 正控。最终两路线完整 hvigor 构建、签包 verifyApp 通过，收据见 `test/reports/t257/T501-*-delivery.json`，机器报告 `test/reports/t501/delivery.json`，复跑见 `test/t501/README.md`。

这些是非机实现检查，未宣称设备物理峰值或长期资源泄漏已证实/排除。T500 的独立八组合32 cycle/driver/RSS证据由 tester 接续，R2/R3 冷协议仍按独立架构卡处理。
