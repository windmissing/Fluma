import worker, { ErrorEvent, MessageEvents } from '@ohos.worker';
import { LiquidCAConfig, LiquidCAEventPayload, LiquidCAFramePayload,
  LiquidCAMetricPayload, LiquidInputSource, LiquidCARenderer, LiquidRenderSnapshot } from './LiquidCATypes';
import { LiquidCATelemetry } from './LiquidCATelemetry';

export interface LiquidCAWorkerPort {
  postMessage(message: Object, transfer?: ArrayBuffer[]): void;
  terminate(): void;
  onmessage?: (event: MessageEvents) => void;
  onerror?: (event: ErrorEvent) => void;
  onmessageerror?: (event: MessageEvents) => void;
  onexit?: (code: number) => void;
}

interface WorkerRequest {
  type: string;
  config?: LiquidCAConfig;
  seed?: number;
  intensity?: number;
  enabled?: boolean;
  eventTimeMs?: number;
  frameTimeMs?: number;
  deltaSeconds?: number;
  source?: LiquidInputSource;
  sequence?: number;
  snapshotId?: number;
  cells?: Uint8Array;
  phase?: Uint8Array;
  shade?: Uint8Array;
}

interface WorkerResponse {
  type: string;
  event?: LiquidCAEventPayload;
  frame?: LiquidCAFramePayload;
  metric?: LiquidCAMetricPayload;
  snapshot?: LiquidRenderSnapshot;
  snapshotId?: number;
  errorCode?: string;
  message?: string;
  sequence?: number;
}

export class LiquidCAController {
  private renderer: LiquidCARenderer;
  private telemetry: LiquidCATelemetry;
  private workerFactory: () => LiquidCAWorkerPort;
  private worker: LiquidCAWorkerPort | null = null;
  private sequence: number = 0;
  private frameSequence: number = 0;
  private pendingFrames: number = 0;
  private lastVsyncTimeMs: number = 0;
  private mergedBackpressureMs: number = 0;
  private frameTelemetryQueue: Map<number, LiquidCAFramePayload> = new Map<number, LiquidCAFramePayload>();
  private nextTelemetrySequence: number = 1;
  private startResolve: (() => void) | null = null;
  private startReject: ((error: Error) => void) | null = null;
  private stopPromise: Promise<void> | null = null;
  private stopResolve: (() => void) | null = null;
  private caseId: string = '';

  constructor(renderer: LiquidCARenderer, telemetry: LiquidCATelemetry,
    workerFactory?: () => LiquidCAWorkerPort) {
    this.renderer = renderer;
    this.telemetry = telemetry;
    this.workerFactory = workerFactory === undefined ?
      (): LiquidCAWorkerPort => new worker.ThreadWorker('entry/ets/workers/LiquidCAWorker.ets') : workerFactory;
  }

  async start(config: LiquidCAConfig): Promise<void> {
    await this.stop();
    this.pendingFrames = 0;
    this.frameSequence = 0;
    this.frameTelemetryQueue.clear();
    this.nextTelemetrySequence = 1;
    this.lastVsyncTimeMs = 0;
    this.mergedBackpressureMs = 0;
    return new Promise<void>((resolve, reject) => {
      try {
        this.worker = this.workerFactory();
      } catch (error) {
        reject(error);
        return;
      }
      this.startResolve = resolve;
      this.startReject = reject;
      this.worker.onmessage = (event: MessageEvents): void => {
        this.handleMessage(event.data as WorkerResponse);
      };
      this.worker.onerror = (error: ErrorEvent): void => {
        this.handleWorkerError('WORKER_INIT_FAILED', error.message);
      };
      this.worker.onmessageerror = (): void => {
        this.handleWorkerError('WORKER_MESSAGE_ERROR', 'Worker message deserialization failed');
      };
      this.worker.onexit = (code: number): void => {
        this.handleWorkerError('WORKER_EXITED', `LiquidCA worker exited unexpectedly (${code})`);
      };
      if (!this.post({ type: 'init', config: config, eventTimeMs: Date.now(), sequence: ++this.sequence })) {
        this.handleWorkerError('WORKER_POST_FAILED', 'Failed to post init message to LiquidCA worker');
      }
    });
  }

