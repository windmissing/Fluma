import { describe, it, expect } from '@ohos/hypium';
import { ErrorEvent, MessageEvents } from '@ohos.worker';
import { LiquidCAController, LiquidCAWorkerPort } from '../main/ets/physics/LiquidCAController';
import { LiquidCAField } from '../main/ets/physics/LiquidCAField';
import { LiquidCATelemetry } from '../main/ets/physics/LiquidCATelemetry';
import { defaultLiquidCAConfig, LiquidCAConfig, LiquidCAEventPayload, LiquidCAFramePayload,
  LiquidCARenderer, LiquidInputSource, LiquidRenderSnapshot } from '../main/ets/physics/LiquidCATypes';

function snapshotOf(field: LiquidCAField): LiquidRenderSnapshot {
  const snapshot: LiquidRenderSnapshot = {
    cols: 0,
    rows: 0,
    cellPx: 4,
    gravityInverted: false,
    cells: new Uint8Array(field.gridCellCount),
    phase: new Uint8Array(field.gridCellCount),
    shade: new Uint8Array(field.gridCellCount)
  };
  field.copyRenderState(snapshot);
  return snapshot;
}

function firstHeavyRow(snapshot: LiquidRenderSnapshot, col: number): number {
  for (let row = 0; row < snapshot.rows; row++) {
    const index = row * snapshot.cols + col;
    if (snapshot.cells[index] === 1 && snapshot.phase[index] === 0) {
      return row;
    }
  }
  return -1;
}

function runSeconds(field: LiquidCAField, seconds: number): void {
  const frames = Math.ceil(seconds * 120);
  for (let index = 0; index < frames; index++) {
    field.stepFrame(1 / 120, 1000 / 120, index + 1);
  }
}

function stableLiquidField(seed: number): LiquidCAField {
  const config = defaultLiquidCAConfig();
  config.cols = 40;
  config.rows = 40;
  config.seed = seed;
  const field = new LiquidCAField(config);
  runSeconds(field, 5);
  return field;
}

function configuredField(cols: number, rows: number, seed: number): LiquidCAField {
  const config = defaultLiquidCAConfig();
  config.cols = cols;
  config.rows = rows;
  config.seed = seed;
  return new LiquidCAField(config);
}

class RecordingRenderer implements LiquidCARenderer {
  errors: string[] = [];

  render(snapshot: LiquidRenderSnapshot, frame: LiquidCAFramePayload): void {
  }

  onModelError(errorCode: string): void {
    this.errors.push(errorCode);
  }
}

class RecordingTelemetry extends LiquidCATelemetry {
  frame: TelemetryFrameLog | null = null;

  protected emit(name: string, payload: object): void {
    if (name === 'LIQ_FRAME') {
      this.frame = payload as TelemetryFrameLog;
    }
  }
}

interface TelemetryFrameLog {
  caseId: string;
  snapshot_time_ms: number;
}

interface FakeWorkerRequest {
  type: string;
  config?: LiquidCAConfig;
  sequence?: number;
  frameTimeMs?: number;
}

interface FakeWorkerResponse {
  type: string;
  event?: LiquidCAEventPayload;
  frame?: LiquidCAFramePayload;
  snapshot?: LiquidRenderSnapshot;
  snapshotId?: number;
  sequence?: number;
}

class FakeWorker implements LiquidCAWorkerPort {
  holdStop: boolean = false;
  onmessage?: (event: MessageEvents) => void;
  onerror?: (event: ErrorEvent) => void;
  onmessageerror?: (event: MessageEvents) => void;
  onexit?: (code: number) => void;

  postMessage(message: Object, transfer?: ArrayBuffer[]): void {
    const request = message as FakeWorkerRequest;
    if (request.type === 'init' && request.config !== undefined) {
      const valid = request.config.maxCells > 0;
      const event: LiquidCAEventPayload = {
        type: valid ? 'init' : 'model_error', sequence: 1, eventTimeMs: 1,
        boundary: true, source: LiquidInputSource.INTERNAL, forceInvert: false,
        shakeIntensity: 0, errorCode: valid ? '' : 'GRID_TOO_LARGE',
        cols: request.config.cols, rows: request.config.rows, cellPx: request.config.cellPx,
        initialHeavy: 0, initialLight: 0, seed: request.config.seed, scaled: false
      };
      this.emit({ type: 'event', event: event, sequence: request.sequence });
      return;
    }
    if (request.type === 'frame') {
      const frame: LiquidCAFramePayload = {
        sequence: request.sequence === undefined ? 1 : request.sequence, caseId: '',
        frameTimeMs: request.frameTimeMs === undefined ? 0 : request.frameTimeMs,
        physicsTimeMs: 1, snapshotTimeMs: 1, substeps: 1, clippedTimeMs: 0, valid: true
      };
      const snapshot: LiquidRenderSnapshot = {
        cols: 1, rows: 1, cellPx: 1, gravityInverted: false,
        cells: new Uint8Array(1), phase: new Uint8Array(1), shade: new Uint8Array(1)
      };
      this.emit({ type: 'frame', frame: frame, snapshot: snapshot,
        snapshotId: 1, sequence: request.sequence });
      return;
    }
    if (request.type === 'stop' && !this.holdStop) {
      this.emit({ type: 'stopped', sequence: request.sequence });
    }
  }

