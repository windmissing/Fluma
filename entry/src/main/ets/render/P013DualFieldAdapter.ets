import { P013_COLUMN_PROJECTION, p013ProjectX, p013ProjectY, P013ProjectionDiagnostic } from './P013ParticleProjection';
import { P011PublishedRenderState } from '../physics/P011PublishedRenderState';
import { P013FrameAdapter } from './P013FrameAdapter';
import { P013DualFieldFrame } from './P013RenderTypes';
import { P012FrameContext } from './P012RenderTypes';
export class P013DualFieldUploadSlot {
  readonly frame: P013DualFieldFrame = new P013DualFieldFrame();
  readonly tdata: Float32Array = new Float32Array(512);
  readonly particleXY: Float32Array; readonly particleMeta: Uint32Array;
  readonly capacity: number;
  projection: P013ProjectionDiagnostic = new P013ProjectionDiagnostic();
  constructor(capacity: number) {
    this.capacity = capacity;
    this.particleXY = new Float32Array(2 * capacity); this.particleMeta = new Uint32Array(2 * capacity);
  }
}
export class P013ParticleRejection {
  slot: number = -1; id: number = 0; state: number = 0; phase: number = 0;
  mass: number = 0; x64: number = 0; y64: number = 0; xF32: number = 0; yF32: number = 0;
}
export class P013DualFieldAdapter {
  lastRejected: P013ParticleRejection | null = null;
  private readonly profile: P013FrameAdapter = new P013FrameAdapter();
  packInto(state: P011PublishedRenderState, context: P012FrameContext, out: P013DualFieldUploadSlot, projectionRevision: string = 'RAW_F32_V1'): string {
    if (projectionRevision !== 'RAW_F32_V1' && projectionRevision !== P013_COLUMN_PROJECTION) return 'PROJECTION_VERSION';
    const project = projectionRevision === P013_COLUMN_PROJECTION;
    const diagnostic = new P013ProjectionDiagnostic(); diagnostic.revision = projectionRevision;
    const field = state.field; const pool = field.particleState;
    if (field.schemaVersion !== 4 || pool === undefined || pool === null) return 'SCHEMA_UNAVAILABLE';
    if (!state.particleRouteActive || state.routeId !== 'dual-field-particle-v1') return 'ROUTE_INACTIVE';
    if (!state.runId || !Number.isSafeInteger(state.stateEpoch) || state.stateEpoch < 1 ||
      !Number.isSafeInteger(state.physicsCommitId) || state.physicsCommitId < 1 ||
      field.physicsCommitId !== state.physicsCommitId || context.physicsCommitId !== state.physicsCommitId)
      return 'PUBLICATION_MISMATCH';
    if (pool.nmax !== out.capacity || state.particleCapacity !== pool.nmax || pool.liveCount !== state.liveCount ||
      pool.cols !== 128 || pool.liveCount < 0 || pool.liveCount > pool.nmax) return 'CAPACITY_MISMATCH';
    this.lastRejected = null;
    let count = 0;
    for (let i = 0; i < pool.nmax; i++) {
      if (pool.pState[i] === 0) continue;
      const rejected = new P013ParticleRejection(); rejected.slot=i; rejected.id=pool.pId[i];
      rejected.state=pool.pState[i]; rejected.phase=pool.pPhase[i]; rejected.mass=pool.pMass[i];
      rejected.x64=pool.pX[i]; rejected.y64=pool.pY[i];
      rejected.xF32=Math.fround(pool.pX[i]); rejected.yF32=Math.fround(pool.pY[i]);
      this.lastRejected=rejected;
      // Strict source-domain check precedes projection. Never repair illegal physics.
      if (!Number.isFinite(pool.pX[i]) || pool.pX[i] < 0 || pool.pX[i] >= 128 ||
        !Number.isFinite(pool.pY[i]) || pool.pY[i] < 0 || pool.pY[i] > field.capacity) return 'INVALID_PARTICLE';
      const qx = project ? p013ProjectX(pool.pX[i]) : Math.fround(pool.pX[i]);
      const qy = project ? p013ProjectY(pool.pY[i], field.capacity) : Math.fround(pool.pY[i]);
      if ((pool.pState[i] !== 1 && pool.pState[i] !== 2) || pool.pPhase[i] > 1 ||
        !Number.isFinite(pool.pMass[i]) || pool.pMass[i] <= 0 ||
        !Number.isFinite(qx) || !Number.isFinite(qy) ||
        qx < 0 || qx >= 128 || qy < 0 || qy > field.capacity ||
        !Number.isInteger(pool.pId[i]) || pool.pId[i] < 1 || pool.pId[i] > 0xffffffff) return 'INVALID_PARTICLE';
      for (let j = 0; j < i; j++) if (pool.pState[j] !== 0 && pool.pId[j] === pool.pId[i]) return 'DUPLICATE_ID';
      diagnostic.maxXError = Math.max(diagnostic.maxXError, Math.abs(qx - pool.pX[i]));
      diagnostic.maxYError = Math.max(diagnostic.maxYError, Math.abs(qy - pool.pY[i]));
      if (qx !== Math.fround(pool.pX[i]) || qy !== Math.fround(pool.pY[i])) {
        diagnostic.correctedCount++;
        if (diagnostic.firstSlot < 0) { diagnostic.firstSlot=i; diagnostic.sourceX=pool.pX[i];
          diagnostic.roundedX=Math.fround(pool.pX[i]); diagnostic.projectedX=qx; }
      }
      count++;
    }
    this.lastRejected = null;
    if (count !== pool.liveCount) return 'LIVE_COUNT_MISMATCH';
    if (!this.profile.pack(field)) return 'INVALID_FIELD';
    out.tdata.set(this.profile.data); count = 0;
    for (let i = 0; i < pool.nmax; i++) {
      if (pool.pState[i] === 0) continue;
      out.particleXY[2 * count] = project ? p013ProjectX(pool.pX[i]) : pool.pX[i];
      out.particleXY[2 * count + 1] = project ? p013ProjectY(pool.pY[i], field.capacity) : pool.pY[i];
      out.particleMeta[2 * count] = pool.pPhase[i] | (pool.pState[i] << 8);
      out.particleMeta[2 * count + 1] = pool.pId[i]; count++;
    }
    out.projection = diagnostic;
    const f = out.frame;
    f.runId = state.runId; f.stateEpoch = state.stateEpoch; f.physicsCommitId = state.physicsCommitId;
    f.particleCount = count; f.particleCapacity = pool.nmax;
    f.particleLayerEnabled = context.particleLayerEnabled !== false;
    f.capacity = field.capacity; f.fieldEpoch = context.fieldEpoch; f.geometryEpoch = context.geometryEpoch;
    f.continuityEpoch = context.continuityEpoch; f.appearanceConfigRevision = context.configRevision;
    f.visualInverted = context.visualInverted;
    return 'OK';
  }
}
