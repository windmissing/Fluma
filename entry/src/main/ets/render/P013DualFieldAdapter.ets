import { P011PublishedRenderState } from '../physics/P011PublishedRenderState';
import { P013FrameAdapter } from './P013FrameAdapter';
import { P013DualFieldFrame } from './P013RenderTypes';
import { P012FrameContext } from './P012RenderTypes';
export class P013DualFieldUploadSlot {
  readonly frame: P013DualFieldFrame = new P013DualFieldFrame();
  readonly tdata: Float32Array = new Float32Array(512);
  readonly particleXY: Float32Array; readonly particleMeta: Uint32Array;
  readonly capacity: number;
  constructor(capacity: number) {
    this.capacity = capacity;
    this.particleXY = new Float32Array(2 * capacity); this.particleMeta = new Uint32Array(2 * capacity);
  }
}
export class P013DualFieldAdapter {
  private readonly profile: P013FrameAdapter = new P013FrameAdapter();
  packInto(state: P011PublishedRenderState, context: P012FrameContext, out: P013DualFieldUploadSlot): string {
    const field = state.field; const pool = field.particleState;
    if (field.schemaVersion !== 4 || pool === undefined || pool === null) return 'SCHEMA_UNAVAILABLE';
    if (!state.particleRouteActive || state.routeId !== 'dual-field-particle-v1') return 'ROUTE_INACTIVE';
    if (!state.runId || !Number.isSafeInteger(state.stateEpoch) || state.stateEpoch < 1 ||
      !Number.isSafeInteger(state.physicsCommitId) || state.physicsCommitId < 1 ||
      field.physicsCommitId !== state.physicsCommitId || context.physicsCommitId !== state.physicsCommitId)
      return 'PUBLICATION_MISMATCH';
    if (pool.nmax !== out.capacity || state.particleCapacity !== pool.nmax || pool.liveCount !== state.liveCount ||
      pool.cols !== 128 || pool.liveCount < 0 || pool.liveCount > pool.nmax) return 'CAPACITY_MISMATCH';
    let count = 0;
    for (let i = 0; i < pool.nmax; i++) {
      if (pool.pState[i] === 0) continue;
      if ((pool.pState[i] !== 1 && pool.pState[i] !== 2) || pool.pPhase[i] > 1 ||
        !Number.isFinite(pool.pMass[i]) || pool.pMass[i] <= 0 ||
        !Number.isFinite(Math.fround(pool.pX[i])) || !Number.isFinite(Math.fround(pool.pY[i])) ||
        pool.pX[i] < 0 || pool.pX[i] >= 128 || pool.pY[i] < 0 || pool.pY[i] > field.capacity ||
        !Number.isInteger(pool.pId[i]) || pool.pId[i] < 1) return 'INVALID_PARTICLE';
      for (let j = 0; j < i; j++) if (pool.pState[j] !== 0 && pool.pId[j] === pool.pId[i]) return 'DUPLICATE_ID';
      count++;
    }
    if (count !== pool.liveCount) return 'LIVE_COUNT_MISMATCH';
    if (!this.profile.pack(field)) return 'INVALID_FIELD';
    out.tdata.set(this.profile.data); count = 0;
    for (let i = 0; i < pool.nmax; i++) {
      if (pool.pState[i] === 0) continue;
      out.particleXY[2 * count] = pool.pX[i]; out.particleXY[2 * count + 1] = pool.pY[i];
      out.particleMeta[2 * count] = pool.pPhase[i] | (pool.pState[i] << 8);
      out.particleMeta[2 * count + 1] = pool.pId[i]; count++;
    }
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
