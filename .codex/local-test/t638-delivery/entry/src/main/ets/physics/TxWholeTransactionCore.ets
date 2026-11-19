import { TxCertificates } from './tx/TxCertificates';
import { TxFieldViews } from './tx/TxFieldViews';
import { P011MergeExactIndex } from './P011MergeExactIndex';
import { TxStorageRegistry } from './tx/TxStorageRegistry';
import { TxBranchView, TxViewHandle } from './tx/TxBranchView';
import { particleSnapshotBufferAtHot } from './P011ParticleField';
import { P011ParticleHotWorkspace } from './P011ParticleHot';
import { P011ParticleSnapshot } from './P011ParticleField';
import { TxPairAdjAvl } from './tx/TxPairAdjAvl';
import { TxWorkTrace } from './tx/TxWorkTrace';
export type TxHotBanks = P011ParticleHotWorkspace;
export class TxPairResult {
    status = 'NO_PAIR';
    slotA = -1;
    slotB = -1;
    errorCode = 'NONE';
    aId = 0;
    bId = 0;
    generationA = 0;
    generationB = 0;
    branchGeneration = 0;
    contentRev = 0;
    viewSlot = -1;
    backend = 'TX_ADJ_AVL';
}
export class TxWholePairAdapter {
    readonly trace = new TxWorkTrace();
    readonly certificates = new TxCertificates();
    readonly fieldViews = new TxFieldViews(128);
    readonly nmax: number;
    private readonly legacy: P011MergeExactIndex;
    private banks: P011ParticleSnapshot[] = [];
    private indexes: TxPairAdjAvl[] = [];
    private valid = new Uint8Array(4);
    private canonical = -1;
    readonly result = new TxPairResult();
    backend = 'TX_ADJ_AVL';
    ownerBound = false;
    private registry: TxStorageRegistry | null = null;
    private views: TxBranchView | null = null;
    private rootView: TxViewHandle | null = null;
    private work: P011ParticleHotWorkspace | null = null;
    constructor(nmax: number) { this.nmax = nmax; this.legacy = new P011MergeExactIndex(nmax); }
    attach(w: TxHotBanks, radius: number): void { if (this.ownerBound)
        return; this.work = w; this.banks = [w.frozenPool, w.patchBefore, w.patchAfter, w.patchScratch]; for (const p of this.banks)
        this.indexes.push(new TxPairAdjAvl(p, radius)); const buffers: ArrayBuffer[] = []; for (let i = 0; i < this.banks.length; i++) {
        for (let k = 0; k < 13; k++)
            buffers.push(particleSnapshotBufferAtHot(this.banks[i], k));
        for (const b of this.indexes[i].buffers())
            buffers.push(b);
    } this.registry = new TxStorageRegistry(w.ownerId, buffers); for (let i = 0; i < this.banks.length; i++) {
        const pool = this.banks[i], index = this.indexes[i];
        this.registry.registerScalar(w.ownerId, 'pool.' + i + '.live', (): number => pool.liveCount, (v: number): void => { pool.liveCount = v; });
        this.registry.registerScalar(w.ownerId, 'pool.' + i + '.free', (): number => pool.freeCount, (v: number): void => { pool.freeCount = v; });
        this.registry.registerScalar(w.ownerId, 'pool.' + i + '.nextId', (): number => pool.nextId, (v: number): void => { pool.nextId = v; });
        this.registry.registerScalar(w.ownerId, 'index.' + i + '.root', (): number => index.root, (v: number): void => { index.root = v; });
        this.registry.registerScalar(w.ownerId, 'index.' + i + '.idRoot', (): number => index.ids.root, (v: number): void => { index.ids.root = v; });
    } this.registry.register(w.ownerId, 'index.valid', this.valid.buffer as ArrayBuffer, 1, 0, this.valid.length); this.registry.registerScalar(w.ownerId, 'canonical', (): number => this.canonical, (v: number): void => { this.canonical = v; }); this.views = new TxBranchView(this.registry); w.pool.setTxCanonicalInvalidator((): void => { this.invalidateCanonical(); }); this.ownerBound = true; w.txD1 = this; }
    beginRoot(): void { this.trace.beginRoot(); if (this.views !== null) {
        if (this.rootView !== null)
            this.views.close(this.rootView);
        this.rootView = this.views.enterRoot();
    } this.canonical = -1; this.valid.fill(0); }
    role(role: number): void { this.trace.context[12] = role; this.trace.context[13]++; }
    syncContext(): void { const w = this.work; if (w === null)
        return; const c = this.trace.context; c[0] = w.ownerId; c[1] = w.runToken; c[2] = w.stepEpoch; c[3] = w.attemptToken; c[4] = w.substepId; c[5] = w.seed; c[6] = w.paramsRevision; c[7] = w.inputRevision; c[8] = w.sourceEpoch; c[9] = w.dt; c[10] = this.views === null ? 0 : this.views.branchGeneration; c[11] = this.trace.root; }
    invalidateCanonical(): void { this.canonical = -1; this.trace.add(37); }
    endRoot(): void { if (this.views !== null && this.rootView !== null) {
        this.views.close(this.rootView);
        this.rootView = null;
    } }
    private at(p: P011ParticleSnapshot): number { for (let i = 0; i < this.banks.length; i++)
        if (this.banks[i] === p)
            return i; return -1; }
    private ready(p: P011ParticleSnapshot): TxPairAdjAvl | null { const i = this.at(p); if (i < 0)
        return null; if (!this.valid[i]) {
        const x = this.indexes[i], v = x.visits, nt = x.neighborTests, rot = x.rotations, iv = x.ids.visits, ins = x.inserts;
        x.rebuild();
        this.valid[i] = 1;
        this.trace.add(16);
        this.trace.add(17, x.inserts - ins);
        this.trace.add(20, x.neighborTests - nt);
        this.trace.add(46, x.visits - v);
        this.trace.add(47, x.rotations - rot);
        this.trace.add(48, x.ids.visits - iv);
    } return this.indexes[i]; }
    before(p: P011ParticleSnapshot, slot: number): void { const x = this.ready(p); if (x !== null) {
        const n = x.erases, v = x.visits, nt = x.neighborTests, rot = x.rotations, iv = x.ids.visits;
        x.eraseSlot(slot);
        this.trace.add(18, x.erases - n);
        this.trace.add(20, x.neighborTests - nt);
        this.trace.add(46, x.visits - v);
        this.trace.add(47, x.rotations - rot);
        this.trace.add(48, x.ids.visits - iv);
    } }
    after(p: P011ParticleSnapshot, slot: number): void { const x = this.ready(p); if (x !== null) {
        const n = x.inserts, v = x.visits, nt = x.neighborTests, rot = x.rotations, iv = x.ids.visits;
        x.insertSlot(slot);
        this.trace.add(17, x.inserts - n);
        this.trace.add(20, x.neighborTests - nt);
        this.trace.add(46, x.visits - v);
        this.trace.add(47, x.rotations - rot);
        this.trace.add(48, x.ids.visits - iv);
    } }
    copy(from: P011ParticleSnapshot, to: P011ParticleSnapshot): void { const a = this.at(from), b = this.at(to); this.trace.add(28, 55 * to.nmax + 16 * to.cols); if (b < 0)
        return; if (a < 0 || !this.valid[a]) {
        this.valid[b] = 0;
        return;
    } this.indexes[b].copyIndex(this.indexes[a]); this.valid[b] = 1; this.trace.add(38, 96 * to.nmax); }
    transfer(out: P011ParticleSnapshot, restore: boolean): void { const i = this.at(out); this.trace.add(28, 55 * out.nmax + 16 * out.cols); if (i < 0) {
        this.canonical = -1;
        this.valid.fill(0);
        this.trace.add(37);
        return;
    } if (restore) {
        this.ready(out);
        this.canonical = i;
    }
    else if (this.canonical >= 0) {
        const source = this.canonical;
        this.indexes[i].copyIndex(this.indexes[source]);
        this.valid[i] = this.valid[source];
        this.trace.add(38, 96 * out.nmax);
    }
    else {
        this.valid[i] = 0;
        this.ready(out);
        this.canonical = i;
    } }
    invalidate(p: P011ParticleSnapshot): void { const i = this.at(p); if (i >= 0)
        this.valid[i] = 0; }
    lookup(p: P011ParticleSnapshot, id: number): number { const x = this.ready(p); if (x !== null && x.certified) {
        const v = x.ids.visits, result = x.ids.lookup(id);
        this.trace.add(48, x.ids.visits - v);
        return result;
    } this.trace.add(37); for (let i = 0; i < p.nmax; i++) {
        this.trace.add(39);
        if (p.pState[i] !== 0 && p.pId[i] === id)
            return i;
    } return -1; }
    selectPair(p: P011ParticleSnapshot, radius: number, w: TxHotBanks): TxPairResult { if (!this.ownerBound)
        this.attach(w, radius);
    else if (this.work !== w)
        throw new Error('TX_FOREIGN_OWNER'); return this.selectBound(p, radius); }
    selectBound(p: P011ParticleSnapshot, radius: number): TxPairResult {
        const o = this.result;
        o.slotA = o.slotB = -1;
        o.aId = o.bId = o.generationA = o.generationB = 0;
        o.viewSlot = this.at(p);
        o.backend = this.backend;
        o.errorCode = 'NONE';
        this.trace.add(11);
        const x = this.ready(p);
        if (this.backend === 'TX_ADJ_AVL' && x !== null && x.certified && x.radius === radius) {
            const nv = x.neighborTests, rv = x.rankVisits, rg = x.rangeVisits;
            const f = x.find();
            this.trace.add(20, x.neighborTests - nv);
            this.trace.add(21, x.rankVisits - rv);
            this.trace.add(22, x.rangeVisits - rg);
            o.status = f ? 'PAIR' : 'NO_PAIR';
            o.slotA = x.selectedA;
            o.slotB = x.selectedB;
            o.aId = o.slotA < 0 ? 0 : p.pId[o.slotA];
            o.bId = o.slotB < 0 ? 0 : p.pId[o.slotB];
            o.generationA = o.slotA < 0 ? 0 : x.slotGeneration[o.slotA];
            o.generationB = o.slotB < 0 ? 0 : x.slotGeneration[o.slotB];
            o.branchGeneration = this.views === null ? 0 : this.views.branchGeneration;
            o.contentRev = x.revision;
            this.trace.add(f ? 12 : 13);
            return o;
        }
        // Uncontrolled/invalid writers retain the original full exact physical-order search.
        o.backend = this.backend === 'LEGACY_SCAN' ? 'LEGACY_SCAN' : 'LEGACY_UNCERTIFIED';
        this.trace.add(37);
        if (x === null || !x.certified) {
            const found = this.legacy.find(p, radius);
            o.slotA = this.legacy.selectedA;
            o.slotB = this.legacy.selectedB;
            o.status = found ? 'PAIR' : 'NO_PAIR';
            this.trace.add(found ? 12 : 13);
            return o;
        }
        let a = -1, b = -1;
        for (let i = 0; i < p.nmax; i++)
            for (let j = 0; j < p.nmax; j++) {
                this.trace.add(39);
                if (!p.pState[i] || !p.pState[j] || p.pId[i] >= p.pId[j] || p.pPhase[i] !== p.pPhase[j] || p.pSource[i] !== p.pSource[j] || p.pSourceId[i] !== p.pSourceId[j] || p.pState[i] !== p.pState[j] || Math.floor(p.pX[i]) !== Math.floor(p.pX[j]) || Math.abs(p.pY[i] - p.pY[j]) > radius)
                    continue;
                if (a < 0 || p.pId[i] < p.pId[a] || (p.pId[i] === p.pId[a] && p.pId[j] < p.pId[b])) {
                    a = i;
                    b = j;
                }
            }
        o.slotA = a;
        o.slotB = b;
        o.status = a < 0 ? 'NO_PAIR' : 'PAIR';
        this.trace.add(a < 0 ? 13 : 12);
        return o;
    }
}
