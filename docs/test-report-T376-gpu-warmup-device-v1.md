# T376 GPU 预热与双源上屏真机验证 — W1 结构 PASS、W2/W3 FAIL、W4 阻断、失败先 architect

- 卡面：按 T341 W1–W5 核首次提交及后续 GAP、原始时间线、accepted≥1 和沙水双源同 token 上屏；性能按实际分母，失败先 architect。
- 包：T375-signed.hap SHA256 `b63d6ec9843cee97d352259f8ed886d377ad2914cbbd23c58e70f94212eb0fe1`（T375 交付，卡面确认解锁）。
- 路由：`grsm1=true` → `pages/GpuResidentM1`（pid 3347，三次 run epoch 2/3/4）。证据：`test/reports/t376/`（20 文件，SHA 见 `t376_verdicts.json`）。

## 时间线取证通道（包身份注册）

诊断面板 `TextInput` 需要外部签包 SHA256 才允许时间线冷导出（`PACKAGE_SHA_REQUIRED` 门）。uitest `text` 命令按 4×16 hex 分四段注入 64 位 SHA **逐段成功零丢字**（对比 T373 短串反而丢字——长 hex 串稳定），`dumpLayout` 逐位核对 64 字符全等后导出：**`COMPLETE STABLE_PREFIX_FULL_TRACE`**，84 页 334 行 10 组（epoch/run/surface/TID）。此注册手法可复用为后续 GPU 线取证标准动作。

## W1–W5 判定

| 判据 | 结果 | 实测 |
|---|---|---|
| **W1 stage42 对 + 顺序** | **结构 PASS / 数值低于预期带** | 三 run 各恰一对：carry0(delta=0)/carry1(delta=原值)，root 双 0，同 TID 3475；**42@serial49/50 < Ready@51 < AnchorSet@55**（run2/3 同构）——顺序与 T375 契约精确一致。warmup 原始 ns：**23.38/27.68/28.27ms**，稳定可复现；判据 A 预期 33–41ms 带，实测低于下界 28%（同 ms 量级；PC 43.66s pbuffer 路径不外推，设备驱动差异解释交 architect） |
| **W2 热路径首批 SubmitExit ≤8ms** | **FAIL** | Enter→Exit **52.17/50.21/56.59ms**（三 run 一致）。窗口含 RootPoll 等待（RootSubmit s95→RootPoll s122=51.96ms）——与 T337 首批结构不同（现结构含完整两批+RootPoll） |
| **W3 分批 flush 各批 ≤1ms** | **批1 PASS / 批2 FAIL** | 批1 **0.433/0.373/0.346ms**（预热把首 flush 税挪走的直接证据）；批内逐 op 间隔 p50 0.002ms/max 0.004ms 干净；**批2 46.79/41.82/48.22ms FAIL**——尾 gap（末 op Exit→批结束）**46.69/41.72/48.13ms**：预热后出现**新的 ~47ms 固定停顿变体**，位置在批2 尾、RootPoll 之前 |
| **W4 accepted≥1 + 双源同 token 上屏** | **BLOCKED** | 完整时间线 **SwapBegin/SwapEnd/DrawBegin/DrawEnd 零行**——root 提交完成后无绘制帧消费；JS snapshot `physicsCommit=0:0`、state 永不 CURRENT（UI 停'等待仿真就绪'，尽管原生 Ready 行存在）；双源真实触摸注入（画布 bounds [0,579][1280,986] 内沙+水各 6 轮）后提交计数仍 0:0 |
| **W5 无回归 + K5 保持** | **部分** | 时间线导出 COMPLETE STABLE_PREFIX_FULL_TRACE（K5 环 4096 语义保持）；六项回归清单在无绘制帧路径下不可完整枚举 |

## 移交 architect（三项）

1. **W2/W3 的 ~47ms 批2 尾停顿**：预热消除了批1 首 flush 税（0.43ms 证据），但新停顿出现在**批2 尾**（T337 时为首批尾 33.66ms）。假说：固定税未消失而是**转移到下一个首次触发的同步点**（RootPoll 前的 fence/flush？），或 DVFS/驱动对第二波提交的再次空闲税。判别入口：批2 尾 st35→st25 边界的 GL 调用序列。
2. **W1 数值带偏差**：设备 23–28ms vs 预期 33–41ms——是否影响挪税结论与 Amendment 3 warmupWallNs 披露语义。
3. **W4 断链**：RootSubmit→RootPoll 完成但 Draw/Swap 零行、snapshot 恒 0——提交→绘制帧→publication 链在设备上未闭合（host 侧无此现象）。判别入口：M1 帧循环（`FrameCallback.onFrame`→`model.tick`→`snapshot`）与提交泵完成信号的衔接。

## 边界

未改业务/native/判据；未回退共享树；双源注入与 SHA 注册全程 uitest（无 T218DBG 直注作判据）；PC 43.66s 未外推设备；性能分母用实际墙钟。
