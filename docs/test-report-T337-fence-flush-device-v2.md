# T337 fence 修复真机验证（K1-K5，重测 T339 修复后）（真机，完成——修复无效，新机理发现）

2026-10-31，tester 真机（VYG-AL00 62T0225B10010020）。验证对象：T336（`8e2662a`，Amendment 2 分批 flush + 环 4096）+ T339 校验器同步（`e8d70d1`）。

## 0. 包身份（T151 三件套）

- HEAD `e8d70d1` 重建（CompileArkTS 重编 8.1s + PackageHap 刷新）→ 重打包 → localSign + verify-app **PASS**。
- **T337b-signed.hap SHA256 `b51067055f1306dea4513091d222f5be27853281fda913683f1ff45b3f5f1276`**，TextInput 外部输入核对一致，manifest identity 回读同值。

## 1. K5 环 4096 导出（T339 修复验证）：**PASS**

三次冷启导出全 `COMPLETE STABLE_PREFIX_FULL_TRACE`（T324_STAGE41_V1，42 字典，dropped=0）——T337 首轮 BLOCKED 的 INVALID_PAGE 根因（ArkTS 校验器 1024 硬编码）由 T339 一行同步彻底修复，旧 T336 BLOCKED 报告的结论闭合。

## 2. K1-K4 判定（三 run 逐位一致复现）

三 run（rows 133/run，run1 m2.json、run2 m3.json、run3 m4.json 全归档）模式完全相同：首批 10 op（carry 0..9）→ **SubmitEnter→Exit 35.244ms** → 预算耗尽退出循环 → GAP（anchor→pause 35.27ms > 33ms 窗）→ SubmitCancel code=1（BATCH_ACTIVE）。**没有任何 run 到达末批 fence**。

| 判据 | 结果 | 证据 |
|---|---|---|
| K1 末批 fence 停顿 ≤5ms | **NOT_MEASURED**（末批未达；但发现取代性事实，见 §3） | FenceEnter/Exit 三 run 均 0 次 |
| K2 submitPumpWallNs ≤8ms | **FAIL** | Pump(105→129)=35.27ms（三 run 一致），远超 8ms 预算 |
| K3 accepted ≥1 + 双源链 | **FAIL / NOT_RUN** | 三 run 均 cancel（code=1），root 永不 inflight，accepted=0 |
| K4 六项无回归 | **部分回归** | ①nonce 一次消耗 ✓（SubmitRootBegin 每 run 恰 1）②**逐批 ≤8ms ✗ 回归**——首批 35.244ms（T330 基线 1.712/0.554ms）③carry 0..9 单调 ✓ ④取消无崩溃 ✓（epoch 正常推进）⑤submitting≠inflight ✓ ⑥41 字典 ✓ |
| K5 三 run COMPLETE | **PASS** | 见 §1 |

## 3. 核心新发现：T335 归因机理被真机证伪——停顿载体是 flush 调用本身，与队列深度无关

首批内部分账（三 run 逐位一致）：SubmitEnter→首个 DispatchEnter **0.850ms**（staging）+ 10 dispatch 合计 **0.70ms**（5-11µs/dispatch，与 T330 同量级）+ **末 DispatchExit→SubmitExit 33.660ms**（首批尾 glFlush 调用）。

对照 T330 基线：同样的首批（10 op、无 flush）仅 1.712/0.554ms；T330 的 37.65/41.28ms 停顿在末批 glFenceSync。**T336 把 flush 挪到非末批尾后，停顿没有消失，而是精确转移到首个 flush 调用点（33.66ms）**——且此时命令队列只有 10 个 dispatch（Amendment 2 机理"残余降至单批 ~10 dispatch→停顿 ~3ms"的前提已满足），停顿仍是 33ms。

**修正后的机理**：真机 GLES 驱动对**首次 flush/fence 类提交同步调用**有 ~33-41ms 固定阻塞开销，与未提交命令数量基本无关（T330：130 dispatch 堆积后 fence 37-41ms；T337：10 dispatch 后 flush 33.66ms）。T335 的"摊销模型"（296/325µs/dispatch 线性指纹）不成立——停顿不是累积量线性函数，而是**每次 flush/fence 调用的固定税**。推论：分批 flush 方向无法修复此问题（13 批=12 次 flush × 33ms 更糟）；flush 应**回到末批一次**（恢复 T329 布局，接受末批 37-41ms 单次税），修复方向应转向**消除该固定税本身**（如：进程启动后预热性首次 flush 在 Run 构造/Ready 阶段发出，把税从热路径挪走；或驱动行为确认后由 architect 重估 GAP 窗语义）。

## 4. 结论与移交

- K5 PASS（T339 修复有效）；K2/K3 FAIL、K4 部分回归（flush 停顿所致）、K1 未达。
- **Amendment 2（分批 flush）真机证伪**：停顿转移而非消除，且首批税使六项之"逐批 ≤8ms"回归。移交 architect：按 §3 修正机理重估（候选：启动期预热 flush 挪税 / 回退 T329 布局 / GAP 窗语义裁定），本卡不预派。
- 取证工装：本卡同时验证了 T339 校验器同步与反漂移测试的有效性。

证据：`test/reports/t337b/`（m2/m3/m4.json + p3/p4.ndjson + rows_run2/3/4.json + layout0-3.json）；签包 `.codex/local-test/t318-sign/T337b-signed.hap`。
