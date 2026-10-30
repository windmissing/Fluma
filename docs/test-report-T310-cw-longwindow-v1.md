# T310 明水长窗源到像素真机闭环（真机，结束待验收）

2026-10-30，tester 真机（VYG-AL00 62T0225B10010020）。包：T309-signed.hap，SHA256 `1799e103c44e2eee99f8040976f51707b1dac835743df69f1ae7bf2047ca612a`（实测一致；cwdiagSha 外部输入通道核对）。单一新 run `cw-run-1793341689224-1-1`（pid 51207），切水→30s 长按注水→冷分段导出。

## 一、critical 零丢失（T309 V2 容量真机验证）

- **criticalCount=18 / capacity=16384，criticalDropped=0**，`incomplete=false`、END 完整；stringBytes 1626 / 8,388,608 预算余量充足；firstLostKey 全零槽（未触发）。
- 低优先 lane：lowCount=52、lowDropped=0、**lowSkipped=3055 单列**（重复 STOP 采样规则生效，未混入 critical）。
- 冷分段：manifest + critical-0 + frames + low 四文件，SHA256 独立核对全部一致。

## 二、同 run 长窗源账（逐笔，旧 T293 缺口补齐）

critical 事件分布：**DOWN=1、SELECT=1、ENQUEUE=2、BATCH=2、TERMINAL_QUERY=2、CLOCK_WINDOW=2、STOP=8**——T293 时代被 256 环淹没（dropped 2913）的源账逐笔全保留：

| 笔 | admitted | W before→after | disposition |
|---|---|---|---|
| 1 | 0.011120567 | 0 → 0.011120567 | ACCEPTED |
| 2 | 0.022238770 | 0.011120567 → **0.033359337** | ACCEPTED |

（PENDING→ACCEPTED 轨迹在 dispositions 可见；30s 注水窗只 2 笔 ENQUEUE = 源速率 8units/s 下长按合并入队语义。）

## 三、同 run 后继 frame token 与像素链

- frames 900/901/902 三帧同 runId，`CW_FRAME_TOKEN_C1|cw-run-…-1-1|…|900|…` 完整 token（configDigest/opticsRev/sigma 0.025/tint/gain/seed 全在），lastGoodTokens 同 token。
- **nativeWater = ledgerWater = 0.033359337** 三帧 × 三层逐位一致，且 == critical 末笔 TERMINAL 的 `globalWaterAfter`（同值同 token，长窗源账→物理→渲染闭环）。
- 动态 ROI (1172,0,4×4)=列127（T293 同款反推）：**层4 水面后第二色 `22252bff`**（RGB 34,37,43 vs 背景 18,22,31，非零字节差）；层2/3 背景 `12161fff`——body 薄层不显与 surface 非零分开记录（与 T293 窄证一致，公式吻合、参数域，未调参数）。

## 四、结论

T310 **结束待验收**：critical 零丢失 + 同 run DOWN→ENQUEUE→TERMINAL→W→native→ROI surface 非零全链闭环（T293 缺口正式补齐）。旧 C F64 参考线路不冒 M1 权威；真机效果/可见性定标仍归 designer/effector（另卡）。

证据：`test/reports/t310/`（manifest + critical/frames/low 四文件 + 截屏 + cold END 日志）。
