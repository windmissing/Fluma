import { SandGrid } from '../physics/SandGrid';
import { FluidField } from '../physics/FluidField';
import { WaterSandField, WaterSandSnapshot, WaterSandParams, WaterSandScene, WaterSandRegion, WaterSandP011InputSource, WaterSandP011Metric, WaterSandP011Bottom, defaultWaterSandParams, nextP011TimeMs, waterSandFrameHookPayload } from '../physics/WaterSandField';
import { LiquidCAController } from '../physics/LiquidCAController';
import { LiquidCATelemetry } from '../physics/LiquidCATelemetry';
import { defaultLiquidCAConfig, LiquidCAConfig, LiquidCAFramePayload, LiquidCARenderer,
  LiquidInputSource, LiquidRenderSnapshot } from '../physics/LiquidCATypes';
import sensor from '@ohos.sensor';
import { FrameCallback } from '@ohos.arkui.UIContext';
import { ComponentContent } from '@ohos.arkui.node';
import { BusinessError } from '@ohos.base';
import hilog from '@ohos.hilog';
import { decodeP011TiltWant } from '../physics/P011TiltWant';
import { P011SchedulePlayer } from '../physics/P011SchedulePlayer';
import common from '@ohos.app.ability.common';
import {
  P011NaturalObservationProbe,
  P011NaturalObservationRecord,
  P011_NATURAL_OBSERVATION_ROUTE_ID
} from '../physics/P011NaturalObservation';
import {
  P011NaturalObservationStore,
  createP011NaturalObservationStore
} from '../physics/P011NaturalObservationStore';
import { P011InputModeState, P011ModeRequestSource, P011InputSelection,
  transitionP011InputMode, acceptP011SensorSample, selectP011Input,
  canP011GestureOwnInput } from '../physics/P011InputMode';
// P-012 E-batch：表现层独立 renderer + 薄分发（architecture-P012.md §9.2）
import { P012WaterSandRenderer, p012ShouldUseP012Renderer } from '../render/P012WaterSandRenderer';
import { P012AppearanceTier, P012FrameContext, P012Ablation, P012FillBox } from '../render/P012RenderTypes';

/** 帧回调适配：postFrameCallback 需要传入 FrameCallback 子类实例 */
class MyFrameCallback extends FrameCallback {
  owner: SandCanvas | null = null;
  onFrame(frameTimeInNano: number): void {
    if (this.owner !== null) {
      this.owner.frame(frameTimeInNano);
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
const MODE_COUNT = 5;
/** 液体模式（M1'）：模拟层 SandGrid.liquidMode，渲染层=流体带结构 */
const MODE_LIQUID = 3;
const MODE_WATERSAND = 4;

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
  percolationRate: number = 6;
  mixBandPx: number = 1.5;
  shakeInput: number = 0;
  /** P-012 E-batch：外观档位镜像（builder 刷新用） */
  p012Tier: number = 0;
}

interface LiquidWantCommand {
  p011TiltWantSequence?: number;
  renderMode: number;
  testMode: boolean;
  forceInvert: boolean;
  shake: number;
  seed: number;
  cellPx: number;
  caseId: string;
  warnings: string[];
}

class SandCanvasLiquidRenderer implements LiquidCARenderer {
  private owner: SandCanvas;
  private config: LiquidCAConfig;
  private telemetry: LiquidCATelemetry;

  constructor(owner: SandCanvas, config: LiquidCAConfig, telemetry: LiquidCATelemetry) {
    this.owner = owner;
    this.config = config;
    this.telemetry = telemetry;
  }

  render(snapshot: LiquidRenderSnapshot, frame: LiquidCAFramePayload): void {
    this.owner.renderLiquidSnapshot(snapshot, frame);
  }

  onModelError(errorCode: string): void {
    this.telemetry.onEvent({
      type: 'model_error', sequence: 0, eventTimeMs: Date.now(), caseId: this.owner.liquidTestCaseId(), boundary: true,
      source: LiquidInputSource.INTERNAL, forceInvert: this.owner.forceInvert,
      shakeIntensity: 0, errorCode: errorCode, cols: this.config.cols,
      rows: this.config.rows, cellPx: this.config.cellPx, initialHeavy: 0,
      initialLight: 0, seed: this.config.seed, scaled: false
    });
  }
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

    Button(params.tm ? '测试模式（模拟姿态）' : '体验模式（传感器）')
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
        : params.renderMode === MODE_HYBRID ? '\u6A21\u5F0F\uFF1A\u6DF7\u5408'
        : params.renderMode === MODE_LIQUID ? '\u6A21\u5F0F\uFF1A\u6DB2\u4F53'
        : '\u6A21\u5F0F\uFF1A\u6C34\u6C99')
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

    if (params.renderMode === MODE_WATERSAND) {
      Button(params.p012Tier === 0 ? '外观:OFF(原渲染)'
        : params.p012Tier === 1 ? '外观:E(光照色调)'
        : params.p012Tier === 2 ? '外观:E+A(云雾)'
        : '外观:E+A+B(颗粒)')
        .fontSize(12)
        .width('100%')
        .backgroundColor(params.p012Tier > 0 ? '#00695C' : '#455A64')
        .onClick(() => {
          if (params.owner === null) {
            return;
          }
          params.owner.cycleP012AppearanceTier();
        })
    }

    Text(`流速: ${params.flowSpeed}`).fontSize(13).fontColor('#CCCCCC').width('100%')
    Slider({ value: params.flowSpeed, min: 1, max: 6, step: 1 })
      .blockColor('#43A047').selectedColor('#43A047')
      .onChange((value: number) => {
        if (params.owner === null) {
          return;
        }
        params.owner.setFlowSpeed(value);
      })

    if (params.renderMode === MODE_WATERSAND || params.renderMode === MODE_LIQUID) {
      Text(`\u5206\u9009\u5F3A\u5EA6: ${params.percolationRate.toFixed(1)}/s`)
        .fontSize(13).fontColor('#CCCCCC').width('100%')
      Slider({ value: params.percolationRate, min: 0, max: 12, step: 0.5 })
        .blockColor('#D9A64A').selectedColor('#D9A64A')
        .onChange((value: number) => {
          if (params.owner === null) {
            return;
          }
          params.owner.setWaterSandPercolationRate(value);
        })

      Text(`\u6DF7\u5408\u5E26\u5BBD: ${params.mixBandPx.toFixed(1)}px`)
        .fontSize(13).fontColor('#CCCCCC').width('100%')
      Slider({ value: params.mixBandPx, min: 1, max: 2, step: 0.5 })
        .blockColor('#63A35D').selectedColor('#63A35D')
        .onChange((value: number) => {
          if (params.owner === null) {
            return;
          }
          params.owner.setWaterSandMixBand(value);
        })

      Text(`\u6D4B\u8BD5\u6270\u52A8: ${params.shakeInput.toFixed(1)}`)
        .fontSize(13).fontColor('#CCCCCC').width('100%')
      Slider({ value: params.shakeInput, min: 0, max: 1, step: 0.1 })
        .blockColor('#F6E2A7').selectedColor('#F6E2A7')
        .onChange((value: number) => {
          if (params.owner === null) {
            return;
          }
          if (params.renderMode === MODE_LIQUID) {
            params.owner.setLiquidShakeInput(value);
        } else {
          params.owner.setWaterSandShakeInput(value);
          }
        })
    }
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
  @State mixBandPx: number = 1.5;
  @State waterSandShakeInput: number = 0;
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
  private waterSandField: WaterSandField | null = null;
  private liquidController: LiquidCAController | null = null;
  private liquidTelemetry: LiquidCATelemetry = new LiquidCATelemetry();
  private liquidSeed: number = 0;
  private liquidCaseId: string = '';
  private liquidShakeInput: number = 0;
  private liquidWantCommandSeq: number = 0;
  private p011TiltWantConsumedSeq: number = 0;
  private p011ScriptTargetTilt: number | null = null;
  // v3 schedule player (§26.4): command generator only — dispatched targets
  // flow through the existing setTilt + planner/guard path. Mutual
  // exclusion with single commands is enforced at the consumption points.
  private p011SchedulePlayer: P011SchedulePlayer = new P011SchedulePlayer();
  private p011ScheduleWantConsumedSeq: number = 0;
  private waterSandSnapshot: WaterSandSnapshot | null = null;
  private waterSandParams: WaterSandParams = defaultWaterSandParams();
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
  private readonly maxOmega: number = 720;
  private lastFrameTime: number = 0;
  private waterSandPreset: string = 'default';
  private waterSandCols: number = 128;
  private waterSandCapacity: number = 192;
  private waterSandVSandTopMax: number = 28;
  private waterSandSnapshotEnabled: boolean = false;
  private naturalObservationProbe: P011NaturalObservationProbe | null = null;
  private naturalObservationStore: P011NaturalObservationStore | null = null;
  private naturalObservationStoreLogged: boolean = false;
  private waterSandVisualInverted: boolean = false;
  // ── P-012 E-batch 薄分发状态（§9.2：全部为分发与 epoch 递增，材质逻辑零侵入） ──
  /** P-012 独立 renderer（延迟创建：首次启用非 OFF 档时构造） */
  private p012Renderer: P012WaterSandRenderer | null = null;
  /** 外观档位（OFF/E/EA/EAB）；OFF 时走原 renderWaterSand（canonical） */
  @State p012AppearanceTier: number = P012AppearanceTier.OFF;
  /** 外观配置代次（tier/seed 变化时递增，进 frameContext） */
  private p012ConfigRevision: number = 0;
  /** 外观独立随机种子（不调用/消耗物理随机源） */
  private p012RenderSeed: number = 20261020;
  /** §10.4 消融开关（want p012ablate；默认 OFF=全部启用） */
  private p012Ablation: P012Ablation = P012Ablation.OFF;
  /** §10.4 fillRect 策略（want p012fillbox；默认 TIGHT） */
  private p012FillBox: P012FillBox = P012FillBox.TIGHT;
  /** 物理实例代次（reset/initField 递增） */
  private p012FieldEpoch: number = 0;
  /** 连续性代次（invert180/mode boundary 递增） */
  private p012ContinuityEpoch: number = 0;
  /** 几何代次（initField 重算 box 时递增） */
  private p012GeometryEpoch: number = 0;
  /** 页面帧采样序号（每帧 render 前 +1） */
  private p012FrameSerial: number = 0;
  /** 单调时间源（sampleTimeMs） */
  private p012LastSampleTimeMs: number = 0;
  private waterRenderA: Float64Array = new Float64Array(0);
  private waterRenderB: Float64Array = new Float64Array(0);
  private waterRenderC: Float64Array = new Float64Array(0);
  private waterRenderD: Float64Array = new Float64Array(0);
  private waterRenderE: Float64Array = new Float64Array(0);
  private lastWaterSandMetricTime: number = 0;
  private lastWaterSandTilt: number | null = null;
  private lastWaterSandDormant: boolean | null = null;
  private lastTiltCommandTime: number = 0;
  private lastSensorCommandTime: number = 0;
  private hasFreshSensorSample: boolean = false;
  private hilogDomain: number = 0x0000;
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
    this.sheetParams.percolationRate = this.waterSandParams.percolationRate;
    this.sheetParams.mixBandPx = this.mixBandPx;
    this.sheetParams.shakeInput = this.renderMode === MODE_LIQUID
      ? this.liquidShakeInput : this.waterSandShakeInput;
    this.sheetParams.p012Tier = this.p012AppearanceTier;
    this.sheetContent.update(this.sheetParams);
  }

