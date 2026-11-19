import { TxWorkTrace } from './TxWorkTrace';
import { TxAvlTree } from './TxAvlTree';
class TxTicketTree extends TxAvlTree {
    readonly keys: Float64Array;
    constructor(keys: Float64Array) { super(keys.length); this.keys = keys; }
    protected compare(a: number, b: number): number { return this.keys[a] === this.keys[b] ? a - b : this.keys[a] < this.keys[b] ? -1 : 1; }
    lookup(key: number): number { let v = this.root, best = -1; while (v >= 0) {
        this.visits++;
        if (this.keys[v] === key) {
            best = v;
            v = this.left[v];
        }
        else
            v = key < this.keys[v] ? this.left[v] : this.right[v];
    } return best; }
}
export class TxTicketIndex {
    private trace: TxWorkTrace | null;
    readonly ordinals: Float64Array;
    readonly tokens: Float64Array;
    readonly ordinalTree: TxTicketTree;
    readonly tokenTree: TxTicketTree;
    readonly rowLive: Int32Array;
    readonly rowFlags: Int32Array;
    constructor(capacity: number, tokens: Float64Array, trace: TxWorkTrace | null = null) { this.trace = trace; this.ordinals = new Float64Array(capacity); this.tokens = tokens; this.ordinalTree = new TxTicketTree(this.ordinals); this.tokenTree = new TxTicketTree(tokens); this.rowLive = new Int32Array(capacity); this.rowFlags = new Int32Array(capacity); }
    clear(): void { this.ordinalTree.clear(); this.tokenTree.clear(); this.rowLive.fill(0); this.rowFlags.fill(0); }
    add(row: number, ordinal: number): void { this.ordinals[row] = ordinal; this.rowLive[row] = 1; this.ordinalTree.insert(row); this.tokenTree.insert(row); }
    lookupOrdinal(n: number): number { const v = this.ordinalTree.visits, r = this.ordinalTree.lookup(n); this.trace?.add(49, this.ordinalTree.visits - v); return r; }
    lookupToken(n: number): number { const v = this.tokenTree.visits, r = this.tokenTree.lookup(n); this.trace?.add(49, this.tokenTree.visits - v); return r; }
    // Closing retains both identity nodes. Original closed[] check remains authority.
    buffers(): ArrayBuffer[] { return this.ordinalTree.buffers().concat(this.tokenTree.buffers(), [this.ordinals.buffer, this.rowLive.buffer, this.rowFlags.buffer]); }
}