  stop(): Promise<void> {
    if (this.stopPromise !== null) {
      return this.stopPromise;
    }
    this.rejectStart('WORKER_STOPPED', 'WORKER_STOPPED');
    if (this.worker === null) {
      this.clearTransientState();
      return Promise.resolve();
    }
    this.stopPromise = new Promise<void>((resolve) => {
      this.stopResolve = resolve;
    });
    if (!this.post({ type: 'stop', sequence: ++this.sequence })) {
      this.terminateWorker();
      this.clearTransientState();
      this.resolveStop();
    }
    return this.stopPromise;
  }

  setCaseId(caseId: string): void {
    this.caseId = caseId;
  }

  onVsync(frameTimeMs: number): void {
    if (this.worker === null || this.stopPromise !== null) {
      return;
    }
    const deltaMs = this.lastVsyncTimeMs > 0 ? Math.max(0, frameTimeMs - this.lastVsyncTimeMs) : 0;
    this.lastVsyncTimeMs = frameTimeMs;
    if (this.pendingFrames >= 2) {
      this.mergedBackpressureMs += deltaMs;
      this.queueFrameTelemetry({
        sequence: ++this.frameSequence,
        caseId: this.caseId,
        frameTimeMs: deltaMs,
        physicsTimeMs: 0,
        snapshotTimeMs: 0,
        substeps: 0,
        clippedTimeMs: deltaMs,
        valid: false
      });
      return;
    }
    const mergedDeltaMs = deltaMs + this.mergedBackpressureMs;
    this.mergedBackpressureMs = 0;
    this.pendingFrames++;
    if (!this.post({
      type: 'frame',
      deltaSeconds: mergedDeltaMs / 1000,
      frameTimeMs: mergedDeltaMs,
      sequence: ++this.frameSequence
    })) {
      this.pendingFrames = Math.max(0, this.pendingFrames - 1);
      this.handleWorkerError('WORKER_POST_FAILED', 'Failed to post frame request to LiquidCA worker');
    }
  }

  setShake(intensity: number, source: LiquidInputSource): void {
    this.postCommand({ type: 'shake', intensity: intensity, eventTimeMs: Date.now(),
      source: source, sequence: ++this.sequence }, 'shake command');
  }

  setForceInvert(enabled: boolean, source: LiquidInputSource): void {
    this.postCommand({ type: 'invert', enabled: enabled, eventTimeMs: Date.now(),
      source: source, sequence: ++this.sequence }, 'invert command');
  }

  reset(seed: number, source: LiquidInputSource): void {
    this.postCommand({ type: 'reset', seed: seed, eventTimeMs: Date.now(),
      source: source, sequence: ++this.sequence }, 'reset command');
  }

  private handleMessage(response: WorkerResponse): void {
    if (response.type === 'stopped') {
      this.terminateWorker();
      this.clearTransientState();
      this.resolveStop();
      return;
    }
    if (response.type === 'event' && response.event !== undefined) {
      response.event.caseId = this.caseId;
      this.telemetry.onEvent(response.event);
      if (response.event.type === 'init' && this.startResolve !== null) {
        const resolve = this.startResolve;
        this.startResolve = null;
        this.startReject = null;
        resolve();
      } else if (response.event.type === 'model_error') {
        this.terminateWorker();
        this.clearTransientState();
        this.renderer.onModelError(response.event.errorCode);
        this.rejectStart(response.event.errorCode, response.event.errorCode);
        this.resolveStop();
      }
      return;
    }
    if (response.type === 'metric' && response.metric !== undefined) {
      response.metric.caseId = this.caseId;
      this.telemetry.onMetric(response.metric);
      return;
    }
    if (response.type === 'frame') {
      this.pendingFrames = Math.max(0, this.pendingFrames - 1);
      if (response.frame !== undefined) {
        response.frame.caseId = this.caseId;
        this.queueFrameTelemetry(response.frame);
      }
      if (response.frame !== undefined && response.snapshot !== undefined) {
        try {
          this.renderer.render(response.snapshot, response.frame);
        } finally {
          this.returnSnapshot(response.snapshotId, response.snapshot);
        }
      }
      return;
    }
    if (response.type === 'error') {
      this.handleWorkerError(response.errorCode === undefined ? 'WORKER_ERROR' : response.errorCode,
        response.message === undefined ? '' : response.message);
    }
  }

