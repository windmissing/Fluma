import worker, { MessageEvents } from '@ohos.worker';
import { LiquidCAField } from '../physics/LiquidCAField';
import { LiquidCAConfig, LiquidCAEventPayload, LiquidCAFramePayload,
  LiquidCAMetricPayload, LiquidInputSource, LiquidRenderSnapshot } from '../physics/LiquidCATypes';

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

interface SnapshotBuffer {
  id: number;
  cells: Uint8Array;
  phase: Uint8Array;
  shade: Uint8Array;
}

const workerPort = worker.workerPort;
const SNAPSHOT_BUFFER_COUNT = 3;
let field: LiquidCAField | null = null;
let lastMetricSimulationTime = -1;
let nextSnapshotId = 1;
let freeSnapshotBuffers: SnapshotBuffer[] = [];

function post(response: WorkerResponse, transfer?: ArrayBuffer[]): boolean {
  try {
    if (transfer === undefined || transfer.length === 0) {
      workerPort.postMessage(response);
    } else {
      workerPort.postMessage(response, transfer);
    }
    return true;
  } catch (error) {
    try {
      workerPort.close();
    } catch (closeError) {
    }
    return false;
  }
}

function resetSnapshotBuffers(cellCount: number): void {
  freeSnapshotBuffers = [];
  for (let index = 0; index < SNAPSHOT_BUFFER_COUNT; index++) {
    freeSnapshotBuffers.push({
      id: nextSnapshotId++,
      cells: new Uint8Array(cellCount),
      phase: new Uint8Array(cellCount),
      shade: new Uint8Array(cellCount)
    });
  }
}

function acquireSnapshotBuffer(): SnapshotBuffer | null {
  if (freeSnapshotBuffers.length === 0) {
    return null;
  }
  const buffer = freeSnapshotBuffers[freeSnapshotBuffers.length - 1];
  freeSnapshotBuffers.pop();
  return buffer;
}

function returnSnapshotBuffer(request: WorkerRequest): void {
  if (request.snapshotId === undefined || request.cells === undefined ||
    request.phase === undefined || request.shade === undefined) {
    return;
  }
  freeSnapshotBuffers.push({
    id: request.snapshotId,
    cells: new Uint8Array(request.cells.buffer),
    phase: new Uint8Array(request.phase.buffer),
    shade: new Uint8Array(request.shade.buffer)
  });
}

function snapshotFromBuffer(buffer: SnapshotBuffer): LiquidRenderSnapshot {
  return {
    cols: field === null ? 0 : field.gridCellCount,
    rows: field === null ? 0 : 0,
    cellPx: 0,
    gravityInverted: false,
    cells: buffer.cells,
    phase: buffer.phase,
    shade: buffer.shade
  };
}

function frameResponse(sequence: number, deltaSeconds: number, frameTimeMs: number): void {
  if (field === null) {
    post({ type: 'error', errorCode: 'MODEL_NOT_INITIALIZED',
      message: 'LiquidCA model is not initialized', sequence: sequence });
    return;
  }
  const snapshotBuffer = acquireSnapshotBuffer();
  if (snapshotBuffer === null) {
    const frame = field.stepFrame(0, frameTimeMs, sequence);
    frame.valid = false;
    frame.clippedTimeMs = frameTimeMs;
    post({ type: 'frame', frame: frame, sequence: sequence });
    post({ type: 'error', errorCode: 'SNAPSHOT_BACKPRESSURE',
      message: 'All snapshot buffers are busy', sequence: sequence });
    return;
  }

  const physicsStart = Date.now();
  const frame = field.stepFrame(deltaSeconds, frameTimeMs, sequence);
  frame.physicsTimeMs = Date.now() - physicsStart;
  const snapshotStart = Date.now();
  const snapshot = snapshotFromBuffer(snapshotBuffer);
  field.copyRenderState(snapshot);
  frame.snapshotTimeMs = Date.now() - snapshotStart;
  post({
    type: 'frame',
    frame: frame,
    snapshot: snapshot,
    snapshotId: snapshotBuffer.id,
    sequence: sequence
  }, [snapshotBuffer.cells.buffer, snapshotBuffer.phase.buffer, snapshotBuffer.shade.buffer]);

  const metricStart = Date.now();
  const metric = field.metrics();
  metric.metricTimeMs = Date.now() - metricStart;
  if (metric.simulationTime - lastMetricSimulationTime >= 0.25) {
    lastMetricSimulationTime = metric.simulationTime;
    post({ type: 'metric', metric: metric, sequence: sequence });
  }
}

workerPort.onmessage = (event: MessageEvents): void => {
  const request = event.data as WorkerRequest;
  const sequence = request.sequence === undefined ? 0 : request.sequence;
  if (request.type === 'init') {
    const config = request.config;
    if (config === undefined) {
      post({ type: 'error', errorCode: 'INVALID_CONFIG', message: 'Missing config', sequence: sequence });
      return;
    }
    lastMetricSimulationTime = -1;
    field = new LiquidCAField(config);
    const init = field.initialize(request.eventTimeMs === undefined ? 0 : request.eventTimeMs);
    resetSnapshotBuffers(field.gridCellCount);
    post({ type: 'event', event: init, sequence: sequence });
    return;
  }
  if (request.type === 'return') {
    returnSnapshotBuffer(request);
    return;
  }
  if (request.type === 'reset') {
    if (field === null) {
      post({ type: 'error', errorCode: 'MODEL_NOT_INITIALIZED',
        message: 'LiquidCA model is not initialized', sequence: sequence });
      return;
    }
    const seed = request.seed === undefined ? 0 : request.seed;
    const eventTimeMs = request.eventTimeMs === undefined ? 0 : request.eventTimeMs;
    const source = request.source === undefined ? LiquidInputSource.WANT : request.source;
    const resetEvent = field.reset(seed, eventTimeMs);
    post({ type: 'event', event: resetEvent, sequence: sequence });
    return;
  }
  if (request.type === 'shake') {
    if (field === null) {
      post({ type: 'error', errorCode: 'MODEL_NOT_INITIALIZED',
        message: 'LiquidCA model is not initialized', sequence: sequence });
      return;
    }
    const intensity = request.intensity === undefined ? 0 : request.intensity;
    const eventTimeMs = request.eventTimeMs === undefined ? 0 : request.eventTimeMs;
    const source = request.source === undefined ? LiquidInputSource.WANT : request.source;
    post({ type: 'event', event: field.setShake(intensity, eventTimeMs, source), sequence: sequence });
    return;
  }
  if (request.type === 'invert') {
    if (field === null) {
      post({ type: 'error', errorCode: 'MODEL_NOT_INITIALIZED',
        message: 'LiquidCA model is not initialized', sequence: sequence });
      return;
    }
    const enabled = request.enabled === true;
    const eventTimeMs = request.eventTimeMs === undefined ? 0 : request.eventTimeMs;
    const source = request.source === undefined ? LiquidInputSource.WANT : request.source;
    post({ type: 'event', event: field.invert180(enabled, eventTimeMs, source), sequence: sequence });
    return;
  }
  if (request.type === 'frame') {
    frameResponse(sequence,
      request.deltaSeconds === undefined ? 0 : request.deltaSeconds,
      request.frameTimeMs === undefined ? 0 : request.frameTimeMs);
    return;
  }
  if (request.type === 'stop') {
    field = null;
    freeSnapshotBuffers = [];
    post({ type: 'stopped', sequence: sequence });
  }
};

workerPort.onmessageerror = (): void => {
  post({ type: 'error', errorCode: 'WORKER_MESSAGE_ERROR', message: 'Worker message deserialization failed' });
};
