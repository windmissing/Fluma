import { particleSnapshotBufferAtHot } from '../P011ParticleField';
import { P011PublishedRenderState, P011RenderPublicationKind } from '../P011PublishedRenderState';
import { P011EndpointCommitKey } from '../P011EndpointMetrics';
import { P011ExternalAdmissionRecord } from '../P011ExternalAdmissions';
import { DerivedCoordinatorV2 } from './P011DerivedTupleBarrierV2';
import { DerivedStatus, DERIVED_POLICY_V2, DERIVED_CACHE_V2, DERIVED_ENVELOPE_V2, DERIVED_PUBLICATION_V2 } from './P011DerivedTypesV2';
import { createDerivedPayloadV2, copyDerivedPayloadV2 } from './P011DerivedSnapshotV2';
import { Z08PublicationIdentity } from './P011Z08Archive';
export function readZ08PublicationIdentity(state: P011PublishedRenderState): Z08PublicationIdentity|null {
    const seal=state.derivedSeal, key=state.ledgerCommitKey;
    if (seal===null || !seal.matches(state) || state.publicationKind!=='STEP' ||
        key===null || key.runId!==state.runId || key.attemptId===null) return null;
    return {
        kind:state.publicationKind, run:state.runId, epoch:state.stateEpoch, commit:state.physicsCommitId,
        sourceEpoch:state.field.externalSourceState?.sourceEpoch??0, ledgerKey:JSON.stringify(key),
        substep:key.substepId, attempt:key.attemptId, committedOrdinal:key.committedOrdinal,
        seal:seal.serial, certificate:seal.certificate, content:state.poolContentSerial
    };
}
class DerivedSealKeyV2 {
}
const SEAL_KEY_V2 = new DerivedSealKeyV2();
export class DerivedPublicationSealV2 {
    private readonly state: P011PublishedRenderState;
    private readonly buffers: ArrayBuffer[] = [];
    readonly serial: number;
    readonly commit: number;
    readonly run: string;
    readonly certificate: number;
    private readonly motionRevision:string|undefined;
    private constructor(state: P011PublishedRenderState, serial: number, certificate: number) {
        this.state = state;
        this.serial = serial;
        this.commit = state.physicsCommitId;
        this.run = state.runId;
        this.certificate = certificate;this.motionRevision=state.motionRevision;
        const f = state.field;
        this.buffers = [f.waterTop.buffer as ArrayBuffer, f.bh.buffer as ArrayBuffer, f.bl.buffer as ArrayBuffer, f.mh.buffer as ArrayBuffer, f.ml.buffer as ArrayBuffer, f.suspH.buffer as ArrayBuffer, f.suspL.buffer as ArrayBuffer, f.sandTop.buffer as ArrayBuffer, f.hs.buffer as ArrayBuffer, f.hl.buffer as ArrayBuffer];
        const p = f.particleState;
        if (p === undefined || p === null)
            throw new Error('DERIVED_SEAL_POOL');
        for (let k = 0; k < 13; k++)
            this.buffers.push(particleSnapshotBufferAtHot(p, k));
    }
    static mintTrusted(state: P011PublishedRenderState, serial: number, certificate: number, key: DerivedSealKeyV2): DerivedPublicationSealV2 {
        if (key !== SEAL_KEY_V2)
            throw new Error('DERIVED_SEAL_AUTHORITY');
        return new DerivedPublicationSealV2(state, serial, certificate);
    }
    matches(state: P011PublishedRenderState): boolean {
        if (state !== this.state || state.derivedSeal !== this || state.physicsCommitId !== this.commit || state.runId !== this.run || state.motionRevision!==this.motionRevision || state.field.physicsCommitId !== this.commit || state.numericPolicy !== DERIVED_POLICY_V2 || state.cacheRevision !== DERIVED_CACHE_V2 || state.envelopeRevision !== DERIVED_ENVELOPE_V2 || state.publicationRevision !== DERIVED_PUBLICATION_V2 || state.derivedCertificateId !== this.certificate)
            return false;
        const f = state.field, p = f.particleState;
        if (p === undefined || p === null)
            return false;
        for (let k = 0; k < 13; k++)
            if (particleSnapshotBufferAtHot(p, k) !== this.buffers[10 + k])
                return false;
        return f.hs.buffer === this.buffers[8] && f.hl.buffer === this.buffers[9] && f.waterTop.buffer === this.buffers[0] && f.bh.buffer === this.buffers[1] && f.bl.buffer === this.buffers[2] && f.mh.buffer === this.buffers[3] && f.ml.buffer === this.buffers[4] && f.suspH.buffer === this.buffers[5] && f.suspL.buffer === this.buffers[6] && f.sandTop.buffer === this.buffers[7];
    }
}
export class DerivedCommitV2 {
    private readonly banks: P011PublishedRenderState[] = [];
    private readonly byteBuffers:ArrayBuffer[]=[];private readonly byteViews:Uint8Array[]=[];
    private active = -1;
    private prepared = -1;
    private serial = 0;
    private preparedContent = -1;
    private readonly owner: DerivedCoordinatorV2;
    private readonly fill: (out: P011PublishedRenderState) => void;
    constructor(owner: DerivedCoordinatorV2, nmax: number, fill: (out: P011PublishedRenderState) => void) {
        this.owner = owner;
        this.fill = fill;
        for (let i = 0; i < 2; i++)
            this.banks.push(new P011PublishedRenderState(createDerivedPayloadV2(nmax)));
        for(const bank of this.banks){const p=bank.field.particleState!;for(let k=0;k<13;k++)this.bindBytes(particleSnapshotBufferAtHot(p,k));const f=bank.field;for(const a of [f.waterTop,f.bh,f.bl,f.mh,f.ml,f.suspH,f.suspL,f.sandTop])this.bindBytes(a.buffer as ArrayBuffer);}
        for(let k=0;k<13;k++)this.bindBytes(particleSnapshotBufferAtHot(owner.canonical.pool,k));const f=owner.domain.field;for(const a of [f.waterTop,f.bh,f.bl,f.mh,f.ml,f.suspH,f.suspL,f.surface])this.bindBytes(a.buffer as ArrayBuffer);
    }
    prepareInactive(kind: P011RenderPublicationKind, run: string, epoch: number, commit: number, key: P011EndpointCommitKey | null, source: P011ExternalAdmissionRecord | null = null): DerivedStatus {
        if (!Number.isSafeInteger(commit) || commit < 0 || !Number.isSafeInteger(epoch) || epoch < 0 || this.serial >= Number.MAX_SAFE_INTEGER)
            return this.owner.fail(DerivedStatus.INPUT_INVALID, 'PUBLICATION_IDENTITY');
        const i = this.active === 0 ? 1 : 0, state = this.banks[i];
        state.derivedSeal = null;
        const fillScope=this.owner.budget.enter(8);try{this.fill(state);}finally{this.owner.budget.leave(fillScope);}
        const status = this.owner.verifyPublicBoundary(kind + ':' + commit);
        if (status !== 0)
            return status;
        const verifiedSerial = this.owner.contentSerial;
        if (!this.matchesAuthority(state))
            return this.owner.fail(DerivedStatus.CERT_STALE, 'PUBLICATION_BYTES_CHANGED_AFTER_VERIFY');
        state.runId = run;
        state.stateEpoch = epoch;
        state.physicsCommitId = commit;
        state.field.physicsCommitId = commit;
        state.publicationKind = kind;
        state.ledgerCommitKey = key;
        state.numericPolicy = DERIVED_POLICY_V2;
        state.cacheRevision = DERIVED_CACHE_V2;
        state.envelopeRevision = DERIVED_ENVELOPE_V2;
        state.publicationRevision = DERIVED_PUBLICATION_V2;
        state.derivedRevision = this.owner.canonical.revision;
        state.poolContentSerial = this.owner.contentSerial;
        state.derivedCertificateId = this.owner.verifyCertificate;
        if (source !== null) {
            const s = state.field.externalSourceState;
            if (s === undefined)
                return this.owner.fail(DerivedStatus.PUBLICATION_DOMAIN_MISMATCH, 'SOURCE_PAYLOAD');
            s.sourceInjectedH += source.admittedH;
            s.sourceInjectedL += source.admittedL;
            s.sourceEpoch++;
            s.admissions.count = source.cumulativeCount;
            s.admissions.observedTransactions = source.admissionTransactionId;
            if (s.admissions.admittedH !== null)
                s.admissions.admittedH += source.admittedH;
            if (s.admissions.admittedL !== null)
                s.admissions.admittedL += source.admittedL;
            s.admissions.lastRecord = new P011ExternalAdmissionRecord();
            const d = s.admissions.lastRecord;
            d.runId = source.runId;
            d.sourceId = source.sourceId;
            d.sequence = source.sequence;
            d.inputTime = source.inputTime;
            d.admittedH = source.admittedH;
            d.admittedL = source.admittedL;
            d.cumulativeCount = source.cumulativeCount;
            d.admissionTransactionId = source.admissionTransactionId;
            state.field.dryLastSequence = source.sequence;
            state.field.dryLastInputTime = source.inputTime;
        }
        state.derivedSeal = this.mintSeal(state,this.serial+1,state.derivedCertificateId);
        this.owner.budget.identity(run,commit,epoch,state.particleCapacity,state.liveCount);this.prepared = i;
        this.preparedContent = verifiedSerial;
        const pool=state.field.particleState!;const written=7*128*8+55*pool.nmax+16*128;this.owner.counters.add(13,written);
        return DerivedStatus.OK;
    }
    private mintSeal(state:P011PublishedRenderState,serial:number,certificate:number):DerivedPublicationSealV2{const scope=this.owner.budget.enter(11);try{return DerivedPublicationSealV2.mintTrusted(state,serial,certificate,SEAL_KEY_V2);}finally{this.owner.budget.leave(scope);}}
    private bindBytes(buffer:ArrayBuffer):void{this.byteBuffers.push(buffer);this.byteViews.push(new Uint8Array(buffer));}
    private byteView(buffer:ArrayBuffer,i:number):Uint8Array{if(i>=0&&this.byteBuffers[i]===buffer&&this.byteViews[i].byteLength===buffer.byteLength&&buffer.byteLength>0)return this.byteViews[i];this.owner.budget.add(17);return new Uint8Array(buffer);}
    private sameBuffer(a: ArrayBuffer, b: ArrayBuffer, ai:number=-1,bi:number=-1): boolean {
        if (a.byteLength !== b.byteLength)
            return false;
        const x = this.byteView(a,ai), y = this.byteView(b,bi);
        for (let i = 0; i < x.length; i++) {
            if (x[i] !== y[i]){this.owner.counters.add(16,i+1);this.owner.budget.add(16,i+1);return false;}
        }
        this.owner.counters.add(16,x.length);this.owner.budget.add(16,x.length);return true;
    }
    private matchesAuthority(state: P011PublishedRenderState): boolean {
 const t351meter=this.owner.budget,t351scope=t351meter?.enter(10)??-1;try {
        const bankBase=state===this.banks[0]?0:21;const p = state.field.particleState, q = this.owner.canonical.pool;
        if (p === null || p === undefined || p.liveCount !== q.liveCount || p.freeCount !== q.freeCount || p.nextId !== q.nextId) {
            this.owner.fault.detail = 'POOL_HEADER';
            return false;
        }
        for (let k = 0; k < 13; k++)
            if (!this.sameBuffer(particleSnapshotBufferAtHot(p, k), particleSnapshotBufferAtHot(q, k),bankBase+k,42+k)) {
                this.owner.fault.detail = 'POOL_BUFFER:' + k;
                return false;
            }
        const a = state.field, b = this.owner.domain.field;
        return this.sameBuffer(a.waterTop.buffer as ArrayBuffer, b.waterTop.buffer as ArrayBuffer,bankBase+13,55) && this.sameBuffer(a.bh.buffer as ArrayBuffer, b.bh.buffer as ArrayBuffer,bankBase+14,56) && this.sameBuffer(a.bl.buffer as ArrayBuffer, b.bl.buffer as ArrayBuffer,bankBase+15,57) && this.sameBuffer(a.mh.buffer as ArrayBuffer, b.mh.buffer as ArrayBuffer,bankBase+16,58) && this.sameBuffer(a.ml.buffer as ArrayBuffer, b.ml.buffer as ArrayBuffer,bankBase+17,59) && this.sameBuffer(a.suspH.buffer as ArrayBuffer, b.suspH.buffer as ArrayBuffer,bankBase+18,60) && this.sameBuffer(a.suspL.buffer as ArrayBuffer, b.suspL.buffer as ArrayBuffer,bankBase+19,61) && this.sameBuffer(a.sandTop.buffer as ArrayBuffer, b.surface.buffer as ArrayBuffer,bankBase+20,62);
    }finally{t351meter?.leave(t351scope);}
}
    abortPrepared():void {
        if(this.prepared>=0&&this.prepared!==this.active)this.banks[this.prepared].derivedSeal=null;
        this.prepared=-1;this.preparedContent=-1;
    }
    commitPrepared(): void {
        if (this.prepared < 0 || this.preparedContent !== this.owner.contentSerial)
            throw new Error('DERIVED_PREPARED_STALE');
        this.owner.budget.add(this.banks[this.prepared].publicationKind==='STEP'?7:this.banks[this.prepared].publicationKind==='EXTERNAL_ADMISSION'?8:23);
        this.serial++;
        this.active = this.prepared;
        this.prepared = -1;
    }
    readActiveZ08Identity(): Z08PublicationIdentity|null {
        return this.active<0 ? null : readZ08PublicationIdentity(this.banks[this.active]);
    }
    copyPublishedInto(out: P011PublishedRenderState): string {
 const t351meter=this.owner.budget,t351scope=t351meter?.enter(12)??-1;try {
        if (this.active < 0)
            return 'SCHEMA_UNAVAILABLE';
        const state = this.banks[this.active], seal = state.derivedSeal;
        if (seal === null || !seal.matches(state))
            return 'INVALID_TOKEN';
        if (out.field.particleState === undefined || out.field.particleState === null || out.field.particleState.nmax !== state.particleCapacity)
            return 'CAPACITY_MISMATCH';
        this.owner.budget.identity(state.runId,state.physicsCommitId,state.stateEpoch,state.particleCapacity,state.liveCount);
        out.derivedSeal = null;
        const bytes = copyDerivedPayloadV2(state.field, out.field,this.owner.budget);
        out.motionRevision=state.motionRevision;
        out.effectiveTiltDeg = state.effectiveTiltDeg;
        out.runId = state.runId;
        out.stateEpoch = state.stateEpoch;
        out.physicsCommitId = state.physicsCommitId;
        out.publicationKind = state.publicationKind;
        out.ledgerCommitKey = state.ledgerCommitKey;
        out.routeId = state.routeId;
        out.particleRouteActive = state.particleRouteActive;
        out.particleCapacity = state.particleCapacity;
        out.liveCount = state.liveCount;
        out.mediumMode = state.mediumMode;
        out.airDynamicsRevision = state.airDynamicsRevision;
        out.birthPlacementRevision = state.birthPlacementRevision;
        out.numericPolicy = state.numericPolicy;
        out.cacheRevision = state.cacheRevision;
        out.envelopeRevision = state.envelopeRevision;
        out.publicationRevision = state.publicationRevision;
        out.derivedRevision = state.derivedRevision;
        out.poolContentSerial = state.poolContentSerial;
        out.derivedCertificateId = state.derivedCertificateId;
        out.derivedSeal = this.mintSeal(out,seal.serial,seal.certificate);
        this.owner.counters.add(13, bytes);
        return seal === state.derivedSeal ? 'OK' : 'INVALID_TOKEN';
    }finally{t351meter?.leave(t351scope);}
}
}
export function derivedPublicationGateV2(state: P011PublishedRenderState): boolean {
    if (state.numericPolicy === DERIVED_POLICY_V2)
        return state.derivedSeal !== null && state.derivedSeal.matches(state);
    return (state.numericPolicy === undefined || state.numericPolicy === 'P011_NUMERIC_V1') && (state.cacheRevision === undefined || state.cacheRevision === 'P011_CACHE_V1') && (state.envelopeRevision === undefined || state.envelopeRevision === 'P011_ENVELOPE_V1') && (state.publicationRevision === undefined || state.publicationRevision === 'P011_PUBLICATION_V1') && (state.derivedSeal === undefined || state.derivedSeal === null);
}
