// P-013 P1-G0 平台探针页（真机取证）
// 五项探针：①surface 生命周期 ②sRGB attachment ③三层 z 序 ④透明 ⑤输入透传
//
// PROBE-ONLY：仅供 P-013 P1-G0 取证使用，严禁并入发布分支。
//
// 叠层顺序刻意镜像 P-013 真实集成（SandCanvas 末位追加 XComponent 为顶层）：
//   Stack { Canvas bg(底色+sRGB 对照色块) ; Canvas fg(应用画布角色+触摸计数) ; XComponent(GLES 顶层) }
//
// native 库二选一（want p013g0lib）：
//   hello    → libp013hello.so：恒定清屏色 glClearColor(0.18,0.20,0.22,1.0)（P0a 已验证可用）
//   renderer → libp013renderer.so：P1 诊断 pass（当前真机帧提交被 native 判型拒绝，见验收单 §4.2）
//
// 判读要点：
//   ②sRGB  → GLES 区实测 vs Canvas 直写同值色块；相等=两栈色彩空间一致，
//              显著偏亮(=linear→sRGB 转换后) → framebuffer 被当线性处理（R5 风险实锤）
//   ③z 序  → GLES 层若整屏覆盖 → XComponent surface 恒在应用 UI 之上（Canvas 前景职责不可实现）
//   ④透明  → 需要 renderer 诊断 pass 的 alpha 通道行为（hello 为 alpha=1.0 不透明）
//   ⑤输入  → 顶层 XComponent 是否吞掉下层 Canvas 的触摸（hit=none/transparent/default 三档）
import p013hello from 'libp013hello.so';
import p013renderer from 'libp013renderer.so';
import { FrameCallback } from '@ohos.arkui.UIContext';
import hilog from '@ohos.hilog';
import { P013FrameInput, P013LightParams, P013NativeRenderer, P013RendererState,
  P013FrameResult, P013TelemetryBatch, P013TelemetryRecord } from '../render/P013RenderTypes';

const DOMAIN: number = 0x0000;
const TAG: string = 'P013G0';
const COMPONENT_ID: string = 'p013_g0_surface';
const HELLO_COMPONENT_ID: string = 'p013_g0_hello';
const COLS: number = 128;
const CAPACITY: number = 192;
/** 合成平坦沙面：sandTop=96（半高）、waterTop=172、bh=0、mix=0 → b≤m≤s≤w≤capacity 成立 */
const SAND_TOP: number = 96;
const WATER_TOP: number = 172;
/** sRGB 对照色块 = hello 模块清屏色的 8bit 直写值 (0.18,0.20,0.22)×255 → (46,51,56) */
const SRGB_R: number = 46;
const SRGB_G: number = 51;
const SRGB_B: number = 56;

declare interface P013HelloNative {
  isReady(): boolean;
  setActive(active: boolean): boolean;
  drawFrame(): boolean;
  getFrameCount(): number;
}

class P013G0FrameCallback extends FrameCallback {
  owner: P013P1G0Probe | null = null;
  onFrame(frameTimeInNano: number): void {
    if (this.owner !== null) {
      this.owner.tickFrame(frameTimeInNano);
    }
  }
}

@Entry
@Component
struct P013P1G0Probe {
  @State status: string = '等待 XComponent onLoad';
  @State touchCount: number = 0;
  @State frameCount: number = 0;
  @State lastCode: number = -1;
  @State boxInfo: string = '';
  private ctxBg: CanvasRenderingContext2D = new CanvasRenderingContext2D(new RenderingContextSettings(true));
  private ctxFg: CanvasRenderingContext2D = new CanvasRenderingContext2D(new RenderingContextSettings(true));
  private drawn: boolean = false;
  /**
   * native 绑定必须惰性获取：字段初始化器会在 XComponent 创建之前触碰 .so 的 napi 绑定
   * → .so 提前 dlopen → napi Init 时 OH_NATIVE_XCOMPONENT_OBJ 尚不存在 → native Hook 早退
   * → 永远收不到 surface window（state 停在 WAIT_WINDOW、surface 全黑并盖住下层 Canvas）。
   * 必须在 XComponent onLoad 之后才取用（与 P0a hello 页同源教训）。
   */
  private helloRef: P013HelloNative | null = null;
  private nativeRef: P013NativeRenderer | null = null;
  private vp2pxFactor: number = 0;
  private frameCb: P013G0FrameCallback = new P013G0FrameCallback();
  private frameCbStarted: boolean = false;
  private pageActive: boolean = false;
  private inited: boolean = false;
  private sessionId: number = 0;
  private frameId: number = 0;
  private snapshotId: number = 0;
  private tdata: Float32Array = new Float32Array(COLS * 4);
  private diagOn: boolean = true;
  private hitMode: string = 'default';
  private useHello: boolean = true;
  private touchTotal: number = 0;
  private canvasW: number = 0;
  private canvasH: number = 0;

