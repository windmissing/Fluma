import { TxAvlTree } from './TxAvlTree';
import { TxIdIndex } from './TxIdIndex';
import { P011ParticleSnapshot } from '../P011ParticleField';
export class TxPairAdjAvl extends TxAvlTree {
    readonly size: Int32Array;
    readonly prev: Int32Array;
    readonly next: Int32Array;
    readonly flags: Int32Array;
    readonly minEligibleSlot: Int32Array;
    readonly minIdSlot: Int32Array;
    readonly slotLive: Int32Array;
    readonly reserved: Int32Array;
    readonly keyRev: Float64Array;
    readonly slotGeneration: Float64Array;
    readonly minEligibleId: Float64Array;
    readonly minId: Float64Array;
    readonly ids: TxIdIndex;
    readonly pool: P011ParticleSnapshot;
    radius: number;
    certified = true;
    selectedA = -1;
    selectedB = -1;
    builds = 0;
    neighborTests = 0;
    rankVisits = 0;
    rangeVisits = 0;
    revision = 0;
    constructor(p: P011ParticleSnapshot, radius: number) { super(p.nmax); this.pool = p; this.radius = radius; this.size = new Int32Array(p.nmax); this.prev = new Int32Array(p.nmax); this.next = new Int32Array(p.nmax); this.flags = new Int32Array(p.nmax); this.minEligibleSlot = new Int32Array(p.nmax); this.minIdSlot = new Int32Array(p.nmax); this.slotLive = new Int32Array(p.nmax); this.reserved = new Int32Array(p.nmax); this.keyRev = new Float64Array(p.nmax); this.slotGeneration = new Float64Array(p.nmax); this.minEligibleId = new Float64Array(p.nmax); this.minId = new Float64Array(p.nmax); this.ids = new TxIdIndex(p); this.reset(); }
    private reset(): void { this.clear(); this.ids.clear(); this.slotLive.fill(0); this.flags.fill(0); this.size.fill(0); this.prev.fill(-1); this.next.fill(-1); this.minEligibleSlot.fill(-1); this.minIdSlot.fill(-1); }
    private group(a: number, b: number): number { const p = this.pool; for (let k = 0; k < 5; k++) {
        const x = k === 0 ? p.pPhase[a] : k === 1 ? p.pSource[a] : k === 2 ? p.pSourceId[a] : k === 3 ? p.pState[a] : Math.floor(p.pX[a]), y = k === 0 ? p.pPhase[b] : k === 1 ? p.pSource[b] : k === 2 ? p.pSourceId[b] : k === 3 ? p.pState[b] : Math.floor(p.pX[b]);
        if (x !== y)
            return x < y ? -1 : 1;
    } return 0; }
    protected compare(a: number, b: number): number { const g = this.group(a, b), p = this.pool; if (g !== 0)
        return g; if (p.pY[a] !== p.pY[b])
        return p.pY[a] < p.pY[b] ? -1 : 1; return p.pId[a] === p.pId[b] ? a - b : p.pId[a] < p.pId[b] ? -1 : 1; }
    private smaller(a: number, b: number): number { if (a < 0)
        return b; if (b < 0)
        return a; const p = this.pool; return p.pId[a] === p.pId[b] ? Math.min(a, b) : p.pId[a] < p.pId[b] ? a : b; }
    protected update(a: number): void { super.update(a); const l = this.left[a], r = this.right[a]; this.size[a] = 1 + (l < 0 ? 0 : this.size[l]) + (r < 0 ? 0 : this.size[r]); this.minIdSlot[a] = this.smaller(a, this.smaller(l < 0 ? -1 : this.minIdSlot[l], r < 0 ? -1 : this.minIdSlot[r])); this.minEligibleSlot[a] = this.smaller(this.flags[a] ? a : -1, this.smaller(l < 0 ? -1 : this.minEligibleSlot[l], r < 0 ? -1 : this.minEligibleSlot[r])); this.minId[a] = this.pool.pId[this.minIdSlot[a]]; const e = this.minEligibleSlot[a]; this.minEligibleId[a] = e < 0 ? 0 : this.pool.pId[e]; }
    private adjacent(a: number, b: number): boolean { this.neighborTests++; return b >= 0 && this.group(a, b) === 0 && Math.abs(this.pool.pY[a] - this.pool.pY[b]) <= this.radius; }
    private eligibility(a: number): void { if (a < 0)
        return; this.flags[a] = this.adjacent(a, this.prev[a]) || this.adjacent(a, this.next[a]) ? 1 : 0; this.refresh(a); }
    insertSlot(a: number): void {
        if (this.pool.pState[a] === 0)
            return;
        const p = this.pool;
        if (!Number.isFinite(p.pX[a]) || !Number.isFinite(p.pY[a]) || p.pId[a] === 0) {
            this.certified = false;
            return;
        }
        if (this.ids.lookup(p.pId[a]) >= 0)
            this.certified = false;
        let v = this.root, pr = -1, nx = -1;
        while (v >= 0) {
            this.visits++;
            if (this.compare(a, v) < 0) {
                nx = v;
                v = this.left[v];
            }
            else {
                pr = v;
                v = this.right[v];
            }
        }
        this.prev[a] = pr;
        this.next[a] = nx;
        if (pr >= 0)
            this.next[pr] = a;
        if (nx >= 0)
            this.prev[nx] = a;
        this.flags[a] = 0;
        this.slotLive[a] = 1;
        this.slotGeneration[a]++;
        this.keyRev[a] = ++this.revision;
        this.insert(a);
        this.ids.insert(a);
        this.eligibility(a);
        this.eligibility(pr);
        this.eligibility(nx);
    }
    eraseSlot(a: number): void { if (!this.slotLive[a])
        return; const pr = this.prev[a], nx = this.next[a]; if (pr >= 0)
        this.next[pr] = nx; if (nx >= 0)
        this.prev[nx] = pr; this.erase(a); this.ids.erase(a); this.slotLive[a] = 0; this.prev[a] = this.next[a] = -1; this.keyRev[a] = ++this.revision; this.eligibility(pr); this.eligibility(nx); }
    rebuild(): void { this.builds++; this.certified = Number.isFinite(this.radius) && this.radius >= 0; this.reset(); for (let i = 0; i < this.pool.nmax; i++)
        this.insertSlot(i); }
    copyIndex(from: TxPairAdjAvl): void { super.copyFrom(from); this.ids.copyFrom(from.ids); this.radius = from.radius; this.certified = from.certified; this.revision++; const aa = this.extraI(), bb = from.extraI(); for (let i = 0; i < aa.length; i++)
        aa[i].set(bb[i]); this.keyRev.set(from.keyRev); this.slotGeneration.set(from.slotGeneration); this.minEligibleId.set(from.minEligibleId); this.minId.set(from.minId); }
    private extraI(): Int32Array[] { return [this.size, this.prev, this.next, this.flags, this.minEligibleSlot, this.minIdSlot, this.slotLive, this.reserved]; }
    select(k: number): number { let v = this.root; while (v >= 0) {
        this.rankVisits++;
        const left = this.left[v], n = left < 0 ? 0 : this.size[left];
        if (k === n)
            return v;
        if (k < n)
            v = left;
        else {
            k -= n + 1;
            v = this.right[v];
        }
    } return -1; }
    private range(v: number, start: number, lo: number, hi: number): number { if (v < 0 || hi <= start || lo >= start + this.size[v])
        return -1; this.rangeVisits++; if (lo <= start && start + this.size[v] <= hi)
        return this.minIdSlot[v]; const l = this.left[v], at = start + (l < 0 ? 0 : this.size[l]); let best = at >= lo && at < hi ? v : -1; best = this.smaller(best, this.range(l, start, lo, hi)); return this.smaller(best, this.range(this.right[v], at + 1, lo, hi)); }
    find(): boolean {
        this.selectedA = this.selectedB = -1;
        if (!this.certified || this.root < 0)
            return false;
        const a = this.minEligibleSlot[this.root];
        if (a < 0)
            return false;
        const n = this.size[this.root], y = this.pool.pY[a];
        let lo = 0, hi = n;
        // Monotone bounds use original RN64 subtraction, never rounded y +/- radius.
        while (lo < hi) {
            const m = Math.floor((lo + hi) / 2), b = this.select(m), g = this.group(b, a);
            if (g < 0 || (g === 0 && this.pool.pY[b] < y && Math.abs(y - this.pool.pY[b]) > this.radius))
                lo = m + 1;
            else
                hi = m;
        }
        const begin = lo;
        lo = 0;
        hi = n;
        while (lo < hi) {
            const m = Math.floor((lo + hi) / 2), b = this.select(m), g = this.group(b, a);
            if (g > 0 || (g === 0 && this.pool.pY[b] > y && Math.abs(y - this.pool.pY[b]) > this.radius))
                hi = m;
            else
                lo = m + 1;
        }
        const end = lo;
        let at = this.left[a] < 0 ? 0 : this.size[this.left[a]], v = a;
        while (this.parent[v] >= 0) {
            const p = this.parent[v];
            if (this.right[p] === v)
                at += 1 + (this.left[p] < 0 ? 0 : this.size[this.left[p]]);
            v = p;
            this.rankVisits++;
        }
        const b = this.smaller(this.range(this.root, 0, begin, at), this.range(this.root, 0, at + 1, end));
        if (b < 0)
            return false;
        this.selectedA = a;
        this.selectedB = b;
        return true;
    }
    buffers(): ArrayBuffer[] { const b = super.buffers().concat(this.ids.buffers()); for (const a of this.extraI())
        b.push(a.buffer as ArrayBuffer); b.push(this.keyRev.buffer as ArrayBuffer, this.slotGeneration.buffer as ArrayBuffer, this.minEligibleId.buffer as ArrayBuffer, this.minId.buffer as ArrayBuffer); return b; }
}