  aboutToAppear(): void {
    this.frameCb.owner = this;
    this.p011TiltWantConsumedSeq = AppStorage.get<number>('p011TiltWantConsumedSeq') ?? 0;
    // s78：构造 BindSheet 内容（owner 桥接 this，builder 内按钮经公开方法改状态）
    this.sheetContent = new ComponentContent(this.getUIContext(), wrapBuilder(panelSheetBuilder), this.makePanelParams());
    // 测试自动化：want 参数（EntryAbility 写入 AppStorage）初始化测试状态，
    // 零 UI 依赖（面板按钮热区随布局变化不可预测，多轮实测已弃用坐标点击）
    const tm = AppStorage.get<boolean>('testmode');
    this.applyInputMode(tm === true, 'startup', AppStorage.get<number>('p011TiltWantModeSequence') ?? 0);
    const liquidCaseId = AppStorage.get<string>('liquidCaseId');
    if (liquidCaseId !== undefined) {
      this.liquidCaseId = liquidCaseId;
    }
    this.liquidTelemetry.setTestMode(this.testMode);
    const fi = AppStorage.get<boolean>('forceinvert');
    if (fi !== undefined) {
      this.forceInvert = this.testMode && fi;
    }
    const rm = AppStorage.get<number>('rendermode');
    if (rm !== undefined) {
      this.renderMode = rm;
    }
    const liquidSeed = AppStorage.get<number>('liquidSeed');
    if (liquidSeed !== undefined && Number.isInteger(liquidSeed) && liquidSeed >= 0) {
      this.liquidSeed = liquidSeed;
    }
    const liquidCellPx = AppStorage.get<number>('liquidCellPx');
    if (liquidCellPx !== undefined && Number.isInteger(liquidCellPx) && liquidCellPx > 0) {
      this.cellPx = liquidCellPx;
    }
    const liquidStartShake = AppStorage.get<number>('liquidStartShake');
    if (liquidStartShake !== undefined && liquidStartShake >= 0 && liquidStartShake <= 1) {
      this.liquidShakeInput = liquidStartShake;
    }
    const wsPreset = AppStorage.get<string>('wspreset');
    if (wsPreset !== undefined) {
      this.waterSandPreset = wsPreset;
    }
    const wsCols = AppStorage.get<number>('wscols');
    if (wsCols !== undefined && Number.isInteger(wsCols) && wsCols >= 2) {
      this.waterSandCols = wsCols;
    }
    const wsCapacity = AppStorage.get<number>('wscapacity');
    if (wsCapacity !== undefined && Number.isInteger(wsCapacity) && wsCapacity >= 2) {
      this.waterSandCapacity = wsCapacity;
    }
    const wsMixBand = AppStorage.get<number>('wsmixband');
    if (wsMixBand !== undefined && wsMixBand >= 1 && wsMixBand <= 2) {
      this.mixBandPx = wsMixBand;
    }
    const wsSnapshot = AppStorage.get<boolean>('wssnapshot');
    if (wsSnapshot !== undefined) {
      this.waterSandSnapshotEnabled = wsSnapshot;
    }
    const wsSandTopMax = AppStorage.get<number>('wssandtopmax');
    if (wsSandTopMax !== undefined && wsSandTopMax > 0 && wsSandTopMax <= 30) {
      this.waterSandVSandTopMax = wsSandTopMax;
    }
    // P-012 E-batch：外观 want 参数（p012tier：0=OFF 1=E 2=EA 3=EAB；p012seed）
    const p012Tier = AppStorage.get<number>('p012tier');
    const p012Seed = AppStorage.get<number>('p012seed');
    if (p012Seed !== undefined && Number.isInteger(p012Seed) && p012Seed >= 0) {
      this.p012RenderSeed = p012Seed;
    }
    if (p012Tier !== undefined && Number.isInteger(p012Tier) &&
      p012Tier >= P012AppearanceTier.OFF && p012Tier <= P012AppearanceTier.EAB) {
      // 经 setP012AppearanceTier：非 OFF 档时创建 renderer（延迟创建入口）
      this.setP012AppearanceTier(p012Tier, 'want');
    }
    // §10.4 消融参数（AppStorage 字符串；非法值已在 EntryAbility 过滤，
    // 这里再防御一次：不认识的值不覆盖默认）
    const p012Ablate = AppStorage.get<string>('p012ablate');
    if (p012Ablate === 'off' || p012Ablate === 'grain' ||
      p012Ablate === 'fog' || p012Ablate === 'spark') {
      this.p012Ablation = P012Ablation[p012Ablate === 'off' ? 'OFF'
        : p012Ablate === 'grain' ? 'GRAIN'
        : p012Ablate === 'fog' ? 'FOG' : 'SPARK'];
      console.info(`[P012] ablation -> ${p012Ablate} (want)`);
    }
    const p012FillBox = AppStorage.get<string>('p012fillbox');
    if (p012FillBox === 'full' || p012FillBox === 'tight') {
      this.p012FillBox = p012FillBox === 'full' ? P012FillBox.FULL : P012FillBox.TIGHT;
      console.info(`[P012] fillbox -> ${p012FillBox} (want)`);
    }
    console.info(`[ForceInvert] aboutToAppear: testMode=${this.testMode} forceInvert=${this.forceInvert} useSensor=${this.useSensor} renderMode=${this.renderMode}`);
    try {
      sensor.on(sensor.SensorId.ACCELEROMETER, (data: sensor.AccelerometerResponse) => {
        // P-011 dual-input-v1: rejected source cannot update filter or timestamps.
        if (this.testMode) {
          return;
        }
        const wasFresh = this.hasFreshSensorSample;
        const next = acceptP011SensorSample(this.readInputModeState(), data.x, data.y,
          nextP011TimeMs(), this.waterSandParams.inputSmoothingAlpha);
        this.writeInputModeState(next);
        if (!wasFresh && next.hasFreshSensorSample) {
          this.logWaterSand(`P011_MODE sensor_fresh ${JSON.stringify({
            inputMode: 'EXPERIENCE', source: 'sensor', callback: true,
            runId: this.waterSandField === null ? '' : this.waterSandField.getP011RunId(),
            callbackTimestamp: data.timestamp, commandTime: next.lastSensorCommandTime,
            x: data.x, y: data.y, filteredX: next.sensorGx, filteredY: next.sensorGy
          })}`);
        }
      });
    } catch (e) {
    }
  }

