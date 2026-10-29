# T247 GPU 窗口与驻留资源真机补证报告（v1）

日期：2026-10-29；tester；**真机 VYG-AL00（62T0225B10010020）**，串行 GPU 优先。被测对象：T246 独立窗口 probe v2（T257 已签 HAP `T246-signed.hap`，SHA `d1b7ea95…`），按 T242 任务书执行。原卡 `6ae28fade4b0c1a6bd692301`。**probe 尾包 private（productCommit=false），本报告不冒 T236 产品 M0 整步/同 token 接线 PASS。**

## 结论

**9 PASS / 0 FAIL**——真实 XComponent window 同 context 全链（compute→fragment→blit→swap→**屏幕呈现**）12 次 run 全部 CURRENT（0 FAULT、rawGL=0），六档资源全部实际分配+读写通过，1024B 私有尾包 12/12 verified，lease hold/旧代拒绝/destroy-recreate 重建全部按合同工作。

## 真机 stage limits（与 T232 pbuffer 私有上下文对照）

| 查询项 | 真机 window context | T232 pbuffer |
|---|---|---|
| GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT | **256** | 256 |
| GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS | **16** | 16 |
| GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS | 0 | 0 |

与 T232 记录一致；256B 为 range 起点对齐约束已在 range contract 断言中验证（错位 offset 拒绝分支不触发于合法路径）。

## 六档资源分配（实际 glBufferData 成功 + 首尾 word 读写回读核验）

| profile | 设计 payload | 真机 liveBytes | 对象数 | 结果 |
|---|---|---|---|---|
| M0 K192 | 15,060,496B | 35,014,416B（含三全帧 target+尾包） | 26+3+1 | PASS |
| M0 K512 | 36,997,904B | 56,951,824B | 26+3+1 | PASS |
| M1 Kw64 K192 | 23,343,248B | 43,297,168B | 37+3+1 | PASS |
| M1 Kw64 K512 | 55,111,056B | 75,064,976B | 37+3+1 | PASS |
| M1 Kw128 K192 | 25,472,400B | 45,426,320B | 37+3+1 | PASS |
| M1 Kw128 K512 | 57,240,208B | **77,194,128B（峰值）** | 37+3+1 | PASS |

六档共存分配全部成功、逐对象可读写、峰值 77.19MB 无 GL 错误。设计 payload 与实际 live 差值为三全帧 target（8*w*h）+对齐+尾包+program，与 T242 算术一致。

## 同 context 全链与呈现

- 12 次 run（token1–11 ctx1/surf1、token13 **ctx2/surf3**）compute 写尾包 → barrier → fragment SSBO 只读 fetch → blitFramebuffer → eglSwapBuffers，全部 CURRENT、`privateTailVerified=true`。
- **截图证据 `t247f.jpeg`**：深蓝背景+黄色沙粒点阵+橙色床条真实上屏（probe 通过配色；品红=失败未出现）。这是 T232 以来首份"真实 window swap 后可见呈现"证据。
- run 提交→真实 glFenceSync→poll（100ms 定时器非阻塞），wait≈37µs 量级；frameCpuInclusive 14–92ms（含首次分配）。

## 生命周期与旧代拒绝

| 场景 | 结果 |
|---|---|
| lease hold 期间新提交 | **LEASE_HELD, reused=false**（181 帧 poll 持续拒绝，release 后恢复）✅ PASS |
| XComponent 隐藏期 run（token12） | **FAULT/CURRENT（surface=0）**，不冒成功 ✅ PASS |
| cancel（3 次） | CANCEL_REQUESTED；因真机 fence≈37µs 完成，未捕获到 CANCELLED_LATE_RETIRED 实例（慢路径仅 64 轮，不足以压住 Maleoon 920）——**cancel 拒绝逻辑由 gate 单元合同+隐藏期 FAULT 佐证，late-cancel 实例 NOT_OBSERVED** |
| destroy/recreate XComponent（各 1 次，SurfaceCreated 4 / Destroyed 1） | 重建后 **新 context epoch=2、surface epoch=3**，token13 完整 CURRENT；旧 ctx1 资源随 OnSurfaceDestroyed 退休 ✅ PASS |
| 后台/前台 setActive | FOREGROUND_RECREATE_REQUIRED 语义在（JS 日志 1 例）；context loss 拒绝路径由 destroy 分支同代码覆盖 |

## NOT_MEASURED / 边界（如实）

- **GPU timer disjoint、program/driver 实际字节数**：NOT_MEASURED（probe 未接 GL timer query），不编造帧成本。
- **sharegroup 双向 fence**：本轮未测共享 context（单 context 全链已证），保持 NOT_MEASURED。
- **late-cancel 实例**：NOT_OBSERVED（见上表），不冒已测。
- 尾包为 private identity tail（owner/context/surface/lease/token/done），**非 T241 产品 completion256**；同 token 指 probe 私有 token 链，不等价产品 publication lease。
- 未跑产品 M0 状态/事件（T236 接线归 T236/T262）。

## 证据

- 截屏：`test/reports/t247/t247a.jpeg`（初始）、`t247f.jpeg`（**GPU 渲染上屏**）、`t247c/t247e`（重建前后）。
- 布局 dump：`t247b/t247c/t247d.json`（destroy 前后按钮坐标变化佐证 XComponent 显隐）。
- hilog 原始行：本报告各 token JSON 摘录自 `hilog -x`（JSAPP: T246），关键行已逐条列于上文；应用进程 23785 全程存活。
