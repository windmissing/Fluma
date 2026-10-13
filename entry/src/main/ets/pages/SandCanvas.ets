import { SandGrid } from '../physics/SandGrid';
import { FluidField } from '../physics/FluidField';
import sensor from '@ohos.sensor';
import { FrameCallback } from '@ohos.arkui.UIContext';
import { ComponentContent } from '@ohos.arkui.node';
import { BusinessError } from '@ohos.base';

/** 帧回调适配：postFrameCallback 需要传入 FrameCallback 子类实例 */
class MyFrameCallback extends FrameCallback {
  owner: SandCanvas | null = null;
  onFrame(frameTimeInNano: number): void {
    if (this.owner !== null) {
      this.owner.frame();
    }
  }
}

/**
 * 两相流麻：CA 元胞自动机引擎 + 双模式渲染
 * - 物理：绿（重）/蓝（轻）两相，倾斜流动，蓝被压在绿下时逐粒渗透上浮
 * - 渲染模式 A「细沙」：逐格游程合并 + 8 档 shade 颗粒质感
 * - 渲染模式 B「流体」：界面高度曲线移动平均 + 平滑插值色带 + 表面高光
 *   （物理仍是 CA 渗透，只是把颗粒轮廓拟合成连续流体界面）
 * - 360° 任意重力方向（含倒立）由重力向量直接驱动
 */
/** 8 档 shade 颜色表（绿=重相 / 蓝=轻相），模块级常量避免每次渲染分配 */
const SHADE_GREEN: string[] = [
  '#2E7D32', '#338A35', '#388E3C', '#3D9440', '#43A047', '#4CAF50', '#58BE5A', '#66CC68'
];
const SHADE_BLUE: string[] = [
  '#1565C0', '#1A6FD0', '#1E88E5', '#2492EE', '#2E9AF5', '#3FA6F8', '#55B4FA', '#6FC3FC'
];
/** 流体模式：两相渐变色端点 */
const FLUID_BLUE_TOP = '#5BC0F8';
const FLUID_BLUE_BOT = '#1565C0';
const FLUID_GREEN_TOP = '#7BCB7F';
const FLUID_GREEN_BOT = '#1B5E20';
/** 渲染模式：0=细沙（A），1=流体（B），2=混合（C：FluidField+混合带） */
const MODE_SAND = 0;
const MODE_FLUID = 1;
const MODE_HYBRID = 2;
/** 模式总数（四态循环：M1' 新增液体模式走 CA 液体相规则，渲染复用 B 结构） */
const MODE_COUNT = 4;
/** 液体模式（M1'）：模拟层 SandGrid.liquidMode，渲染层=流体带结构 */
const MODE_LIQUID = 3;

/** BindSheet 面板参数桥接类（s78）：全局 @Builder 无组件 this 绑定，
 *  经 owner 引用回组件实例读写 @State 与方法（wrapBuilder 传参载体）。
 *  tm/fi/renderMode/flowSpeed 是镜像快照：ComponentContent.update 触发
 *  builder 重建时读这些值刷新文案（builder 非响应式，owner 的 @State
 *  变化不会自动触发 sheet 内重渲染——s78 真机实测确认）。 */
class PanelParams {
  owner: SandCanvas | null = null;
  tm: boolean = false;
  fi: boolean = false;
  renderMode: number = MODE_FLUID;
  flowSpeed: number = 3;
}

/** 面板内容全局 Builder（BindSheet 挂载）：内容与原 Stack 内 Column 一致。
 *  按钮 onClick 日志链路保留（验证方 dumpLayout vs hilog 双向一致验收用）。
 *  @State 变更经 owner 触发组件重渲染；builder 内读 owner 的 @State 也能
 *  随之刷新（同一状态对象引用）。 */
@Builder
function panelSheetBuilder(params: PanelParams) {
  Column({ space: 10 }) {
    Text('流麻参数').fontSize(16).fontColor('#FFFFFF').margin({ bottom: 4 })

    Button(params.fi ? '强制倒立:开(180°)' : '强制倒立:关')
      .fontSize(12)
      .width('100%')
      .backgroundColor(params.fi ? '#E65100' : '#455A64')
      .onClick(() => {
        if (params.owner === null) {
          return;
        }
        params.owner.toggleForceInvert();
      })

    Button(params.tm ? '测试模式:开(传感器禁用)' : '测试模式:关(传感器启用)')
      .fontSize(12)
      .width('100%')
      .backgroundColor(params.tm ? '#C62828' : '#455A64')
      .onClick(() => {
        if (params.owner === null) {
          return;
        }
        params.owner.toggleTestMode();
      })

      Button(params.renderMode === MODE_SAND ? '模式:细沙'
        : params.renderMode === MODE_FLUID ? '模式:流体'
        : params.renderMode === MODE_HYBRID ? '模式:混合' : '模式:液体')
        .fontSize(14)
        .width('100%')
        .onClick(() => {
          if (params.owner === null) {
            return;
          }
          params.owner.cycleRenderMode();
        })

    Button('重置')
      .fontSize(14)
      .width('100%')
      .onClick(() => {
        if (params.owner === null) {
          return;
        }
        params.owner.resetField();
      })

    Text(`流速: ${params.flowSpeed}`).fontSize(13).fontColor('#CCCCCC').width('100%')
    Slider({ value: params.flowSpeed, min: 1, max: 6, step: 1 })
      .blockColor('#43A047').selectedColor('#43A047')
      .onChange((value: number) => {
        if (params.owner === null) {
          return;
        }
        params.owner.setFlowSpeed(value);
      })
  }
  .width('100%')
  .padding(16)
}

