import { P013_COLUMN_PROJECTION } from './P013ParticleProjection';
import { P011PublishedRenderState } from '../physics/P011PublishedRenderState';
import { P013DualFieldAdapter, P013DualFieldUploadSlot } from './P013DualFieldAdapter';
import { P013DualFieldReceipt, P013DualFieldVisualParams } from './P013RenderTypes';
import { P013CompositionProfile, P013CompositionReceipt } from './P013RenderTypes';
// P-013 P0 v1: called only after the existing Canvas render, never owns a ticker.
import p013renderer from 'libp013renderer.so';
import systemDateTime from '@ohos.systemDateTime';
import { WaterSandSnapshot } from '../physics/WaterSandField';
import { P012FrameContext } from './P012RenderTypes';
import { P013FrameAdapter } from './P013FrameAdapter';
import { P013Mode, P013NativeRenderer, P013FrameInput, P013LightParams,
  P013TelemetryBatch, P013FrameResult, P013ControllerRecord, P013EvidenceBatch, P013TelemetryRecord, P013Stats } from './P013RenderTypes';
export function p013NowMs(): number {
  return systemDateTime.getUptime(systemDateTime.TimeType.ACTIVE, true) / 1000000;
}
/** Descriptive nearest-rank summaries; formal windows remain the tester's contract. */
export function p013Stats(values: number[], failCount: number): P013Stats {
  values.sort((a: number, b: number): number => a - b);
  const n = values.length;
  return {n: n, p50: n > 0 ? values[Math.ceil(n * 0.5) - 1] : null,
    p95: n > 0 ? values[Math.ceil(n * 0.95) - 1] : null,
    max: n > 0 ? values[n - 1] : null, failCount: failCount};
}
export class P013RenderController {
  readonly componentId: string = 'p013_lmat_surface';
  readonly light: P013LightParams = new P013LightParams();
  readonly frame: P013FrameInput = new P013FrameInput();
  readonly adapter: P013FrameAdapter = new P013FrameAdapter();
  mode: P013Mode = P013Mode.CANVAS_BASELINE;
  diagnosticVisible: boolean = false;
  failureTelemetry: P013TelemetryBatch | null = null;
  fallbackReason: string = '';
  packMs: number = 0;
  drawCallMs: number = 0;
  totalAddedCpuMs: number = 0;
  attemptCount: number = 0;
  swapSuccessCount: number = 0;
  clockResolutionNs: number = 0;
  private native: P013NativeRenderer | null = null;
  private frameId: number = 0;
  private sessionId: number = 0;
  private records: P013ControllerRecord[] = [];
  private recordHead: number = 0;
  private recordCount: number = 0;
  private droppedRecords: number = 0;
  private previousFrameTimeNs: number = 0;
  private nativeRecords: Array<P013TelemetryRecord | null> = [];
  private nativeHead: number = 0;
  private nativeCount: number = 0;
  private nativeQueueDropped: number = 0;
  private lastNativeDropped: number = 0;
  private summaryCpu: Float64Array = new Float64Array(256);
  private summaryCount: number = 0;
  private summaryFailures: number = 0;
  private summaryDropped: number = 0;
  private telemetryDrainMs: number = 0;
  private snapshotId: number = 0;
  private loaded: boolean = false;
  private active: boolean = true;
  private disposed: boolean = false;
  private lastSummaryMs: number = 0;
  private previousCallbackMs: number = 0;
  private maxAddedCpuMs: number = 0;
  private maxFrameIntervalMs: number = 0;
  private pendingMode: P013Mode | null = null;
  private lastResult: P013FrameResult | null = null;
  constructor() {
    for (let i = 0; i < 2048; i++) {
      this.records.push(new P013ControllerRecord());
      this.nativeRecords.push(null);
    }
    // Measured clock resolution is registered separately from the API's ns unit.
    let previous = p013NowMs();
    let minimum = Number.POSITIVE_INFINITY;
    for (let i = 0; i < 128; i++) {
      const current = p013NowMs();
      if (current > previous) {
        minimum = Math.min(minimum, (current - previous) * 1000000);
      }
      previous = current;
    }
    this.clockResolutionNs = Number.isFinite(minimum) ? minimum : 0;
  }
  private dualAdapter: P013DualFieldAdapter = new P013DualFieldAdapter();
  private dualSlots: P013DualFieldUploadSlot[] = [];
  private dualCursor: number = 0;
  private dualVisual: P013DualFieldVisualParams = new P013DualFieldVisualParams();
  lastPresentedCommit: number = 0;
  dualFailure: string = '';
  private lastDualLogMs: number = -1000;
  lastDualReceipt: P013DualFieldReceipt | null = null;
  submitDualFieldSurfacePx(stateSlot: P011PublishedRenderState, context: P012FrameContext,
    frameTimeNs: number, radiusPx: number): P013DualFieldReceipt | null {
    return this.submitDualField(stateSlot, context, 0, 0, frameTimeNs, 'SURFACE_PX_V1', radiusPx);
  }
  submitDualField(stateSlot: P011PublishedRenderState, context: P012FrameContext,
    width: number, height: number, frameTimeNs: number, geometrySpace: string = 'LOGICAL_V1', radiusPx: number = 1): P013DualFieldReceipt | null {
    const native = this.native;
    if (native === null || !this.loaded || !this.active || this.disposed) return null;
    try {
      let state = native.getState();
      if (state.state === 'WAIT_WINDOW') return null;
      if (this.frame.sessionId === 0) {
        const init = native.init(this.componentId);
        if (init.code !== 0) { this.dualFailure = `INIT_${init.code}`; return null; }
        if (this.sessionId !== init.sessionId) { this.frameId = 0; this.sessionId = init.sessionId; }
        this.frame.sessionId = init.sessionId; state = native.getState();
      }
      const pixels = geometrySpace === 'SURFACE_PX_V1';
      if (geometrySpace !== 'LOGICAL_V1' && !pixels) { this.dualFailure='GEOMETRY_VERSION'; return null; }
      if (pixels) {
        const prepared = native.prepareDualFieldSurface(this.sessionId, state.surfaceEpoch);
        if (prepared.code !== 0 || prepared.sizeSource !== 'EGL_QUERY_SURFACE' ||
          prepared.geometrySpace !== 'SURFACE_PX_V1' || !Number.isInteger(prepared.widthPx) ||
          !Number.isInteger(prepared.heightPx) || prepared.widthPx <= 0 || prepared.heightPx <= 0) {
          this.dualFailure = `WAIT_SURFACE_${prepared.code}_${prepared.errorStage}`; return null;
        }
        state = native.getState();
        if (state.surfaceEpoch !== prepared.surfaceEpoch || state.contextEpoch !== prepared.contextEpoch ||
          state.widthPx !== prepared.widthPx || state.heightPx !== prepared.heightPx) {
          this.dualFailure='STALE_PREPARED_SURFACE'; return null;
        }
        if (!Number.isFinite(radiusPx) || radiusPx <= 0) { this.dualFailure='RADIUS_PX'; return null; }
      }
      if (this.dualSlots.length === 0 || this.dualSlots[0].capacity !== stateSlot.particleCapacity) {
        this.dualSlots = [new P013DualFieldUploadSlot(stateSlot.particleCapacity), new P013DualFieldUploadSlot(stateSlot.particleCapacity)];
        this.dualCursor = 0;
      }
      const slot = this.dualSlots[this.dualCursor];
      const packed = this.dualAdapter.packInto(stateSlot, context, slot, pixels ? P013_COLUMN_PROJECTION : 'RAW_F32_V1');
      if (packed !== 'OK') {
        this.dualFailure = packed;
        const now = p013NowMs();
        if (now - this.lastDualLogMs >= 1000) { this.lastDualLogMs=now;
          console.info(`WS_EVENT p013_dual_rejected ${JSON.stringify({reason:packed,runId:stateSlot.runId,stateEpoch:stateSlot.stateEpoch,physicsCommitId:stateSlot.physicsCommitId,particle:this.dualAdapter.lastRejected})}`); }
        return null;
      }
      const f = slot.frame;
      const sw = state.widthPx > 0 ? state.widthPx : Math.round(width);
      const sh = state.heightPx > 0 ? state.heightPx : Math.round(height);
      const sx = pixels ? 1 : sw / width, sy = pixels ? 1 : sh / height;
      if (!Number.isFinite(sx) || !Number.isFinite(sy) || sx <= 0 || sy <= 0 || Math.abs(sx - sy) > 1e-6) {
        this.dualFailure = 'NONUNIFORM_SCALE'; return null;
      }
      f.sessionId = this.sessionId; f.frameId = ++this.frameId; f.snapshotId = ++this.snapshotId;
      f.surfaceEpoch = state.surfaceEpoch; f.contextEpoch = state.contextEpoch; f.frameTimeNs = frameTimeNs;
      f.surfaceWidthPx = sw; f.surfaceHeightPx = sh;
      f.boxXPx = pixels ? 0 : context.boxX * sx; f.boxYPx = pixels ? 0 : context.boxY * sy;
      f.boxWidthPx = pixels ? sw : context.boxW * sx; f.boxHeightPx = pixels ? sh : context.boxH * sy;
      f.diagnosticEnabled = true;
      this.dualVisual.radiusPx = pixels ? radiusPx : sx;
      const result = native.drawDualFieldFrame(f, slot.tdata, slot.particleXY, slot.particleMeta, this.dualVisual);
      this.lastDualReceipt = result;
      const now = p013NowMs();
      if (now - this.lastDualLogMs >= 1000 || result.code >= 200) {
        this.lastDualLogMs = now;
        console.info(`WS_EVENT p013_dual_frame ${JSON.stringify(result)}`);
        console.info(`WS_EVENT p013_dual_geometry ${JSON.stringify({geometrySpace:geometrySpace,surfaceWidthPx:sw,surfaceHeightPx:sh,surfaceEpoch:f.surfaceEpoch,contextEpoch:f.contextEpoch,physicsCommitId:f.physicsCommitId,projection:slot.projection})}`);
      }
      this.dualCursor = 1 - this.dualCursor;
      this.diagnosticVisible = result.code === 0 && result.swapSucceeded;
      this.dualFailure = this.diagnosticVisible ? '' : result.errorStage;
      if (this.diagnosticVisible) this.lastPresentedCommit = stateSlot.physicsCommitId;
      return result;
    } catch (error) { this.dualFailure = 'DUAL_EXCEPTION'; this.diagnosticVisible = false; return null; }
  }
  requestComposition(profile: P013CompositionProfile): P013CompositionReceipt | null {
    const native = this.native;
    if (!this.loaded || native === null) return null;
    try {
      const result = native.requestComposition(profile);
      if (result.code !== 0) this.disableComposition(result.errorStage);
      return result;
    } catch (error) { this.disableComposition('COMPOSITION_EXCEPTION'); return null; }
  }
  disableComposition(reason: string): void { this.forceBaseline(reason); }
  submitPreparedFrame(frameToken: string, snapshot: WaterSandSnapshot, context: P012FrameContext,
    width: number, height: number, frameTimeNs: number, controlCostMs: number = 0): P013FrameResult | null {
    // No second physics capture: caller owns the one prepared snapshot/token.
    if (frameToken !== `${context.fieldEpoch}:${context.frameSerial}:${snapshot.physicsCommitId ?? 0}`) {
      this.disableComposition('PREPARED_TOKEN_MISMATCH'); return null;
    }
    this.submit(snapshot, context, width, height, frameTimeNs, controlCostMs);
    return this.lastResult;
  }
  isOverlay(): boolean {
    return this.mode === P013Mode.GLES_LMAT_DIAG_OFF || this.mode === P013Mode.GLES_LMAT_DIAG_ON;
  }
  setMode(mode: P013Mode): void {
    if (this.isOverlay() && mode !== this.mode) {
      // ON->OFF/baseline first submits a transparent frame on the same ticker.
      this.pendingMode = mode;
    } else {
      this.mode = mode;
      this.fallbackReason = '';
    }
  }
  onLoad(): void {
    // XComponent must register its native object before the default export is accessed.
    this.native = p013renderer as P013NativeRenderer;
    this.diagnosticVisible = false;
    this.disposed = false;
    this.loaded = true;
    this.frame.sessionId = 0;
  }
  setActive(value: boolean): void {
    this.active = value;
    if (!value) {
      this.diagnosticVisible = false;
    }
    const native = this.native;
    if (this.loaded && !this.disposed && native !== null) {
      try {
        const result = native.setActive(value);
        if (result.code !== 0) {
          this.fail(`ACTIVE_${result.code}`);
        }
      } catch (error) {
        this.fail('ACTIVE_EXCEPTION');
      }
    }
  }
  release(): void {
    const native = this.native;
    if (this.loaded && !this.disposed && native !== null) {
      try {
        this.collectNative();
      } catch (error) {
        // Preserve the last captured fault even when allocation of a final drain fails.
      }
      try {
        native.release();
      } catch (error) {
        console.error('[P013] release failed; component removal remains mandatory');
      }
    }
    this.diagnosticVisible = false;
    this.loaded = false;
    this.native = null;
    this.disposed = true;
    this.frame.sessionId = 0;
    this.pendingMode = null;
  }
  forceBaseline(reason: string): void {
    this.mode = P013Mode.CANVAS_BASELINE;
    this.pendingMode = null;
    this.fallbackReason = reason;
    this.release();
  }
  private fail(reason: string): void {
    try {
      this.failureTelemetry = this.loaded ? this.collectNative() : null;
    } catch (error) {
      this.failureTelemetry = null;
    }
    this.fallbackReason = reason;
    this.mode = P013Mode.CANVAS_BASELINE;
    this.pendingMode = null;
    this.release();
    console.error(`[P013] fallback=${reason}`);
  }
  drainTelemetry(): P013TelemetryBatch {
    const native = this.native;
    return native === null ? {records: [], droppedTelemetry: 0, telemetryLiveCount: 0} : native.drainTelemetry();
  }
  private collectNative(): P013TelemetryBatch {
    const start = p013NowMs();
    const batch = this.drainTelemetry();
    this.lastNativeDropped = batch.droppedTelemetry;
    for (let i = 0; i < batch.records.length; i++) {
      if (this.nativeCount === 2048) {
        this.nativeRecords[this.nativeHead] = null;
        this.nativeHead = (this.nativeHead + 1) % 2048;
        this.nativeCount--;
        this.nativeQueueDropped++;
      }
      this.nativeRecords[(this.nativeHead + this.nativeCount) % 2048] = batch.records[i];
      this.nativeCount++;
    }
    this.telemetryDrainMs = p013NowMs() - start;
    return batch;
  }
  drainEvidence(): P013EvidenceBatch {
    const start = p013NowMs();
    this.collectNative();
    const nativeRows: P013TelemetryRecord[] = [];
    for (let i = 0; i < this.nativeCount; i++) {
      const index = (this.nativeHead + i) % 2048;
      const row = this.nativeRecords[index];
      if (row !== null) {
        nativeRows.push(row);
      }
      this.nativeRecords[index] = null;
    }
    this.nativeHead = 0;
    this.nativeCount = 0;
    const nativeBatch: P013TelemetryBatch = {records: nativeRows,
      droppedTelemetry: this.lastNativeDropped + this.nativeQueueDropped,
      telemetryLiveCount: nativeRows.length};
    const records: P013ControllerRecord[] = [];
    for (let i = 0; i < this.recordCount; i++) {
      records.push(this.records[(this.recordHead + i) % 2048].copy());
    }
    this.recordHead = 0;
    this.recordCount = 0;
    return {native: nativeBatch, controller: records,
      droppedControllerTelemetry: this.droppedRecords, telemetryDrainMs: p013NowMs() - start};
  }
  private recordAttempt(start: number, code: number, controlCostMs: number): void {
    if (this.recordCount === 2048) {
      this.recordHead = (this.recordHead + 1) % 2048;
      this.recordCount--;
      this.droppedRecords++;
    }
    const row = this.records[(this.recordHead + this.recordCount) % 2048];
    this.recordCount++;
    row.sessionId = this.frame.sessionId;
    row.frameId = this.frame.frameId;
    row.snapshotId = this.frame.snapshotId;
    row.code = code;
    row.packMs = this.packMs;
    row.profileBuildMs = this.packMs;
    row.drawCallMs = this.drawCallMs;
    row.totalAddedCpuMs = p013NowMs() - start + controlCostMs;
    row.callbackStartIntervalMs = this.previousCallbackMs > 0 ? start - this.previousCallbackMs : 0;
    row.frameIntervalMs = this.previousFrameTimeNs > 0 ? (this.frame.frameTimeNs - this.previousFrameTimeNs) / 1000000 : 0;
    row.mode = this.frame.diagnosticEnabled ? P013Mode.GLES_LMAT_DIAG_ON : P013Mode.GLES_LMAT_DIAG_OFF;
    row.diagnosticVisible = code === 0;
    if (this.summaryCount < 256) {
      this.summaryCpu[this.summaryCount++] = row.totalAddedCpuMs;
    } else {
      this.summaryDropped++;
    }
    if (code !== 0) {
      this.summaryFailures++;
    }
    this.totalAddedCpuMs = row.totalAddedCpuMs;
    this.previousCallbackMs = start;
    this.previousFrameTimeNs = this.frame.frameTimeNs;
  }
  submit(s: WaterSandSnapshot, ctx: P012FrameContext, width: number, height: number,
    frameTimeNs: number, controlCostMs: number = 0): void {
    if (!this.isOverlay() || !this.loaded || !this.active || this.disposed) {
      return;
    }
    const native = this.native;
    if (native === null) {
      return;
    }
    const start = p013NowMs();
    try {
      let state = native.getState();
      if (state.state === 'WAIT_WINDOW') {
        return;
      }
      if (this.frame.sessionId === 0) {
        const init = native.init(this.componentId);
        if (init.code === 1) {
          return;
        }
        if (init.code !== 0) {
          this.fail(`INIT_${init.code}`);
          return;
        }
        if (this.sessionId !== init.sessionId) {
          this.frameId = 0;
          this.sessionId = init.sessionId;
        }
        this.frame.sessionId = init.sessionId;
        state = native.getState();
      }
      const packStart = p013NowMs();
      if (!this.adapter.pack(s)) {
        this.fail('INVALID_TDATA');
        return;
      }
      this.packMs = p013NowMs() - packStart;
      const f = this.frame;
      f.frameId = ++this.frameId;
      f.snapshotId = ++this.snapshotId;
      f.surfaceEpoch = state.surfaceEpoch;
      f.fieldEpoch = ctx.fieldEpoch;
      f.geometryEpoch = ctx.geometryEpoch;
      f.continuityEpoch = ctx.continuityEpoch;
      f.appearanceConfigRevision = ctx.configRevision;
      f.lightConfigRevision = 0;
      f.frameTimeNs = frameTimeNs;
      // Bootstrap dimensions are a proposal only: first native query may reject them.
      f.surfaceWidthPx = state.widthPx > 0 ? state.widthPx : Math.round(width);
      f.surfaceHeightPx = state.heightPx > 0 ? state.heightPx : Math.round(height);
      const scaleX = f.surfaceWidthPx / width;
      const scaleY = f.surfaceHeightPx / height;
      f.boxXPx = ctx.boxX * scaleX;
      f.boxYPx = ctx.boxY * scaleY;
      f.boxWidthPx = ctx.boxW * scaleX;
      f.boxHeightPx = ctx.boxH * scaleY;
      f.capacity = s.capacity;
      f.visualInverted = ctx.visualInverted;
      f.diagnosticEnabled = this.mode === P013Mode.GLES_LMAT_DIAG_ON && this.pendingMode === null;
      const callStart = p013NowMs();
      this.attemptCount++;
      const result = native.drawFrame(f, this.adapter.data, this.light);
      if (f.frameId === 1 || result.code === 100) console.info(`WS_EVENT p013_tdata_boundary ${JSON.stringify({
        frameId: f.frameId, length: this.adapter.data.length, byteLength: this.adapter.data.byteLength,
        byteOffset: this.adapter.data.byteOffset, nativeByteLength: result.nativeTypedByteLength,
        nativeByteOffset: result.nativeTypedByteOffset, nativeBufferBytes: result.nativeBufferByteLength, errorStage: result.errorStage })}`);
      this.drawCallMs = p013NowMs() - callStart;
      this.lastResult = result;
      this.recordAttempt(start, result.code, controlCostMs);
      if (result.code === 103 || result.code === 1) {
        this.diagnosticVisible = false;
        return;
      }
      if (result.code !== 0 || !result.swapSucceeded) {
        this.fail(`DRAW_${result.code}`);
        return;
      }
      this.swapSuccessCount++;
      this.diagnosticVisible = true;
      if (this.pendingMode !== null) {
        const next = this.pendingMode;
        this.pendingMode = null;
        this.mode = next;
        if (!this.isOverlay()) {
          this.release();
        }
      }
      this.maxAddedCpuMs = Math.max(this.maxAddedCpuMs, this.totalAddedCpuMs);
      this.maxFrameIntervalMs = Math.max(this.maxFrameIntervalMs,
        this.records[(this.recordHead + this.recordCount - 1) % 2048].callbackStartIntervalMs);
      if (start - this.lastSummaryMs >= 1000) {
        this.lastSummaryMs = start;
        const nativeBatch = this.collectNative();
        const cpuValues: number[] = [];
        for (let i = 0; i < this.summaryCount; i++) {
          cpuValues.push(this.summaryCpu[i]);
        }
        const onRows = nativeBatch.records.filter((r: P013TelemetryRecord): boolean => r.mode === 'GLES_LMAT_DIAG_ON');
        const failed = nativeBatch.records.filter((r: P013TelemetryRecord): boolean => r.code !== 0).length;
        const failedOn = onRows.filter((r: P013TelemetryRecord): boolean => r.code !== 0).length;
        console.info(`P013_FRAME ${JSON.stringify({schemaVersion: 1, sessionId: f.sessionId,
          frameId: f.frameId, snapshotId: f.snapshotId, mode: this.mode,
          packMs: this.packMs, profileBuildMs: this.packMs, profileCopyMs: 0, packedBytes: 2048,
          drawCallMs: this.drawCallMs, totalAddedCpuMs: this.totalAddedCpuMs,
          cpuSummary: p013Stats(cpuValues, this.summaryFailures), summaryDropped: this.summaryDropped,
          uploadOnSummary: p013Stats(onRows.map((r: P013TelemetryRecord): number => r.uploadPathMs), failedOn),
          nativeBridgeSummary: p013Stats(nativeBatch.records.map((r: P013TelemetryRecord): number => r.bridgeCpuMs), failed),
          swapSummary: p013Stats(nativeBatch.records.map((r: P013TelemetryRecord): number => r.swapMs), failed),
          nativeDroppedTelemetry: this.lastNativeDropped + this.nativeQueueDropped,
          droppedControllerTelemetry: this.droppedRecords, telemetryDrainMs: this.telemetryDrainMs,
          maxAddedCpuMs: this.maxAddedCpuMs, maxCallbackStartIntervalMs: this.maxFrameIntervalMs,
          attemptCount: this.attemptCount, swapSuccessCount: this.swapSuccessCount,
          diagnosticVisible: this.diagnosticVisible, fallbackReason: this.fallbackReason,
          arkClockName: 'systemDateTime.getUptime(ACTIVE,true)', arkClockResolutionNs: this.clockResolutionNs,
          presentedSource: 'UNAVAILABLE', presentCorrelationValid: false})}`);
        this.summaryCount = 0;
        this.summaryFailures = 0;
        this.summaryDropped = 0;
        this.maxAddedCpuMs = 0;
        this.maxFrameIntervalMs = 0;
      }
    } catch (error) {
      this.fail('BRIDGE_EXCEPTION');
    }
  }
}