  aboutToAppear(): void {
    this.frameCb.owner = this;
    this.diagOn = AppStorage.get<boolean>('p013g0diag') !== false;
    const hit = AppStorage.get<string>('p013g0hit');
    this.hitMode = hit !== undefined ? hit : 'default';
    const lib = AppStorage.get<string>('p013g0lib');
    this.useHello = lib === undefined ? true : lib !== 'renderer';
    for (let c = 0; c < COLS; c++) {
      this.tdata[c * 4] = SAND_TOP;
      this.tdata[c * 4 + 1] = WATER_TOP;
      this.tdata[c * 4 + 2] = 0;
      this.tdata[c * 4 + 3] = 0;
    }
  }

  aboutToDisappear(): void {
    this.pageActive = false;
    if (this.helloRef !== null) {
      this.helloRef.setActive(false);
    }
    if (this.nativeRef !== null) {
      this.nativeRef.setActive(false);
    }
    this.frameCb.owner = null;
  }

  onPageShow(): void {
    this.pageActive = true;
    if (this.helloRef !== null) {
      this.helloRef.setActive(true);
    }
    if (this.nativeRef !== null) {
      this.nativeRef.setActive(true);
    }
    if (!this.frameCbStarted) {
      this.frameCbStarted = true;
      this.getUIContext().postFrameCallback(this.frameCb);
    }
  }

  onPageHide(): void {
    this.pageActive = false;
    if (this.helloRef !== null) {
      this.helloRef.setActive(false);
    }
    if (this.nativeRef !== null) {
      this.nativeRef.setActive(false);
    }
  }

  /** 每帧 tick：hello 模式驱动恒定清屏色；renderer 模式走 P1 诊断 pass */
  tickFrame(frameTimeNs: number): void {
    this.frameCbStarted = false;
    if (!this.pageActive || this.frameCb.owner === null) {
      return;
    }
    this.ensureDrawn();
    try {
      if (this.useHello) {
        this.tickHello();
      } else {
        this.tickRenderer(frameTimeNs);
      }
    } catch (e) {
      hilog.warn(DOMAIN, TAG, 'WS_EVENT p013_g0_exception %{public}s', JSON.stringify(e));
    }
    this.frameCbStarted = true;
    this.getUIContext().postFrameCallback(this.frameCb);
  }

  /** hello 模式：恒定清屏色 surface（surface 生命周期 + sRGB + z 序 + 输入透传取证） */
  private tickHello(): void {
    const hello = this.helloRef;
    if (hello === null) {
      return;
    }
    const ok = hello.drawFrame();
    this.frameCount++;
    this.lastCode = ok ? 0 : 1;
    if (this.frameCount % 60 === 0) {
      const ready = hello.isReady();
      const nativeFrames = hello.getFrameCount();
      hilog.info(DOMAIN, TAG, 'WS_EVENT p013_g0_hello %{public}s', JSON.stringify({
        frames: this.frameCount, nativeFrames: nativeFrames, ready: ready, ok: ok,
        hitMode: this.hitMode, lib: 'p013hello'
      }));
      this.status = `hello frames=${this.frameCount} native=${nativeFrames} ready=${ready}`;
    }
  }

