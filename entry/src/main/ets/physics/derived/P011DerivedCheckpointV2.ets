import { P011ParticleParams } from '../P011ParticleField';
import { defaultP011ParticleParams } from '../P011ParticleM2Field';
import { WaterSandSnapshot } from '../WaterSandField';
import { createDerivedPayloadV2, copyDerivedPayloadV2 } from './P011DerivedSnapshotV2';
import { DERIVED_CHECKPOINT_V2, DERIVED_POLICY_V2, DERIVED_CACHE_V2, DERIVED_ENVELOPE_V2 } from './P011DerivedTypesV2';
export class DerivedCheckpointV2 {
    readonly backend='CPU_ORIGINAL_F64';readonly writerRegistry='T216_DRY_WRITERS_V2';readonly particleParams:P011ParticleParams=defaultP011ParticleParams();
    readonly revision = DERIVED_CHECKPOINT_V2;
    readonly numericPolicy = DERIVED_POLICY_V2;
    readonly cacheRevision = DERIVED_CACHE_V2;
    readonly envelopeRevision = DERIVED_ENVELOPE_V2;
    readonly authority: WaterSandSnapshot;
    readonly acc = new Uint16Array(256 * 70);
    readonly count = new Uint32Array(256);
    readonly dirty = new Uint8Array(128);
    readonly generations: Float64Array;
    readonly ids: Uint32Array;
    runId = '';
    stateEpoch = 0;
    commit = 0;
    derivedRevision = 0;
    replayStatus = 'QUIESCENT_COLD_COPY';
    constructor(nmax: number) {this.particleParams.nmax=nmax; this.authority = createDerivedPayloadV2(nmax); this.generations = new Float64Array(nmax); this.ids = new Uint32Array(nmax); }
    cloneCold(): DerivedCheckpointV2 {
        const p = this.authority.particleState;
        if (p === null || p === undefined)
            throw new Error('DERIVED_CHECKPOINT_POOL');
        const out = new DerivedCheckpointV2(p.nmax);
        copyDerivedPayloadV2(this.authority, out.authority);
        out.acc.set(this.acc);
        out.count.set(this.count);
        out.dirty.set(this.dirty);
        out.generations.set(this.generations);
        out.ids.set(this.ids);
        out.runId = this.runId;
        out.stateEpoch = this.stateEpoch;
        out.commit = this.commit;
        out.derivedRevision = this.derivedRevision;
        out.replayStatus = this.replayStatus;
        return out;
    }
}
