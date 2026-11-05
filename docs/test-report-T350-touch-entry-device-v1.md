# T350 真实触摸入口与默认参数真机核对（R1-R8）（真机，完成——R3-C BLOCKED 先 architect）

2026-11-05，tester 真机（VYG-AL00 62T0225B10010020）。验证对象：T347 包 `0195c935`（HEAD 41c5351，未改业务代码）。消费 T348 preflight 纠偏 + T347 报告。用例：`docs/test-cases.md` §21 R1-R8（commit 5970fbd）。

## 1. R1 入口身份核对：**PASS（三入口可区分）**

| 入口 | Want 解析（hilog 实证） | 关键状态 |
|---|---|---|
| A. EmptySandDemo 默认（`emptysanddemo=1`） | `rendermode=1 testmode=0`（**FLUID 默认，非 4**），无 particle_route 行 | **STRUCTURAL_ZERO_OFF 证实**（T348 preflight ②：p011ParticleRequested 默认 false） |
| B. EmptySandDemo 显式（+`rendermode=4 p011particle=1`） | `rendermode=4`，binding 经 initializeDryParticleDemo 固定建立 | 粒子路线活（见 R2-B） |
| C. V2 显式（`p051v2=1`） | V2 页 route 固定 | T347 全链形态复现 |

## 2. R2 空场→触摸出生（真实 DOWN/MOVE/UP，禁 T218DBG）：**A=PARTIAL / B=PASS / C=PASS**

- **A（默认参数）**：真实触摸 3s → dry.receipt ACCEPTED admitted 0.36-0.62（注入链在位 ✓）但画布**完全空**（截图 A_default）——沙直接沉积无粒子，STRUCTURAL_ZERO_OFF 视觉证实。判定 PARTIAL（链在位、粒子路线关）。
- **B（显式 p011particle=1）**：触摸 16 笔 sum admitted 15.4 units → **~15 悬浮粒子可见**（截图 B2）——出生成功。
- **C（V2 页）**：触摸 9 笔 → live=126 出生（T347STEP）。

## 3. R3 pY 下降→LAND/床增长：**A 不适用 / B 无读数通道 / C=BLOCKED（新发现，先 architect）**

- **C 项关键发现（UP 后粒子悬浮冻结）**：注入期 pY 正常下降（frame 360: pYmin=114.5→frame 480: 54.65，commit 推进正常）；**UP 停止注入后 pY 完全冻结**——frame 480→720（8s 模拟时间）pYmin=54.646947334308884 / pYmean=88.04859555536524 **逐位不变**，而 motion=126（pVy 非零）、commit 966→1446 正常推进、页面无故障。
- **假说（供 architect 判读，不代裁）**：粒子 sink/LAND 只在 dry-batch 注入事务提交时推进（T347 的 LAND 发生于持续注入期；UP 后物理 commit 空转、粒子不 sink）。**T347 报告 frame 360→480 的"近恒定"当时判读为着陆静置，本卡证明悬空也冻结——T347 E3 判读需修正**（其 mh 增长仍真实，发生于注入期）。
- 按红线不改代码，BLOCKED 移交 architect（判别入口：P051DerivedDemoV2.tick 的 step(dt) 与 M2Sink 的提交驱动关系）。

## 4. R4 同 token 双态零非法拒绝：**B=FAIL（V1 固有）/ C=PASS**

- **B：code100 ×19**（V1 固有 SCENE_MATERIAL_DATA，状态栏显示"新管线故障"）——注入累积后触发，T342 只修 V2 链。真实触摸在 V1+粒子路线下**可复现 code100**（T202 链真机新证据）。
- **C：CURRENT 56/56、request==actual、零拒绝** ✓。

## 5. R5 UP 停源：**PASS** —— 最后 dry.request 19:57:32.524 与最后 UP 吻合，其后 6s 零新 request。

## 6. R6 前后台切换：**PASS（无崩溃）** —— Home 出→回 pid 2542 不变、零 jscrash/THREAD_BLOCK；code100 持续存在（V1 固有，非切换引发）。

## 7. R7 真实模拟时间与 fps：**已记录（不设门槛）**

- C 入口：首注入后 wall 35s 模拟时间 3.57s（fps 坍塌 12.6→6.0→2.7→2.0→1.7，与 T347 曲线同形态）。
- **如实声明：1.3-2fps 下的模拟时间累积极慢，任何依赖落地的观测都需分钟级墙钟——不宣体验达标**。

## 8. R8 摇晃传感器流向：**已分列记录 + 新发现（先 architect）**

- **target/actual 分列**：`targetTiltDeg=89`（传感器重力方向，测试机平放）恒定；**`effectiveTiltDeg=30.0` 恒定**——目标 89 与实际 30 全程脱钩（钳位或平滑收敛异常，commit 480→1200 无变化）。
- shakeInput=0、shakeMag 0.006-0.04 m/s²（阈值 shakeOnMps2=1 未达，设备静止无摇晃事件，如实记录；无加速度计注入通道，物理摇晃不可行）。
- **89→30 脱钩移交 architect**（与 R3-C 冻结同批判读）。

## 9. 结论与移交

- 三入口身份/注入链/出生/停源/生命周期核对完成；**R3-C（UP 后 pY 冻结）与 R8（tilt 89→30 脱钩）两项工程缺陷 BLOCKED 先 architect**（T350 红线：不照旧错误前提返修、不改业务代码）。
- 生产 V2 切换裁决输入：V2 页 R2/R4 形态健康；**V1 页默认 STRUCTURAL_ZERO_OFF + V1 固有 code100 真机复现（真实触摸链）**——默认参数现状不可作粒子体验入口。
- T347 报告 E3 判读修正建议：LAND 发生于注入期；UP 后冻结是新机理（T346"模拟时间不足"解释不覆盖）。

证据：`test/reports/t350/`（hilog_A_default/B_particle1/B2/C_fgswitch/C_v2/D_shake.txt + A_default/B_particle1/B2/ layout + 截图）；签包 T347-signed.hap SHA256 0195c935（未重签，未改业务代码）。