  aboutToDisappear(): void {
    this.frameCb.owner = null;
    // §26.4 risk 1: clear the schedule player's activation flag on exit —
    // a crashed/abandoned player must never permanently reject commands.
    this.p011SchedulePlayer.forceStop();
    // P-012 E-batch：断帧回调后释放表现层资源（dispose 幂等）
    this.p012Renderer?.dispose();
    this.p012Renderer = null;
    this.stopLiquidController();
    try {
      sensor.off(sensor.SensorId.ACCELEROMETER);
    } catch (e) {
    }
    // Disposed page cannot replay old pending/filter state on the next appearance.
    const disposedState = this.readInputModeState();
    disposedState.testMode = !this.testMode;
    this.writeInputModeState(transitionP011InputMode(disposedState, this.testMode,
      AppStorage.get<number>('p011TiltWantCommandSeq') ?? 0, 0).state);
    this.waterSandField?.resetInputState();
    const observationStore = this.naturalObservationStore;
    this.naturalObservationStore = null;
    this.naturalObservationProbe = null;
    this.waterSandField?.setNaturalObservationProbe(null);
    observationStore?.close().catch((): void => {
    });
    if (this.sheetContent !== null) {
      this.sheetContent.dispose();
    }
  }

  /** ===== 面板按钮桥接方法（s78：全局 builder 经 PanelParams.owner 调用） =====
   *  每个 toggle 后 syncSheet()：镜像快照写入 params 并 updateBindSheet 强制
   *  builder 重建 → sheet 内按钮文案与实际状态一致（缺陷 2 根治）。 */
  toggleForceInvert(): void {
    if (!this.testMode) {
      return;
    }
    this.forceInvert = !this.forceInvert;
    if (this.renderMode === MODE_LIQUID && this.liquidController !== null) {
      this.liquidController.setForceInvert(this.forceInvert, LiquidInputSource.PANEL);
    }
    console.info(`[ForceInvert] panel toggle -> ${this.forceInvert} (testMode=${this.testMode})`);
    this.syncSheet();
  }

  toggleTestMode(): void {
    this.applyInputMode(!this.testMode, 'panel');
  }

  // P-011 dual-input-v1: these adapters write helper results back to the real UI/physics inputs.
  private readInputModeState(): P011InputModeState {
    return { testMode: this.testMode, useSensor: this.useSensor,
      scriptTargetTilt: this.p011ScriptTargetTilt, consumedSequence: this.p011TiltWantConsumedSeq,
      tiltX: this.tiltX, lastTiltX: this.lastTiltX, forceInvert: this.forceInvert,
      wasForceInvert: this.wasForceInvert, gestureHoldUntil: this.gestureHoldUntil,
      sensorGx: this.sensorGx, sensorGy: this.sensorGy, hasFreshSensorSample: this.hasFreshSensorSample,
      lastSensorCommandTime: this.lastSensorCommandTime, lastTiltCommandTime: this.lastTiltCommandTime };
  }

  private writeInputModeState(state: P011InputModeState): void {
    this.testMode = state.testMode;
    this.useSensor = state.useSensor;
    this.p011ScriptTargetTilt = state.scriptTargetTilt;
    this.p011TiltWantConsumedSeq = state.consumedSequence;
    AppStorage.setOrCreate('p011TiltWantConsumedSeq', state.consumedSequence);
    this.tiltX = state.tiltX;
    this.lastTiltX = state.lastTiltX;
    this.forceInvert = state.forceInvert;
    this.wasForceInvert = state.wasForceInvert;
    this.gestureHoldUntil = state.gestureHoldUntil;
    this.sensorGx = state.sensorGx;
    this.sensorGy = state.sensorGy;
    this.hasFreshSensorSample = state.hasFreshSensorSample;
    this.lastSensorCommandTime = state.lastSensorCommandTime;
    this.lastTiltCommandTime = state.lastTiltCommandTime;
  }

  private applyInputMode(nextTestMode: boolean, source: P011ModeRequestSource,
    currentWantSequence: number = 0): void {
    const pending = AppStorage.get<number>('p011TiltWantCommandSeq') ?? 0;
    const transition = transitionP011InputMode(this.readInputModeState(), nextTestMode,
      pending, currentWantSequence);
    this.writeInputModeState(transition.state);
    if (transition.changed) {
      this.lastWaterSandTilt = null;
      this.waterSandField?.resetInputState();
      if (this.renderMode === MODE_LIQUID && this.liquidController !== null) {
        this.liquidController.setForceInvert(false, LiquidInputSource.PANEL);
      }
      this.logWaterSand(`P011_MODE boundary ${JSON.stringify({
        boundary: true, inputMode: nextTestMode ? 'TEST' : 'EXPERIENCE', source,
        pendingDiscardedThrough: transition.state.consumedSequence,
        retainedWantSequence: nextTestMode ? currentWantSequence : 0
      })}`);
    }
    AppStorage.setOrCreate('testmode', this.testMode);
    AppStorage.setOrCreate('forceinvert', this.forceInvert);
    this.liquidTelemetry.setTestMode(this.testMode);
    this.needRedraw = true;
    this.syncSheet();
  }

  cycleRenderMode(): void {
    if (this.renderMode === MODE_LIQUID) {
      this.stopLiquidController();
    }
    this.renderMode = (this.renderMode + 1) % MODE_COUNT;
    if (this.renderMode === MODE_LIQUID) {
      this.startLiquidController();
    }
    this.needRedraw = true;
    if (this.renderMode === MODE_WATERSAND) {
      this.logWaterSand(`WS_EVENT mode ${JSON.stringify({ renderMode: this.renderMode })}`);
    }
    console.info(`[ForceInvert] panel renderMode -> ${this.renderMode}`);
    this.syncSheet();
  }

  resetField(): void {
    // §26.4 risk 1: reset tears the schedule player down (mutual-exclusion
    // deadlock window) and clears the drift binding on the field.
    this.p011SchedulePlayer.forceStop();
    this.waterSandField?.setP011ScheduleDrift(null);
    if (this.renderMode === MODE_LIQUID) {
      if (this.liquidController !== null) {
        this.liquidController.reset(this.liquidSeed, LiquidInputSource.PANEL);
      } else {
        this.initField();
      }
      console.info('[ForceInvert] panel reset');
      return;
    }
    this.initField();
    if (this.renderMode === MODE_WATERSAND) {
      this.logWaterSand(`WS_EVENT reset ${JSON.stringify({
        preset: this.waterSandPreset,
        ledgerVersion: 2,
        paramsVersion: 'v2.2',
        vH: this.waterSandParams.heavySettleSpeed,
        vL: this.waterSandParams.lightSettleSpeed,
        liftHRate: this.waterSandParams.heavyLiftRate,
        liftLRate: this.waterSandParams.lightLiftRate,
        shakeRampRate: this.waterSandParams.shakeRampRate,
        vSandTopMax: this.waterSandParams.vSandTopMax,
        boundary: true,
        percolationRate: this.waterSandParams.percolationRate
      })}`);
      if (this.waterSandSnapshotEnabled) {
        this.logWaterSandSnapshot();
      }
    }
    console.info(`[ForceInvert] panel reset`);
  }

  setFlowSpeed(v: number): void {
    this.flowSpeed = v;
  }

  setWaterSandPercolationRate(value: number): void {
    const clipped = Math.max(0, Math.min(12, value));
    this.waterSandParams.percolationRate = Math.max(0.1, clipped);
    this.waterSandField?.configure(this.waterSandParams);
    this.syncSheet();
  }

  setWaterSandMixBand(value: number): void {
    this.mixBandPx = Math.max(1, Math.min(2, value));
    this.syncSheet();
  }

  setWaterSandShakeInput(value: number): void {
    this.waterSandShakeInput = Math.max(0, Math.min(1, value));
    if (this.renderMode === MODE_WATERSAND) {
      this.waterSandField?.setShakeInput(this.waterSandShakeInput);
      if (this.waterSandShakeInput > 0) {
        this.logWaterSand(`WS_EVENT shake ${JSON.stringify({
          input: this.waterSandShakeInput,
          boundary: true
        })}`);
      }
    }
    this.syncSheet();
  }

  setLiquidShakeInput(value: number): void {
    this.liquidShakeInput = Math.max(0, Math.min(1, value));
    if (this.renderMode === MODE_LIQUID && this.liquidController !== null) {
      this.liquidController.setShake(this.liquidShakeInput, LiquidInputSource.PANEL);
    }
    this.syncSheet();
  }

  private logWaterSand(message: string): void {
    hilog.info(this.hilogDomain, 'WaterSand', '%{public}s', message);
  }

