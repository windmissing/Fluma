import m1 from 'libgrsm1.so';
import sensor from '@ohos.sensor';
import systemDateTime from '@ohos.systemDateTime';
import { FrameCallback } from '@ohos.arkui.UIContext';
import { P051GravityConfig, P051GravityInputAdapter } from '../physics/P051GravityInputAdapter';
import { GpuM1DemoController, GpuM1DemoStatus } from '../physics/GpuM1DemoController';

interface ExperienceCapability {
  revision: string;
  buildProfile: string;
  GRS_PROGRESS_V2_EXPERIMENT: boolean;
  K: number;
  Kw: number;
  features: number;
  scenePolicy: string;
}

interface ExperienceStatus extends GpuM1DemoStatus {
  code?: number;
}

class ExperienceFrame extends FrameCallback {
  owner: GpuSandWaterExperience | null = null;
  session: string = '0';
  sequence: number = 0;

  onFrame(ns: number): void {
    if (this.owner === null || this.session === '0' || !Number.isSafeInteger(ns) || ns < 0) return;
    this.owner.tick(ns, m1.pagePulse(this.session, String(++this.sequence), String(ns)));
  }
}

@Entry @Component
struct GpuSandWaterExperience {
  private readonly model: GpuM1DemoController = new GpuM1DemoController(m1);
  private readonly sensorConfig: P051GravityConfig = new P051GravityConfig();
  private readonly adapter: P051GravityInputAdapter = new P051GravityInputAdapter(this.sensorConfig);
  private frame: ExperienceFrame = new ExperienceFrame();
  private shown: boolean = false;
  private active: boolean = false;
  private sensorActive: boolean = false;
  private supportsV2: boolean = false;
  private initialized: boolean = false;
  @State private validCapability: boolean = false;
  private lastTextNs: number = -1;

  @StorageLink('emptySandDemoForeground') @Watch('syncActive') private foreground: boolean = true;
  @State private manual: boolean = false;
  @State private manualDegrees: number = 0;
  @State private source: string = '沙';
  @State private capability: string = '';
  @State private progress: string = '准备中';
  @State private operation: string = '';
  @State private stateText: string = '等待窗口';
  @State private receiptText: string = '';
  @State private gravityText: string = '等待传感器';

  private readonly accelerometer = (data: sensor.AccelerometerResponse): void => {
    if (!this.active || this.manual) return;
    const ms = systemDateTime.getUptime(systemDateTime.TimeType.ACTIVE, true) / 1000000;
    const sample = this.adapter.consumeAccelerometer(data.x, data.y, data.z, ms);
    if (sample.valid && sample.targetTiltDeg !== null) {
      this.gravityText = this.model.sensor(sample.targetTiltDeg, sample.shakeInput);
    } else {
      this.gravityText = 'INVALID_SENSOR';
    }
  };

  onPageShow(): void {
    if (this.initialized) { this.shown = true; this.syncActive(); return; }
    try {
      const caps = JSON.parse(m1.experienceCapabilities()) as ExperienceCapability;
      this.validCapability = caps.revision === 'GRS_M1_EXPERIENCE_V1' && caps.features === 3;
      this.capability = caps.buildProfile + ' · K' + caps.K + '/Kw' + caps.Kw + ' · ' + caps.scenePolicy;
      if (!this.validCapability) { this.operation = 'UNSUPPORTED_CAPABILITY_REVISION'; return; }
      this.supportsV2 = caps.GRS_PROGRESS_V2_EXPERIMENT;
      if (this.supportsV2) m1.setProgressV2(false);
      this.model.reset();
      this.initialized = true;
    } catch (e) {
      this.operation = 'CAPABILITY_READ_FAILED';
      this.validCapability = false;
    }
    this.shown = true;
    this.syncActive();
  }

  onPageHide(): void { this.shown = false; this.syncActive(); }
  aboutToDisappear(): void { this.onPageHide(); }

  private syncActive(): void {
    const active = this.shown && this.foreground && this.validCapability;
    if (this.active === active) return;
    this.active = active;
    this.model.setActive(active);
    if (active) {
      this.frame = new ExperienceFrame();
      this.frame.owner = this;
      this.frame.session = m1.beginPageSession();
      this.getUIContext().postFrameCallback(this.frame);
    } else {
      this.frame.owner = null;
      if (this.frame.session !== '0') m1.endPageSession(this.frame.session);
    }
    this.updateSensor();
  }

  private updateSensor(): void {
    const need = this.active && !this.manual;
    if (need && !this.sensorActive) {
      try {
        sensor.on(sensor.SensorId.ACCELEROMETER, this.accelerometer,
          { interval: this.sensorConfig.sensorIntervalNs });
        this.sensorActive = true;
      } catch (e) { this.gravityText = 'SENSOR_UNAVAILABLE'; }
    } else if (!need && this.sensorActive) {
      try { sensor.off(sensor.SensorId.ACCELEROMETER, this.accelerometer); } catch (e) {
        this.gravityText = 'SENSOR_UNSUBSCRIBE_FAILED';
      }
      this.sensorActive = false;
      this.adapter.reset();
    }
  }

