# T50 空容器 DEMO：传感器到重力方向映射

日期：2026-10-25。角色：designer。任务：`6addc96fe4b03273ad6a83c9`。状态：**DESIGN_READY_FOR_T51 / NOT_DEVICE_VALIDATED / NOT_CALIBRATED**。不使用真机；本稿只定义交互和接口，不改产品代码、物理约束或判据。

## 1. 结论

DEMO 使用手机传感器估计容器平面内的**有效重力方向**，同时单独估计摇晃强度：

- **设备倾斜与平面内摇晃加速度**共同影响传感器测得的容器局部合加速度方向，将其平滑后映射为 `targetTiltDeg`，通过 `WaterSandField.setTilt()` 进入已有物理路径。摇动对方向的贡献来自传感器 X/Y 合向量本身，不是由 shake 强度值伪造出来。
- **快速分量的大小**单独映射为归一化 `shakeInput∈[0,1]`，通过 `WaterSandField.setShakeInput()` 进入已有悬沙/扬沙 ramp。不能用 `abs(tilt)`、设备角度变化率或画面旋转替代 shakeInput。
- 对场内高度/沉积沙与绑定的真实粒子，`WaterSandField` 的同一物理步是权威输入；粒子在该步消费同一个 `effectiveTilt`。渲染重力方向也从同一次提交的有效角派生，不能另设画面方向。

**本次最小交互含义：**倾斜手机使有效重力方向偏转；平面内摇动造成的加速度也会在传感器合向量中短时改变该方向，同时其高频幅值另行提高扬沙强度。停止后方向回到设备姿态对应方向，shakeInput归零，既有衰减/沉降机制恢复。物理仍只接收一个二维平面角度和一个 shake 标量；不模拟 Z 轴出屏幕输运，也不改写求解器/预算。

## 2. 现状与事实边界

