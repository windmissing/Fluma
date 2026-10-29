# T244 明水 C 行为与 native 独立补验 · 报告 v1

- 角色：tester（**离线**，未用真机）· 日期：2026-10-29 · 状态：**结束待验收**
- 消费：T235 报告（8 PASS 公式/符号面——本卡不重复符号检查，不扩大宣称）；T226 0eeaa8d 实现
- 产物：`test/p244_c_behavior_drive.cjs`（ETS 行为驱动）、`test/p013_native/t244_clear_water_independent.cpp`（独立 native-host，不改 T226 测试文件）、`test/reports/t244/ets-drive.json` + `native-independent.json`
- 边界显式：native-host 全部 GPU 替身（FaultGpu），**device=false, gpu=NOT_MEASURED, boundary=no-GL-host-stubs**

## 面 A：真实 source 生命周期（ETS 引擎真实驱动）—— 7/7 PASS

| 用例 | 内容 | 结果 |
|---|---|---|
| A-01 | down→双 tick 有效窗口→sample→enqueue→commit→**真实 ACK**：terminal=ACCEPTED，admitted≈0.0167（rateSand×秒），transactionId=1，kindSuccessSeq=1，sourceCommit=1 | PASS |
| A-02 | 同 key replay：同一 request 重查 → TERMINAL 同终态、sourceCommit 不变、无新 source | PASS |
| A-03 | stop：pending=0、inputGeneration 2→3、status OK | PASS |
| A-04 | reset：pending+dedup 清零（终态可见性重置） | PASS |
| A-05 | clock domain 变化 → tick invalid → 自动 stop → pending 清零 | PASS |
| A-06 | accepted 角 token：dt=0 OK no-op；angle=31 拒 INVALID_VALUE；边界 30 通过身份核（phase A 返 UNSUPPORTED_FEATURE=19——B coupling 未接入如实） | PASS |
| A-07 | stale 时钟：合法窗口 OK；重复 id 拒 21；previous 不连续拒 21；窗口>1/30s 拒 21 | PASS |

关键语义发现（测试构造层面，非产品缺陷）：`down()` 内部 `stop('DOWN')` 会 `clock.reset()`，因此 down 后需要**两个** tick 才产生有效采样窗口（第一次 tick 恒 FIRST_OR_CHANGED、seconds=0、不发射）——这是「切源后必须新 DOWN + 时钟重建」合同的真实行为。

## 面 B：独立 native-host 帧包原子性 —— 6/6 PASS

| 用例 | 内容 | 结果 |
|---|---|---|
| B1 | 重复帧 STALE_FRAME 拒绝，零 GL 调用、不重复 commit | PASS |
| B2 | Begin/Layer0-5/Present/Swap 共 9 个失败点逐一注入：全部不 commit、lastGood 保持前 token、无部分提交 | PASS |
| B3 | F32 跨列投影：proof 65.0/128.0 与 xy32 不一致 → 校验拒；原始一致包通过（独立投影参考语义与 ETS projectX/Y 一致） | PASS |
| B4 | 12 突变（proof 质量/profile W/xy 越界/header −0/optics rev/box 宽/optics g/ledger commit/origin 置零/header 版本/token hash-only/proof 尾槽）全部 GPU 前拒绝 | PASS |
| B5 | 独立面积 Fresnel 求和：邻列各 0.5、和精确=1（gain=1 语义下） | PASS |
| B6 | 同 id 重放（frameId=1 已消费后再提交）→ STALE_FRAME（code=102）、不 commit | PASS |

B6 顺带证实一个正确拒绝语义：frameId=0 会被 `frameId<1` 在校验层拒（101 CW_V3_IDENTITY/VERSION_MISMATCH），倒退帧不会以非法身份漏进 STALE 层。

## 面 C：独立面积/预乘参考 —— PASS

邻列求和=1、预乘 body 域检查与 T235 公式一致（集成层复核，未重复 T235 判定）。

## 未实现面（精确 NOT_IMPLEMENTED）

- `advanceSubstepInto` 非 B 包时返回 UNSUPPORTED_FEATURE、implementation=`NOT_IMPLEMENTED_B_COUPLING`（T226 phase A 如实边界——水直进 W 的物理耦合未实现，本卡不冒通过）。
- T227 FlightV2 / T228 V4：零命中（T235 已证，本卡未重扫）。
- 设备显示（displayPresentVerified）、真实 GLES shader 编译：归设备卡。本卡给出的**设备窄矩阵**建议：①同 XComponent context 下六层真实 GL 渲染一帧 CURRENT；②真实 swap 后截屏像素含水面/床层；③切源 DOWN 后新 source ACK 与旧源终态隔离。

## 执行注记

- MSVC 无 BOM UTF-8 中文注释按 GBK 解码吞换行（旧坑重现）：独立 cpp 全部注释已转 ASCII。
- canonical 包 frameId=1 是下界——倒退帧构造需走「同 id 重放」而非 id--。

## 失败上报

无阻断。
