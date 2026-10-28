import { copyDerivedPayloadV2 } from './P011DerivedSnapshotV2';
import { P013SceneController } from '../../render/P013SceneController';
import { P013SceneReceipt } from '../../render/P013SceneTypes';
import { P012FrameContext, p012FactoryAppearance } from '../../render/P012RenderTypes';
import { DerivedFailureRawV2, DerivedWorkV2 } from './P011DerivedTupleBarrierV2';
import { DerivedCheckpointV2 } from './P011DerivedCheckpointV2';
import { createDerivedPayloadV2 } from './P011DerivedSnapshotV2';
import { WaterSandField, WaterSandStepResult } from '../WaterSandField';
import { defaultP011ParticleParams } from '../P011ParticleM2Field';
import { P051_DRY_REVISION, P051DryAdmissionRequest, P051DryAdmissionReceipt } from '../P051DryParticleAdmission';
import { P011PublishedRenderState } from '../P011PublishedRenderState';
import { DerivedFaultV2, DERIVED_POLICY_V2, DERIVED_CACHE_V2, DERIVED_PUBLICATION_V2 } from './P011DerivedTypesV2';
import { Exact70 } from './P011Exact70';
class DerivedOwnerPermitV2 {
}
const OWNER_PERMIT_V2 = new DerivedOwnerPermitV2();
export interface DerivedConfigV2 {
    isolated: boolean;
    numericPolicy: string;
    nmax: number;
    sourceId: number;
    backend: string;
    writerRegistry: string;
}
export interface DerivedStepResultV2 {
    ok: boolean;
    originalError: number | null;
    derivedFault: DerivedFaultV2 | null;
    physicalCommit: number;
    captureStatus: string;
    original: WaterSandStepResult | null;
}
export interface DerivedSourceResultV2 {
    ok: boolean;
    original: P051DryAdmissionReceipt | null;
    derivedFault: DerivedFaultV2 | null;
    physicalCommit: number;
    captureStatus: string;
}
// The field and writable arrays are private. Consumers receive command receipts
// and explicit cold independent copies; the live owner is never returned.
export class P051DerivedFacadeV2 {
    private field: WaterSandField;
    private readonly nmax: number;
    private readonly source: number;
    private stopped = false;
    static isOwnerPermit(permit: object): boolean { return permit === OWNER_PERMIT_V2; }
    constructor(config: DerivedConfigV2) {
        if (!config.isolated || config.numericPolicy !== DERIVED_POLICY_V2 || !Number.isInteger(config.nmax) || config.nmax < 2 || config.nmax > 512 || !Number.isSafeInteger(config.sourceId) || config.sourceId < 1 || config.sourceId > 0xffffffff || config.backend !== 'CPU_ORIGINAL_F64' || config.writerRegistry !== 'T216_DRY_WRITERS_V2')
            throw new Error('DERIVED_CONFIG_UNSUPPORTED');
        if (!Exact70.probe())
            throw new Error('NUMERIC_ENVIRONMENT_UNSUPPORTED');
        this.nmax = config.nmax;
        this.source = config.sourceId;
        this.field = this.create();
    }
    private create(checkpoint: DerivedCheckpointV2 | null = null): WaterSandField {
        const f = new WaterSandField(128, 256), p = defaultP011ParticleParams();
        p.nmax = this.nmax;
        const r = f.initializeDryParticleDemo({ revision: P051_DRY_REVISION, sourceId: this.source, particleParams: p });
        if (!r.ok)
            throw new Error('DERIVED_INIT:' + r.code);
        if (checkpoint !== null)
            f.restoreFromSnapshot(checkpoint.authority);
        f.enableDerivedV2Cold(OWNER_PERMIT_V2, checkpoint);
        return f;
    }
    exportFailureCold(): DerivedFailureRawV2 | null { return this.field.derivedFailureRawColdV2(OWNER_PERMIT_V2); }
    exportCheckpointCold(): DerivedCheckpointV2 { return this.field.exportDerivedCheckpointColdV2(OWNER_PERMIT_V2); }
    restoreCold(checkpoint: DerivedCheckpointV2): void {
        if (!(checkpoint instanceof DerivedCheckpointV2) || checkpoint.revision !== 'P011_DERIVED_CHECKPOINT_V2' || checkpoint.numericPolicy !== DERIVED_POLICY_V2 || checkpoint.cacheRevision !== DERIVED_CACHE_V2 || checkpoint.envelopeRevision !== 'P011_DRY_ENVELOPE_ORDERED_V2' || checkpoint.authority.drySourceId !== this.source)
            throw new Error('DERIVED_CHECKPOINT_VERSION_CONFIG');
        const expected=defaultP011ParticleParams();expected.nmax=this.nmax;if(checkpoint.backend!=='CPU_ORIGINAL_F64'||checkpoint.writerRegistry!=='T216_DRY_WRITERS_V2'||JSON.stringify(checkpoint.particleParams)!==JSON.stringify(expected))throw new Error('DERIVED_CHECKPOINT_PARAMETER_GRAPH'); const owned = checkpoint.cloneCold();
        const prior = new DerivedPublishedSlotV2(this.nmax);
        if (this.copyPublishedSceneStateInto(prior) !== 'OK')
            throw new Error('DERIVED_RESTORE_OLD_PUBLICATION');
        owned.stateEpoch = Math.max(owned.stateEpoch, prior.stateEpoch);
        const candidate = this.create(owned);
        this.field = candidate;
        this.stopped = false;
    }
    reset(): void { const candidate = this.create(); this.field = candidate; this.stopped = false; }
    setTilt(degrees: number): void {
        if (!this.stopped)
            this.field.setTilt(degrees);
    }
    setShakeInput(value: number): void {
        if (!this.stopped)
            this.field.setShakeInput(value);
    }
    inject(request: P051DryAdmissionRequest): DerivedSourceResultV2 {
        if (this.stopped)
            return { ok: false, original: null, derivedFault: null, physicalCommit: this.field.getRenderPhysicsCommitId(), captureStatus: 'STOPPED' };
        this.field.startDerivedCommandV2(OWNER_PERMIT_V2, 1, request.sequence);
        let receipt: P051DryAdmissionReceipt | null = null;
        try {
            receipt = this.field.injectDryGestureSand({ revision: request.revision, sourceId: request.sourceId, sequence: request.sequence, inputTimeMs: request.inputTimeMs, seed: request.seed, x: request.x, y: request.y, requestedH: request.requestedH, requestedL: request.requestedL });
        }
        catch (e) {
            const fault = this.field.derivedFaultColdV2(OWNER_PERMIT_V2);
            if (fault === null)
                throw e as Error;
            this.field.abortDerivedCommandV2(OWNER_PERMIT_V2);
        }
        const fault = this.field.derivedFaultColdV2(OWNER_PERMIT_V2);
        if (fault !== null) {
            this.stopped = true;
            fault.outerTransportStatus = receipt === null ? 'EXCEPTION' : receipt.rejectedReason ?? 'SOURCE_RETURN';
        }
        this.field.finishDerivedCommandV2(OWNER_PERMIT_V2, receipt !== null && receipt.rejectedReason === null && fault === null);
        return { ok: receipt !== null && receipt.rejectedReason === null && fault === null, original: receipt, derivedFault: fault, physicalCommit: this.field.getRenderPhysicsCommitId(), captureStatus: 'NOT_CAPTURED' };
    }
    step(dt: number): DerivedStepResultV2 {
        if (this.stopped)
            return { ok: false, originalError: null, derivedFault: null, physicalCommit: this.field.getRenderPhysicsCommitId(), captureStatus: 'STOPPED', original: null };
        this.field.startDerivedCommandV2(OWNER_PERMIT_V2, 2);
        let r: WaterSandStepResult | null = null;
        try {
            r = this.field.step(dt);
        }
        catch (e) {
            const fault = this.field.derivedFaultColdV2(OWNER_PERMIT_V2);
            if (fault === null)
                throw e as Error;
            this.field.abortDerivedCommandV2(OWNER_PERMIT_V2);
        }
        const fault = this.field.derivedFaultColdV2(OWNER_PERMIT_V2);
        if (fault !== null) {
            this.stopped = true;
            fault.outerTransportStatus = r === null ? 'EXCEPTION' : String(r.errorCode);
        }
        this.field.finishDerivedCommandV2(OWNER_PERMIT_V2, r !== null && r.ok && fault === null);
        return { ok: r !== null && r.ok && fault === null, originalError: fault !== null ? null : r === null ? null : r.errorCode, derivedFault: fault, physicalCommit: this.field.getRenderPhysicsCommitId(), captureStatus: 'NOT_CAPTURED', original: r };
    }
    createPublishedSlotCold(): DerivedPublishedSlotV2 { return new DerivedPublishedSlotV2(this.nmax); }
    copyPublishedRenderStateInto(slot: DerivedPublishedSlotV2): string { return slot.copyOwner(OWNER_PERMIT_V2, this.field, false); }
    copyPublishedSceneStateInto(slot: DerivedPublishedSlotV2): string { return slot.copyOwner(OWNER_PERMIT_V2, this.field, true); }
    workCold(): DerivedWorkV2 { return this.field.derivedWorkColdV2(OWNER_PERMIT_V2); }
    countersCold(): Float64Array { return this.field.derivedCountersColdV2(OWNER_PERMIT_V2); }
    get numericPolicy(): string { return DERIVED_POLICY_V2; }
}
// Controlled render adapter: UI/plugin never obtains the writable sealed payload.
export class DerivedPublishedSlotV2 {
    private readonly state: P011PublishedRenderState;
    constructor(nmax: number) { this.state = new P011PublishedRenderState(createDerivedPayloadV2(nmax)); }
    copyOwner(permit: object, field: WaterSandField, scene: boolean): string {
        if (permit !== OWNER_PERMIT_V2)
            return 'INVALID_TOKEN';
        return scene ? field.copyPublishedSceneStateInto(this.state) : field.copyPublishedRenderStateInto(this.state);
    }
    get runId(): string { return this.state.runId; }
    get physicsCommitId(): number { return this.state.physicsCommitId; }
    get stateEpoch(): number { return this.state.stateEpoch; }
    get effectiveTiltDeg(): number { return this.state.effectiveTiltDeg; }
    get publicationKind(): string { return this.state.publicationKind; }
    drawNewPipeline(renderer: P013SceneController, context: P012FrameContext, timeNs: number, scale: number): P013SceneReceipt | null { return renderer.drawNewPipelineFrame(this.state, context, p012FactoryAppearance(), 0, timeNs, scale); }
    exportPayloadCold(): DerivedCheckpointV2 { const cp = new DerivedCheckpointV2(this.state.particleCapacity); copyDerivedPayloadV2(this.state.field, cp.authority); cp.runId = this.state.runId; cp.commit = this.state.physicsCommitId; cp.stateEpoch = this.state.stateEpoch; cp.replayStatus = 'PUBLICATION_COPY_NOT_CHECKPOINT'; return cp; }
}
