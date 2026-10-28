import { TxAvlTree } from './TxAvlTree';
import { P011ParticleSnapshot } from '../P011ParticleField';
// Duplicate IDs retain first physical slot; no new validation or early failure.
export class TxIdIndex extends TxAvlTree {
    private pool: P011ParticleSnapshot;
    constructor(p: P011ParticleSnapshot) { super(p.nmax); this.pool = p; }
    protected compare(a: number, b: number): number { const x = this.pool.pId[a], y = this.pool.pId[b]; return x === y ? a - b : x < y ? -1 : 1; }
    lookup(id: number): number { let v = this.root, best = -1; while (v >= 0) {
        this.visits++;
        const key = this.pool.pId[v];
        if (key === id) {
            best = v;
            v = this.left[v];
        }
        else
            v = id < key ? this.left[v] : this.right[v];
    } return best; }
}
