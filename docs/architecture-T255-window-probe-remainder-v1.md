# T255 GPU 窗口探针剩余工程分流

2026-10-29，architect；原卡 `6ae2a419e4b0ab67702dea7c`。消费 T246 报告、`test/t246/protocol.h`/`probe.cpp` 与 T242/T233/T241。修订 T242/T233 工程接点。不使用真机、不重跑测试、不改业务代码。

T246 的独立 SURFACE XComponent + fragment SSBO 示意帧、六档 GL 分配与三 RGBA8 target、aligned range、1024B tail host 篡改/旧代拒绝已有编译和离线测试；真实 window shader/swap 与实际 device alloc **NOT_RUN**。`protocol.h` 尾包只含 private magic/owner/context/surface/lease/token/done，`probe.cpp` 的 hook 只在消费前改变内存身份；它不含 T241 产品 completion 的全量 stage/audit/replay/stock/receipt，也没运行真正 M0 root。`tailVerified` 与示意 CURRENT 只能证明探针内部协议，不能驱动物理 commit、accepted bank 或产品 publication。

| 接续面 | 可与 T236 并行的 T246 专属 probe 工程 | 真正依赖产品接口的集成 |
| --- | --- | --- |
| 窗口/资源 | 独立窗口同 context compute→fragment SSBO→前背景/示意床粒完整 swap；六档实际分配、读写、live/peak、GL错误与清理，设备由 T247 测 | M0/M1 权威 bank 与真实 publication 的同 token draw 需产品接口 |
| 尾包 | 保留 private T246 revision、1024B 长度/range/迟到拒绝微探针，明确只为平台同步 | T241 completion256 全字段及按 root 的 audit/replay/stock/receipt 校验、尾 fence 后 native 一次 bank+revision+receipt 接受 |
| 生命周期 | 增可控真实慢 GPU fence、取消、leased slot、后台/前台、resize、窗口销毁/重建及完整 callback/GL/epoch/rawEGL 日志；设备 T247 按事件真实触发 | 产品 inflight root tombstone、candidate quarantine、已接受前缀、publication release fence、context loss 后旧 GPU authority 拒绝 |

**T246 developer 续工，不使用真机**：保留私有 probe 模块并补真实异步迟到/lease/后台重建可触发入口与客观日志，不再把 `hook==3/4/5` 模拟错代称真实生命周期；编译/非真机自测，明确 FAULT 或上一完整有效新管线 STALE。T236 产品 completion 出口到件后，再由唯一平台 owner 在独立版本 adapter 中逐 word 对齐 T241 真实 wire，拒探针尾包进入物理 selector；共享公共 EGL/NAPI/CMake hunk 串行。T247 tester 真机先做独立平台 probe，即使 M0 未到也可取真实窗口能力、实际六档资源、真正迟到/后台重建；产品物理 commit/scene 证据要等 adapter/M0 到件另段执行。签包统一 T253。T246 保留部分交付，不能因 probe 已编译宣产品探针全齐，也不因等 M0 整卡停工。