@Entry
@Component
struct SandCanvas {
  @State displayInfo: string = '';
  @State showPanel: boolean = false;
  @State flowSpeed: number = 3;      // 每帧模拟步数（预留，当前 rAF 单步）
  private tiltX: number = 0;
  private lastTiltX: number = 0;
  private gestureHoldUntil: number = 0;
  private fps: number = 0;
  private canvasW: number = 0;
  private canvasH: number = 0;
  private ctx: CanvasRenderingContext2D = new CanvasRenderingContext2D(new RenderingContextSettings(true));
  private lastTime: number = 0;
  private frameCount: number = 0;
  private fpsTime: number = 0;
  private rafId: number = -1;
  private field: SandGrid | null = null;
  private fluidField: FluidField | null = null;
  private boxX: number = 0;
  private boxY: number = 0;
  private boxW: number = 0;
  private boxH: number = 0;
  private cellPx: number = 4;        // 每粒像素高
  private sensorGx: number = 0;
  private sensorGy: number = 0;
  private useSensor: boolean = true;
  /** 测试模式：true 时传感器永不接管，重力完全由手势序列构造（录制自动化用）。
   *  背景：测试机放置面不水平（USB 线张力），传感器接管会让"手势归零"后液体
   *  被真实重力继续拉斜，TC-06 归零收敛判定失效。测试模式由 uitest 全权构造重力。
   *  s78 修复：必须 @State——此前是 private，build() 读它不建立响应式依赖，
   *  toggle 后按钮文案不刷新（dumpLayout 永远读到首帧值，s77r 步骤 5 证据）。 */
  @State testMode: boolean = false;
  /** 强制倒立（仅 testMode 生效）：重力直接置 180°，主轴反转。
   *  用于 TC-09/10 倒立场景补测（手势钳位 ±80° 到不了 90°-180°）。
   *  必须 @State：否则按钮文案不随状态刷新，dumpLayout 验证会读到陈旧值（假阳性）。 */
  @State forceInvert: boolean = false;
  /** 上次长按触发时刻（ms）：长按抬起附带的单击要忽略，防 forceInvert 误翻转 */
  private lastLongPressTime: number = 0;
  /** frame 内倒立状态边沿检测（s77r）：状态翻转时打一条日志，供 hilog 判定传导链 */
  private wasForceInvert: boolean = false;
  private needRedraw: boolean = true;
  private lastMoved: number = 0;
  /** 帧回调对象（postFrameCallback 需要类实例，循环复用避免分配） */
  private frameCb: MyFrameCallback = new MyFrameCallback();
  /** 手势像素→角度映射系数（提高跟手性：200px 满行程 ≈ 80°） */
  private readonly gestureSensitivity: number = 0.4;
  /** 渲染模式：0=细沙 A，1=流体 B（面板可切换） */
  @State renderMode: number = MODE_FLUID;
  /** 传感器角速度上限 °/s（液体转动惯性，翻转时渐变过渡） */
  private readonly maxOmega: number = 240;
  private lastFrameTime: number = 0;
  /** BindSheet 内容（s78：面板迁移至系统半模态浮层，与画布手势 hitTest 隔离）。
   *  ComponentContent 在 aboutToAppear 构造；@Builder 函数里通过 params 镜像
   *  快照渲染文案，toggle 后 syncSheet() 更新快照并 updateBindSheet 重建。 */
  private sheetContent: ComponentContent<PanelParams> | null = null;
  private sheetParams: PanelParams = new PanelParams();

  private makePanelParams(): PanelParams {
    this.sheetParams.owner = this;
    return this.sheetParams;
  }

  /** 镜像快照写入 + updateBindSheet 强制 builder 重建（缺陷 2 根治） */
  private syncSheet(): void {
    if (this.sheetContent === null) {
      return;
    }
    this.sheetParams.tm = this.testMode;
    this.sheetParams.fi = this.forceInvert;
    this.sheetParams.renderMode = this.renderMode;
    this.sheetParams.flowSpeed = this.flowSpeed;
    this.sheetContent.update(this.sheetParams);
  }

  aboutToAppear(): void {
    this.frameCb.owner = this;
    // s78：构造 BindSheet 内容（owner 桥接 this，builder 内按钮经公开方法改状态）
    this.sheetContent = new ComponentContent(this.getUIContext(), wrapBuilder(panelSheetBuilder), this.makePanelParams());
    // 测试自动化：want 参数（EntryAbility 写入 AppStorage）初始化测试状态，
    // 零 UI 依赖（面板按钮热区随布局变化不可预测，多轮实测已弃用坐标点击）
    const tm = AppStorage.get<boolean>('testmode');
    if (tm !== undefined) {
      this.testMode = tm;
      if (tm) {
        this.useSensor = false;
        this.gestureHoldUntil = Date.now() + 24 * 3600 * 1000;
      }
    }
    const fi = AppStorage.get<boolean>('forceinvert');
    if (fi !== undefined) {
      this.forceInvert = fi;
    }
    const rm = AppStorage.get<number>('rendermode');
    if (rm !== undefined) {
      this.renderMode = rm;
    }
    console.info(`[ForceInvert] aboutToAppear: testMode=${this.testMode} forceInvert=${this.forceInvert} useSensor=${this.useSensor} renderMode=${this.renderMode}`);
    try {
      sensor.on(sensor.SensorId.ACCELEROMETER, (data: sensor.AccelerometerResponse) => {
        // 轻度低通：跟手优先
        this.sensorGx = this.sensorGx * 0.65 + data.x * 0.35;
        this.sensorGy = this.sensorGy * 0.65 + data.y * 0.35;
      });
    } catch (e) {
    }
  }

  aboutToDisappear(): void {
    this.frameCb.owner = null;
    try {
      sensor.off(sensor.SensorId.ACCELEROMETER);
    } catch (e) {
    }
    if (this.sheetContent !== null) {
      this.sheetContent.dispose();
    }
  }

