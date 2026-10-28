export const TX_TRACE_REVISION = 'P011_TX_WORK_TRACE_V1';
// Fixed counters and bounded detailed trace; overflow never changes physics.
export class TxWorkTrace {
    readonly counts = new Float64Array(128);
    readonly rows: Float64Array;
    readonly stack = new Int32Array(128);
    readonly context = new Float64Array(16);
    readonly wallInclusive = new Float64Array(1024);
    readonly wallExclusive = new Float64Array(1024);
    readonly wallCalls = new Float64Array(1024);
    private readonly stackData = new Float64Array(128 * 20);
    clock: () => number = (): number => Date.now();
    readonly clockDomain = 'UTC_WALL_MS';
    count = 0;
    depth = 0;
    maxDepth = 0;
    serial = 0;
    root = 0;
    incomplete = false;
    detail = false;
    constructor(capacity: number = 16384) { this.rows = new Float64Array(capacity * 32); }
    add(id: number, n: number = 1): void { const x = this.counts[id] + n; if (!Number.isSafeInteger(x)) {
        this.incomplete = true;
        return;
    } this.counts[id] = x; }
    // ENTER_SCOPE, LEAVE_SCOPE. Rows contain scope/parent/root/cut and original code.
    enter(cut: number): number { if (this.serial >= Number.MAX_SAFE_INTEGER) {
        this.incomplete = true;
        this.add(0);
        this.add(4);
        return -1;
    } const id = ++this.serial; this.add(0); this.add(4); if (this.depth >= this.stack.length) {
        this.incomplete = true;
        return -1;
    } const parent = this.depth ? this.stack[this.depth - 1] : 0; const at = this.depth * 20; this.stackData[at] = this.clock(); this.stackData[at + 1] = 0; for (let k = 0; k < 16; k++)
        this.stackData[at + 4 + k] = this.context[k]; this.stack[this.depth++] = id; this.maxDepth = Math.max(this.maxDepth, this.depth); this.emit(1, id, parent, cut, 0, this.stackData[at], NaN, NaN, NaN, at); return id; }
    leave(id: number, cut: number, code: number = 0): void { if (id < 0) {
        this.add(4, -1);
        this.add(2);
        return;
    } if (this.depth === 0 || this.stack[this.depth - 1] !== id)
        this.incomplete = true;
    else
        this.depth--; this.add(4, -1); this.add(code === 0 ? 1 : 2); const at = this.depth * 20, end = this.clock(), inclusive = end - this.stackData[at], exclusive = inclusive - this.stackData[at + 1]; if (!Number.isFinite(inclusive) || inclusive < 0 || exclusive < 0)
        this.incomplete = true; if (cut >= 0 && cut < this.wallCalls.length) {
        this.wallInclusive[cut] += inclusive;
        this.wallExclusive[cut] += exclusive;
        this.wallCalls[cut]++;
    }
    else
        this.incomplete = true; if (this.depth > 0)
        this.stackData[at - 20 + 1] += inclusive; this.emit(code === 0 ? 2 : 3, id, this.depth ? this.stack[this.depth - 1] : 0, cut, code, this.stackData[at], end, inclusive, exclusive, at); }
    private emit(kind: number, id: number, parent: number, cut: number, code: number, start: number, end: number, inclusive: number, exclusive: number, at: number): void { if (!this.detail)
        return; if (this.count * 32 >= this.rows.length) {
        this.incomplete = true;
        return;
    } const i = this.count++ * 32; this.rows[i] = kind; this.rows[i + 1] = id; this.rows[i + 2] = parent; this.rows[i + 3] = this.root; this.rows[i + 4] = cut; this.rows[i + 5] = code; this.rows[i + 6] = start; this.rows[i + 7] = end; this.rows[i + 8] = inclusive; this.rows[i + 9] = exclusive; for (let k = 0; k < 16; k++)
        this.rows[i + 16 + k] = this.stackData[at + 4 + k]; }
    cut(id: number): void { const s = this.enter(id); this.leave(s, id, 0); }
    beginRoot(): void { this.root++; }
}
// Counter dictionary: B_REQ=8 B_ENTER=9 B_COMMIT=10 M_FIND_ENTER=11
// M_FOUND=12 M_NO_PAIR=13 M_ERROR=14 MERGE_COMMIT=15 PAIR_BUILD=16
// KEY_INSERT=17 KEY_ERASE=18 RELOCATE=19 NEIGHBOR_TEST=20 RANK_SELECT=21 RANGE_NODE=22
// AUDIT_LOGICAL=23 AUDIT_EXECUTE=24 AUDIT_CERT_HIT=25 FOLD_SLOTS=26 FOLD_COLUMNS=27
// COPY_POOL_BYTES=28 COPY_FIELD_BYTES=29 UNDO_WRITES=30 FIELD_VISITS=31
// PUBLICATION_BYTES=32 ENCODE_BYTES=33 WRITE_BYTES=34 FSYNC_BYTES=35 SHA_BLOCKS=36
// LEGACY_UNCERTIFIED=37 COPY_INDEX_BYTES=38 RETAINED_LOOP_ITERATIONS=39
