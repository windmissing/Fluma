import { P011ParticleSnapshot, P011ParticleDerivedBindingV2, particleSnapshotBufferAtHot } from '../P011ParticleField';
import { DerivedViewV2, DerivedMutationTokenV2 } from './P011DerivedViewV2';
import { DerivedStatus, DerivedCountersV2, DerivedFaultV2 } from './P011DerivedTypesV2';
import { DerivedFieldV2, DryDomainV2 } from './P011DryDomainV2';
export class DerivedCoordinatorV2 implements P011ParticleDerivedBindingV2 {
    readonly ownerId: number;
    readonly counters = new DerivedCountersV2();
    private readonly rootBefore = new Float64Array(32);
    private readonly rootRows = new Float64Array(1024 * 40);
    private rootCount = 0;
    private rootStart = 0;
    private rootKind = 0;
    private rootOrdinal = 0;
    private rootCommitBefore = 0;
    private rootActive = false;
    private traceIncomplete = false;
    startCommand(kind: number, ordinal: number, commit: number): void { this.startRoot(); this.rootKind = kind; this.rootOrdinal = ordinal; this.rootCommitBefore = commit; this.rootBefore.set(this.counters.values); this.rootStart = Date.now(); this.rootActive = true; }
    finishCommand(ok: boolean, commit: number): void {
        if (!this.rootActive)
            return;
        this.rootActive = false;
        if (this.rootCount >= 1024) {
            this.traceIncomplete = true;
            return;
        }
        const at = this.rootCount++ * 40, elapsed = Date.now() - this.rootStart;
        if (elapsed < 0 || !Number.isFinite(elapsed))
            this.traceIncomplete = true;
        this.rootRows[at] = this.root;
        this.rootRows[at + 1] = this.rootKind;
        this.rootRows[at + 2] = this.rootOrdinal;
        this.rootRows[at + 3] = ok ? 1 : 0;
        this.rootRows[at + 4] = this.rootCommitBefore;
        this.rootRows[at + 5] = commit;
        this.rootRows[at + 6] = elapsed;
        for (let i = 0; i < 32; i++)
            this.rootRows[at + 8 + i] = this.counters.values[i] - this.rootBefore[i];
    }
    exportWorkCold(): DerivedWorkV2 {
        const out = new DerivedWorkV2();
        out.rows = this.rootRows.slice(0, this.rootCount * 40);
        out.counters = this.counters.values.slice();
        out.incomplete = this.traceIncomplete;
        out.unscopedCounters.set(out.counters);
        for (let at = 0; at < out.rows.length; at += 40)
            for (let i = 0; i < 32; i++)
                out.unscopedCounters[i] -= out.rows[at + 8 + i];
        return out;
    }
    private readonly pools: P011ParticleSnapshot[] = [];
    private readonly views: DerivedViewV2[] = [];
    readonly canonical: DerivedViewV2;
    readonly domain: DryDomainV2;
    readonly audit: DerivedViewV2;
    readonly fault = new DerivedFaultV2();
    status = DerivedStatus.OK;
    root = 0;
    contentSerial = 0;
    lastVerifiedSerial = -1;
    lastBoundary = '';
    verifyCertificate = 0;
    readonly viewSlots = 12;
    private readonly rawBuffers: ArrayBuffer[] = [];
    private readonly rawCopies: Uint8Array[] = [];
    private readonly rawReaders: Uint8Array[] = [];
    private readonly rawNames: string[] = [];
    readonly faultHeaders = new Float64Array(64);
    private readonly scalarReaders: (() => number)[] = [];
    private readonly scalarNames: string[] = [];
    registerFailureScalarCold(name: string, read: () => number): void {
        if (this.scalarReaders.length >= 48)
            throw new Error('DERIVED_FAULT_SCALAR_CAPACITY');
        this.scalarReaders.push(read);
        this.scalarNames.push(name);
    }
    private rawFrozen = false;
    registerFailureBufferCold(name: string, buffer: ArrayBuffer): void {
        if (this.rawBuffers.indexOf(buffer) >= 0)
            return;
        this.rawBuffers.push(buffer);
        this.rawReaders.push(new Uint8Array(buffer));
        this.rawCopies.push(new Uint8Array(buffer.byteLength));
        this.rawNames.push(name);
    }
    exportFailureCold(): DerivedFailureRawV2 | null {
        if (!this.rawFrozen)
            return null;
        const out = new DerivedFailureRawV2();
        out.headers = this.faultHeaders.slice();
        out.scalarNames = this.scalarNames.slice();
        out.names = this.rawNames.slice();
        for (const bytes of this.rawCopies)
            out.buffers.push(bytes.slice());
        out.fault = JSON.parse(JSON.stringify(this.fault)) as DerivedFaultV2;
        return out;
    }
    private readonly freeSeen: Uint8Array;
    private readonly idA: Uint32Array;
    private readonly idB: Uint32Array;
    private readonly radix = new Uint32Array(256);
    private readonly fieldMarks: Float64Array[] = [];
    private readonly dirtyMarks: Uint8Array[] = [];
    private readonly fieldKeys: object[] = [];
    constructor(canonical: P011ParticleSnapshot, field: DerivedFieldV2, ownerId: number = 0) { this.ownerId = ownerId; this.freeSeen = new Uint8Array(canonical.nmax); this.idA = new Uint32Array(canonical.nmax); this.idB = new Uint32Array(canonical.nmax); this.canonical = this.register(canonical); this.audit = new DerivedViewV2(canonical, 12, this.counters, true, this.ownerId); this.domain = new DryDomainV2(field, this.canonical, this.counters); }
    register(p: P011ParticleSnapshot): DerivedViewV2 {
        const found = this.index(p);
        if (found >= 0)
            return this.views[found];
        if (this.views.length >= this.viewSlots)
            throw new Error('DERIVED_COLD_VIEW_CAPACITY');
        const v = new DerivedViewV2(p, this.views.length, this.counters, false, this.ownerId);
        this.pools.push(p);
        this.views.push(v);
        for (let k = 0; k < 13; k++)
            this.registerFailureBufferCold('pool.' + v.viewId + '.' + k, particleSnapshotBufferAtHot(p, k));
        for (let k = 0; k < v.persistentBuffers().length; k++)
            this.registerFailureBufferCold('derived.' + v.viewId + '.' + k, v.persistentBuffers()[k]);
        return v;
    }
    private index(p: P011ParticleSnapshot): number {
        for (let i = 0; i < this.pools.length; i++) {
            const old = this.pools[i];
            if (old === p)
                return i;
            if (old.pMass.buffer === p.pMass.buffer) {
                for (let k = 0; k < 13; k++)
                    if (particleSnapshotBufferAtHot(old, k) !== particleSnapshotBufferAtHot(p, k))
                        return -1;
                return i;
            }
        }
        return -1;
    }
    knows(p: P011ParticleSnapshot): boolean { return this.index(p) >= 0; }
    view(p: P011ParticleSnapshot): DerivedViewV2 | null { const i = this.index(p); return i < 0 ? null : this.views[i]; }
    startRoot(): void { this.root++; this.status = DerivedStatus.OK; this.fault.code = DerivedStatus.OK; this.fault.firstFaultFrozen = false; this.rawFrozen = false; this.fault.rolledBack = false; this.lastVerifiedSerial = -1; }
    fail(code: DerivedStatus, reason: string, p: P011ParticleSnapshot | null = null): DerivedStatus {
        this.status = code;
        if (!this.fault.firstFaultFrozen) {
            this.fault.code = code;
            this.fault.reason = reason;
            this.fault.root = this.root;
            this.fault.ordinal = this.rootOrdinal;
            this.fault.cut = reason;
            this.fault.firstFaultFrozen = true;
            for (let i = 0; i < this.rawCopies.length; i++) {
                this.rawCopies[i].set(this.rawReaders[i]);
                this.counters.add(12, this.rawCopies[i].byteLength);
            }
            const cp = this.canonical.pool;
            this.faultHeaders[0] = cp.liveCount;
            this.faultHeaders[1] = cp.freeCount;
            this.faultHeaders[2] = cp.nextId;
            this.faultHeaders[3] = this.contentSerial;
            this.faultHeaders[4] = this.root;
            for (let i = 0; i < this.scalarReaders.length; i++)
                this.faultHeaders[16 + i] = this.scalarReaders[i]();
            this.rawFrozen = true;
            if (p !== null) {
                const v = this.view(p);
                if (v !== null) {
                    this.fault.owner = this.ownerId;
                    this.fault.view = v.viewId;
                    this.fault.branch = v.branch;
                    for (let k = 0; k < 16; k++) {
                        this.fault.oldTupleBits[k] = v.tuples[k];
                        this.fault.newTupleBits[k] = v.tuples[k + 16];
                    }
                }
            }
        }
        return code;
    }
    begin(p: P011ParticleSnapshot, a: number, b: number = -1, cut: string = 'ACTUAL_TUPLE'): DerivedMutationTokenV2 | null {
        const v = this.view(p);
        if (v === null) {
            this.fail(DerivedStatus.CERT_STALE, 'UNREGISTERED_WRITER', p);
            return null;
        }
        v.root = this.root;
        v.cut = cut;
        const token = v.begin(a, b);
        if (token === null)
            this.fail(v.lastStatus, 'BEFORE_TUPLE', p);
        return token;
    }
    finish(p: P011ParticleSnapshot, token: DerivedMutationTokenV2): DerivedStatus {
        const v = this.view(p);
        if (v === null)
            return this.fail(DerivedStatus.CERT_STALE, 'FINISH_FOREIGN_VIEW');
        const status = v.finishActual(token);
        if (status !== 0)
            return this.fail(status, 'ACTUAL_TUPLE', p);
        this.contentSerial++;
        this.lastVerifiedSerial = -1;
        return DerivedStatus.OK;
    }
    copy(from: P011ParticleSnapshot, to: P011ParticleSnapshot): void {
        const a = this.view(from), b = this.view(to);
        if (a === null || b === null)
            throw new Error('DERIVED_UNREGISTERED_COPY');
        b.copyFrom(a);
        this.contentSerial++;
        this.lastVerifiedSerial = -1;
    }
    exportCopy(from: P011ParticleSnapshot, to: P011ParticleSnapshot): void {
        if (this.knows(to))
            this.copy(from, to);
    }
    markField(c: number = -1): void { this.domain.mark(c); this.contentSerial++; this.lastVerifiedSerial = -1; }
    flush(): DerivedStatus { const code = this.domain.flushBeforeRead(); return code === 0 ? code : this.fail(code, 'DRY_DOMAIN'); }
    verifyPublicBoundary(boundary: string): DerivedStatus {
        if (this.status !== 0)
            return this.status;
        if (this.lastVerifiedSerial === this.contentSerial && this.lastBoundary === boundary)
            return DerivedStatus.OK;
        this.counters.add(8);
        if (!this.canonical.bindingValid())
            return this.fail(DerivedStatus.CERT_STALE, 'POOL_BINDING');
        const structure = this.verifyStructure();
        if (structure !== 0)
            return this.fail(structure, 'PUBLIC_ALLOCATOR_ID');
        const status = this.audit.build();
        if (status !== 0)
            return this.fail(status, 'PUBLIC_POOL');
        const a = this.audit, v = this.canonical;
        for (let i = 0; i < a.acc.length; i++) {
            this.counters.add(3);
            if (a.acc[i] !== v.acc[i])
                return this.fail(DerivedStatus.CERT_STALE, 'ACC_MISMATCH');
        }
        for (let c = 0; c < 256; c++) {
            if (a.count[c] !== v.count[c])
                return this.fail(DerivedStatus.CERT_STALE, 'COUNT_MISMATCH');
            const x = c < 128 ? a.cacheH[c] : a.cacheL[c - 128], y = c < 128 ? v.cacheH[c] : v.cacheL[c - 128];
            a.bits.setFloat64(0, x, true);
            v.bits.setFloat64(0, y, true);
            if (a.bits.getUint32(0, true) !== v.bits.getUint32(0, true) || a.bits.getUint32(4, true) !== v.bits.getUint32(4, true))
                return this.fail(DerivedStatus.CERT_STALE, 'CACHE_BITS_MISMATCH');
        }
        const strict = this.domain.verifyStrict();
        if (strict !== 0)
            return this.fail(strict, 'PUBLIC_STRICT_DOMAIN');
        this.lastVerifiedSerial = this.contentSerial;
        this.lastBoundary = boundary;
        this.verifyCertificate++;
        return DerivedStatus.OK;
    }
    private verifyStructure(): DerivedStatus {
        const p = this.canonical.pool;
        if (p.liveCount + p.freeCount !== p.nmax || p.freeCount < 0 || p.freeCount > p.nmax || p.nextId < 1 || p.nextId > 4294967296)
            return DerivedStatus.INPUT_INVALID;
        this.freeSeen.fill(0);
        for (let i = 0; i < p.freeCount; i++) {
            const slot = p.freeSlots[i];
            if (slot < 0 || slot >= p.nmax || this.freeSeen[slot] || p.pState[slot] !== 0)
                return DerivedStatus.INPUT_INVALID;
            this.freeSeen[slot] = 1;
        }
        let n = 0;
        for (let i = 0; i < p.nmax; i++) {
            if (p.pState[i]) {
                if (p.pSource[i] > 1 || p.pSourceId[i] === 0 || p.pId[i] >= p.nextId || !Number.isFinite(p.pY[i]) || p.pY[i] < 0 || p.pY[i] > 256 || !Number.isFinite(p.pVx[i]) || !Number.isFinite(p.pVy[i]))
                    return DerivedStatus.INPUT_INVALID;
                this.idA[n++] = p.pId[i];
            }
            else if (!this.freeSeen[i])
                return DerivedStatus.INPUT_INVALID;
        }
        if (n !== p.liveCount)
            return DerivedStatus.INPUT_INVALID;
        let from = this.idA, to = this.idB;
        for (let pass = 0; pass < 4; pass++) {
            this.radix.fill(0);
            for (let i = 0; i < n; i++)
                this.radix[(from[i] >>> (pass * 8)) & 255]++;
            let total = 0;
            for (let k = 0; k < 256; k++) {
                const count = this.radix[k];
                this.radix[k] = total;
                total += count;
            }
            for (let i = 0; i < n; i++)
                to[this.radix[(from[i] >>> (pass * 8)) & 255]++] = from[i];
            const swap = from;
            from = to;
            to = swap;
        }
        for (let i = 1; i < n; i++)
            if (from[i] === from[i - 1])
                return DerivedStatus.INPUT_INVALID;
        return DerivedStatus.OK;
    }
    registerFieldMark(key: object): void { this.fieldKeys.push(key); this.fieldMarks.push(new Float64Array(128 + 2)); this.dirtyMarks.push(new Uint8Array(128)); }
    saveField(key: object): void {
        const i = this.fieldKeys.indexOf(key);
        if (i < 0)
            throw new Error('DERIVED_FIELD_MARK');
        const row = this.fieldMarks[i];
        row.set(this.domain.field.surface);
        row[128] = this.domain.revision;
        row[129] = this.canonical.revision;
        this.dirtyMarks[i].set(this.domain.dirty);
        this.counters.add(9, row.byteLength + 128);
    }
    restoreField(key: object): void {
        const i = this.fieldKeys.indexOf(key);
        if (i < 0)
            throw new Error('DERIVED_FIELD_MARK');
        const row = this.fieldMarks[i];
        for (let c = 0; c < 128; c++)
            this.domain.field.surface[c] = row[c];
        this.domain.revision = row[128];
        this.domain.dirty.set(this.dirtyMarks[i]);
        this.contentSerial++;
        this.lastVerifiedSerial = -1;
        this.counters.add(11);
        this.fault.rolledBack = true;
    }
}
export class DerivedFailureRawV2 {
    revision = 'P011_DERIVED_FAILURE_RAW_V2';
    status = 'COMPLETE_REGISTERED_BUFFER_DOMAIN';
    missing = 'External callback/renderer/native raw are separate domains';
    headers = new Float64Array(0);
    scalarNames: string[] = [];
    names: string[] = [];
    buffers: Uint8Array[] = [];
    fault: DerivedFaultV2 | null = null;
}
export class DerivedWorkV2 {
    revision = 'P011_DERIVED_WORK_V2';
    clockDomain = 'UTC_WALL_MS';
    rows = new Float64Array(0);
    counters = new Float64Array(0);
    unscopedCounters = new Float64Array(32);
    incomplete = false;
}