  /** ===== 面板按钮桥接方法（s78：全局 builder 经 PanelParams.owner 调用） =====
   *  每个 toggle 后 syncSheet()：镜像快照写入 params 并 updateBindSheet 强制
   *  builder 重建 → sheet 内按钮文案与实际状态一致（缺陷 2 根治）。 */
  toggleForceInvert(): void {
    this.forceInvert = !this.forceInvert;
    console.info(`[ForceInvert] panel toggle -> ${this.forceInvert} (testMode=${this.testMode})`);
    this.syncSheet();
  }

  toggleTestMode(): void {
    this.testMode = !this.testMode;
    console.info(`[ForceInvert] panel toggle testMode -> ${this.testMode}`);
    if (this.testMode) {
      // 进入测试模式：冻结手势角度为重力源，传感器数据不再参与
      this.useSensor = false;
      this.gestureHoldUntil = Date.now() + 24 * 3600 * 1000;
    } else {
      // 退出测试模式：恢复传感器路径，同时清掉强制倒立
      this.useSensor = true;
      this.gestureHoldUntil = 0;
      this.forceInvert = false;
    }
    this.syncSheet();
  }

  cycleRenderMode(): void {
    this.renderMode = (this.renderMode + 1) % MODE_COUNT;
    this.needRedraw = true;
    console.info(`[ForceInvert] panel renderMode -> ${this.renderMode}`);
    this.syncSheet();
  }

  resetField(): void {
    this.initField();
    console.info(`[ForceInvert] panel reset`);
  }

  setFlowSpeed(v: number): void {
    this.flowSpeed = v;
  }

  /** 打开浮层前先同步一次快照（防上次会话遗留陈旧文案） */
  openPanelSheet(): void {
    if (this.sheetContent === null) {
      return;
    }
    this.getUIContext().openBindSheet(this.sheetContent, {
      height: 460,
      showClose: true,
      backgroundColor: '#0D0D1A',
      dragBar: false,
      onDisappear: () => {
        // 浮层下拉/点关闭键关闭时同步状态，下次点"调参"才能再弹
        if (this.showPanel) {
          this.showPanel = false;
          console.info('[ForceInvert] sheet onDisappear');
        }
      }
    }).then(() => {
      console.info('[ForceInvert] openBindSheet success');
      this.syncSheet();
    }).catch((err: BusinessError) => {
      console.error(`[ForceInvert] openBindSheet error: ${err.code} ${err.message}`);
    });
  }

  private initField(): void {
    this.boxW = Math.floor(this.canvasW * 0.72);
    this.boxH = Math.floor(this.canvasH * 0.42);
    this.boxX = Math.floor((this.canvasW - this.boxW) / 2);
    this.boxY = Math.floor((this.canvasH - this.boxH) / 2);
    const cols = Math.floor(this.boxW / this.cellPx);
    const rows = Math.floor(this.boxH / this.cellPx);
    this.field = new SandGrid(cols, rows);
    // 渗透速率用 SandGrid 默认值 sinkChance=0.022（Node 标定：约 40s 渗完，慢渗透）

    // 静态背景
    const ctx = this.ctx;
    const bgGrad = ctx.createLinearGradient(0, 0, 0, this.canvasH);
    bgGrad.addColorStop(0, '#0a0a14');
    bgGrad.addColorStop(1, '#1a1a2e');
    ctx.fillStyle = bgGrad;
    ctx.fillRect(0, 0, this.canvasW, this.canvasH);

    // 初始分层：蓝（轻）在上 25%~50%，绿（重）在下 50%~85%
    // 填充率 60%（留 40% 空气供流动，防 CA 密实锁死）
    const g = this.field;
    const topRow = Math.floor(rows * 0.25);
    const midRow = Math.floor(rows * 0.5);
    const botRow = Math.floor(rows * 0.85);
    for (let c = 0; c < cols; c++) {
      for (let r = topRow; r < midRow; r++) {
        g.setCell(c, r, SandGrid.SAND, 1);   // 蓝（轻）
      }
      for (let r = midRow; r < botRow; r++) {
        g.setCell(c, r, SandGrid.SAND, 0);   // 绿（重）
      }
    }
    // 混合模型（C）：FluidField 同步初始化
    const ff = new FluidField(cols, rows);
    for (let c = 0; c < cols; c++) {
      ff.light[c] = midRow - topRow;
      ff.heavy[c] = botRow - midRow;
    }
    this.fluidField = ff;
    this.needRedraw = true;
  }