  terminate(): void {
  }

  emitModelError(errorCode: string): void {
    const event: LiquidCAEventPayload = {
      type: 'model_error', sequence: 2, eventTimeMs: 2, boundary: true,
      source: LiquidInputSource.INTERNAL, forceInvert: false, shakeIntensity: 0,
      errorCode: errorCode, cols: 0, rows: 0, cellPx: 0, initialHeavy: 0,
      initialLight: 0, seed: 0, scaled: false
    };
    this.emit({ type: 'event', event: event, sequence: 2 });
  }

  private emit(response: FakeWorkerResponse): void {
    if (this.onmessage === undefined) {
      return;
    }
    const event: MessageEvents = { type: 'message', timeStamp: 0, data: response };
    this.onmessage(event);
  }
}

class FailingWorker implements LiquidCAWorkerPort {
  onmessage?: (event: MessageEvents) => void;
  onerror?: (event: ErrorEvent) => void;
  onmessageerror?: (event: MessageEvents) => void;
  onexit?: (code: number) => void;

  postMessage(message: Object, transfer?: ArrayBuffer[]): void {
    throw new Error('post failed');
  }

  terminate(): void {
  }
}

function invertedRearrangeSeconds(): number {
  const field = stableLiquidField(2);
  field.invert180(true, 5000, 1);
  let seconds = 0;
  while (seconds < 5) {
    field.stepFrame(1 / 120, 1000 / 120, 0);
    seconds += 1 / 120;
    const metric = field.metrics();
    if (metric.heavyCentroidY < metric.lightCentroidY && metric.interfaceRangeCells <= 2) {
      break;
    }
  }
  return seconds;
}

function uprightRearrangeSeconds(): number {
  const field = stableLiquidField(2);
  field.invert180(true, 5000, 1);
  runSeconds(field, 5);
  field.invert180(false, 10000, 1);
  let seconds = 0;
  while (seconds < 5) {
    field.stepFrame(1 / 120, 1000 / 120, 0);
    seconds += 1 / 120;
    const metric = field.metrics();
    if (metric.lightCentroidY < metric.heavyCentroidY && metric.interfaceRangeCells <= 2) {
      break;
    }
  }
  return seconds;
}

