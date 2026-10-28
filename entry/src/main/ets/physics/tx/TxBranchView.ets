import { TxStorageRegistry } from './TxStorageRegistry';
// Opaque handles contain no writable bank; the owner validates reference identity.
export class TxViewHandle {
    readonly ownerSlot: number;
    readonly ownerGeneration = 1;
    readonly viewSlot: number;
    readonly branchGeneration: number;
    private constructor(owner: number, view: number, branch: number) { this.ownerSlot = owner; this.viewSlot = view; this.branchGeneration = branch; }
    static cold(owner: number = 0, view: number = -1, branch: number = 0): TxViewHandle { return new TxViewHandle(owner, view, branch); }
}
export class TxSavepoint {
    private constructor() { }
    static cold(): TxSavepoint { return new TxSavepoint(); }
}
export class TxReadLease {
    private constructor() { }
    static cold(): TxReadLease { return new TxReadLease(); }
}
export class TxBranchView {
    private readonly views: TxViewHandle[] = [];
    private readonly points: TxSavepoint[] = [];
    private readonly leases: TxReadLease[] = [];
    private readonly active = new Uint8Array(8);
    private readonly generations = new Float64Array(8);
    private readonly pointGenerations = new Float64Array(8);
    private readonly leaseGenerations = new Float64Array(8);
    private readonly storage: TxStorageRegistry;
    depth = 0;
    contentRev = 0;
    branchGeneration = 0;
    maxDepth = 0;
    uncertified = 0;
    constructor(storage: TxStorageRegistry) { this.storage = storage; for (let i = 0; i < 8; i++) {
        this.views.push(TxViewHandle.cold());
        this.points.push(TxSavepoint.cold());
        this.leases.push(TxReadLease.cold());
    } storage.freeze(8); }
    enterRoot(): TxViewHandle | null { if (this.depth !== 0) {
        this.uncertified++;
        return null;
    } return this.fork(); }
    fork(): TxViewHandle | null { if (this.depth >= 8) {
        this.uncertified++;
        return null;
    } const i = this.depth++; this.views[i] = TxViewHandle.cold(this.storage.ownerId, i, this.branchGeneration + 1); this.active[i] = 1; this.generations[i] = ++this.branchGeneration; this.contentRev++; this.maxDepth = Math.max(this.maxDepth, this.depth); return this.views[i]; }
    private slot(view: TxViewHandle): number { const i = this.views.indexOf(view); if (i < 0 || !this.active[i])
        throw new Error('TX_FOREIGN_OR_CLOSED_VIEW'); return i; }
    mark(view: TxViewHandle): TxSavepoint { const i = this.slot(view); this.storage.mark(i); this.points[i] = TxSavepoint.cold(); this.pointGenerations[i] = this.generations[i]; return this.points[i]; }
    rollback(view: TxViewHandle, point: TxSavepoint): void { const i = this.slot(view); if (point !== this.points[i] || this.pointGenerations[i] !== this.generations[i])
        throw new Error('TX_STALE_SAVEPOINT'); this.storage.rollback(i); this.contentRev++; }
    commitChild(child: TxViewHandle, parent: TxViewHandle, point: TxSavepoint): void {
        const c=this.slot(child), p=this.slot(parent);
        if(c!==p+1 || point!==this.points[p] || this.pointGenerations[p]!==this.generations[p]) throw new Error('TX_STALE_PARENT_SAVEPOINT');
        // The parent's whole bank remains untouched, so parent rollback includes child success.
        this.close(child);
    }
    sealRead(view: TxViewHandle): TxReadLease { const i = this.slot(view); this.leases[i] = TxReadLease.cold(); this.leaseGenerations[i] = this.contentRev; return this.leases[i]; }
    validRead(view: TxViewHandle, lease: TxReadLease): boolean { const i = this.slot(view); return lease === this.leases[i] && this.leaseGenerations[i] === this.contentRev; }
    releaseRead(lease: TxReadLease): void { const i = this.leases.indexOf(lease); if (i >= 0)
        this.leaseGenerations[i] = -1; }
    close(view: TxViewHandle): void { const i = this.slot(view); if (i !== this.depth - 1)
        throw new Error('TX_CHILD_STILL_ACTIVE'); this.active[i] = 0; this.depth--; this.contentRev++; }
}