  frame(): void {
    const f = this.field;
    if (f === null) {
      return;
    }
    // FPS 统计
    const now = Date.now();
    this.lastFrameTime = now;
    if (this.lastTime > 0) {
      const delta = now - this.lastTime;
      this.frameCount++;
      this.fpsTime += delta;
      if (this.fpsTime >= 500) {
        this.fps = Math.round(this.frameCount * 1000 / this.fpsTime);
        this.frameCount = 0;
        this.fpsTime = 0;
        this.displayInfo = `FPS=${this.fps}`;
      }
    }
    this.lastTime = now;

    // 重力向量：手势（保持期内）优先，其次传感器
    // CA 引擎吃重力向量 (gx, gy)，360° 任意方向自动正确（含倒立）
    let gx: number;
    let gy: number;
    if (this.useSensor && !this.testMode && now >= this.gestureHoldUntil) {
      // VYG-AL00 加速度计 x 轴与屏幕左右相反，取反镜像
      const sx = -this.sensorGx;
      const sy = this.sensorGy;
      // 平放检测：合成加速度过小时 atan2 噪声主导 → 保持上次角度
      let tilt = this.tiltX;
      if (Math.sqrt(sx * sx + sy * sy) > 1.5) {
        tilt = Math.atan2(sx, sy) * 180 / Math.PI;
        // 角速度限制（液体转动惯性）：快翻时渐变过渡
        let diff = tilt - this.tiltX;
        while (diff > 180) {
          diff -= 360;
        }
        while (diff < -180) {
          diff += 360;
        }
        const dt = this.lastFrameTime > 0 ? (now - this.lastFrameTime) / 1000 : 0;
        const maxStep = this.maxOmega * dt;
        if (Math.abs(diff) > maxStep && maxStep > 0) {
          let next = this.tiltX + Math.sign(diff) * maxStep;
          if (next > 180) {
            next -= 360;
          } else if (next < -180) {
            next += 360;
          }
          this.tiltX = next;
        } else {
          this.tiltX = tilt;
        }
      }
      tilt = this.tiltX;
      // 角度 → 重力向量（屏幕坐标：y 向下为正；θ=0 重力朝下）
      const rad = tilt * Math.PI / 180;
      gx = Math.sin(rad);
      gy = Math.cos(rad);
    } else {
      // 测试模式：重力全由手势/testMode 开关构造
      if (this.testMode && this.forceInvert) {
        // 强制倒立：重力直接置 180°（主轴反转），绕过手势 tiltX
        // 仍走 tiltX 变量以保持 maxOmega 渐变语义：向 180° 渐变过渡而非瞬跳
        // 状态边沿日志（s77r）：倒立开始生效打一条，供验证方 hilog 判定传导链
        if (!this.wasForceInvert) {
          console.info(`[ForceInvert] frame: invert ACTIVE, tiltX=${this.tiltX.toFixed(1)} -> 180`);
        }
        this.wasForceInvert = true;
        let diff = 180 - this.tiltX;
        while (diff > 180) {
          diff -= 360;
        }
        while (diff < -180) {
          diff += 360;
        }
        const dt = this.lastFrameTime > 0 ? (now - this.lastFrameTime) / 1000 : 0;
        const maxStep = this.maxOmega * dt;
        if (Math.abs(diff) > maxStep && maxStep > 0) {
          let next = this.tiltX + Math.sign(diff) * maxStep;
          if (next > 180) {
            next -= 360;
          } else if (next < -180) {
            next += 360;
          }
          this.tiltX = next;
        } else {
          this.tiltX = 180;
        }
      } else if (this.wasForceInvert) {
        // 倒立关闭边沿（s80 修复 bug A）：tiltX 从 180 渐变回 0（maxOmega 限速）。
        // s79 及之前此处只打日志不更新 tiltX——tiltX 残留 180°，重力向量
        // (sin180, cos180)=(0,-1) 仍指向上方，物理永远回不了正（TC-12/13 FAIL
        // 根因：hilog "invert OFF tiltX=180.0 -> 0" 只是文案，实际未渐变）。
        console.info(`[ForceInvert] frame: invert OFF, tiltX=${this.tiltX.toFixed(1)} -> 0`);
        this.wasForceInvert = false;
        let diff = 0 - this.tiltX;
        while (diff > 180) {
          diff -= 360;
        }
        while (diff < -180) {
          diff += 360;
        }
        const dtOff = this.lastFrameTime > 0 ? (now - this.lastFrameTime) / 1000 : 0;
        const maxStepOff = this.maxOmega * dtOff;
        if (Math.abs(diff) > maxStepOff && maxStepOff > 0) {
          let nextOff = this.tiltX + Math.sign(diff) * maxStepOff;
          if (nextOff > 180) {
            nextOff -= 360;
          } else if (nextOff < -180) {
            nextOff += 360;
          }
          this.tiltX = nextOff;
        } else {
          this.tiltX = 0;
        }
      } else if (this.tiltX !== 0 && this.testMode) {
        // 倒立关闭后的渐变续程（s80）：OFF 只在边沿帧触发一次，此分支接续
        // 后续帧的 tiltX -> 0 渐变直至归零（否则渐变只走一步就停）。
        let diff2 = 0 - this.tiltX;
        while (diff2 > 180) {
          diff2 -= 360;
        }
        while (diff2 < -180) {
          diff2 += 360;
        }
        const dt2 = this.lastFrameTime > 0 ? (now - this.lastFrameTime) / 1000 : 0;
        const maxStep2 = this.maxOmega * dt2;
        if (Math.abs(diff2) > maxStep2 && maxStep2 > 0) {
          let next2 = this.tiltX + Math.sign(diff2) * maxStep2;
          if (next2 > 180) {
            next2 -= 360;
          } else if (next2 < -180) {
            next2 += 360;
          }
          this.tiltX = next2;
        } else {
          this.tiltX = 0;
        }
      }
      const rad = this.tiltX * Math.PI / 180;
      gx = Math.sin(rad);
      gy = Math.cos(rad);
    }

    const g = this.field;
    if (g === null) {
      this.getUIContext().postFrameCallback(this.frameCb);
      return;
    }
    if (this.renderMode === MODE_HYBRID) {
      const ff = this.fluidField;
      if (ff !== null) {
        ff.step(this.tiltX);
        this.needRedraw = true;
      }
      if (this.needRedraw) {
        this.renderHybrid(this.ctx);
        this.needRedraw = false;
      }
      this.getUIContext().postFrameCallback(this.frameCb);
      return;
    }
    // 模式开关同步到模拟层（M1'：液体模式走 CA 液体相规则）
    g.setLiquidMode(this.renderMode === MODE_LIQUID);
    g.step(gx, gy);
    if (g.hasMoved()) {
      this.needRedraw = true;
    }
    if (this.needRedraw) {
      this.render();
      this.needRedraw = false;
    }
    // 持续帧循环（vsync 对齐，60Hz）
    this.getUIContext().postFrameCallback(this.frameCb);
  }