export default function liquidCAFieldTest() {
  describe('LiquidCAFieldTest', () => {
    it('fixedStepUsesProvidedRenderDeltaTime', 0, () => {
      const config = defaultLiquidCAConfig();
      config.cols = 12;
      config.rows = 12;
      const field = new LiquidCAField(config);
      const first = field.stepFrame(1 / 120, 1000 / 120, 1);
      expect(first.substeps).assertEqual(1);
      expect(first.frameTimeMs).assertEqual(1000 / 120);
      expect(first.clippedTimeMs).assertEqual(0);

      const second = field.stepFrame(0.02, 20, 2);
      expect(second.substeps).assertEqual(2);
      expect(second.clippedTimeMs).assertEqual(0);

      const third = field.stepFrame(1 / 120, 1000 / 120, 3);
      expect(third.substeps).assertEqual(1);
      expect(third.clippedTimeMs).assertEqual(0);
    });

    it('metricSemanticsUseInterfacePixelsAreaAndChangedCells', 0, () => {
      const config = defaultLiquidCAConfig();
      config.cols = 10;
      config.rows = 10;
      config.cellPx = 3;
      const field = new LiquidCAField(config);
      const metric = field.metrics();
      expect(metric.interfaceRowsPx.length).assertEqual(8);
      expect(metric.interfaceRowsPx[0]).assertEqual(15);
      expect(metric.interfaceRangeCells).assertEqual(0);
      expect(metric.lightPixelArea).assertEqual(metric.lightCount * 9);
      expect(metric.heavyPixelArea).assertEqual(metric.heavyCount * 9);
      expect(metric.changedCells).assertEqual(0);

      field.stepFrame(1 / 120, 1000 / 120, 1);
      const changedMetric = field.metrics();
      expect(changedMetric.changedCells >= 0).assertTrue();
      expect(changedMetric.heavyDrift).assertEqual(0);
      expect(changedMetric.lightDrift).assertEqual(0);
    });

    it('conservationAndDeterministicStep', 0, () => {
      const config = defaultLiquidCAConfig();
      config.cols = 16;
      config.rows = 16;
      config.seed = 2;
      const first = new LiquidCAField(config);
      const second = new LiquidCAField(config);
      const frame = first.stepFrame(1 / 60);
      second.stepFrame(1 / 60);
      const metric = first.metrics();
      expect(frame.substeps).assertEqual(2);
      expect(metric.heavyDrift).assertEqual(0);
      expect(metric.lightDrift).assertEqual(0);
      expect(metric.finite).assertTrue();
      const left = snapshotOf(first);
      const right = snapshotOf(second);
      for (let index = 0; index < left.cells.length; index++) {
        expect(left.cells[index]).assertEqual(right.cells[index]);
        expect(left.phase[index]).assertEqual(right.phase[index]);
      }
    });

    it('invertOnlyChangesGravity', 0, () => {
      const config = defaultLiquidCAConfig();
      config.cols = 12;
      config.rows = 12;
      const field = new LiquidCAField(config);
      const before = snapshotOf(field);
      const event = field.invert180(true, 123, 0);
      const after = snapshotOf(field);
      expect(event.type).assertEqual('invert');
      expect(event.boundary).assertTrue();
      expect(field.isGravityDown).assertFalse();
      for (let index = 0; index < before.cells.length; index++) {
        expect(before.cells[index]).assertEqual(after.cells[index]);
        expect(before.phase[index]).assertEqual(after.phase[index]);
      }
    });

    it('dormantAfterStableSubsteps', 0, () => {
      const config = defaultLiquidCAConfig();
      config.cols = 16;
      config.rows = 16;
      const field = new LiquidCAField(config);
      for (let index = 0; index < 240 && !field.isDormant; index++) {
        field.stepFrame(1 / 60);
      }
      expect(field.isDormant).assertTrue();
    });

    it('tc11DormantAfterStable', 0, () => {
      const field = configuredField(16, 16, 0);
      runSeconds(field, 5);
      expect(field.isDormant).assertTrue();
    });

    it('tc11ZeroPixelChangeAfterStable', 0, () => {
      const field = configuredField(16, 16, 0);
      runSeconds(field, 5);
      field.metrics();
      runSeconds(field, 1);
      const metric = field.metrics();
      expect(metric.changedCells).assertEqual(0);
    });

    it('tc11ConservationOrderAndInterface', 0, () => {
      const field = configuredField(16, 16, 0);
      runSeconds(field, 5);
      const metric = field.metrics();
      expect(metric.heavyDrift).assertEqual(0);
      expect(metric.lightDrift).assertEqual(0);
      expect(metric.lightCentroidY < metric.heavyCentroidY).assertTrue();
      expect(metric.interfaceRangeCells <= 2).assertTrue();
      expect(metric.finite).assertTrue();
    });

    it('tc12ShakeMixesThenRecovers', 0, () => {
      const config = defaultLiquidCAConfig();
      config.cols = 16;
      config.rows = 16;
      config.seed = 1;
      const field = new LiquidCAField(config);
      runSeconds(field, 5);
      const baseline = field.metrics().mixedBandShare;
      field.setShake(1, 5000, 1);
      runSeconds(field, 6);
      const mixed = field.metrics().mixedBandShare;
      field.setShake(0, 11000, 1);
      runSeconds(field, 4);
      const recovered = field.metrics();
      expect(mixed > baseline).assertTrue();
      expect(recovered.mixedBandShare < mixed).assertTrue();
      expect(recovered.lightCentroidY < recovered.heavyCentroidY).assertTrue();
      expect(recovered.heavyDrift).assertEqual(0);
      expect(recovered.lightDrift).assertEqual(0);
      expect(recovered.interfaceRangeCells <= 2).assertTrue();
    });

    it('tc13InvertedMinimumSeconds', 0, () => {
      expect(invertedRearrangeSeconds() >= 2).assertTrue();
    });

    it('tc13InvertedMaximumSeconds', 0, () => {
      expect(invertedRearrangeSeconds() <= 4).assertTrue();
    });

    it('tc13UprightMinimumSeconds', 0, () => {
      expect(uprightRearrangeSeconds() >= 2).assertTrue();
    });

    it('tc13UprightMaximumSeconds', 0, () => {
      expect(uprightRearrangeSeconds() <= 4).assertTrue();
    });

    it('tc13IsolatedHeavyAboveLightDoesNotPenetrate', 0, () => {
      const field = configuredField(40, 40, 2);
      const before = snapshotOf(field);
      field.invert180(true, 5000, 1);
      field.stepFrame(1 / 120, 1000 / 120, 0);
      const after = snapshotOf(field);
      const beforeRow = firstHeavyRow(before, 20);
      const afterRow = firstHeavyRow(after, 20);
      expect(afterRow).assertEqual(beforeRow - 2);
      expect(after.cells[afterRow * after.cols + 20]).assertEqual(1);
      expect(after.phase[afterRow * after.cols + 20]).assertEqual(0);
    });

    it('controllerInitInjectsCaseIdAndNonZeroSnapshotTime', 0, async () => {
      const renderer = new RecordingRenderer();
      const telemetry = new RecordingTelemetry(true);
      const fakeWorker = new FakeWorker();
      const controller = new LiquidCAController(renderer, telemetry,
        (): LiquidCAWorkerPort => fakeWorker);
      controller.setCaseId('TC-LIFE');
      await controller.start(defaultLiquidCAConfig());
      controller.onVsync(16);
      expect(telemetry.frame !== null).assertTrue();
      if (telemetry.frame !== null) {
        expect(telemetry.frame.caseId).assertEqual('TC-LIFE');
          expect(telemetry.frame.snapshot_time_ms > 0).assertTrue();
      }
      await controller.stop();
    });

    it('controllerInitFailureRejectsStartAndReportsError', 0, async () => {
      const renderer = new RecordingRenderer();
      const fakeWorker = new FakeWorker();
      const controller = new LiquidCAController(renderer, new LiquidCATelemetry(false),
        (): LiquidCAWorkerPort => fakeWorker);
      const config = defaultLiquidCAConfig();
      config.maxCells = 0;
      let rejected = false;
      try {
        await controller.start(config);
      } catch (error) {
        rejected = true;
      }
      expect(rejected).assertTrue();
      expect(renderer.errors.length > 0).assertTrue();
    });

    it('controllerInitPostFailureRejectsStart', 0, async () => {
      const renderer = new RecordingRenderer();
      const failingWorker = new FailingWorker();
      const controller = new LiquidCAController(renderer, new LiquidCATelemetry(false),
        (): LiquidCAWorkerPort => failingWorker);
      let rejected = false;
      try {
        await controller.start(defaultLiquidCAConfig());
      } catch (error) {
        rejected = true;
      }
      expect(rejected).assertTrue();
      expect(renderer.errors.length > 0).assertTrue();
    });

    it('controllerStopIsIdempotent', 0, async () => {
      const renderer = new RecordingRenderer();
      const fakeWorker = new FakeWorker();
      const controller = new LiquidCAController(renderer, new LiquidCATelemetry(false),
        (): LiquidCAWorkerPort => fakeWorker);
      await controller.start(defaultLiquidCAConfig());
      const first = controller.stop();
      const second = controller.stop();
      await Promise.all([first, second]);
    });

    it('controllerStopAndModelErrorRaceCompletes', 0, async () => {
      const renderer = new RecordingRenderer();
      const fakeWorker = new FakeWorker();
      const controller = new LiquidCAController(renderer, new LiquidCATelemetry(false),
        (): LiquidCAWorkerPort => fakeWorker);
      await controller.start(defaultLiquidCAConfig());
      fakeWorker.holdStop = true;
      const stopPromise = controller.stop();
      fakeWorker.emitModelError('MODEL_ERROR_RACE');
      await stopPromise;
      expect(renderer.errors.includes('MODEL_ERROR_RACE')).assertTrue();
    });

    it('controllerWorkerErrorCompletesAndReports', 0, async () => {
      const renderer = new RecordingRenderer();
      const fakeWorker = new FakeWorker();
      const controller = new LiquidCAController(renderer, new LiquidCATelemetry(false),
        (): LiquidCAWorkerPort => fakeWorker);
      await controller.start(defaultLiquidCAConfig());
      const error: ErrorEvent = {
        type: 'error', timeStamp: 0, message: 'worker error test', filename: '',
        lineno: 0, colno: 0, error: new Error('worker error test')
      };
      if (fakeWorker.onerror !== undefined) {
        fakeWorker.onerror(error);
      }
      expect(renderer.errors.includes('WORKER_INIT_FAILED')).assertTrue();
    });

    it('telemetryTestModeFalseDoesNotEmitFrame', 0, () => {
      const telemetry = new RecordingTelemetry(false);
      telemetry.onFrame({
        sequence: 1, caseId: 'TC-LIFE', frameTimeMs: 16, physicsTimeMs: 1,
        snapshotTimeMs: 1, substeps: 1, clippedTimeMs: 0, valid: true
      });
      expect(telemetry.frame === null).assertTrue();
    });
  });
}