- HarmonyOS 加速度计上报设备 X/Y/Z 三轴加速度，**包含重力加速度**，单位为 m/s²。[华为传感器概览](https://developer.huawei.com/consumer/en/doc/harmonyos-guides-V5/sensor-overview-V5)；[Sensor API参考](https://developer.huawei.com/consumer/en/doc/harmonyos-references-V14/_sensor-V14)。
- `SandCanvas.ets` 已有传感器输入：测试模式拒绝传感器样本；体验模式用 `acceptP011SensorSample` 对 X/Y 做现有 EMA；在 VYG-AL00 当前映射中 X 取反、Y 保持，`atan2(-sensorGx,sensorGy)`得到角度。其 `maxOmega=720` 属原页面约束值，本设计不搬用为新物理标定。
- `WaterSandField.setTilt(deg)`将目标角限制在 `[-89°,89°]`，再由场内有效角推进、规划与 guard 决定可提交角度；绑定的粒子路线从同一 `effectiveTilt` 取输入。`invert180()`是独立物理事件，会镜像/重排已存场与粒子，不是把角度数组从 +89° 连续跨到 -89° 的别名。
- `WaterSandField.setShakeInput(value)`接受有限值并钳制到 `[0,1]`；随后使用既有 `shakeRampRate`、`shakeDecay` 控制扬起和衰减。它不是重力方向输入。
- 当前 `EmptySandDemo.ets` 还固定传 `gravityDirX=0, gravityDirY=1`，页面提示“重力方向接线待 T50”；`P051EmptySandDemo.applyDefinedTilt()`已是 `field.setTilt()`薄封装，但还没有 shake 输入封装。

上面的 VYG-AL00 轴变换依据当前应用既有代码，不代表所有设备方向/设备型号通用，也没有由本任务重新真机标定。T52 首次真机验证时应核验设备/屏幕方向与正负号；若设备旋转导致屏幕方向映射变化，报告为未覆盖并另开适配工作，不在 T50 假定自动旋转已处理。

## 3. 坐标、单位与映射

### 3.1 输入与局部坐标

输入样本为 `{x,y,z}`，三轴单位 m/s²，另带应用单调时钟 `sampleTimeMs`。传感器原始 `timestamp` 可保存作诊断，不用于与 `Date.now()` 混算。无效/非有限/时间不递增样本拒收，不能污染滤波状态。姿态估计对 X/Y 的低频分量复用当前 EMA 系数来源 `inputSmoothingAlpha`；T51 不改写共用 P011 双输入状态机。

本 DEMO 的物理是二维容器平面：屏幕 `+x` 向右、`+y` 向下，`θ=0°` 表示重力朝下，正角表示重力向屏幕右侧。对 X/Y 合向量应用现有 VYG-AL00 轴约定：

```text
sx = -filteredTotalAccelX
sy =  filteredTotalAccelY
projectedMagnitude = hypot(sx, sy)
if projectedMagnitude > 0.3 m/s²:
    targetTiltDeg = atan2(sx, sy) * 180 / π
else:
    hold last valid targetTiltDeg
targetTiltDeg = clamp(targetTiltDeg, -89°, +89°)
```

`0.3 m/s²`仅复用现有 SandCanvas 平放/低投影守卫，不宣称本任务重新定标。滤波输入保留加速度计中的重力和线性加速度合量，所以倾斜与平面内摇动都能改变 `targetTiltDeg`；物理内部仍会经既有目标角限幅、速率推进、planner/guard后得到 accepted/effective angle。**本场仅接收二维方向角，不改变重力模长或每步重力强度。**有效屏幕重力方向从物理实际提交值导出：`gravityDir=(sin(effectiveTilt), cos(effectiveTilt))`，不要从原始传感器/目标角构造第二个渲染方向。输入映射所用 X/Y 符号应以 T52 设备确认结果为准。

### 3.2 快速摇晃与扬沙强度

用三轴原始信号的低通估计 `aLP` 与原始值 `aRaw` 的差估计快速分量。这里 `aLP` 同时用于姿态方向平滑；对方向计算用其 X/Y 投影，对摇晃强度计算用三轴残差：

```text
aHigh = aRaw - aLP
shakeMagnitude = norm(aHigh)               // m/s²
shakeInput = clamp((shakeMagnitude - A_on) / (A_full - A_on), 0, 1)
```

`A_on`、`A_full` 是配置项，需 `0 ≤ A_on < A_full`；本稿不填数值、不声称定标。T51可先用确定性合成样本验证映射与 `0/1` 边界，产品默认值必须保持可追踪并标 `NOT_CALIBRATED`，T52采集真实运动后再单独提出参数建议。过滤器时间常数、传感器回调频率和门槛均入报告身份。不得通过改变 `WaterSandField` 的 `shakeRampRate`、`shakeDecay` 或物理预算来弥补传感器标定。

在设备姿态固定但有平面内高频摇动时，平滑合向量方向可以随摇动短时摆动，`shakeInput`也按残差大小升高；二者分别经过 tilt 安全路径与 shake ramp，不互相推导。缓慢倾斜、无高频运动时，方向持续变化而 `shakeInput`应接近0。手机停止摇动后，残差衰减使适配器给 `shakeInput=0`；方向回到设备姿态对应的低频重力方向，悬沙衰减由 `WaterSandField` 既有 shake ramp/decay 处理。设备仍保持倾斜时不得把重力角强制归零；只有用户把设备回正时，姿态输入才回到约0°。

### 3.3 角域、反转与安全

- 所有传感器目标都通过已有 `setTilt()`；不得绕过其 ±89° 限制或绕过场内 planner/guard。适配器可钳位用于防止非法命令，但不得把钳位值记作 accepted/effective angle。
- T50 不授权连续穿过 90°或任意传感器 wrap 自动调用 `invert180()`。传感器读数跨 `±180°` 时，适配器先作角差最短弧归一化供输入平滑；物理目标仍只在 `[-89°,89°]`。若 DEMO 需要完整倒置演示，应通过明确的 invert 事件调用既有 `field.invert180()`，并把该事件同步应用于高度场与粒子池；不能只翻屏幕或只反转向量。
- 页面隐藏/传感器注销/输入超时：停止触摸注入并发送 `shakeInput=0`；最后一个有效姿态可在短暂缺样期间保持，状态标 `STALE`，不得用缺样伪造回正。重置空场时将 tilt 与 shake 输入一起设回0并清除滤波历史。

## 4. T51 最小接口契约

T51负责新 DEMO 文件/页面与 sensor lifecycle；建议新增隔离模块 `entry/src/main/ets/physics/P051GravityInputAdapter.ets`，不要修改 `P011InputMode.ets` 或 `SandCanvas.ets` 共用输入代码。它接收原始样本、维护独立滤波器并返回纯数据：

```ts
interface P051GravityInput {
  source: 'sensor' | 'synthetic' | 'stale';
  sampleTimeMs: number;
  valid: boolean;
  targetTiltDeg: number | null; // null = hold last accepted target
  shakeInput: number;           // normalized [0, 1]; stale/reset = 0
  shakeMagnitudeMps2: number;   // diagnostic only
}

consumeAccelerometer(xMps2: number, yMps2: number, zMps2: number,
  sampleTimeMs: number): P051GravityInput;
reset(): void;
```

`P051EmptySandDemo` 应增加单一物理入口（或等价薄封装）：

```ts
applyDefinedInput(input: P051GravityInput): void {
  if (input.targetTiltDeg !== null) this.field.setTilt(input.targetTiltDeg);
  this.field.setShakeInput(input.shakeInput);
}
```

页面收到样本后只更新 adapter 的最新输入；在下一物理 tick **先成组调用倾角与 shake，再 `field.step(dt)` 一次**。该 field 已绑定 particle route，场内粒子与沉积高度场自然共享 effective tilt / shake commit。发布渲染状态之后，用发布状态中的 effective angle生成 `gravityDirX/Y` 并交给 T49 renderer，同一个 `physicsCommitId`；不从 UI callback 独立提交物理或渲染角。

### 状态与故障约定

| 情形 | 适配器输出/物理处理 |
|---|---|
| 合法姿态样本 | 返回目标角与独立 shakeInput；场内 setter/step继续执行业务安全与预算 |
| X/Y投影过小或姿态暂不可判 | `targetTiltDeg=null`保留最后角；shake仍按三轴高频量单独计算 |
| 无效样本 | 丢弃，不覆盖上次输入；诊断记录 invalid |
| 输入过期/页面隐藏 | source=stale，shake=0，姿态保持最后有效值；停止注沙并注销 sensor |
| 物理 step失败 | 停止后续注沙/输入更新，保留失败码；不由 renderer 伪造方向或继续推进 |
| Reset | adapter.reset；`field.reset()`/场初始化后，tilt=0、shake=0、particle pool和源账本按空场契约为0 |

## 5. Tester 可执行场景与预期

下表先用于 T51 的非真机合成 RED/GREEN 用例。重力加速度 `g`只作为合成尺度符号，不据此声明真实设备标定；`gx/gy`取值按当前 VYG 映射公式构造。传感器实际方向需 T52 真机核对。

| 场景 | 输入 | 预期 |
|---|---|---|
| 静置正向 | `x=0, y=+g`连续样本 | target≈0°；shakeInput=0；场与粒子方向均向下 |
| 正/负倾斜 | `x=−g·sinθ, y=g·cosθ`，θ=±15°、±30° | 目标符号/角度按映射与钳位一致；同 tick `WaterSandField`和粒子消费同一有效角；不是只转动图像 |
| 斜向样本 | 多组非零 X/Y | 输出 `atan2` 的方向；同一方向的幅值缩放不改变角（超过低投影守卫） |
| 局部投影过小 | `hypot(−x,y)≤0.3` | target为null/保持上个角；不得出现 `atan2` 噪声跳变；shake仍独立 |
| 仅高频摇动 | 低通重力方向固定、三轴高频残差递增 | `targetTiltDeg`稳定，shakeInput按配置单调增加且不超 `[0,1]`；shakeEnergy通过既有 ramp 上升并观测到既有扬起路径 |
| 缓慢倾斜无摇动 | 慢变重力方向，无高频残差 | target按角度变更，shakeInput保持0附近；不得由角度大小触发扬沙 |
| 停止摇动但仍倾斜 | 高通残差回0，低频方向保持斜角 | shakeInput回0，方向仍保持倾斜，后续按现有沉降机制恢复 |
| 回正 | `x→0,y→+g`无高频残差 | target回到0°，摇晃强度保持0；实际沉积变化继续由场物理演化 |
| ±89° 边界 | 合成角超边界与跨角 wrap 序列 | setter仅能收到受限角；不得连续穿过90°或自动触发 `invert180()`；guard拒绝时不得以目标角冒充有效角 |
| 生命周期函数 | hide、停传感器、reset后重入 | 注沙停；shake归0；重置后空场仍水/沙/粒子库存零；旧滤波/样本不污染新会话 |

报告需分开记录输入目标角、`targetTilt`、`effectiveTilt`/accepted tilt、`shakeInput`、`shakeEnergy`、source、样本时刻、physicsCommitId、粒子/高度状态；误差源归因区分传感器轴映射、过滤/时延、物理限幅/guard、渲染发布。仅有 `gravityDirX/Y` 改变或屏幕旋转不构成物理通过。

## 6. 风险、限制与解锁

1. **摇动会造成方向短时摆动。** 加速度计总值包含重力和设备运动；本版用平滑后的 X/Y 合向量表示有效方向，并用高频三轴残差幅值表示扬沙强度。若滤波太慢会削弱方向响应，太快会把噪声变成抖动；具体滤波频响仍待T52测量，不得靠调物理判据掩盖。
2. **单设备轴映射不通用。** 当前 X 取反来自现有 VYG-AL00 应用实现；横竖屏、旋转锁定及其他型号映射待 T52 确认，不能用模拟器结果宣称设备一致。
3. **强度映射未标定。** `A_on/A_full`与滤波时常只作为配置输入，须经后续真机数据决定。未标定前 DEMO 可用合成输入验证接口，但需标 `NOT_CALIBRATED`。
4. **二维模型边界。** Z轴仅参与摇动强度，不模拟出屏幕平面的重力/沙堆搬运；如果用户要求完整三维容器物理，需要独立目标、模型与守恒审查。
5. **P-011角域/反转约束继续有效。** 本方案不扩到 ±45°、±90°连续物理轨迹，不增加预算，不绕过 planner/guard，不用渲染旋转伪装物理翻转。倒置只由明确事件执行并复用现有 `invert180()`。

## 7. T51 交接摘要

- 复用：`sensor.ACCELEROMETER`、既有传感器权限/lifecycle模式、输入源职责；`WaterSandField.setTilt()`、`setShakeInput()`、场内既有 ramp/decay、耦合 particle route。
- 新增：只放在 DEMO 专用 adapter 的三轴滤波/角度映射/shake magnitude 归一化、synthetic test seam 与诊断输出；在 `P051EmptySandDemo` 增加合组输入薄封装。
- 不改：`SandCanvas.ets`、共用 `P011InputMode.ets`、P011算法/阈值/预算；不由T50实现业务代码。
- T52 真机任务验证真实方向符号、传感器频率/延迟、抖动分离、停摇衰减、触摸注沙与物理/渲染同帧；T52结果前不标定任何强度参数或宣称真实摇晃效果通过。

## 8. 记录

核验日期：2026-10-25。读取 `docs/goals.md`、T50/T51卡片、`SandCanvas.ets` 传感器输入和角度转换、`P011InputMode.ets`、`WaterSandField.ets` 的 `setTilt`/`setShakeInput`/`invert180`、`P051EmptySandDemo.ets` 与 `EmptySandDemo.ets`；使用官方 HarmonyOS Sensor Service 文档核对加速度计含重力、m/s²单位。无真机、无代码测试、未修改共享物理或渲染实现。目标是交付可实现的输入契约，不宣称 DEMO 已接入或设备已校准。
