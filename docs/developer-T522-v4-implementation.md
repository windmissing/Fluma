# T522 V4 开发交付记录

2026-11-16，developer，不使用真机。依据 T497/T508/T498 v2；独立 m3a V4/features15，旧 M1/M2 默认入口保留。产品仅 XComponent/shader，无 Canvas 或软件 GL 回退。

## 已实现

- SOURCE72：λ/η、共享 seed、最多八 trial、全有或全无、普通沙/水及 BULK 湿向量、原始输入去重、sourceEvent/sourceMeta/raw dedup 同账本审核，独立 SPARE 重放、源成功前缀和 V4 seal。
- 五角色 QUANTUM：REQUESTED/CLAMPED/OLD/AUTHORITATIVE/CONSISTENCY，waterLevel40 与 occupied medium 全 writer，权威流锁存、语义归一后逐字比较、三态/全流审核；四档固定 549/619/594/664 表同源生成。15 个固定 opcode 特化程序仅改变静态操作，不由 native 计算分支或角度。
- native transport/controller：唯一 EGL owner、四 SSBO、每批最多10槽/8ms、原250ms和双GAP、单1024B尾读回、校验前不接受、publication 双槽/释放栅栏/原期限呈现检查。原 T475 Store 复用。
- 独立 GRS_M3_PROGRESS_CLOCK_V3：B/BC U128整数时间账本、BLOCKED/真实意图再入、同锁退休/旧尾拒、quota256；16包有界FIFO，背压不登记意图，额外控制账本不超过1024B。
- 独立 V4 scene/lease/reader 与隔离页面，FrameCallback 提供真实页面脉冲，显式手动源与角度；旧冷协议入口拒绝 M3。所有产品背景/前景走 shader。
- T507 冷所有权工具显式适配新V4 payload、quota/control/input。普通最大27,847,808B、双槽55,695,616B；FULL_BRANCH 单独新运行、20个独立e流block及32768B descriptor，冷副本单独额度和 schema。每计划冻结、完成导入不可变冷副本后才能复用GPU槽；未执行流零填充并由mask标识。客户端分页16KiB、流式落盘，普通4MiB序列化额度保留；显式FULL模式另给有界4GiB流式额度，超限拒绝，未一次聚合。

## 复跑入口与检查

从项目根执行；ANGLE软件后端只用于离线fixture，不进入产品：

1. `python test/t522/generate_contract.py`；`build_contracts.cmd`：四档表、预算、对齐和mask。
2. `build_control.cmd`、`build_blocked.cmd`：quota/native owner/lease/旧版本、BQ01–BQ21（BQ04仅本地确定性夹具，不声称设备证据）。
3. `solver_cases.py` + `build_solver.cmd`：11项λ/η/water实际GL；`seed_shader.py` + `build_seed.cmd`：共享origin、升序seed和连续ID。
4. `python test/t522/assemble_kernels.py`；`build_source.cmd`：SOURCE72 bulk/retry/sand/waterDirect/waterFlight、篡改trial拒绝、零量、physics最大计数、seed-resolution回滚。
5. `build_root.cmd`：完整549槽 REQUESTED、frame偏移7、实际CLAMPED及FULL分支流GPU复制。正常五角色权威/consistency/审核与seal通过。
6. `build_allocations.cmd`：四档正常28对象及FULL额外21对象实际GL分配/查尺寸/删除；`build_cold.cmd`：新预算/布局/schema、mask越界、双槽取消/pinned reader/旧reader拒V4。
7. `python test/t522/prepare_native_host.py`；`build_native_gl.cmd`：真实production transport/controller。SOURCE已成功72槽接受，planned/observed上传3192B；V4冷快照REQUESTED→TRACE_READY及page通过。QUANTUM在软件后端触发原期限或GAP时拒绝，SOURCE保留、不接受Q；这是保护行为，不能称性能通过。
8. `node test/t522/generate.cjs .codex/local-test/t522-delivery`；在该目录 DevEco `assembleHap --no-daemon`：ArkTS/native BUILD SUCCESSFUL，34 tasks，57.669s。`python test/t257/sign.py T522-delivery .codex/local-test/t522-delivery`：verifyApp PASS。签名证据 `test/reports/t257/T522-delivery.json`。

## 证据与限制

离线原始输出归档在 `test/reports/t522/`。完整功能根实际运行是192/64；其它档目前为同源表/资源分配检查，未声称各档设备运行通过。真机、端到端、设备250ms和效果指标均NOT_RUN，交T523独立验证；不作为开发卡关闭前置。native并发离线软件运行真实触发CLOCK_GAP，保留原错和计数，不放宽阈值、不冒充产品PASS。

已有T499 oracle有正式合同差异：`solve_lambda`遗漏T497 §4.5保守触顶拒绝。N128、T0、W=[5,5,0…]、cap=[5,CQ…]、r=[0,4,0…]、λ65536，oracle返回OK/eHi1，而eta0[0]+eHi>cap[0]。GPU按正式合同拒绝；`truncation_contract`保留负控，原卡已报告，不修改tester资产。

## 构建产物

签名HAP：`.codex/local-test/t257-sign/T522-delivery/T522-delivery-signed.hap`，SHA256 `70d2742c6a76d11bc6aef5450a3e8e68df6c6a513ef7b9969cec54aadb49c3d7`。隔离生成器保留旧默认入口，并显式链接新M3页；构建不改共享签名配置，凭据不写日志。
