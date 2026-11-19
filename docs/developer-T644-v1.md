# T644 developer 交付 v1

2026-11-19。不使用真机；仅开发、编译与静态产物核对。**COMPILE_PASS_NOT_PRODUCT_PASS**。测试、签名、部署和设备/效果取证未执行，归 T645/既定 tester、effector 卡。

## 范围与入口

按 T641 四表、T647 正式接受合同实现独立 `entry/src/main/cpp/gpu_resident/g9rx/`、`libgrs_g9rx.so`、`GpuResidentG9Rx.ets`、专用 ability、类型声明、生成器与冷档工具。默认 OFF，须显式研究配置与 active/pulse；域固定 `NATIVE_RECEIPT_SPATIAL_CONTROL`，不声称 REAL_TOUCH、硬件采样年龄、厂商 MMI 链或材料标定通过。

本卡没有改默认 g9dm/g9spf 或 G11 owner/scene；错误父源封存在 frozen/，生成时逐文件 SHA 校验，只有修正后的私有变体进入 CMake。默认 DM/双父共享接口仍归 T648。

`.gitattributes` 仅追加本 profile/SHA 引用输入的原字节保留规则；冻结父源保留原始 EOF 空行，不为格式检查改其 SHA。

构建准备：`node tools/g9rx/prepare_build.cjs .codex/local-test/<fresh-tree>`。该工具生成独立、未签名研究入口，不改默认页面/ability/CMake。源码保持默认旧路由。CMake 生成缺输入或身份不符即失败，无旧资产兜底。

## 实现接线

- `bridge.cpp::Touch` 首项 CheckedClockRead；实际 XComponent UI env/线程绑定；不等待的 ingress gate、协议 try-lock 快照、严格 T629 ReadTypedValue<64>、同 scope 校验与最终 8ms ready 切点。回包保 boot、完整 scope、generation/attempt/序号、raw length/unit/normalized count/provider 原件、首错及 UNKNOWN sample age。
- RAW64 不接受 r/J/cap/verified。DOWN/MOVE/UP/CANCEL 使用同一固定存储，MOVE≤256 只摘要；位移按原位型 trunc Q20，固定 Dref125ms，半开映射、deadzone、SAT、U128 cap，GPU 在正式 gate 重算。
- FIFO16 同时承载 SOURCE/IMPULSE，SOURCE 有意义 intent 与 IMPULSE 互斥；原 UP receipt 不刷新。Offer/Claim/Freeze/Commit 查 250ms；完整正文保留、重复/陈旧分流，只有真实 typed commit 后 pop/晋升 BASE。SOURCE 不替换最后消费 IMPULSE，BLOCKED/FAULT 不晋升。过期头不启动空根。
- `loaded_identity.h` 用 dladdr、实际映射 PT_NOTE 与同 descriptor 文件 GNU BuildId、checked stat/路径、whole-file SHA 建 ≤1024B 加载收据；不内嵌自身 ELF SHA。失败无资格。
- generation 计数不 wrap；Stop/换代先撤权，旧 flight/fence 真退休后再恢复新 inputEpoch/pageSession/clock/lifecycle generation，保已提交物理状态和历史 BASE。固定 source intent sidecar 使用 U32 显式读写，未把 U32 数组 alias 为其它对象。
- `transport.cpp` 有效性与时间值分开；时钟失败锁存至新代重新资格。真实 dispatch/barrier/fence/原始 GL/EGL 错来自当前 flight。私有 FrozenExpectedView 流式生成精确224word SHA，非组合 V/视觉字段为零；proof 绑定 producer、完整 permit、当前 raw tail，raw Commit 不能绕过 typed gate。提交短锁内同时切 bank、记真实 commit observation、消费 FIFO/预算。
- Scene 用新 XComponent+shader/FBO 与整数实例属性，无 Canvas/软件回退。只有真实成功 Swap、同 token 复核、native EGL extent/density/rotation 资格后发布 GeometryKey/FrameWitness；geometry 排除 commit/publication。失败撤 mapping、保存原始 scene 错并报告 stale/fault；页面诊断区固定高度，避免回包文本改变 viewport。没有 accepted 场时不能以空白 FBO 授输入资格。

## DM 与接受合同

完整 Config64 上传 RootConfig64..127，八 DM 参数只从 WORK76..83 取；种子按 S40 修正模板构造、动态 stamp 后重新 SHA，再同一原件上传五 bank。冷 GPU 初审核身份、参数、库存/map/来源/质量/动量。

DM48 位于正式 WORK 尾，不占旧 ColumnStage 临时区；实现宽 head/Hs/chiS、最大可行 q、真实 q+1、top-first 连续来源前缀与原子 S→F。私有 post-FOLD 预测有真实 pTrial0/完整摩擦 prox/pEnd/pMid；corrector 面读 pMid，实际动量仍从 pEntry 只加一次力。真实摩擦使用入口 mode，rest 32/J；停止剪切读实际 post-friction p。kind15/16 按正式字典封账，合法拒前缀只证真实执行前缀，全部 enabled scope 核 seen/cert16/cert17。低16族由实际检查/N/A 汇总，不写常量65535冒 PASS。

