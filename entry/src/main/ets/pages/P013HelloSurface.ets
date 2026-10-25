// P-013 P0a hello-surface 独立测试页
// 问题 ID：P-013；方案版本：proposal-P013-incorporation-v1 §2.0/§2.3-c
// 独立诊断页：不接 SandCanvas 主页面（红线：零 Canvas 链改动）。
// 验收：XComponent 区域显示深灰 hello 帧；drawFrame 每帧驱动 eglSwapBuffers。
import p013hello from 'libp013hello.so';
import { FrameCallback } from '@ohos.arkui.UIContext';

/** native 模块接口（libp013hello.so 导出——ArkTS 侧显式类型声明） */
declare interface P013HelloNative {
  isReady(): boolean;
  setActive(active: boolean): boolean;
  drawFrame(): boolean;
  getFrameCount(): number;
}

/** 帧回调适配：postFrameCallback 需要传入 FrameCallback 子类实例（同 SandCanvas 模式） */
class P013FrameCallback extends FrameCallback {
  owner: P013HelloSurface | null = null;
  onFrame(_frameTimeInNano: number): void {
    if (this.owner !== null) {
      this.owner.tickFrame();
    }
  }
}

@Entry
@Component
struct P013HelloSurface {
  @State frameCount: number = 0;
  @State ready: boolean = false;
  @State status: string = 'surface 等待创建';
  private frameCb: P013FrameCallback = new P013FrameCallback();
  private frameCbStarted: boolean = false;
  private pageActive: boolean = false;

  aboutToAppear(): void {
    this.frameCb.owner = this;
  }

  aboutToDisappear(): void {
    // A queued callback exits without re-registering after hide/disposal.
    this.pageActive = false;
    (p013hello as P013HelloNative).setActive(false);
    this.frameCb.owner = null;
  }

  onPageShow(): void {
    this.pageActive = true;
    (p013hello as P013HelloNative).setActive(true);
    if (!this.frameCbStarted) {
      this.frameCbStarted = true;
      this.getUIContext().postFrameCallback(this.frameCb);
    }
  }

  onPageHide(): void {
    this.pageActive = false;
    (p013hello as P013HelloNative).setActive(false);
  }

  /** 每帧 tick：驱动 native hello 帧（清屏+swap）并刷新状态 */
  tickFrame(): void {
    this.frameCbStarted = false;
    if (!this.pageActive || this.frameCb.owner === null) {
      return;
    }
    const native = p013hello as P013HelloNative;
    // Drawing owns lazy EGL creation; isReady remains a read-only EGL-ready query.
    const ok = native.drawFrame();
    this.ready = native.isReady();
    if (this.ready) {
      this.frameCount = native.getFrameCount();
      this.ready = true;
      if (!ok) {
        this.status = 'drawFrame 返回 false（EGL 未就绪）';
      } else if (this.frameCount % 300 === 1) {
        this.status = `hello 帧运行中 #${this.frameCount}`;
      }
    }
    this.frameCbStarted = true;
    this.getUIContext().postFrameCallback(this.frameCb);
  }

  build() {
    Column() {
      Text('P-013 P0a hello-surface')
        .fontSize(18).fontColor('#333333').margin(16)
      Text(this.status)
        .fontSize(14).fontColor('#666666').margin({ bottom: 8 })
      Text(`frames: ${this.frameCount}  ready: ${this.ready}`)
        .fontSize(14).fontColor('#888888').margin({ bottom: 16 })
      // XComponent：libname 触发 libp013hello.so 的 __NATIVE_XCOMPONENT_OBJ__
      // 挂接（cpp 侧 HookXComponent）；surface 生命周期由 native 回调接管
      XComponent({
        id: 'p013_hello_xcomponent',
        type: XComponentType.SURFACE,
        libraryname: 'p013hello'
      })
        .width('100%')
        .height('60%')
        .onLoad(() => {
          this.status = 'XComponent onLoad（libp013hello 已挂接）';
        })
    }
    .width('100%')
    .height('100%')
    .backgroundColor('#F5F5F5')
  }
}