  private render(): void {
    const ctx = this.ctx;
    const g = this.field;
    if (g === null) {
      return;
    }
    // 沙框底色
    ctx.fillStyle = '#101024';
    ctx.fillRect(this.boxX, this.boxY, this.boxW, this.boxH);

    const cell = this.cellPx;
    if (this.renderMode === MODE_FLUID || this.renderMode === MODE_LIQUID) {
      // 流体 B 与液体模式（M1'）共用分相计数渲染（s75 方法论两带结构）
      this.renderFluid(ctx, g, cell);
      // 玻璃框描边
      ctx.strokeStyle = 'rgba(255,255,255,0.35)';
      ctx.lineWidth = 3;
      ctx.strokeRect(this.boxX, this.boxY, this.boxW, this.boxH);
      return;
    }
    // 行游程合并渲染：同色连续格一次 fillRect（66k 格 → ~2k 矩形）
    // 8 档 shade 亮度 = 细沙颗粒质感
    for (let r = 0; r < g.rows; r++) {
      let runStart = -1;
      let runShade = -1;
      let runPhase = -1;
      for (let c = 0; c <= g.cols; c++) {
        const t = c < g.cols ? g.get(c, r) : -1;
        const ph = t === SandGrid.SAND ? g.getPhase(c, r) : -1;
        const sh = t === SandGrid.SAND ? g.getShade(c, r) : -1;
        if (t === SandGrid.SAND && runStart >= 0 && ph === runPhase && sh === runShade) {
          continue; // 游程延续
        }
        // 游程结束 → 画
        if (runStart >= 0) {
          const n = c - runStart;
          if (runPhase === 0) {
            ctx.fillStyle = SHADE_GREEN[runShade];
          } else {
            ctx.fillStyle = SHADE_BLUE[runShade];
          }
          ctx.fillRect(this.boxX + runStart * cell, this.boxY + r * cell, n * cell, cell);
        }
        // 开新游程
        if (t === SandGrid.SAND) {
          runStart = c;
          runPhase = ph;
          runShade = sh;
        } else {
          runStart = -1;
          runPhase = -1;
          runShade = -1;
        }
      }
    }

    // 玻璃框描边
    ctx.strokeStyle = 'rgba(255,255,255,0.35)';
    ctx.lineWidth = 3;
    ctx.strokeRect(this.boxX, this.boxY, this.boxW, this.boxH);
  }

