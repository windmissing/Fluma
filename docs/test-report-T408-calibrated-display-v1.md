# T408 · 单沙独立标定与冻结映射真机配对 — 标定 native 读回数据完整；截图 hold 覆盖 4 轮未达成，按 README 规则拒绝进入 calibrate（不容差凑 PASS），移交 architect

- 卡面：T406 签包，先独立诊断 shader 标定（capture 全程位于 hold），冻结唯一 calibrationId → plan → 静态/动态配对判读。
- 包：T406-complete-signed.hap SHA256 `bb7b0558f4118c57896ca58024829034cf981b196e89dfeba47a0397d1f57cc9`（本地核验一致）。
- 路由：`p051v2=1 --ps singleSand 1`。证据：`test/reports/t408/`（51 文件，SHA 见 t408_verdicts.json）。

## 已完成（标定前置证据完整）

1. **独立几何采集**：页面 `T406_GEOMETRY_V1` 显式输出浮点全局 box 与 vp2px：globalX=14.628574916294642、globalY=627.7142857142857、widthVp=336.45713588169644、heightVp=129.42857142857142、vp2px=3.5 → native 1178×453 与浮点几何换算一致（336.457×3.5=1177.6、129.429×3.5=453.0）。
2. **独立标定图 native 读回完整**（manifest `t394-2-1-30778.json`）：phase.name=CALIBRATED_ROI、captureStatus=CAPTURED、presentationState=CURRENT、三 ROI `(8,8,32,32)/(565,210,48,48)/(1100,400,48,48)` 分布左下/中间/右上、双目标 RGBA 各 22528B=预期、`calibrationShaderSha256=a851733d…f021454` 显式输出、manifest sample token == hold token 逐位一致。
3. **capture.py 原始调用记录**：`T406_CAPTURE_CALL_V1` 收据（startMonoNs/endMonoNs/stdout/exitCode）齐备，未手填任何时刻。
4. **独立时钟同步**：/proc/uptime ↔ Python monotonic 三组同步对，观测 rate≈0.9943（作为 uncertainty 记录）；两种映射法（单点 offset / 线性 rate）结论一致。

## 未达成：截图 hold 覆盖（4 轮如实）

| 轮 | 结果 |
|---|---|
| 1 | 封存失败（无 hold 文件）；截图为裸 uitest（无 capture.py） |
| 2 | hold-2 生成，但 capture.py 映射后在 hold 封存端外 ~206s（截图执行于封存点击之后） |
| 3 | probe 未 CAPTURED（第二 evidence 未 ack 占用 probeBusy）；hold-3 当时不存在 |
| 4 | hold-3 生成但属第三轮 probe 生命周期；第四轮截图映射在 hold-3 窗外（封存后） |

每轮的 capture 收据（calib2/3/4-capture.json）与 hold 文件均原样保留；映射计算过程可复算。**按 T406 README 规则 3（整个截图调用必须位于 hold 中）与规则 6（calibrate 必须拒绝），未运行 calibrate**——不做容差拟合、不用手填时刻替代、不冒 PASS。

## 未行使（被上游阻断）

calibrationId 冻结、planId 冻结、静态 single、动态 pair V2 判读、正反控（1px/1码/缺列/过期身份/越 hold）。T396 历史 NOT_JUDGABLE 未改写。

## 移交 architect（需开发输入）

截图 hold 覆盖是设备端 CAPTURED→封存点击之间的时序约束：手动 uitest 节奏下 capture.py（PowerShell 启动约 2-5s）无法稳定落在 hold 窗内（第二轮 hold span 87s 仍因操作顺序错位失败）。需要 developer 明确以下之一后 tester 可继续：
1. 页面侧 hold 延长/冻结期扩展（如封存前保持 hold 开放固定时长并在 UI 显示剩余窗口）；
2. 或自动封存时序（CAPTURED 后页面自动以固定延迟封存，tester 只需在该延迟内截图）；
3. 或确认正确的手动操作节拍（当前"预约→等 CAPTURED→截图→封存"顺序在第四轮仍错位）。

## 边界

未改业务；未改半径/shader/阈值；不逐 ROI/臂找平移；capture 全部走 capture.py（后三轮）且收据齐备；时钟映射未从被测图拟合。
