import { P011ParticleSnapshot, particleSnapshotBufferAtHot } from '../P011ParticleField';
import { Exact70 } from './P011Exact70';
import { DerivedStatus, DerivedCountersV2, DERIVED_POLICY_V2 } from './P011DerivedTypesV2';
export class DerivedMutationTokenV2 {
    readonly serial: number;
    readonly slotA: number;
    readonly slotB: number;
    readonly owner: number;
    readonly ownerGeneration = 1;
    readonly view: number;
    readonly branchGeneration: number;
    readonly root: number;
    readonly cut: string;
    readonly generationA: number;
    readonly generationB: number;
    readonly oldIdA: number;
    readonly oldIdB: number;
    private constructor(serial: number, a: number, b: number, v: DerivedViewV2) { this.serial = serial; this.slotA = a; this.slotB = b; this.owner = v.ownerId; this.view = v.viewId; this.branchGeneration = v.branch; this.root = v.root; this.cut = v.cut; this.generationA = a < 0 ? 0 : v.generations[a]; this.generationB = b < 0 ? 0 : v.generations[b]; this.oldIdA = a < 0 ? 0 : v.pool.pId[a]; this.oldIdB = b < 0 ? 0 : v.pool.pId[b]; }
    static mint(serial: number, a: number, b: number, v: DerivedViewV2): DerivedMutationTokenV2 { return new DerivedMutationTokenV2(serial, a, b, v); }
}
export class DerivedViewV2 {
    readonly policy = DERIVED_POLICY_V2;
    readonly pool: P011ParticleSnapshot;
    readonly acc = new Uint16Array(2 * 128 * 70);
    readonly count = new Uint32Array(256);
    readonly dirty = new Uint8Array(128);
    readonly generations: Float64Array;
    readonly ids: Uint32Array;
    readonly cacheH: Float64Array;
    readonly cacheL: Float64Array;
    readonly counters: DerivedCountersV2;
    readonly codec: Exact70;
    readonly tuples = new Float64Array(32);
    private readonly changed = new Int32Array(4);
    private readonly backup = new Uint16Array(4 * 70);
    private readonly backupCount = new Uint32Array(4);
    private readonly backupBits = new Uint32Array(8);
    private readonly backupDirty = new Uint8Array(4);
    private readonly encoded = new Uint16Array(70);
    readonly bits = new DataView(new ArrayBuffer(8));
    private readonly bindings: ArrayBuffer[] = [];
    private active: DerivedMutationTokenV2 | null = null;
    private changes = 0;
    private readonly membershipChanged = new Uint8Array(2);
    serial = 0;
    revision = 0;
    branch = 1;
    readonly ownerId: number;
    root = 0;
    cut = 'ACTUAL_TUPLE';
    viewId: number;
    valid = true;
    lastStatus = DerivedStatus.OK;
    constructor(pool: P011ParticleSnapshot, viewId: number, counters: DerivedCountersV2, audit: boolean = false, ownerId: number = 0) {
        this.ownerId = ownerId;
        this.pool = pool;
        this.viewId = viewId;
        this.counters = counters;
        this.codec = new Exact70(counters);
        this.cacheH = audit ? new Float64Array(128) : pool.colParticleMassH;
        this.cacheL = audit ? new Float64Array(128) : pool.colParticleMassL;
        this.generations = new Float64Array(pool.nmax);
        this.ids = new Uint32Array(pool.nmax);
        for (let i = 0; i < 13; i++)
            this.bindings.push(particleSnapshotBufferAtHot(pool, i));
    }
    bindingValid(): boolean {
        const p = this.pool;
        if (p.cols !== 128 || p.nmax < 2 || p.nmax > 512)
            return false;
        for (let i = 0; i < 13; i++)
            if (particleSnapshotBufferAtHot(p, i) !== this.bindings[i])
                return false;
        return true;
    }
    private capture(slot: number, at: number): DerivedStatus {
        const t = this.tuples, p = this.pool;
        for (let k = 0; k < 8; k++)
            t[at + k] = 0;
        t[at] = slot;
        if (slot === -1)
            return DerivedStatus.OK;
        if (slot < 0 || slot >= p.nmax)
            return DerivedStatus.INPUT_INVALID;
        t[at + 1] = p.pId[slot];
        t[at + 2] = this.generations[slot];
        t[at + 3] = p.pState[slot];
        t[at + 4] = p.pPhase[slot];
        t[at + 5] = Math.floor(p.pX[slot]);
        this.bits.setFloat64(0, p.pMass[slot], true);
        t[at + 6] = this.bits.getUint32(0, true);
        t[at + 7] = this.bits.getUint32(4, true);
        if (p.pState[slot] === 0)
            return DerivedStatus.OK;
        if ((p.pState[slot] !== 1 && p.pState[slot] !== 2) || p.pPhase[slot] > 1 || p.pId[slot] === 0 || !Number.isFinite(p.pX[slot]) || p.pX[slot] < 0 || p.pX[slot] >= 128 || !Number.isFinite(p.pMass[slot]) || p.pMass[slot] <= 0 || p.pMass[slot] > 256 || (t[at + 7] >>> 31) !== 0)
            return DerivedStatus.INPUT_INVALID;
        return DerivedStatus.OK;
    }
    begin(a: number, b: number = -1): DerivedMutationTokenV2 | null {
        this.lastStatus = DerivedStatus.OK;
        if (!this.valid || !this.bindingValid() || this.active !== null || a === b || this.serial >= Number.MAX_SAFE_INTEGER) {
            this.lastStatus = DerivedStatus.CERT_STALE;
            return null;
        }
        let status = this.capture(a, 0);
        if (status === 0)
            status = this.capture(b, 8);
        if (status !== 0) {
            this.lastStatus = status;
            return null;
        }
        const token = DerivedMutationTokenV2.mint(++this.serial, a, b, this);
        this.active = token;
        this.counters.add(10);
        this.counters.add(15);
        return token;
    }
    private sameMember(a: number, b: number): boolean {
        const t = this.tuples;
        if (t[a] < 0 || t[a + 3] === 0)
            return t[b] < 0 || t[b + 3] === 0;
        if (t[b] < 0 || t[b + 3] === 0)
            return false;
        return t[a + 4] === t[b + 4] && t[a + 5] === t[b + 5] && t[a + 6] === t[b + 6] && t[a + 7] === t[b + 7];
    }
    private bucket(at: number): number { return this.tuples[at + 4] * 128 + this.tuples[at + 5]; }
    private remember(bucket: number): void {
        for (let i = 0; i < this.changes; i++)
            if (this.changed[i] === bucket)
                return;
        const i = this.changes++;
        this.changed[i] = bucket;
        for (let j = 0; j < 70; j++)
            this.backup[i * 70 + j] = this.acc[bucket * 70 + j];
        this.backupCount[i] = this.count[bucket];
        this.bits.setFloat64(0, bucket < 128 ? this.cacheH[bucket] : this.cacheL[bucket - 128], true);
        this.backupBits[i * 2] = this.bits.getUint32(0, true);
        this.backupBits[i * 2 + 1] = this.bits.getUint32(4, true);
        this.backupDirty[i] = this.dirty[bucket % 128];
    }
    private undoMutation(): void {
        for (let i = 0; i < this.changes; i++) {
            const bucket = this.changed[i];
            for (let j = 0; j < 70; j++)
                this.acc[bucket * 70 + j] = this.backup[i * 70 + j];
            this.count[bucket] = this.backupCount[i];
            this.bits.setUint32(0, this.backupBits[i * 2], true);
            this.bits.setUint32(4, this.backupBits[i * 2 + 1], true);
            if (bucket < 128)
                this.cacheH[bucket] = this.bits.getFloat64(0, true);
            else
                this.cacheL[bucket - 128] = this.bits.getFloat64(0, true);
            this.dirty[bucket % 128] = this.backupDirty[i];
        }
        this.counters.add(11);
    }
    private member(at: number, remove: boolean, building: boolean = false): DerivedStatus {
        if (this.tuples[at] < 0 || this.tuples[at + 3] === 0)
            return DerivedStatus.OK;
        const bucket = this.bucket(at);
        const code = this.codec.encodeMassBits(this.tuples[at + 6], this.tuples[at + 7], this.encoded, 0);
        if (code !== 0)
            return code;
        if (remove && this.count[bucket] === 0)
            return DerivedStatus.ACC_NEGATIVE;
        const status = remove ? this.codec.subtractAt(this.acc, bucket * 70, this.encoded) : this.codec.addAt(this.acc, bucket * 70, this.encoded);
        if (status !== 0)
            return status;
        this.count[bucket] += remove ? -1 : 1;
        this.counters.add(building ? 20 : remove ? 1 : 2);
        return this.count[bucket] > this.pool.nmax ? DerivedStatus.INPUT_INVALID : DerivedStatus.OK;
    }
    finishActual(token: DerivedMutationTokenV2, originalStatus: number = 0): DerivedStatus {
        if (token !== this.active || token.serial !== this.serial || token.owner !== this.ownerId || token.view !== this.viewId || token.branchGeneration !== this.branch || token.root !== this.root || (token.slotA >= 0 && token.generationA !== this.generations[token.slotA]) || (token.slotB >= 0 && token.generationB !== this.generations[token.slotB])) {
            this.lastStatus = DerivedStatus.CERT_STALE;
            return this.lastStatus;
        }
        this.active = null;
        if (originalStatus !== 0)
            return DerivedStatus.OK;
        let status = this.capture(token.slotA, 16);
        if (status === 0)
            status = this.capture(token.slotB, 24);
        if (status !== 0) {
            this.lastStatus = status;
            return status;
        }
        this.changes = 0;
        for (let k = 0; k < 2; k++)
            this.membershipChanged[k] = this.sameMember(k * 8, k * 8 + 16) ? 0 : 1;
        for (let at = 0; at < 32; at += 8)
            if (this.membershipChanged[Math.floor(at / 8) % 2] !== 0 && this.tuples[at] >= 0 && this.tuples[at + 3] !== 0)
                this.remember(this.bucket(at));
        for (let at = 0; at < 16 && status === 0; at += 8)
            if (this.membershipChanged[at / 8] !== 0)
                status = this.member(at, true);
        for (let at = 16; at < 32 && status === 0; at += 8)
            if (this.membershipChanged[at / 8 - 2] !== 0)
                status = this.member(at, false);
        for (let i = 0; i < this.changes && status === 0; i++) {
            const bucket = this.changed[i];
            status = this.codec.roundAt(this.acc, bucket * 70, this.bits);
            const value = this.bits.getFloat64(0, true);
            if (status === 0 && ((this.count[bucket] === 0) !== (value === 0)))
                status = DerivedStatus.INPUT_INVALID;
            if (status === 0) {
                if (bucket < 128)
                    this.cacheH[bucket] = value;
                else
                    this.cacheL[bucket - 128] = value;
                this.dirty[bucket % 128] = 1;
                this.counters.add(5);
            }
        }
        if (status !== 0)
            this.undoMutation();
        else {
            for (let at = 16; at < 32; at += 8) {
                const slot = this.tuples[at];
                if (slot >= 0) {
                    if (this.ids[slot] !== this.pool.pId[slot] || this.tuples[at - 16 + 3] !== this.tuples[at + 3])
                        this.generations[slot]++;
                    this.ids[slot] = this.pool.pId[slot];
                }
            }
            this.revision++;
        }
        this.lastStatus = status;
        return status;
    }
    build(): DerivedStatus {
        this.acc.fill(0);
        this.count.fill(0);
        let live = 0;
        for (let slot = 0; slot < this.pool.nmax; slot++) {
            this.counters.add(0);
            const status = this.capture(slot, 16);
            if (status !== 0)
                return this.lastStatus = status;
            if (this.pool.pState[slot] !== 0) {
                live++;
                const code = this.member(16, false, true);
                if (code !== 0)
                    return this.lastStatus = code;
            }
            this.ids[slot] = this.pool.pId[slot];
        }
        if (live !== this.pool.liveCount)
            return this.lastStatus = DerivedStatus.INPUT_INVALID;
        for (let b = 0; b < 256; b++) {
            const code = this.codec.roundAt(this.acc, b * 70, this.bits);
            if (code !== 0)
                return this.lastStatus = code;
            if (b < 128)
                this.cacheH[b] = this.bits.getFloat64(0, true);
            else
                this.cacheL[b - 128] = this.bits.getFloat64(0, true);
        }
        this.dirty.fill(1);
        this.revision++;
        return this.lastStatus = DerivedStatus.OK;
    }
    copyFrom(from: DerivedViewV2): void { this.acc.set(from.acc); this.count.set(from.count); this.dirty.set(from.dirty); this.generations.set(from.generations); this.ids.set(from.ids); this.revision = from.revision; this.valid = from.valid; this.active = null; this.branch++; this.serial++; this.counters.add(9, this.acc.byteLength + this.count.byteLength + this.dirty.byteLength + this.generations.byteLength + this.ids.byteLength); }
    persistentBuffers(): ArrayBuffer[] { return [this.acc.buffer as ArrayBuffer, this.count.buffer as ArrayBuffer, this.dirty.buffer as ArrayBuffer, this.generations.buffer as ArrayBuffer, this.ids.buffer as ArrayBuffer]; }
}