显式 rootKind、SOURCE12 / QUANTUM2404、精确 mask0xffff / 0x3ffff，完成 DM2/rev2/GNR1。SOURCE 不加 quanta，QUANTUM 只加一次；SOURCE duplicate 不切 bank/不增 commit/时间。中点拒绝5/6/7与硬28/29不放宽。阈值、六J、CFL、来源/功/能量/容量/守恒门没有放松。

## 冷档与 T645 接手

`cold.cpp` 使用进程共享 broker 两 slot/单 builder/单 reader/60s 不续期；分配和实际 delete[]/fence/context 退休后才退信用。FULL 六 arena 单独 GPU lease；带 live 冷档切配置返回 WOULD_BLOCK。ROOT/FULL 四/十段均按 T641 表生成，prefix32768B；无自身 descriptor 或 self-hash。

CURRENT/BASE/Config64/SeedMeta64、四帧 commit sidecar、capability、来源/布局/参数/实际 seed/加载收据/源清单 SHA 均按新字典出档。CURRENT 缺席时 record 与 sidecar 清零。journal 完整原件单独随冷 state/manifest 提供，不占 prefix 保留零区；丢失界限和 rolling SHA 明示。

`G9RxColdArchiveCore` 在 worker 逐页≤1024U32/16KiB 写档，分别记录 rawPrefix/payloadRaw/pagesFile SHA，末 manifest 不自 hash；不在 UI 聚合全场。`tools/g9rx/read_cold.py` 流式重建并 mmap 读取，严格拒旧 profile，核 CURRENT/BASE/配方/FrameWitness/正文与 BASE capsule 链、段 SHA、质量/来源/动量、DM事件及 accepted stream replay。复用现有 T556 公式 oracle，不新增 CPU 仿真路线。离线 reader 不用当前墙钟拒旧档，不把 BASE link 冒前一根完整历史。

`test_seams.h` 仅 `GRS_G9RX_TEST_HOOKS` 独立 native 构建可见；时钟/resolution reader、同核心协议/FIFO/原件 fixture 入口可给 T645 使用，无生产 JS fake-r/J/verified 入口。harness 生成源码/源身份与生产不同。本轮仅编译 seam，未执行 fixture、reader 或产品。

## 编译与实际 ABI

SDK26.0.0/aarch64-linux-ohos、C++17，core 与 NAPI 都显式 no-fast-math/ffp-contract=off；T629 provider 绑定实际 SDK header，单位仍来自真实 NAPI probe，不从 SDK 猜测。

完整 unsigned assembleHap：BUILD SUCCESSFUL（14.411s）；静态 GLSL 编译、Python 编译、native seam syntax-only 均通过。原始日志与机器收据在 `test/reports/t644/`。未运行单测/物理/产品/设备。

ELF 编译数据符号 `GrsG9RxAbi`：payload **40960B / align64**，RxSync **96B / align8**（≤256）；Controller 61632B，Transport 15832B，Expected view 96B，ProgressClock 520B。段 offset：[0, 20480, 21504, 22528, 23552, 24576, 34304]。没有叠加旧22528B输入协议。

真实 native App 静态对象 91392B，含 payload；额外固定 helper 合计 50432B 独立实报，不藏入40960B。K512既定已知包络614208640B再加这些 helper为 614259072B；线程栈、NAPI/JS/worker临时对象、program cache、driver/FBO仍另计，不冒端到端峰值。

未签名 HAP：`.codex\local-test\t644-delivery-v1\entry\build\default\outputs\default\entry-default-unsigned.hap`，7751643B，SHA `cb2154f6e47486a8abd20ab8a988e1122bc961d7b86ec06228c2035f9c51f580`。

未 strip 原生库：`.codex\local-test\t644-delivery-v1\entry\build\default\intermediates\cmake\default\obj\arm64-v8a\libgrs_g9rx.so`，SHA `a04296f2fbc29241972ca0bb418a3f738f52105d19e4d6c1b021614cca52ac9e`，GNU BuildId `a0488247ca999cf3931dc8897ba9b88f03621fbe`。运行时必须重新取实际加载文件 SHA，不把本收据替代设备映射证明。

实际 pipeline/Shader SHA `74c820ecabfa8025361c42e10646642bf22d1f9b49cdb912349e2b856b6c1d25`；source manifest `5c029c8d370754a7ea883d6f57c355cc8f6f35c8c6a482da5f25dc95ae6758f9`；完整 generated 文件及源码 SHA 在机器收据。种子模板 SHA 仅为规格构造实物，实际动态上传 SHA 由运行时重新计算。

开发交付不等于输入/物理/可见帧产品 PASS；真机 root/GAP/30fps 成本、真实加载资格、触摸、GPU接受与效果分别交 T645/既定设备任务，不作为开发卡追加门。
