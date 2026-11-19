# T610 v1 · 目标 13 纯流场新管线观测 DEMO 验证（effector 域，由 tester 代执行）

日期：2026-11-19，执行者：tester（用户裁定 effector 亦由本会话执行），真机
（011124264R000337）。
被测：T608-g13-signed.hap（T608 dd91018 G13 页面/shader/场输出交付），bm clean
后冷启 PID 28449。
证据：`test/reports/t491/t610a/`（五视图截屏 speed/tracer/vorticity/dynP/totP
+ g13-status-t610.json + 布局快照）。

## 执行链与观测结果

1. **OFF 入口显式激活** ✓：页面默认"G13 流场研究 OFF；设备资格与物理指标尚未
   验证"（off 门在档）；配置 REST→激活链路可点击
2. **五视图逐一观测**（速度/示踪/涡量/动压/总压）：XComponent SURFACE 区域
   **全部纯黑**（像素灰度均值 0.0/std 0.1，无任何渲染内容）
3. **根因**（与 T609 v1 一致）：native.state=**CONFIG_PENDING**、scene=
   WAIT_ACCEPTED_FIELD、fieldCommit=0、displayPresentVerified=false、
   presentedMapping=null、earned=0——**accepted field 从未产生，shader 无输入
   可渲染**；无场即无观测对象

## 判定

- 五视图可视化/图例/色标/时间演化/停止边界/stale 标记：**NOT_OBSERVABLE**
  （管线 CONFIG_PENDING 卡死，accepted 场零输出，XComponent 全黑）
- 与 T609 v1 同根因：G13 冷初始化 dispatch 未完成→无 accepted field→scene
  无法 present——同回 architect（g13 域 hilog 埋点+冷 dispatch 失败点）
- 无 Canvas 回退（XComponent 保持黑屏而非降级渲染，符合合同）
- 物理量纲/色标正确性不做判定（本卡为观测域，数值可信归 T609；且无画面可判）

## 所需前置

T609 v1 的 architect 分流项：developer 提供 g13 域日志与冷初始化失败点修复后，
本卡重跑五视图观测（页面入口/视图切换按钮均已在档可复用）。
