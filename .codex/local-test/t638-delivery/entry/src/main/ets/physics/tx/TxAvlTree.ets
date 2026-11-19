// Fixed-capacity node-per-row AVL. Rotations never move a physical row/key.
export class TxAvlTree {
    readonly left: Int32Array;
    readonly right: Int32Array;
    readonly parent: Int32Array;
    readonly height: Int32Array;
    root = -1;
    visits = 0;
    rotations = 0;
    inserts = 0;
    erases = 0;
    constructor(n: number) { this.left = new Int32Array(n); this.right = new Int32Array(n); this.parent = new Int32Array(n); this.height = new Int32Array(n); this.clear(); }
    clear(): void { this.root = -1; this.left.fill(-1); this.right.fill(-1); this.parent.fill(-1); this.height.fill(0); }
    protected compare(a: number, b: number): number { return a - b; }
    protected h(a: number): number { return a < 0 ? 0 : this.height[a]; }
    protected update(a: number): void { this.height[a] = 1 + Math.max(this.h(this.left[a]), this.h(this.right[a])); }
    private replace(a: number, b: number): void { const p = this.parent[a]; if (p < 0)
        this.root = b;
    else if (this.left[p] === a)
        this.left[p] = b;
    else
        this.right[p] = b; if (b >= 0)
        this.parent[b] = p; }
    private rotate(a: number, left: boolean): number { const b = left ? this.right[a] : this.left[a], middle = left ? this.left[b] : this.right[b]; this.replace(a, b); if (left) {
        this.left[b] = a;
        this.right[a] = middle;
    }
    else {
        this.right[b] = a;
        this.left[a] = middle;
    } if (middle >= 0)
        this.parent[middle] = a; this.parent[a] = b; this.update(a); this.update(b); this.rotations++; return b; }
    protected refresh(a: number): void { while (a >= 0) {
        this.visits++;
        this.update(a);
        const balance = this.h(this.left[a]) - this.h(this.right[a]);
        if (balance > 1) {
            const l = this.left[a];
            if (this.h(this.left[l]) < this.h(this.right[l]))
                this.rotate(l, true);
            a = this.rotate(a, false);
        }
        else if (balance < -1) {
            const rr = this.right[a];
            if (this.h(this.right[rr]) < this.h(this.left[rr]))
                this.rotate(rr, false);
            a = this.rotate(a, true);
        }
        a = this.parent[a];
    } }
    insert(a: number): void { this.inserts++; this.left[a] = this.right[a] = -1; this.height[a] = 1; let p = -1, v = this.root; while (v >= 0) {
        this.visits++;
        p = v;
        v = this.compare(a, v) < 0 ? this.left[v] : this.right[v];
    } this.parent[a] = p; if (p < 0)
        this.root = a;
    else if (this.compare(a, p) < 0)
        this.left[p] = a;
    else
        this.right[p] = a; this.refresh(a); }
    erase(a: number): void { this.erases++; while (this.left[a] >= 0 && this.right[a] >= 0) {
        this.rotate(a, this.h(this.right[a]) >= this.h(this.left[a]));
    } const p = this.parent[a], child = this.left[a] >= 0 ? this.left[a] : this.right[a]; this.replace(a, child); this.left[a] = this.right[a] = this.parent[a] = -1; this.height[a] = 0; this.refresh(p); }
    copyFrom(a: TxAvlTree): void { this.root = a.root; this.left.set(a.left); this.right.set(a.right); this.parent.set(a.parent); this.height.set(a.height); }
    buffers(): ArrayBuffer[] { return [this.left.buffer, this.right.buffer, this.parent.buffer, this.height.buffer]; }
}
