// D2 foundation only; D1 uses WHOLE_BANK. Every write appends the previous bits.
export class TxWordUndo {
    readonly address: Uint32Array;
    readonly lo: Uint32Array;
    readonly hi: Uint32Array;
    readonly width: Uint32Array;
    readonly bufferId: Uint32Array;
    readonly reserved: Uint32Array;
    private readonly buffers: DataView[] = [];
    tail = 0;
    constructor(buffers: ArrayBuffer[], capacity: number) { this.address = new Uint32Array(capacity); this.lo = new Uint32Array(capacity); this.hi = new Uint32Array(capacity); this.width = new Uint32Array(capacity); this.bufferId = new Uint32Array(capacity); this.reserved = new Uint32Array(capacity); for (const b of buffers)
        this.buffers.push(new DataView(b)); }
    mark(): number { return this.tail; }
    beforeWrite(id: number, offset: number, width: number): void { if (this.tail >= this.address.length)
        throw new Error('TX_UNDO_TEST_CAPACITY'); if (width !== 1 && width !== 4 && width !== 8)
        throw new Error('TX_UNDO_WIDTH'); const d = this.buffers[id], i = this.tail++; this.bufferId[i] = id; this.address[i] = offset; this.width[i] = width; this.lo[i] = width === 1 ? d.getUint8(offset) : d.getUint32(offset, true); this.hi[i] = width === 8 ? d.getUint32(offset + 4, true) : 0; }
    rollback(mark: number): void { while (this.tail > mark) {
        const i = --this.tail, d = this.buffers[this.bufferId[i]], offset = this.address[i];
        if (this.width[i] === 1)
            d.setUint8(offset, this.lo[i]);
        else {
            d.setUint32(offset, this.lo[i], true);
            if (this.width[i] === 8)
                d.setUint32(offset + 4, this.hi[i], true);
        }
    } }
    commitRoot(): void { this.tail = 0; }
}
