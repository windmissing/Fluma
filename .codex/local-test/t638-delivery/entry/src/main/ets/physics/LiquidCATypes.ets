export interface LiquidCAConfig {
  cols: number;
  rows: number;
  cellPx: number;
  seed: number;
  pSettle: number;
  qMixBase: number;
  pInertia: number;
  shakeDecayRate: number;
  maxCells: number;
}

export enum LiquidInputSource {
  WANT = 0,
  SCRIPT = 1,
  PANEL = 2,
  INTERNAL = 3
}

export interface LiquidCAEventPayload {
  type: string;
  sequence: number;
  eventTimeMs: number;
  caseId?: string;
  boundary: boolean;
  source: LiquidInputSource;
  forceInvert: boolean;
  shakeIntensity: number;
  errorCode: string;
  cols: number;
  rows: number;
  cellPx: number;
  initialHeavy: number;
  initialLight: number;
  seed: number;
  scaled: boolean;
}

export interface LiquidCAFramePayload {
  sequence: number;
  caseId: string;
  frameTimeMs: number;
  physicsTimeMs: number;
  snapshotTimeMs: number;
  substeps: number;
  clippedTimeMs: number;
  valid: boolean;
}

export interface LiquidCAMetricPayload {
  sequence: number;
  caseId?: string;
  metricTimeMs: number;
  heavyCount: number;
  lightCount: number;
  emptyCount: number;
  initialHeavyCount: number;
  initialLightCount: number;
  heavyDrift: number;
  lightDrift: number;
  heavyCentroidX: number;
  heavyCentroidY: number;
  lightCentroidX: number;
  lightCentroidY: number;
  interfaceRowsPx: number[];
  interfaceRangeCells: number;
  mixedBandShare: number;
  changedCells: number;
  lightPixelArea: number;
  heavyPixelArea: number;
  simulationTime: number;
  dormant: boolean;
  finite: boolean;
  gravityInverted: boolean;
}

export interface LiquidRenderSnapshot {
  cols: number;
  rows: number;
  cellPx: number;
  gravityInverted: boolean;
  cells: Uint8Array;
  phase: Uint8Array;
  shade: Uint8Array;
}

export interface LiquidCAObserver {
  onEvent(event: LiquidCAEventPayload): void;
  onFrame(frame: LiquidCAFramePayload): void;
  onMetric(metric: LiquidCAMetricPayload): void;
}

export interface LiquidCARenderer {
  render(snapshot: LiquidRenderSnapshot, frame: LiquidCAFramePayload): void;
  onModelError(errorCode: string): void;
}

export function defaultLiquidCAConfig(): LiquidCAConfig {
  return {
    cols: 80,
    rows: 40,
    cellPx: 4,
    seed: 0,
    pSettle: 0.08,
    qMixBase: 0.30,
    pInertia: 0.20,
    shakeDecayRate: 1.8,
    maxCells: 24000
  };
}
