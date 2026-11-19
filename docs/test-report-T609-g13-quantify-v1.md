# T609 v1 · 目标 13 真实流场独立 GPU 定量验证（首轮：G13 管线 CONFIG_PENDING 卡死）

日期：2026-11-19，tester，真机（011124264R000337）。
被测：T608-g13-signed.hap（T608 dd91018 独立 G13 GPU 管线交付，
inputSha 68b5a892…/signedSha a49229ed…，sign=PASS verify=PASS），
bm clean 后冷启（EntryAbility srcEntry=G13ResearchAbility）。
证据：`test/reports/t491/t609a/`（g13-status.json + l2–lA 布局快照）。

## 执行链

EntryAbility → **G13 · 纯流场独立研究**页（GpuResidentG13.ets，XComponent
SURFACE library grs_g13）→ 配置 REST（BeginSession+g13Configure）→ 激活
（g13SetActive(true)+postFrameCallback 脉冲循环）→ 重力改向 → 冷档导出尝试。

## 结果：G13 管线 CONFIG_PENDING 卡死（60s+ 无异步推进）

| 域 | 实测 |
|---|---|
| native.state | **CONFIG_PENDING**（Configure error==34 走 deferredConfigure_，
  等待 re-apply；Ready()=configured_&&transport_.Ready() 未达） |
| scene | WAIT_ACCEPTED_FIELD / rawGL=0 / fboBytes=0 / staleAttempts=0 |
| 脉冲账 | earned=4146（UI pulse 进账）但 **queued=0 / consumed=0 /
  acceptedQuantum=0** ——root 从未派发 |
| motionBits / capability / compile | 空/null（shader 编译与能力探针未完成回填） |
| coldToken | G13R1:244621396514846:0:076a27c79e5a…（BeginSession+Configure 已跑） |

代码侧定位：transport `ready_` 仅在**冷初始化 dispatch 完成且 ctrl[255]==kSeal、
ctrl[7]==0** 后置位；bridge 的 deferred configure re-apply 需 `RootIdle()`，
BeginGranted 被 `configurePending_` 挡——观察到 earned 持续进账而 root 零派发，
存在**配置重应用/首 root 派发未闭环**的缺陷域（bridge.cpp:340–361 与
transport.cpp:436 链）。

## F01–F16 判定

全部 **NOT_RUN（产品管线未到可测态）**——T606 独立基线（16 case 闭式期望）
已就绪但无实测对照；不以残差或编译冒物理 PASS；T608 "RUNTIME_UNVALIDATED"
如实维持。

## 回 architect（保原件）

- 原始错误：真机 REST 配置+激活后管线永久 CONFIG_PENDING，root 零派发，
  shader 编译/能力探针未完成
- 已试办法：配置→激活→重力改向→双次激活→60s 异步等待→冷档导出——全部无推进
- 所需输入：G13 冷初始化 dispatch 未完成的具体失败点（GL 编译错误/deferred
  configure 环/XComponent surface 就绪时序）——建议 developer 加 g13 域 hilog
  埋点（bridge.cpp 当前零日志）后复测
