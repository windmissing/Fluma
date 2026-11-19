// D1 retains original field visits/folds; dirty columns do not suppress work.
export class TxFieldViews {
    readonly dirty: Uint8Array;
    readonly revision: Float64Array;
    constructor(cols: number) { this.dirty = new Uint8Array(cols); this.revision = new Float64Array(cols); }
    write(column: number): void { this.dirty[column] = 1; this.revision[column]++; }
}