  /**
   * 流体模式渲染（B）：把 CA 颗粒场拟合成连续流体界面（体积守恒版，修 VV-01）。
   * 每列统计 heavyCount / lightCount 分相总粒数（不再取"第一个沙粒/绿粒"行号，
   * 列内交错 [绿,蓝,绿,蓝] 不影响计数）→ 每列蓝带面积恒等于蓝粒总数，
   * 渲染层分相体积严格守恒（VV-01 达标）。
   * 高度定义：界面线 = rows - heavyS；液面线 = rows - heavyS - lightS。
   * 渗入量（界面行以下绿区内的蓝粒数）驱动界面下方蓝绿过渡带，保留"渗入感"；
   * 静置分层态渗入量为 0，过渡带消失，与旧版分层视觉一致。
   */
  private renderFluid(ctx: CanvasRenderingContext2D, g: SandGrid, cell: number): void {
    const n = g.cols;
    const rows = g.rows;
    // 1) 逐列分相计数（总量守恒的关键：计数而非行号）
    const heavyCnt = new Array<number>(n);
    const lightCnt = new Array<number>(n);
    const mixDepth = new Array<number>(n);
    for (let c = 0; c < n; c++) {
      let hc = 0;
      let lc = 0;
      for (let r = 0; r < rows; r++) {
        if (g.get(c, r) === SandGrid.SAND) {
          if (g.getPhase(c, r) === 0) {
            hc++;
          } else {
            lc++;
          }
        }
      }
      heavyCnt[c] = hc;
      lightCnt[c] = lc;
      // 渗入量 = 界面行（rows - hc）以下（绿区）的蓝粒数
      let mix = 0;
      for (let r = rows - hc; r < rows; r++) {
        if (g.get(c, r) === SandGrid.SAND && g.getPhase(c, r) === 1) {
          mix++;
        }
      }
      mixDepth[c] = mix;
    }
    // 2) 双次移动平均平滑（窗口 9 × 2 遍）——对"计数"平滑（线性变换保总量），
    //    而非旧版对"行号"平滑（行号平滑在界面交错后蓝相漏画）
    const smoothPass = (srcArr: number[]): number[] => {
      const out = new Array<number>(n);
      const w = 4;
      for (let c = 0; c < n; c++) {
        let sum = 0;
        let cnt = 0;
        for (let k = -w; k <= w; k++) {
          const cc = Math.max(0, Math.min(n - 1, c + k));
          sum += srcArr[cc];
          cnt++;
        }
        out[c] = sum / cnt;
      }
      return out;
    };
    const heavyS = smoothPass(smoothPass(heavyCnt));
    const lightS = smoothPass(smoothPass(lightCnt));
    const mixS = smoothPass(smoothPass(mixDepth));
    // 倒立渲染（s77/s80）：按 tiltX 连续判定而非 forceInvert 布尔——
    // 布尔判定会在 OFF 边沿瞬间翻转绘制方向（画面阶跃）；归一化角度 >90°
    // 画倒立布局，与物理重力主轴一致，渐变回程（180→0）过 90° 自然切换。
    const tiltNorm = ((this.tiltX % 360) + 360) % 360;
    const inverted = this.testMode && (tiltNorm > 90 && tiltNorm < 270);
    const ifaceY = (c: number): number =>
      inverted ? this.boxY + heavyS[c] * cell : this.boxY + (rows - heavyS[c]) * cell;
    const surfY = (c: number): number =>
      inverted ? this.boxY + (heavyS[c] + lightS[c]) * cell
               : this.boxY + (rows - heavyS[c] - lightS[c]) * cell;
    // 渗入过渡带方向：正立向下（+mixS），倒立向上（-mixS）
    const mixDir = inverted ? -1 : 1;
    // 3) 连续 Path 填充：曲线转二次贝塞尔路径，一次填充消除逐列接缝锯齿
    const bottomY = this.boxY + this.boxH;
    const right = this.boxX + this.boxW;
    // 绿带 Path：从底角沿界面曲线到另一底角（倒立时绿带浮顶，从顶角起）
    ctx.beginPath();
    if (inverted) {
      ctx.moveTo(this.boxX, this.boxY);
      ctx.lineTo(this.boxX, ifaceY(0));
      for (let c = 1; c < n; c++) {
        const yc = ifaceY(c);
        const yPrev = ifaceY(c - 1);
        ctx.quadraticCurveTo(this.boxX + (c - 1) * cell, yPrev, this.boxX + (c - 0.5) * cell, (yPrev + yc) / 2);
      }
      ctx.lineTo(right, ifaceY(n - 1));
      ctx.lineTo(right, this.boxY);
    } else {
      ctx.moveTo(this.boxX, bottomY);
      ctx.lineTo(this.boxX, ifaceY(0));
      for (let c = 1; c < n; c++) {
        const yc = ifaceY(c);
        const yPrev = ifaceY(c - 1);
        ctx.quadraticCurveTo(this.boxX + (c - 1) * cell, yPrev, this.boxX + (c - 0.5) * cell, (yPrev + yc) / 2);
      }
      ctx.lineTo(right, ifaceY(n - 1));
      ctx.lineTo(right, bottomY);
    }
    ctx.closePath();
    const gradG = ctx.createLinearGradient(0, this.boxY, 0, bottomY);
    gradG.addColorStop(0.3, FLUID_GREEN_TOP);
    gradG.addColorStop(1, FLUID_GREEN_BOT);
    ctx.fillStyle = gradG;
    ctx.fill();
    // 渗透过渡带 Path：界面曲线 → 界面下/上方 mixS 深度曲线（方向随重力翻转）
    ctx.beginPath();
    ctx.moveTo(this.boxX, ifaceY(0));
    for (let c = 1; c < n; c++) {
      const yc = ifaceY(c);
      const yPrev = ifaceY(c - 1);
      ctx.quadraticCurveTo(this.boxX + (c - 1) * cell, yPrev, this.boxX + (c - 0.5) * cell, (yPrev + yc) / 2);
    }
    ctx.lineTo(right, ifaceY(n - 1) + mixDir * mixS[n - 1] * cell);
    for (let c = n - 1; c >= 1; c--) {
      const yc = ifaceY(c) + mixDir * mixS[c] * cell;
      const yPrev = ifaceY(c - 1) + mixDir * mixS[c - 1] * cell;
      ctx.quadraticCurveTo(this.boxX + c * cell, yc, this.boxX + (c - 0.5) * cell, (yPrev + yc) / 2);
    }
    ctx.lineTo(this.boxX, ifaceY(0) + mixDir * mixS[0] * cell);
    ctx.closePath();
    const gradMix = ctx.createLinearGradient(0, this.boxY, 0, bottomY);
    gradMix.addColorStop(0, 'rgba(91,192,248,0.55)');
    gradMix.addColorStop(1, 'rgba(91,192,248,0)');
    ctx.fillStyle = gradMix;
    ctx.fill();
    // 蓝带 Path：液面曲线与界面曲线围合（每列蓝带高度 = lightS ≈ lightCount → 守恒）
    // 倒立时蓝带沉于界面之下（液面线 > 界面线），填充方向随重力翻转
    ctx.beginPath();
    if (inverted) {
      ctx.moveTo(this.boxX, surfY(0));
      for (let c = 1; c < n; c++) {
        const yc = surfY(c);
        const yPrev = surfY(c - 1);
        ctx.quadraticCurveTo(this.boxX + (c - 1) * cell, yPrev, this.boxX + (c - 0.5) * cell, (yPrev + yc) / 2);
      }
      ctx.lineTo(right, surfY(n - 1));
      ctx.lineTo(right, ifaceY(n - 1));
      for (let c = n - 1; c >= 1; c--) {
        const yc = ifaceY(c);
        const yPrev = ifaceY(c - 1);
        ctx.quadraticCurveTo(this.boxX + c * cell, yc, this.boxX + (c - 0.5) * cell, (yPrev + yc) / 2);
      }
      ctx.lineTo(this.boxX, ifaceY(0));
    } else {
      ctx.moveTo(this.boxX, surfY(0));
      for (let c = 1; c < n; c++) {
        const yc = surfY(c);
        const yPrev = surfY(c - 1);
        ctx.quadraticCurveTo(this.boxX + (c - 1) * cell, yPrev, this.boxX + (c - 0.5) * cell, (yPrev + yc) / 2);
      }
      ctx.lineTo(right, ifaceY(n - 1));
      for (let c = n - 1; c >= 1; c--) {
        const yc = ifaceY(c);
        const yPrev = ifaceY(c - 1);
        ctx.quadraticCurveTo(this.boxX + c * cell, yc, this.boxX + (c - 0.5) * cell, (yPrev + yc) / 2);
      }
      ctx.lineTo(this.boxX, ifaceY(0));
    }
    ctx.closePath();
    // 倒立时蓝带在界面下方，渐变色也翻转（蓝顶色朝界面、深蓝朝液面）
    const gradB = inverted
      ? ctx.createLinearGradient(0, bottomY, 0, this.boxY)
      : ctx.createLinearGradient(0, this.boxY, 0, bottomY);
    gradB.addColorStop(0, FLUID_BLUE_TOP);
    gradB.addColorStop(0.45, FLUID_BLUE_BOT);
    ctx.fillStyle = gradB;
    ctx.fill();
    // 表面高光：沿液面曲线的亮描边
    ctx.beginPath();
    ctx.moveTo(this.boxX, surfY(0));
    for (let c = 1; c < n; c++) {
      const yc = surfY(c);
      const yPrev = surfY(c - 1);
      ctx.quadraticCurveTo(this.boxX + (c - 1) * cell, yPrev, this.boxX + (c - 0.5) * cell, (yPrev + yc) / 2);
    }
    ctx.strokeStyle = 'rgba(200,235,255,0.55)';
    ctx.lineWidth = 2.5;
    ctx.stroke();
    // 4) 残余颗粒纹理：在色带上叠加低透明度颗粒噪点（保留细沙质感）
    ctx.globalAlpha = 0.12;
    for (let r = 0; r < g.rows; r += 2) {
      for (let c = 0; c < n; c += 2) {
        if (g.get(c, r) === SandGrid.SAND) {
          const sh = g.getShade(c, r);
          ctx.fillStyle = sh % 2 === 0 ? '#FFFFFF' : '#000000';
          ctx.fillRect(this.boxX + c * cell, this.boxY + r * cell, cell, cell);
        }
      }
    }
    ctx.globalAlpha = 1.0;
  }