  /** renderer 模式：P1 诊断 pass（当前真机帧提交被 native 判型拒绝，见验收单 §4.2） */
  private tickRenderer(frameTimeNs: number): void {
    const native = this.nativeRef;
    if (native === null) {
      return;
    }
    let state: P013RendererState = native.getState();
    if (!this.inited) {
      if (state.state === 'WAIT_WINDOW') {
        return;
      }
      const init = native.init(COMPONENT_ID);
      if (init.code !== 0) {
        hilog.warn(DOMAIN, TAG, 'WS_EVENT p013_g0_init_failed code=%{public}d', init.code);
        return;
      }
      this.sessionId = init.sessionId;
      this.inited = true;
      state = native.getState();
      hilog.info(DOMAIN, TAG, 'WS_EVENT p013_g0_init %{public}s', JSON.stringify({
        code: init.code, sessionId: this.sessionId, state: state.state,
        widthPx: state.widthPx, heightPx: state.heightPx, sizeSource: state.sizeSource,
        surfaceEpoch: state.surfaceEpoch, contextEpoch: state.contextEpoch,
        ownerBound: state.ownerBound, active: state.active
      }));
    }
    const pxFactor: number = this.vp2pxFactor > 0 ? this.vp2pxFactor : 1.0;
    const surfW: number = state.widthPx > 0 ? state.widthPx : Math.round(this.canvasW * pxFactor);
    const surfH: number = state.heightPx > 0 ? state.heightPx : Math.round(this.canvasH * pxFactor);
    const bw: number = Math.floor(surfW * 0.45);
    const bh: number = Math.floor(surfH * 0.30);
    const f = {
      sessionId: this.sessionId,
      frameId: ++this.frameId,
      snapshotId: ++this.snapshotId,
      surfaceEpoch: state.surfaceEpoch,
      fieldEpoch: 1,
      geometryEpoch: 1,
      continuityEpoch: 1,
      appearanceConfigRevision: 1,
      lightConfigRevision: 1,
      frameTimeNs: frameTimeNs,
      surfaceWidthPx: surfW,
      surfaceHeightPx: surfH,
      boxXPx: Math.floor((surfW - bw) / 2),
      boxYPx: Math.floor(surfH * 0.25),
      boxWidthPx: bw,
      boxHeightPx: bh,
      capacity: CAPACITY,
      visualInverted: false,
      diagnosticEnabled: this.diagOn,
      T_dataLayoutVersion: 1,
      slopeKernelVersion: 'E2_APPROX_P0',
      zOriginVersion: 1
    } as P013FrameInput;
    const light = {
      la: 0.85, ld: 0.15, lmin: 0.80, lmax: 1.12, lref: 1,
      ellLightMaterial: 24, lightX: -0.55, lightY: -0.55, lightZ: 0.63, sigmaCandidate: 6
    } as P013LightParams;
    const result: P013FrameResult = native.drawFrame(f, this.tdata, light);
    this.frameCount++;
    this.lastCode = result.code;
    if (this.frameCount === 1) {
      this.boxInfo = `box=(${f.boxXPx},${f.boxYPx},${f.boxWidthPx},${f.boxHeightPx}) surface=${surfW}x${surfH}`;
      hilog.info(DOMAIN, TAG, 'WS_EVENT p013_g0_box %{public}s', this.boxInfo);
    }
    if (this.frameCount % 30 === 0) {
      this.dumpTelemetry();
      this.status = `frames=${this.frameCount} code=${result.code} swap=${result.swapSucceeded} ` +
        `eglErr=${result.eglError} glErr=${result.glError} diag=${this.diagOn} hit=${this.hitMode}`;
    }
    if (result.code !== 0) {
      hilog.warn(DOMAIN, TAG, 'WS_EVENT p013_g0_draw_error code=%{public}d frameId=%{public}d',
        result.code, result.frameId);
    }
  }

  /** 抽干 native 遥测并落一行摘要（surface 生命周期 + 帧时 + 标识） */
  private dumpTelemetry(): void {
    const native = this.nativeRef;
    if (native === null) {
      return;
    }
    const batch: P013TelemetryBatch = native.drainTelemetry();
    if (batch.records.length === 0) {
      return;
    }
    const r: P013TelemetryRecord = batch.records[batch.records.length - 1];
    hilog.info(DOMAIN, TAG, 'WS_EVENT p013_g0_telemetry %{public}s', JSON.stringify({
      n: batch.records.length, droppedTelemetry: batch.droppedTelemetry,
      surfaceCreateCount: r.surfaceCreateCount, surfaceDestroyCount: r.surfaceDestroyCount,
      contextCreateCount: r.contextCreateCount, contextDestroyCount: r.contextDestroyCount,
      contextLossCount: r.contextLossCount, recoveryAttemptCount: r.recoveryAttemptCount,
      staleLifecycleCount: r.staleLifecycleCount,
      eglInitMs: r.eglInitMs, makeCurrentMs: r.makeCurrentMs, unbindMs: r.unbindMs,
      swapMs: r.swapMs, shaderBuildMs: r.shaderBuildMs, uploadCpuMs: r.uploadCpuMs,
      uploadBytes: r.uploadBytes, uploadCount: r.uploadCount,
      textureLiveCount: r.textureLiveCount, programLiveCount: r.programLiveCount,
      vaoLiveCount: r.vaoLiveCount, nativeOwnedBytes: r.nativeOwnedBytes,
      gpuPassMs: r.gpuPassMs, gpuTimerValid: r.gpuTimerValid, gpuDisjoint: r.gpuDisjoint,
      shaderHash: r.shaderHash, T_dataHash: r.T_dataHash, errorStage: r.errorStage,
      code: r.code, swapSucceeded: r.swapSucceeded, eglError: r.eglError, glError: r.glError,
      recoveryRequired: r.recoveryRequired, frameGap: r.frameGap, mode: r.mode,
      diagEncoding: r.diagEncoding, surfaceEpoch: r.surfaceEpoch, contextEpoch: r.contextEpoch
    }));
  }