  private completeWorkerFailure(errorCode: string, message: string): void {
    this.terminateWorker();
    this.clearTransientState();
    this.renderer.onModelError(errorCode);
    this.rejectStart(errorCode, message);
    this.resolveStop();
  }

  private handleWorkerError(errorCode: string, message: string): void {
    this.telemetry.onEvent({
      type: 'model_error', sequence: ++this.sequence, eventTimeMs: Date.now(), caseId: this.caseId,
      boundary: true, source: LiquidInputSource.INTERNAL, forceInvert: false, shakeIntensity: 0,
      errorCode: errorCode, cols: 0, rows: 0, cellPx: 0, initialHeavy: 0,
      initialLight: 0, seed: 0, scaled: false
    });
    this.completeWorkerFailure(errorCode, message);
  }

  private rejectStart(errorCode: string, message: string): void {
    const reject = this.startReject;
    if (reject === null) {
      return;
    }
    this.startResolve = null;
    this.startReject = null;
    reject(new Error(message === '' ? errorCode : message));
  }

  private terminateWorker(): void {
    const workerRef = this.worker;
    if (workerRef !== null) {
      workerRef.onmessage = undefined;
      workerRef.onerror = undefined;
      workerRef.onmessageerror = undefined;
      workerRef.onexit = undefined;
      try {
        workerRef.terminate();
      } catch (error) {
      }
    }
    this.worker = null;
  }

  private clearTransientState(): void {
    this.pendingFrames = 0;
    this.frameTelemetryQueue.clear();
    this.nextTelemetrySequence = 1;
    this.lastVsyncTimeMs = 0;
    this.mergedBackpressureMs = 0;
  }

  private resolveStop(): void {
    const resolve = this.stopResolve;
    this.stopResolve = null;
    this.stopPromise = null;
    if (resolve !== null) {
      resolve();
    }
  }

  private returnSnapshot(snapshotId: number | undefined, snapshot: LiquidRenderSnapshot): void {
    if (snapshotId === undefined) {
      return;
    }
    this.post({
      type: 'return',
      snapshotId: snapshotId,
      cells: snapshot.cells,
      phase: snapshot.phase,
      shade: snapshot.shade
    }, [snapshot.cells.buffer, snapshot.phase.buffer, snapshot.shade.buffer]);
  }

  private queueFrameTelemetry(frame: LiquidCAFramePayload): void {
    if (frame.sequence < this.nextTelemetrySequence) {
      return;
    }
    this.frameTelemetryQueue.set(frame.sequence, frame);
    while (this.frameTelemetryQueue.has(this.nextTelemetrySequence)) {
      const nextFrame = this.frameTelemetryQueue.get(this.nextTelemetrySequence);
      if (nextFrame === undefined) {
        break;
      }
      this.frameTelemetryQueue.delete(this.nextTelemetrySequence);
      this.telemetry.onFrame(nextFrame);
      this.nextTelemetrySequence++;
    }
  }

  private post(request: WorkerRequest, transfer?: ArrayBuffer[]): boolean {
    if (this.worker === null) {
      return false;
    }
    if (transfer === undefined || transfer.length === 0) {
      try {
        this.worker.postMessage(request);
        return true;
      } catch (error) {
        return false;
      }
    } else {
      try {
        this.worker.postMessage(request, transfer);
        return true;
      } catch (error) {
        return false;
      }
    }
  }

  private postCommand(request: WorkerRequest, operation: string): void {
    if (this.worker === null || this.stopPromise !== null) {
      return;
    }
    if (!this.post(request)) {
      this.handleWorkerError('WORKER_POST_FAILED', `Failed to post ${operation} to LiquidCA worker`);
    }
  }
}