  tick(ns: number, pulse: string): void {
    if (!this.active) return;
    try {
      this.model.refresh();
      const s = this.model.snapshot as ExperienceStatus | null;
      if (s === null || s.progressClockRevision === undefined) this.model.stop('STATUS_FAULT');
      else if (s.progressClockRevision === 'GRS_M1_PROGRESS_CLOCK_V2' && pulse !== 'PAGE_PULSE_ACCEPTED') {
        this.model.stop(pulse);
      } else this.model.tickSnapshot(ns);
      if (s !== null && (this.lastTextNs < 0 || ns < this.lastTextNs || ns - this.lastTextNs >= 33333334)) {
        this.lastTextNs = ns;
        this.progress = s.progressClockRevision === 'GRS_M1_PROGRESS_CLOCK_V2' ? 'V2' :
          s.progressClockRevision === 'GRS_M1_PROGRESS_CLOCK_V1' ? 'V1' : '准备中';
        const stage = s.stage ?? s.permissionState;
        const reason = stage === 'TRANSPORT_CREATE' && s.code === 5 ? 'MEMORY_PROFILE_UNSUPPORTED' : stage;
        const rawCode = s.code === undefined ? '' : ' #' + s.code;
        this.stateText = s.state + ' ' + reason + rawCode + ' / run ' + s.runHi + ':' + s.runLo;
        this.receiptText = '准入 Q ' + s.sourceAdmittedQ + ' / 拒量 Q ' + s.sourceRejectedQ +
          ' / 在途水 Q ' + s.flightHi + ':' + s.flightLo + ' / 落水 Q ' + s.settledHi + ':' + s.settledLo;
      }
    } catch (e) { this.model.stop('STATUS_FAULT'); this.stateText = 'STATUS_FAULT'; }
    this.getUIContext().postFrameCallback(this.frame);
  }

  private selectProgress(v2: boolean): void {
    if (!this.active) return;
    if (!v2 && !this.supportsV2) { this.model.reset(); return; }
    const result: string = m1.setProgressV2(v2);
    this.operation = result;
    if (result === 'PROGRESS_V2_NEXT_RUN' || result === 'PROGRESS_V1_NEXT_RUN') this.model.reset();
  }

  private touch(event: TouchEvent): void {
    if (event.type === TouchType.Cancel) { this.model.stop('CANCEL'); return; }
    for (let i = 0; i < event.changedTouches.length; ++i) {
      const p = event.changedTouches[i];
      if (event.type === TouchType.Down) {
        if (!this.model.down(p.id, p.x, p.y)) this.operation = this.model.message;
      } else if (event.type === TouchType.Move) this.model.move(p.id, p.x, p.y);
      else if (event.type === TouchType.Up) this.model.up(p.id);
    }
  }

  build() {
    Column({ space: 6 }) {
      Text('GPU 沙水体验 · ' + this.progress).fontSize(18)
      Text(this.capability).fontSize(11)
      Text(this.stateText).fontSize(12)
      Text(this.receiptText).fontSize(11)
      Row({ space: 8 }) {
        Button('沙').onClick(() => { this.model.select(0); this.source = '沙'; })
        Button('水').onClick(() => { this.model.select(1); this.source = '水'; })
        Button('新空场').onClick(() => this.model.reset())
      }
      Text('当前 ' + this.source + ' · 抬手停源，已准入水滴继续下落').fontSize(11)
      if (this.validCapability) {
      XComponent({ id: 'grs-m1-experience', type: XComponentType.SURFACE, libraryname: 'grsm1' })
        .width('100%').layoutWeight(1)
        .onAreaChange((_old: Area, area: Area) => this.model.geometry(Number(area.width), Number(area.height)))
        .onTouch((event: TouchEvent) => this.touch(event))
      }
      Row({ space: 8 }) {
        Text('手动倾角').fontSize(11)
        Toggle({ type: ToggleType.Switch, isOn: this.manual }).onChange((value: boolean) => {
          this.manual = value;
          this.model.gravityMode(!value);
          this.updateSensor();
          if (value) this.model.manualAngle(this.manualDegrees);
        })
        Button('新空场 V1').onClick(() => this.selectProgress(false))
        Button('实验 V2').onClick(() => this.selectProgress(true))
      }
      if (this.manual) {
        Slider({ min: -30, max: 30, value: this.manualDegrees, step: 1 }).onChange((value: number) => {
          this.manualDegrees = value;
          this.model.manualAngle(value);
        })
      } else { Text(this.gravityText).fontSize(11) }
      Text(this.operation).fontSize(11)
    }.height('100%').width('100%').padding({ top: 8, bottom: 8 })
  }
}