  /**
   * 混合模式渲染（C）：FluidField 平直界面 + 渗透混合带。
   * 界面曲线 = 贝塞尔平滑（同 renderFluid）；混合带 = 界面下方 ero 深度内
   * 蓝绿按比例混色（ero 越深混入越多），表现"蓝渗入绿"的渐进过程。
   */
  private renderHybrid(ctx: CanvasRenderingContext2D): void {
    const ff = this.fluidField;
    if (ff === null) {
      return;
    }
    const n = ff.cols;
    const cell = this.cellPx;
    ctx.fillStyle = '#101024';
    ctx.fillRect(this.boxX, this.boxY, this.boxW, this.boxH);
    const bottomY = this.boxY + this.boxH;
    const right = this.boxX + this.boxW;
    // 界面高度曲线转贝塞尔 Path 的辅助：二次贝塞尔中点平滑
    // 绿带 Path：左下角 → 界面曲线 → 右下角
    ctx.beginPath();
    ctx.moveTo(this.boxX, bottomY);
    ctx.lineTo(this.boxX, this.boxY + (ff.cap - ff.heavy[0]) * cell);
    for (let c = 1; c < n; c++) {
      const xc = this.boxX + c * cell;
      const yc = this.boxY + (ff.cap - ff.heavy[c]) * cell;
      const xPrev = this.boxX + (c - 1) * cell;
      const yPrev = this.boxY + (ff.cap - ff.heavy[c - 1]) * cell;
      ctx.quadraticCurveTo(xPrev, yPrev, (xPrev + xc) / 2, (yPrev + yc) / 2);
    }
    ctx.lineTo(right, this.boxY + (ff.cap - ff.heavy[n - 1]) * cell);
    ctx.lineTo(right, bottomY);
    ctx.closePath();
    const gradG = ctx.createLinearGradient(0, this.boxY, 0, bottomY);
    gradG.addColorStop(0.3, FLUID_GREEN_TOP);
    gradG.addColorStop(1, FLUID_GREEN_BOT);
    ctx.fillStyle = gradG;
    ctx.fill();
    // 混合带 Path：界面曲线下方 eroDepth 的带状区域（贝塞尔上下边）
    ctx.beginPath();
    ctx.moveTo(this.boxX, this.boxY + (ff.cap - ff.heavy[0]) * cell);
    for (let c = 1; c < n; c++) {
      const xc = this.boxX + c * cell;
      const yc = this.boxY + (ff.cap - ff.heavy[c]) * cell;
      const xPrev = this.boxX + (c - 1) * cell;
      const yPrev = this.boxY + (ff.cap - ff.heavy[c - 1]) * cell;
      ctx.quadraticCurveTo(xPrev, yPrev, (xPrev + xc) / 2, (yPrev + yc) / 2);
    }
    ctx.lineTo(right, this.boxY + (ff.cap - ff.heavy[n - 1]) * cell + Math.min(ff.light[n - 1], ff.ero[n - 1]) * cell);
    for (let c = n - 1; c >= 1; c--) {
      const xc = this.boxX + c * cell;
      const yc = this.boxY + (ff.cap - ff.heavy[c]) * cell + Math.min(ff.light[c], ff.ero[c]) * cell;
      const xPrev = this.boxX + (c - 1) * cell;
      const yPrev = this.boxY + (ff.cap - ff.heavy[c - 1]) * cell + Math.min(ff.light[c - 1], ff.ero[c - 1]) * cell;
      ctx.quadraticCurveTo(xc, yc, (xPrev + xc) / 2, (yPrev + yc) / 2);
    }
    ctx.lineTo(this.boxX, this.boxY + (ff.cap - ff.heavy[0]) * cell + Math.min(ff.light[0], ff.ero[0]) * cell);
    ctx.closePath();
    const gradMix = ctx.createLinearGradient(0, this.boxY, 0, bottomY);
    gradMix.addColorStop(0, 'rgba(91,192,248,0.55)');
    gradMix.addColorStop(1, 'rgba(91,192,248,0)');
    ctx.fillStyle = gradMix;
    ctx.fill();
    // 蓝带 Path：液面曲线与界面曲线围合
    ctx.beginPath();
    ctx.moveTo(this.boxX, this.boxY + (ff.cap - ff.total(0)) * cell);
    for (let c = 1; c < n; c++) {
      const xc = this.boxX + c * cell;
      const yc = this.boxY + (ff.cap - ff.total(c)) * cell;
      const xPrev = this.boxX + (c - 1) * cell;
      const yPrev = this.boxY + (ff.cap - ff.total(c - 1)) * cell;
      ctx.quadraticCurveTo(xPrev, yPrev, (xPrev + xc) / 2, (yPrev + yc) / 2);
    }
    ctx.lineTo(right, this.boxY + (ff.cap - ff.heavy[n - 1]) * cell);
    for (let c = n - 1; c >= 1; c--) {
      const xc = this.boxX + c * cell;
      const yc = this.boxY + (ff.cap - ff.heavy[c]) * cell;
      const xPrev = this.boxX + (c - 1) * cell;
      const yPrev = this.boxY + (ff.cap - ff.heavy[c - 1]) * cell;
      ctx.quadraticCurveTo(xc, yc, (xPrev + xc) / 2, (yPrev + yc) / 2);
    }
    ctx.lineTo(this.boxX, this.boxY + (ff.cap - ff.heavy[0]) * cell);
    ctx.closePath();
    const gradB = ctx.createLinearGradient(0, this.boxY, 0, bottomY);
    gradB.addColorStop(0, FLUID_BLUE_TOP);
    gradB.addColorStop(0.45, FLUID_BLUE_BOT);
    ctx.fillStyle = gradB;
    ctx.fill();
    // 液面高光描边
    ctx.beginPath();
    ctx.moveTo(this.boxX, this.boxY + (ff.cap - ff.total(0)) * cell);
    for (let c = 1; c < n; c++) {
      const xc = this.boxX + c * cell;
      const yc = this.boxY + (ff.cap - ff.total(c)) * cell;
      const xPrev = this.boxX + (c - 1) * cell;
      const yPrev = this.boxY + (ff.cap - ff.total(c - 1)) * cell;
      ctx.quadraticCurveTo(xPrev, yPrev, (xPrev + xc) / 2, (yPrev + yc) / 2);
    }
    ctx.strokeStyle = 'rgba(200,235,255,0.55)';
    ctx.lineWidth = 2.5;
    ctx.stroke();
    // 玻璃框
    ctx.strokeStyle = 'rgba(255,255,255,0.35)';
    ctx.lineWidth = 3;
    ctx.strokeRect(this.boxX, this.boxY, this.boxW, this.boxH);
  }

