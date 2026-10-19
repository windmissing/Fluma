import hilog from '@ohos.hilog';
import { LiquidCAEventPayload, LiquidCAFramePayload, LiquidCAMetricPayload, LiquidInputSource } from './LiquidCATypes';

interface EventLog {
  type: string;
  sequence: number;
  time: number;
  event_time_ms: number;
  caseId: string;
  boundary: boolean;
  source: LiquidInputSource;
  state: boolean;
  force_invert: boolean;
  shake_intensity: number;
  intensity: number;
  error_code: string;
  cols: number;
  rows: number;
  cell_px: number;
  initial_heavy: number;
  initial_light: number;
  seed: number;
  scaled: boolean;
}

interface FrameLog {
  sequence: number;
  caseId: string;
  frame_time_ms: number;
  physics_time_ms: number;
  snapshot_time_ms: number;
  substeps: number;
  clipped_time_ms: number;
  valid: number;
}

interface MetricLog {
  sequence: number;
  caseId: string;
  time: number;
  metric_time_ms: number;
  heavyCount: number;
  lightCount: number;
  emptyCount: number;
  initialHeavyCount: number;
  initialLightCount: number;
  heavyDrift: number;
  lightDrift: number;
  lightCentroidRow: number;
  heavyCentroidRow: number;
  lightCentroidX: number;
  heavyCentroidX: number;
  interfaceRows: number[];
  interfaceRangeCells: number;
  mixedFraction: number;
  mixedBandShare: number;
  lightPixelArea: number;
  heavyPixelArea: number;
  simulationTime: number;
  dormant: boolean;
  changedCells: number;
  finite: boolean;
  gravityInverted: boolean;
}

export class LiquidCATelemetry {
  private domain: number = 0x0000;
  private tag: string = 'LiquidCA';
  private testMode: boolean = false;

  constructor(testMode: boolean = false) {
    this.testMode = testMode;
  }

  setTestMode(enabled: boolean): void {
    this.testMode = enabled;
  }

  onEvent(event: LiquidCAEventPayload): void {
    if (!this.testMode) {
      return;
    }
    const payload: EventLog = {
      type: event.type,
      sequence: event.sequence,
      time: event.eventTimeMs / 1000,
      event_time_ms: event.eventTimeMs,
      caseId: event.caseId === undefined ? '' : event.caseId,
      boundary: event.boundary,
      source: event.source,
      state: event.type === 'invert' ? event.forceInvert : event.shakeIntensity > 0,
      force_invert: event.forceInvert,
      shake_intensity: event.shakeIntensity,
      intensity: event.shakeIntensity,
      error_code: event.errorCode,
      cols: event.cols,
      rows: event.rows,
      cell_px: event.cellPx,
      initial_heavy: event.initialHeavy,
      initial_light: event.initialLight,
      seed: event.seed,
      scaled: event.scaled
    };
    this.emit('LIQ_EVENT', payload);
  }

  onFrame(frame: LiquidCAFramePayload): void {
    if (!this.testMode) {
      return;
    }
    const payload: FrameLog = {
      sequence: frame.sequence,
      caseId: frame.caseId,
      frame_time_ms: frame.frameTimeMs,
      physics_time_ms: frame.physicsTimeMs,
      snapshot_time_ms: frame.snapshotTimeMs,
      substeps: frame.substeps,
      clipped_time_ms: frame.clippedTimeMs,
      valid: frame.valid ? 1 : 0
    };
    this.emit('LIQ_FRAME', payload);
  }

  onMetric(metric: LiquidCAMetricPayload): void {
    if (!this.testMode) {
      return;
    }
    const payload: MetricLog = {
      sequence: metric.sequence,
      caseId: metric.caseId === undefined ? '' : metric.caseId,
      time: metric.simulationTime,
      metric_time_ms: metric.metricTimeMs,
      heavyCount: metric.heavyCount,
      lightCount: metric.lightCount,
      emptyCount: metric.emptyCount,
      initialHeavyCount: metric.initialHeavyCount,
      initialLightCount: metric.initialLightCount,
      heavyDrift: metric.heavyDrift,
      lightDrift: metric.lightDrift,
      lightCentroidRow: metric.lightCentroidY,
      heavyCentroidRow: metric.heavyCentroidY,
      lightCentroidX: metric.lightCentroidX,
      heavyCentroidX: metric.heavyCentroidX,
      interfaceRows: metric.interfaceRowsPx,
      interfaceRangeCells: metric.interfaceRangeCells,
      mixedFraction: metric.mixedBandShare,
      mixedBandShare: metric.mixedBandShare,
      lightPixelArea: metric.lightPixelArea,
      heavyPixelArea: metric.heavyPixelArea,
      simulationTime: metric.simulationTime,
      dormant: metric.dormant,
      changedCells: metric.changedCells,
      finite: metric.finite,
      gravityInverted: metric.gravityInverted
    };
    this.emit('LIQ_METRIC', payload);
  }

  protected emit(name: string, payload: EventLog | FrameLog | MetricLog): void {
    hilog.info(this.domain, this.tag, '%{public}s', `${name} ${JSON.stringify(payload)}`);
  }
}