  /** 背景层：底色 + sRGB 对照色块（GLES 恒定清屏色的同值直写） */
  private drawBackground(ctx: CanvasRenderingContext2D): void {
    ctx.fillStyle = '#F0F0F0';
    ctx.fillRect(0, 0, this.canvasW, this.canvasH);
    const hex = (v: number): string => v.toString(16).padStart(2, '0');
    ctx.fillStyle = '#' + hex(SRGB_R) + hex(SRGB_G) + hex(SRGB_B);
    ctx.fillRect(20, this.canvasH - 140, 240, 100);
  }

  /** 前景层（应用画布角色）：洋红竖条与底条（z 序与透明判读用） */
  private drawForeground(ctx: CanvasRenderingContext2D): void {
    ctx.fillStyle = '#FF00FF';
    ctx.fillRect(0, 0, 24, this.canvasH);
    ctx.fillRect(0, this.canvasH - 40, 300, 40);
  }

  /** 首帧布局完成后绘制两层 Canvas */
  private ensureDrawn(): void {
    if (this.drawn) {
      return;
    }
    if (this.ctxBg.width <= 0 || this.ctxFg.width <= 0) {
      return;
    }
    this.canvasW = this.ctxBg.width;
    this.canvasH = this.ctxBg.height;
    this.drawBackground(this.ctxBg);
    this.drawForeground(this.ctxFg);
    this.drawn = true;
    const pv: number = this.getUIContext().px2vp(1);
    this.vp2pxFactor = pv > 0 ? 1.0 / pv : 1.0;
    hilog.info(DOMAIN, TAG, 'WS_EVENT p013_g0_canvas %{public}s',
      JSON.stringify({ w: this.canvasW, h: this.canvasH, vp2px: this.vp2pxFactor,
        pxW: Math.round(this.canvasW * this.vp2pxFactor),
        pxH: Math.round(this.canvasH * this.vp2pxFactor) }));
  }

  private handleTouch(e: TouchEvent): void {
    if (e.type === TouchType.Down) {
      this.touchTotal++;
      this.touchCount = this.touchTotal;
      hilog.info(DOMAIN, TAG, 'WS_EVENT p013_g0_input %{public}s',
        JSON.stringify({ down: this.touchTotal, hitMode: this.hitMode, lib: this.useHello ? 'hello' : 'renderer' }));
    }
  }

  build() {
    Stack() {
      Canvas(this.ctxBg)
        .width('100%')
        .height('100%')
      Canvas(this.ctxFg)
        .width('100%')
        .height('100%')
        .onTouch((e: TouchEvent) => { this.handleTouch(e); })
      XComponent({
        id: this.useHello ? HELLO_COMPONENT_ID : COMPONENT_ID,
        type: XComponentType.SURFACE,
        libraryname: this.useHello ? 'p013hello' : 'p013renderer'
      })
        .width('100%')
        .height('100%')
        .hitTestBehavior(this.hitBehavior())
        .onLoad(() => {
          // 惰性取 native 绑定（必须在 XComponent 创建之后，见上方注释）
          if (this.useHello) {
            this.helloRef = p013hello as P013HelloNative;
            this.helloRef.setActive(true);
          } else {
            this.nativeRef = p013renderer as P013NativeRenderer;
            this.nativeRef.setActive(true);
          }
          this.status = 'XComponent onLoad（native 已挂接）';
        })
      Column() {
        Text(this.status).fontSize(12).fontColor('#000000')
        Text(`touches=${this.touchCount} frames=${this.frameCount} code=${this.lastCode}`).fontSize(12).fontColor('#000000')
        Text(this.boxInfo).fontSize(10).fontColor('#333333')
      }
      .alignItems(HorizontalAlign.Start)
      .position({ x: 16, y: 40 })
    }
    .width('100%')
    .height('100%')
    .backgroundColor('#F0F0F0')
  }

  private hitBehavior(): HitTestMode {
    if (this.hitMode === 'none') {
      return HitTestMode.None;
    }
    if (this.hitMode === 'transparent') {
      return HitTestMode.Transparent;
    }
    if (this.hitMode === 'block') {
      return HitTestMode.Block;
    }
    return HitTestMode.Default;
  }
}