  build() {
    Stack() {
      Canvas(this.ctx)
        .width('100%')
        .height('100%')
        .onReady(() => {
          this.canvasW = this.ctx.width;
          this.canvasH = this.ctx.height;
          this.initField();
          this.frame();
        })

      Text(this.displayInfo)
        .fontSize(16)
        .fontColor('#FFFFFF')
        .position({ x: 20, y: 40 })


      Button(this.showPanel ? '收起' : '调参')
        .fontSize(14)
        .width(70)
        .height(36)
        .position({ x: '82%', y: 10 })
        .onClick(() => {
          this.showPanel = !this.showPanel;
          // s78：面板改 BindSheet 系统浮层（缺陷 1 根治）。原 Stack 内绝对定位
          // Column 与根手势（LongPress/Pan）同层竞争：uitest 点击落在按钮视觉
          // 边界时事件先被根手势捕获→吞点击/热区错位（s77r 分步实验步骤 2/4）。
          // BindSheet 独立浮层自带 hitTest 隔离，点击不再落回画布手势。
          if (this.showPanel) {
            this.openPanelSheet();
          } else if (this.sheetContent !== null) {
            this.getUIContext().closeBindSheet(this.sheetContent).then(() => {
              console.info('[ForceInvert] closeBindSheet success');
            }).catch((err: BusinessError) => {
              console.error(`[ForceInvert] closeBindSheet error: ${err.code} ${err.message}`);
            });
          }
        })

    }
    .width('100%')
    .height('100%')
    .gesture(
      // 长按画布（s77 终版）：切换 testMode（同时清 forceInvert）。
      // 自动化路径：长按=testMode 开关；forceInvert 改用面板按钮/want 参数，
      // 不再绑画布单击（s77r 修复：Stack 级 TapGesture 会被面板内点击穿透触发
      // ——开/收面板的 (1100,230) 点击落到画布即误翻转 forceInvert，这正是
      // s77q 验证轮 TC-09/10 FAIL 的根因：收起面板后倒立立即失效）。
      // duration=400ms：uitest longClick 按压时长较短，800ms 触发不了。
      LongPressGesture({ repeat: false, duration: 400 })
        .onAction(() => {
          this.testMode = !this.testMode;
          console.info(`[ForceInvert] longPress toggle testMode -> ${this.testMode}`);
          if (this.testMode) {
            this.useSensor = false;
            this.gestureHoldUntil = Date.now() + 24 * 3600 * 1000;
          } else {
            this.useSensor = true;
            this.gestureHoldUntil = 0;
            this.forceInvert = false;
          }
          this.lastLongPressTime = Date.now();
        })
    )
    .gesture(
      PanGesture()
        .onActionUpdate((e: GestureEvent) => {
          this.useSensor = false;
          this.gestureHoldUntil = Date.now() + 2500;
          // 映射系数 0.4：200px 满行程 ≈ 80°，比旧 0.3 跟手 33%
          this.tiltX = Math.max(-80, Math.min(80, e.offsetX * this.gestureSensitivity));
        })
        .onActionEnd(() => {
          // 保持手势角度 2.5s 后恢复传感器控制（frame 中按 gestureHoldUntil 判断）
          this.useSensor = true;
        })
    )
  }
}