  private logWaterSandP011Pair(metric: WaterSandP011Metric, bottom: WaterSandP011Bottom): void {
    // P-011 bottom-creep-v2.7: one hilog call keeps metric/bottom samples aligned.
    hilog.info(this.hilogDomain, 'WaterSand', 'P011_METRIC %{public}s\nP011_BOTTOM %{public}s',
      JSON.stringify(metric), JSON.stringify(bottom));
  }

  private snapshotJson(snapshot: WaterSandSnapshot): string {
    return JSON.stringify({
      schemaVersion: snapshot.schemaVersion,
      cols: snapshot.cols,
      capacity: snapshot.capacity,
      waterTop: Array.from(snapshot.waterTop),
      bh: Array.from(snapshot.bh),
      bl: Array.from(snapshot.bl),
      mh: Array.from(snapshot.mh),
      ml: Array.from(snapshot.ml),
      suspH: Array.from(snapshot.suspH),
      suspL: Array.from(snapshot.suspL),
      hs: Array.from(snapshot.hs),
      hl: Array.from(snapshot.hl),
      sandTop: Array.from(snapshot.sandTop)
    });
  }

  private logWaterSandSnapshot(): void {
    const snapshot = this.waterSandField?.snapshot();
    if (snapshot === undefined) {
      return;
    }
    this.logWaterSand(`WS_SNAPSHOT ${this.snapshotJson(snapshot)}`);
  }

  private setupNaturalObservation(ws: WaterSandField): void {
    if (!this.waterSandSnapshotEnabled) {
      return;
    }
    const previousStore = this.naturalObservationStore;
    this.naturalObservationStore = null;
    this.naturalObservationProbe = null;
    this.naturalObservationStoreLogged = false;
    ws.setNaturalObservationProbe(null);
    previousStore?.close().catch((): void => {
    });
    const pendingRecords: P011NaturalObservationRecord[] = [];
    const runId = ws.getP011RunId();
    const probe = new P011NaturalObservationProbe(ws.cols, (record) => {
      const store = this.naturalObservationStore;
      if (store === null) {
        pendingRecords.push(record);
        return;
      }
      store.append(record);
    });
    this.naturalObservationProbe = probe;
    ws.setNaturalObservationProbe(probe);
    const context = this.getUIContext().getHostContext() as common.UIAbilityContext;
    createP011NaturalObservationStore(context, runId).then((store) => {
      if (this.waterSandField !== ws || this.naturalObservationProbe !== probe) {
        store.close().catch((): void => {
        });
        return;
      }
      this.naturalObservationStore = store;
      for (const record of pendingRecords) {
        store.append(record);
      }
      pendingRecords.length = 0;
      if (!this.naturalObservationStoreLogged) {
        this.naturalObservationStoreLogged = true;
        this.logWaterSand(`P011_OBS_STORE ${JSON.stringify({
          observationRouteId: P011_NATURAL_OBSERVATION_ROUTE_ID,
          runId: runId,
          filePath: store.filePath
        })}`);
        this.logWaterSand(`P011_RASTER_LAYOUT ${JSON.stringify({
          runId: runId,
          canvasWidth: this.canvasW,
          canvasHeight: this.canvasH,
          boxX: this.boxX,
          boxY: this.boxY,
          boxWidth: this.boxW,
          boxHeight: this.boxH,
          cols: ws.cols,
          capacity: ws.capacity,
          pixelsPerColumn: this.boxW / ws.cols,
          pixelsPerModelUnitY: this.boxH / ws.capacity
        })}`);
      }
    }).catch((error: Error) => {
      ws.setNaturalObservationProbe(null);
      this.naturalObservationProbe = null;
      this.logWaterSand(`P011_OBS_ERROR ${JSON.stringify({
        observationRouteId: P011_NATURAL_OBSERVATION_ROUTE_ID,
        runId: runId,
        message: error.message
      })}`);
    });
  }

  private logNaturalObservationHeartbeat(metric: WaterSandP011Metric): void {
    const context = this.naturalObservationProbe?.getLastContext();
    const audit = this.naturalObservationProbe?.getLastAudit();
    if (context === null || context === undefined || audit === null || audit === undefined) {
      return;
    }
    this.logWaterSand(`P011_OBS_METRIC ${JSON.stringify({
      observationRouteId: context.observationRouteId,
      runId: context.runId,
      sequence: metric.sequence,
      phaseD: context.phaseD,
      targetTilt: context.targetTilt,
      effectiveTilt: context.effectiveTilt,
      effectiveTiltRate: context.effectiveTiltRate,
      shakeEnergy: context.shakeEnergy,
      waterTotal: metric.waterTotal,
      heavyTotal: metric.heavyTotal,
      lightTotal: metric.lightTotal,
      bottomBudgetUsed: metric.bottomCreepBudgetUsed,
      bottomBudgetLimit: metric.bottomCreepBudgetLimit,
      totalBudgetUsed: metric.totalMotionBudgetUsed,
      totalBudgetLimit: metric.totalMotionBudgetLimit,
      auditFinite: audit.finite,
      auditRange: audit.range,
      auditCapacity: audit.capacity,
      auditStock: audit.stock,
      auditConservation: audit.conservation
    })}`);
  }

  private waterSandScene(preset: string, cols: number): WaterSandScene {
    const all: WaterSandRegion = { startCol: 0, endColInclusive: cols - 1, bottom: 0, height: 110 };
    const thickWater: WaterSandRegion = { startCol: 0, endColInclusive: cols - 1, bottom: 144, height: 38 };
    if (preset === 'sorting') {
      return {
        waterRegions: [all],
        heavyDepositedRegions: [],
        lightDepositedRegions: [],
        heavySuspendedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 0, height: 20 }],
        lightSuspendedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 0, height: 20 }]
      };
    }
    if (preset === 'thick') {
      return {
        waterRegions: [thickWater],
        heavyDepositedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 0, height: 72 }],
        lightDepositedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 72, height: 72 }],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      };
    }
    if (preset === 'nowater') {
      return {
        waterRegions: [],
        heavyDepositedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 0, height: 18 }],
        lightDepositedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 18, height: 18 }],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      };
    }
    if (preset === 'heavyonly' || preset === 'lightonly') {
      return {
        waterRegions: [all],
        heavyDepositedRegions: preset === 'heavyonly'
          ? [{ startCol: 0, endColInclusive: cols - 1, bottom: 0, height: 18 }] : [],
        lightDepositedRegions: preset === 'lightonly'
          ? [{ startCol: 0, endColInclusive: cols - 1, bottom: 0, height: 18 }] : [],
        heavySuspendedRegions: [],
        lightSuspendedRegions: []
      };
    }
    return {
      waterRegions: [all],
      heavyDepositedRegions: [],
      lightDepositedRegions: [],
      heavySuspendedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 0, height: 24 }],
      lightSuspendedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 0, height: 16 }]
    };
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

  private async startLiquidController(): Promise<void> {
    if (this.canvasW <= 0 || this.canvasH <= 0) {
      return;
    }
    await this.stopLiquidController();
    this.boxW = Math.floor(this.canvasW * 0.72);
    this.boxH = Math.floor(this.canvasH * 0.42);
    this.boxX = Math.floor((this.canvasW - this.boxW) / 2);
    this.boxY = Math.floor((this.canvasH - this.boxH) / 2);
    let cellPx = this.cellPx;
    let cols = Math.floor(this.boxW / cellPx);
    let rows = Math.floor(this.boxH / cellPx);
    if (cols * rows > 24000) {
      const scale = Math.ceil(Math.sqrt(cols * rows / 24000));
      cellPx *= scale;
      cols = Math.floor(this.boxW / cellPx);
      rows = Math.floor(this.boxH / cellPx);
    }
    if (cols < 2 || rows < 2 || cols * rows > 24000) {
      this.liquidTelemetry.onEvent({
        type: 'model_error', sequence: 0, eventTimeMs: Date.now(), caseId: this.liquidCaseId, boundary: true,
        source: LiquidInputSource.INTERNAL, forceInvert: false, shakeIntensity: 0,
        errorCode: 'GRID_TOO_LARGE', cols: cols, rows: rows, cellPx: cellPx,
        initialHeavy: 0, initialLight: 0, seed: this.liquidSeed, scaled: true
      });
      return;
    }
    this.cellPx = cellPx;
    const config: LiquidCAConfig = defaultLiquidCAConfig();
    config.cols = cols;
    config.rows = rows;
    config.cellPx = cellPx;
    config.seed = this.liquidSeed;
    const renderer = new SandCanvasLiquidRenderer(this, config, this.liquidTelemetry);
    this.liquidController = new LiquidCAController(renderer, this.liquidTelemetry);
    this.liquidController.setCaseId(this.liquidCaseId);
    this.liquidController.start(config).then((): void => {
      if (this.liquidShakeInput > 0) {
        this.liquidController?.setShake(this.liquidShakeInput, LiquidInputSource.WANT);
      }
    }).catch((error: Error): void => {
      console.error(`[LiquidCA] start failed: ${error.message}`);
    });
  }

  liquidTestCaseId(): string {
    return this.liquidCaseId;
  }

  private stopLiquidController(): Promise<void> {
    if (this.liquidController !== null) {
      const controller = this.liquidController;
      this.liquidController = null;
      return controller.stop().catch((error: Error): void => {
        console.error(`[LiquidCA] stop failed: ${error.message}`);
      });
    }
    return Promise.resolve();
  }

  private processLiquidWantCommand(): void {
    const sequence = AppStorage.get<number>('liquidWantCommandSeq');
    const rawCommand = AppStorage.get<string>('liquidWantCommand');
    if (sequence === undefined || rawCommand === undefined || sequence <= this.liquidWantCommandSeq) {
      return;
    }
    this.liquidWantCommandSeq = sequence;
    const command = JSON.parse(rawCommand) as LiquidWantCommand;
    this.applyInputMode(command.testMode, 'want', command.p011TiltWantSequence ?? 0);
    this.liquidCaseId = command.caseId;
    this.liquidTelemetry.setTestMode(this.testMode);
    this.forceInvert = this.testMode && command.forceInvert;
    const seedChanged = command.seed !== this.liquidSeed;
    this.liquidShakeInput = Math.max(0, Math.min(1, command.shake));
    this.liquidSeed = Math.max(0, Math.floor(command.seed));
    if (command.renderMode !== this.renderMode) {
      if (this.renderMode === MODE_LIQUID) {
        this.stopLiquidController();
      }
      this.renderMode = command.renderMode;
      if (this.renderMode === MODE_LIQUID) {
        this.startLiquidController();
      }
    }
    if (this.renderMode === MODE_LIQUID && this.liquidController !== null) {
      this.liquidController.setCaseId(this.liquidCaseId);
      if (seedChanged) {
        this.liquidController.reset(this.liquidSeed, LiquidInputSource.WANT);
      }
      this.liquidController.setForceInvert(this.forceInvert, LiquidInputSource.WANT);
      this.liquidController.setShake(this.liquidShakeInput, LiquidInputSource.WANT);
    }
    this.syncSheet();
  }

  private liquidFrame(frameTimeInNano: number): void {
    const now = frameTimeInNano / 1000000;
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
    this.processLiquidWantCommand();
    this.liquidController?.onVsync(now);
    this.getUIContext().postFrameCallback(this.frameCb);
  }

  renderLiquidSnapshot(snapshot: LiquidRenderSnapshot, frame: LiquidCAFramePayload): void {
    const ctx = this.ctx;
    const bgGrad = ctx.createLinearGradient(0, 0, 0, this.canvasH);
    bgGrad.addColorStop(0, '#0a0a14');
    bgGrad.addColorStop(1, '#1a1a2e');
    ctx.fillStyle = bgGrad;
    ctx.fillRect(0, 0, this.canvasW, this.canvasH);
    const width = snapshot.cols * snapshot.cellPx;
    const height = snapshot.rows * snapshot.cellPx;
    const boxX = Math.floor((this.canvasW - width) / 2);
    const boxY = Math.floor((this.canvasH - height) / 2);
    ctx.fillStyle = '#101024';
    ctx.fillRect(boxX, boxY, width, height);
    for (let row = 0; row < snapshot.rows; row++) {
      let runStart = -1;
      let runPhase = -1;
      let runShade = -1;
      for (let col = 0; col <= snapshot.cols; col++) {
        const index = row * snapshot.cols + col;
        const occupied = col < snapshot.cols && snapshot.cells[index] === 1;
        const phase = occupied ? snapshot.phase[index] : -1;
        const shade = occupied ? snapshot.shade[index] : -1;
        if (occupied && phase === runPhase && shade === runShade) {
          continue;
        }
        if (runStart >= 0) {
          ctx.fillStyle = runPhase === 0 ? SHADE_GREEN[runShade] : SHADE_BLUE[runShade];
          ctx.fillRect(boxX + runStart * snapshot.cellPx, boxY + row * snapshot.cellPx,
            (col - runStart) * snapshot.cellPx, snapshot.cellPx);
        }
        runStart = occupied ? col : -1;
        runPhase = phase;
        runShade = shade;
      }
    }
    ctx.strokeStyle = 'rgba(255,255,255,0.35)';
    ctx.lineWidth = 3;
    ctx.strokeRect(boxX, boxY, width, height);
    this.displayInfo = `FPS=${this.fps} LIQ=${frame.sequence}`;
  }

  private consumeP011TiltWant(): P011InputSelection {
    const sequence = AppStorage.get<number>('p011TiltWantCommandSeq') ?? 0;
    // §26.4 decision 1 (mutual exclusion): while the schedule player is
    // active a fresh single command is NOT consumed and does NOT advance
    // its sequence — the rejection is hilog-only (a rejected command never
    // became an input; nothing to ledger).
    if (this.p011SchedulePlayer.isActive() && sequence > this.p011TiltWantConsumedSeq) {
      hilog.warn(0x0000, 'SandCanvas',
        'P011 want SCHEDULE_ACTIVE_COMMAND_REJECTED seq=%{public}d', sequence);
      this.p011TiltWantConsumedSeq = sequence;
      AppStorage.setOrCreate('p011TiltWantConsumedSeq', sequence);
      const held = this.readInputModeState();
      return { state: held, source: held.scriptTargetTilt !== null ? 'script' : 'gesture',
        targetTilt: held.scriptTargetTilt, freshScriptCommand: null };
    }
    const command = decodeP011TiltWant(AppStorage.get<string>('p011TiltWantCommand'),
      sequence, this.p011TiltWantConsumedSeq);
    const selection = selectP011Input(this.readInputModeState(), sequence, command);
    this.writeInputModeState(selection.state);
    return selection;
  }

  /**
   * §26.4: consume a pending p011schedule want (peakADeg:rate). Symmetric
   * mutual exclusion — a schedule want arriving while a single command
   * holds the session (script target active) is rejected hilog-only, never
   * started. Returns true when the player started this call.
   */
  private consumeP011ScheduleWant(nowMs: number): boolean {
    const seq = AppStorage.get<number>('p011ScheduleWantSeq') ?? 0;
    if (seq <= this.p011ScheduleWantConsumedSeq) {
      return false;
    }
    this.p011ScheduleWantConsumedSeq = seq;
    const state = this.readInputModeState();
    const singleCommandSession = state.scriptTargetTilt !== null;
    if (singleCommandSession || this.p011SchedulePlayer.isActive()) {
      hilog.warn(0x0000, 'SandCanvas',
        'P011 schedule SCHEDULE_ACTIVE_COMMAND_REJECTED seq=%{public}d', seq);
      return false;
    }
    const peakADeg = AppStorage.get<number>('p011SchedulePeakADeg');
    const rateDegPerSec = AppStorage.get<number>('p011ScheduleRate');
    if (peakADeg === undefined || rateDegPerSec === undefined) {
      return false;
    }
    // §26.5: arm the run metadata BEFORE player.start() so the state
    // machine is armed when the first platform substep runs (the first
    // ledger record then carries executedThresholds/decision fields). The
    // setter path is a direct flight — it does NOT go through
    // freshScriptCommand consumption and is unaffected by the mutual
    // exclusion gate. Route binding is orthogonal (D-1): schedule arming
    // never touches setNaturalObservationRouteId.
    const metadataArmed = AppStorage.get<boolean>('p011ScheduleMetadataArmed');
    if (metadataArmed === true) {
      const runRole = AppStorage.get<string>('p011ScheduleRunRole');
      const calibrationId = AppStorage.get<string>('p011ScheduleCalibrationId');
      const thetaStart = AppStorage.get<number>('p011ScheduleThetaStartDeg');
      const thetaStop = AppStorage.get<number>('p011ScheduleThetaStopDeg');
      const wsField = this.waterSandField;
      if (runRole !== undefined && calibrationId !== undefined &&
        thetaStart !== undefined && thetaStop !== undefined && wsField !== null) {
        const role = runRole === 'calibration' || runRole === 'holdout'
          ? runRole : null;
        wsField.setNaturalObservationRunMetadata(role, calibrationId,
          thetaStart, thetaStop);
        // §26.6: arm the execution identity together with the metadata
        // (the parse-level XOR binding guarantees the triple is complete
        // whenever metadata is armed).
        const execCommit = AppStorage.get<string>('p011ScheduleExecSourceCommit');
        const execFileSha = AppStorage.get<string>('p011ScheduleExecSourceFileSha256');
        const execBuildSha = AppStorage.get<string>('p011ScheduleExecBuildSha256');
        if (execCommit !== undefined && execFileSha !== undefined &&
          execBuildSha !== undefined) {
          wsField.setNaturalObservationExecutionIdentity(
            execCommit, execFileSha, execBuildSha);
        }
      }
    }
    const started = this.p011SchedulePlayer.start(peakADeg, rateDegPerSec, nowMs);
    if (started) {
      hilog.info(0x0000, 'SandCanvas',
        'P011 schedule player START peak=%{public}f rate=%{public}f t0=%{public}f',
        peakADeg, rateDegPerSec, nowMs);
    }
    return started;
  }

  /**
   * §26.4: player tick — dispatch every due schedule command through the
   * EXISTING input path: setTilt target override + segment binding + input
   * sequence + drift on the attempt layer. No planner bypass; the substep
   * pipeline runs exactly as for want commands.
   */
  private tickP011SchedulePlayer(nowMs: number,
    setDispatchedCommand: (targetTilt: number, inputSequence: number,
      commandTime: number) => void): void {
    if (!this.p011SchedulePlayer.isActive()) {
      return;
    }
    const ws = this.waterSandField;
    if (ws === null) {
      this.p011SchedulePlayer.forceStop();
      return;
    }
    const dispatches = this.p011SchedulePlayer.tick(nowMs);
    for (const dispatch of dispatches) {
      const command = dispatch.command;
      // segment identity flows exactly like a v3 want command binding.
      ws.setP011CurrentCommandSegment(
        command.phaseKind === 'ramp' ? command.legId : null,
        command.segmentId, command.phaseKind);
      ws.setP011ScheduleDrift(dispatch.driftMs);
      ws.setTilt(command.targetTiltDeg);
      const inputSequence = ws.nextP011InputSequence();
      setDispatchedCommand(command.targetTiltDeg, inputSequence,
        dispatch.actualDispatchMs);
      // Every dispatch logs its drift (the rhythm-evidence stream); the
      // exceeded marker rides on top when > 50 ms.
      hilog.info(0x0000, 'SandCanvas',
        'P011 SCHEDULE_DISPATCH cmd=%{public}d off=%{public}d driftMs=%{public}f',
        command.commandSequence, command.offsetMs, dispatch.driftMs);
      if (dispatch.driftExceeded) {
        hilog.warn(0x0000, 'SandCanvas',
          'P011 SCHEDULE_DRIFT_EXCEEDED cmd=%{public}d driftMs=%{public}f',
          command.commandSequence, dispatch.driftMs);
      }
    }
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
    this.waterSandField = new WaterSandField(this.waterSandCols, this.waterSandCapacity);
    this.waterSandParams = defaultWaterSandParams();
    this.waterSandParams.vSandTopMax = this.waterSandVSandTopMax;
    if (this.waterSandPreset === 'thick') {
      this.waterSandParams.angleDeg = 4;
      this.waterSandParams.maxAvalancheFlux = 80;
      this.waterSandParams.bottomCreepRate = 96;
    }
    this.waterSandField.configure(this.waterSandParams);
    this.waterSandField.initialize(this.waterSandScene(this.waterSandPreset, this.waterSandCols));
    this.setupNaturalObservation(this.waterSandField);
    this.waterSandSnapshot = this.waterSandField.snapshot();
    this.waterSandVisualInverted = false;
    // P-012 E-batch：物理重建 → fieldEpoch 递增 + 断配对（不改物理逻辑）
    this.p012FieldEpoch++;
    this.p012Renderer?.resetContinuity();
    this.lastWaterSandMetricTime = Date.now();
    this.logWaterSand(`WS_EVENT init ${JSON.stringify({
      preset: this.waterSandPreset,
      cols: this.waterSandCols,
      capacity: this.waterSandCapacity,
      ledgerVersion: 2,
      paramsVersion: 'v2.7',
      routeId: this.waterSandParams.routeId,
      vH: this.waterSandParams.heavySettleSpeed,
      vL: this.waterSandParams.lightSettleSpeed,
      liftHRate: this.waterSandParams.heavyLiftRate,
      liftLRate: this.waterSandParams.lightLiftRate,
      shakeRampRate: this.waterSandParams.shakeRampRate,
      vSandTopMax: this.waterSandParams.vSandTopMax,
      totalMotionBudget: this.waterSandParams.totalMotionBudget,
      bottomCreepRate: this.waterSandParams.bottomCreepRate,
      vBottomCreepMax: this.waterSandParams.vBottomCreepMax,
      bottomCreepVelocityGain: this.waterSandParams.bottomCreepVelocityGain,
      bottomCreepDeadZone: this.waterSandParams.bottomCreepDeadZone,
      directionHoldTime: this.waterSandParams.directionHoldTime,
      inputSmoothingAlpha: this.waterSandParams.inputSmoothingAlpha,
      boundary: true,
      percolationRate: this.waterSandParams.percolationRate
    })}`);
    this.logWaterSand(`P011_EVENT reset ${JSON.stringify({
      routeId: this.waterSandParams.routeId,
      runId: this.waterSandField.getP011RunId(),
      sequence: 0,
      eventTime: Date.now(),
      physicsTime: 0,
      reason: 'scene-reset',
      boundary: true
    })}`);
    if (this.waterSandSnapshotEnabled) {
      this.logWaterSandSnapshot();
    }
    if (this.renderMode === MODE_LIQUID) {
      this.startLiquidController();
    } else {
      this.stopLiquidController();
    }
    this.needRedraw = true;
  }

  frame(frameTimeInNano: number = Date.now() * 1000000): void {
    // P-011 dual-input-v1: complete mode envelope before consuming its optional tilt.
    this.processLiquidWantCommand();
    const f = this.field;
    if (f === null) {
      return;
    }
    if (this.renderMode === MODE_LIQUID) {
      this.liquidFrame(frameTimeInNano);
      return;
    }
    // FPS 统计
    const now = Date.now();
    const previousFrameTime = this.lastFrameTime;
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
    const frameDeltaMs = previousFrameTime > 0 ? now - previousFrameTime : 16.7;
    // §26.4: schedule want entry (mutual-exclusion checked inside) and the
    // player tick on the monotonic Date.now() clock. Dispatched commands
    // override the script target through the same variable the want path
    // uses, so the physics substep pipeline is untouched.
    let scheduleDispatchTilt: number | null = null;
    let scheduleDispatchSequence = 0;
    if (this.renderMode === MODE_WATERSAND) {
      this.consumeP011ScheduleWant(now);
      this.tickP011SchedulePlayer(now, (targetTilt, inputSequence): void => {
        scheduleDispatchTilt = targetTilt;
        scheduleDispatchSequence = inputSequence;
        this.p011ScriptTargetTilt = targetTilt;
        this.tiltX = targetTilt;
      });
    }
    const p011Selection = this.renderMode === MODE_WATERSAND ? this.consumeP011TiltWant() : null;
    if (p011Selection !== null && p011Selection.targetTilt !== null) {
      this.tiltX = p011Selection.targetTilt;
    }

    // 重力向量：手势（保持期内）优先，其次传感器
    // CA 引擎吃重力向量 (gx, gy)，360° 任意方向自动正确（含倒立）
    let gx: number;
    let gy: number;
    let inputSource: WaterSandP011InputSource = 'gesture';
    let inputCommandTime = 0;
    if (!this.testMode && this.hasFreshSensorSample) {
      inputSource = 'sensor';
      inputCommandTime = this.lastSensorCommandTime;
      // VYG-AL00 加速度计 x 轴与屏幕左右相反，取反镜像
      const sx = -this.sensorGx;
      const sy = this.sensorGy;
      // 平放检测：合成加速度过小时 atan2 噪声主导 → 保持上次角度
      let tilt = this.tiltX;
      if (Math.sqrt(sx * sx + sy * sy) > 0.3) {
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
        inputCommandTime = nextP011TimeMs();
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
        inputCommandTime = nextP011TimeMs();
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
      } else if (this.p011ScriptTargetTilt !== null && this.testMode) {
        // Fresh and held script targets are stable; only a new Want changes them.
        inputSource = 'script';
      } else if (this.tiltX !== 0 && this.testMode && now >= this.gestureHoldUntil) {
        inputSource = 'script';
        inputCommandTime = nextP011TimeMs();
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
      } else if (now < this.gestureHoldUntil) {
        inputSource = 'gesture';
        inputCommandTime = this.lastTiltCommandTime > 0 ? this.lastTiltCommandTime : nextP011TimeMs();
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
    if (this.renderMode === MODE_WATERSAND) {
      const ws = this.waterSandField;
      if (ws !== null) {
        const p011TiltWant = p011Selection === null ? null : p011Selection.freshScriptCommand;
        let p011ScriptHasEvent = false;
        let p011ScriptCommandTime = 0;
        let p011ScriptAcceptedTime = 0;
        let p011ScriptTargetTilt = 0;
        let p011InputSequence = 0;
        if (p011TiltWant !== null) {
          this.p011ScriptTargetTilt = p011TiltWant.targetTilt;
          this.tiltX = p011TiltWant.targetTilt;
          ws.setNaturalObservationRunMetadata(
            p011TiltWant.observationRunRole,
            p011TiltWant.thresholdCalibrationId,
            p011TiltWant.thetaStartDeg,
            p011TiltWant.thetaStopDeg
          );
          ws.setNaturalObservationScheduleId(p011TiltWant.scheduleId);
          // correction route wiring (D-1 follow-up): the explicitly bound
          // apply route travels with the command; null keeps the legacy
          // apply. Wiring only — activation still requires the armed state
          // machine (correctionRouteActive dual-condition gate).
          ws.setNaturalObservationRouteId(p011TiltWant.correctionRouteId ?? null);
          // Observation-3 v3 (GAP-2/5): schedule segment binding and
          // execution identity flow with the command.
          ws.setP011CurrentCommandSegment(p011TiltWant.v3LegId ?? null,
            p011TiltWant.v3SegmentId ?? null, p011TiltWant.v3PhaseKind ?? null);
          if (p011TiltWant.execSourceCommit !== null ||
            p011TiltWant.execSourceFileSha256 !== null ||
            p011TiltWant.execBuildSha256 !== null) {
            ws.setNaturalObservationExecutionIdentity(
              p011TiltWant.execSourceCommit ?? null,
              p011TiltWant.execSourceFileSha256 ?? null,
              p011TiltWant.execBuildSha256 ?? null);
          }
          inputSource = 'script';
          inputCommandTime = p011TiltWant.commandTime;
          p011ScriptHasEvent = true;
          p011InputSequence = ws.nextP011InputSequence();
          p011ScriptCommandTime = p011TiltWant.commandTime;
          p011ScriptAcceptedTime = nextP011TimeMs();
          p011ScriptTargetTilt = p011TiltWant.targetTilt;
        } else if (scheduleDispatchTilt !== null) {
          // §26.4: a schedule-player command dispatched this frame — the
          // input identity is the player's (script source, dispatch time,
          // the input sequence the tick already allocated).
          inputSource = 'script';
          inputCommandTime = nextP011TimeMs();
          p011ScriptHasEvent = true;
          p011InputSequence = scheduleDispatchSequence;
          p011ScriptTargetTilt = scheduleDispatchTilt;
        } else if (this.p011ScriptTargetTilt !== null && this.testMode && !this.forceInvert) {
          this.tiltX = this.p011ScriptTargetTilt;
        } else {
          this.p011ScriptTargetTilt = null;
        }
        const normalized = ((this.tiltX % 360) + 360) % 360;
        const signedTilt = normalized >= 180 ? normalized - 360 : normalized;
        const visualInverted = Math.abs(signedTilt) > 90;
        if (visualInverted !== this.waterSandVisualInverted) {
          ws.invert180();
          this.waterSandVisualInverted = visualInverted;
          // P-012 E-batch：倒立事件只跟随既有视觉变换并断配对（不二次 invert）
          this.p012ContinuityEpoch++;
          this.p012Renderer?.resetContinuity();
          const drift = ws.getDrift();
          this.logWaterSand(`WS_EVENT invert180 ${JSON.stringify({
            count: 1,
            reason: 'cross90',
            boundary: true,
            driftWater: drift.water,
            driftHeavy: drift.heavy,
            driftLight: drift.light
          })}`);
        }
        const effectiveTilt = visualInverted
          ? Math.max(-89, Math.min(89, signedTilt - (signedTilt > 0 ? 180 : -180)))
          : Math.max(-89, Math.min(89, signedTilt));
        let p011HasEvent = false;
        let p011CommandTime = 0;
        let p011AcceptedTime = 0;
        let p011PhysicsTime = 0;
        let p011TargetTilt = 0;
        const tiltChanged = this.lastWaterSandTilt === null ||
          Math.abs(this.lastWaterSandTilt - effectiveTilt) > 0.01;
        const dormantNoopScriptCommand = p011ScriptHasEvent && !tiltChanged &&
          ws.getMetrics().dormant;
        if (tiltChanged || (p011ScriptHasEvent && !dormantNoopScriptCommand)) {
          let acceptedTime = 0;
          if (this.testMode && !p011ScriptHasEvent) {
            this.logWaterSand(`P011_EVENT input_state ${JSON.stringify({
              routeId: this.waterSandParams.routeId,
              runId: ws.getP011RunId(),
              stateTilt: effectiveTilt,
              source: inputSource,
              diagnosticOnly: true
            })}`);
          }
          ws.setTilt(effectiveTilt);
          this.logWaterSand(`WS_EVENT tilt ${JSON.stringify({
            targetDeg: effectiveTilt,
            effectiveDeg: effectiveTilt
          })}`);
          this.lastWaterSandTilt = effectiveTilt;
        }
        if (p011ScriptHasEvent) {
          p011HasEvent = true;
          p011CommandTime = p011ScriptCommandTime;
          p011AcceptedTime = p011ScriptAcceptedTime;
          p011TargetTilt = p011ScriptTargetTilt;
        }
        if (this.waterSandPreset === 'thick') {
          ws.setShakeInput(Math.min(1, Math.abs(effectiveTilt) / 90));
        }
        const stepResult = this.testMode
          ? ws.step(frameDeltaMs / 1000, (metric, bottom) => {
            if (p011HasEvent && p011PhysicsTime === 0) {
              p011PhysicsTime = metric.physicsTime;
            }
            this.logWaterSandP011Pair(metric, bottom);
            this.logNaturalObservationHeartbeat(metric);
          })
          : ws.step(frameDeltaMs / 1000);
        if (p011HasEvent && stepResult.dormant) {
          ws.emitP011NoopTelemetry((metric, bottom) => {
            if (p011PhysicsTime === 0) {
              p011PhysicsTime = metric.physicsTime;
            }
            this.logWaterSandP011Pair(metric, bottom);
          });
        }
        if (p011HasEvent && p011PhysicsTime > 0) {
          this.logWaterSand(`P011_EVENT input ${JSON.stringify({
            routeId: this.waterSandParams.routeId,
            runId: ws.getP011RunId(),
            sequence: p011InputSequence,
            commandTime: p011CommandTime,
            acceptedTime: p011AcceptedTime,
            physicsTime: p011PhysicsTime,
            targetTilt: p011TargetTilt,
            source: 'script'
          })}`);
        }
        const modelDormant = ws.getMetrics().dormant;
        if (this.lastWaterSandDormant === null) {
          this.lastWaterSandDormant = modelDormant;
        } else if (this.lastWaterSandDormant !== modelDormant) {
          this.logWaterSand(`WS_EVENT ${modelDormant ? 'dormant' : 'wake'} ${JSON.stringify({
            value: modelDormant
          })}`);
          this.lastWaterSandDormant = modelDormant;
        }
        if (!stepResult.ok) {
          this.logWaterSand(`WS_EVENT model_error ${JSON.stringify({
            code: stepResult.errorCode,
            column: stepResult.errorColumn
          })}`);
        }
        if (this.waterSandSnapshot === null) {
          this.waterSandSnapshot = ws.snapshot();
        } else {
          ws.copyStateTo(this.waterSandSnapshot);
        }
        // P-012 E-batch 薄分发（§9.2）：一次帧只走一个完整渲染分支，禁止叠加；
        // canonicalEvidence（物理证据采集 wssnapshot=true）强制 OFF 分支。
        if (p012ShouldUseP012Renderer(this.p012Renderer,
          this.p012AppearanceTier as P012AppearanceTier, this.waterSandSnapshotEnabled)) {
          this.p012Renderer?.render(this.ctx, this.waterSandSnapshot,
            this.buildP012FrameContext());
        } else {
          this.renderWaterSand(this.ctx, this.waterSandSnapshot);
        }
        if (this.testMode) {
          this.logWaterSand(`WS_FRAME ${JSON.stringify(
            waterSandFrameHookPayload(frameDeltaMs, stepResult)
          )}`);
        }
        if (now - this.lastWaterSandMetricTime >= 250) {
          this.lastWaterSandMetricTime = now;
          this.logWaterSand(`WS_METRIC ${JSON.stringify(ws.getMetrics(this.cellPx))}`);
          if (this.waterSandSnapshotEnabled) {
            this.logWaterSandSnapshot();
          }
        }
      }
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

  private smoothWaterSandProfile(source: Float64Array, target: Float64Array): void {
    const n = source.length;
    if (n === 0) {
      return;
    }
    for (let c = 0; c < n; c++) {
      const left = source[Math.max(0, c - 1)];
      const right = source[Math.min(n - 1, c + 1)];
      target[c] = (left + source[c] + right) / 3;
    }
  }

  private mixedSandColor(heavyShare: number): string {
    const heavyRed = 0x24 + (0x63 - 0x24) * heavyShare;
    const heavyGreen = 0x57 + (0xA3 - 0x57) * heavyShare;
    const heavyBlue = 0x1F + (0x5D - 0x1F) * heavyShare;
    const lightRed = 0xD9 + (0xF6 - 0xD9) * heavyShare;
    const lightGreen = 0xA6 + (0xE2 - 0xA6) * heavyShare;
    const lightBlue = 0x4A + (0xA7 - 0x4A) * heavyShare;
    const red = Math.round(lightRed * (1 - heavyShare) + heavyRed * heavyShare);
    const green = Math.round(lightGreen * (1 - heavyShare) + heavyGreen * heavyShare);
    const blue = Math.round(lightBlue * (1 - heavyShare) + heavyBlue * heavyShare);
    return `rgb(${red},${green},${blue})`;
  }

  /** P-012 E-batch：构造 frameContext（复用页面既有字段，不新建事件源） */
  private buildP012FrameContext(): P012FrameContext {
    const sampleTimeMs = Date.now();
    const frameDeltaMs = this.p012LastSampleTimeMs > 0
      ? sampleTimeMs - this.p012LastSampleTimeMs : 16.7;
    this.p012LastSampleTimeMs = sampleTimeMs;
    this.p012FrameSerial++;
    return {
      fieldEpoch: this.p012FieldEpoch,
      continuityEpoch: this.p012ContinuityEpoch,
      geometryEpoch: this.p012GeometryEpoch,
      frameSerial: this.p012FrameSerial,
      sampleTimeMs: sampleTimeMs,
      frameDeltaMs: frameDeltaMs,
      inputMode: this.testMode ? 'TEST' : 'EXPERIENCE',
      sourceKind: this.testMode ? 'script' : 'sensor',
      physicalRouteId: this.waterSandParams.routeId,
      runIdentity: this.waterSandField?.getP011RunId() ?? '',
      boxX: this.boxX,
      boxY: this.boxY,
      boxW: this.boxW,
      boxH: this.boxH,
      pixelRatio: 1,
      visualInverted: this.waterSandVisualInverted,
      appearanceTier: this.p012AppearanceTier as P012AppearanceTier,
      renderSeed: this.p012RenderSeed,
      configRevision: this.p012ConfigRevision,
      ablation: this.p012Ablation,
      fillBox: this.p012FillBox,
      canonicalEvidence: this.waterSandSnapshotEnabled
    };
  }

  /** P-012 E-batch：外观档位循环（OFF→E→EA→EAB→OFF；面板按钮）。
   *  延迟创建 renderer；OFF→非 OFF 重新建基线（不跨 OFF 累计）；切 OFF 不释放
   *  静态 tile（确定性资源可复用），仅断配对。 */
  cycleP012AppearanceTier(): void {
    const next = (this.p012AppearanceTier + 1) % 4;
    this.setP012AppearanceTier(next, 'panel');
  }

  setP012AppearanceTier(tier: number, source: string): void {
    const clipped = Math.max(P012AppearanceTier.OFF, Math.min(P012AppearanceTier.EAB, tier));
    if (clipped !== this.p012AppearanceTier) {
      this.p012AppearanceTier = clipped;
      this.p012ConfigRevision++;
      // 从 OFF 重新启用：先重建运动配对，不能跨 OFF 时段累计
      this.p012Renderer?.resetContinuity();
    }
    if (clipped !== P012AppearanceTier.OFF && this.p012Renderer === null) {
      // 延迟创建：首次启用非 OFF 档时构造（aboutToAppear 时 waterSandField 可能
      // 尚未 initField，但 renderer 只依赖 cols/capacity 常量）
      this.p012Renderer = new P012WaterSandRenderer(this.waterSandCols, this.waterSandCapacity);
    }
    console.info(`[P012] appearanceTier -> ${clipped} (source=${source})`);
    this.syncSheet();
  }

  private renderWaterSand(ctx: CanvasRenderingContext2D, snapshot: WaterSandSnapshot): void {
    const n = snapshot.cols;
    if (this.waterRenderA.length !== n) {
      this.waterRenderA = new Float64Array(n);
      this.waterRenderB = new Float64Array(n);
      this.waterRenderC = new Float64Array(n);
      this.waterRenderD = new Float64Array(n);
      this.waterRenderE = new Float64Array(n);
    }
    const bh = this.waterRenderA;
    const mixStart = this.waterRenderB;
    const sandTop = this.waterRenderC;
    const waterTop = this.waterRenderE;
    for (let c = 0; c < n; c++) {
      bh[c] = snapshot.bh[c];
      mixStart[c] = snapshot.bh[c] + snapshot.bl[c];
      sandTop[c] = snapshot.sandTop[c];
      waterTop[c] = snapshot.waterTop[c];
    }
    const scratch = this.waterRenderD;
    this.smoothWaterSandProfile(bh, scratch);
    this.smoothWaterSandProfile(scratch, bh);
    this.smoothWaterSandProfile(bh, scratch);
    this.smoothWaterSandProfile(scratch, bh);
    this.smoothWaterSandProfile(mixStart, scratch);
    this.smoothWaterSandProfile(scratch, mixStart);
    this.smoothWaterSandProfile(mixStart, scratch);
    this.smoothWaterSandProfile(scratch, mixStart);
    this.smoothWaterSandProfile(sandTop, scratch);
    this.smoothWaterSandProfile(scratch, sandTop);
    this.smoothWaterSandProfile(sandTop, scratch);
    this.smoothWaterSandProfile(scratch, sandTop);
    this.smoothWaterSandProfile(waterTop, scratch);
    this.smoothWaterSandProfile(scratch, waterTop);
    this.smoothWaterSandProfile(waterTop, scratch);
    this.smoothWaterSandProfile(scratch, waterTop);

    const centerX = this.boxX + this.boxW / 2;
    const centerY = this.boxY + this.boxH / 2;
    ctx.save();
    if (this.waterSandVisualInverted) {
      ctx.translate(centerX, centerY);
      ctx.rotate(Math.PI);
      ctx.translate(-centerX, -centerY);
    }
    ctx.fillStyle = '#101024';
    ctx.fillRect(this.boxX, this.boxY, this.boxW, this.boxH);
    const columnWidth = this.boxW / n;
    const heightToY = (height: number): number =>
      this.boxY + (snapshot.capacity - Math.max(0, Math.min(snapshot.capacity, height))) * this.boxH / snapshot.capacity;
    for (let c = 0; c < n; c++) {
      const x = this.boxX + c * columnWidth;
      const waterTopY = heightToY(waterTop[c]);
      const sandTopY = heightToY(sandTop[c]);
      const mixStartY = heightToY(mixStart[c]);
      const bhY = heightToY(bh[c]);
      const bottomY = this.boxY + this.boxH;
      ctx.fillStyle = 'rgba(64,148,222,0.42)';
      ctx.fillRect(x, waterTopY, columnWidth, Math.max(0, sandTopY - waterTopY));
      ctx.fillStyle = '#24571F';
      ctx.fillRect(x, bhY, columnWidth, Math.max(0, bottomY - bhY));
      ctx.fillStyle = '#D9A64A';
      ctx.fillRect(x, mixStartY, columnWidth, Math.max(0, bhY - mixStartY));
      const mixTotal = snapshot.mh[c] + snapshot.ml[c];
      const heavyShare = mixTotal > 0 ? snapshot.mh[c] / mixTotal : 0.5;
      ctx.fillStyle = this.mixedSandColor(heavyShare);
      ctx.fillRect(x, sandTopY, columnWidth, Math.max(0, mixStartY - sandTopY));
    }
    ctx.strokeStyle = 'rgba(210,240,255,0.70)';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(this.boxX, heightToY(waterTop[0]));
    for (let c = 1; c < n; c++) {
      ctx.lineTo(this.boxX + c * columnWidth, heightToY(waterTop[c]));
    }
    ctx.stroke();
    ctx.strokeStyle = 'rgba(255,224,130,0.95)';
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.moveTo(this.boxX, heightToY(sandTop[0]));
    for (let c = 1; c < n; c++) {
      ctx.lineTo(this.boxX + c * columnWidth, heightToY(sandTop[c]));
    }
    ctx.stroke();
    ctx.strokeStyle = 'rgba(255,255,255,0.35)';
    ctx.lineWidth = 3;
    ctx.strokeRect(this.boxX, this.boxY, this.boxW, this.boxH);
    ctx.restore();
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
          this.applyInputMode(!this.testMode, 'longPress');
          this.lastLongPressTime = Date.now();
        })
    )
    .gesture(
      PanGesture()
        .onActionUpdate((e: GestureEvent) => {
          if (!canP011GestureOwnInput(this.readInputModeState())) {
            return;
          }
          this.gestureHoldUntil = Date.now() + 2500;
          this.lastTiltCommandTime = nextP011TimeMs();
          // 映射系数 0.4：200px 满行程 ≈ 80°，比旧 0.3 跟手 33%
          this.tiltX = Math.max(-80, Math.min(80, e.offsetX * this.gestureSensitivity));
        })
        .onActionEnd(() => {
          if (!canP011GestureOwnInput(this.readInputModeState())) {
            return;
          }
          // TEST never restores sensor ownership on gesture end.
          this.useSensor = !this.testMode;
        })
    )
  }
}
