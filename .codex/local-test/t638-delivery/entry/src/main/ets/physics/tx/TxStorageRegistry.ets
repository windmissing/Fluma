import { TxWordUndo } from './TxWordUndo';
class TxScalarBinding {
    readonly read: () => number;
    readonly write: (v: number) => void;
    readonly bits: DataView;
    constructor(read: () => number, write: (v: number) => void) { this.read = read; this.write = write; this.bits = new DataView(new ArrayBuffer(8)); }
}
// Cold unique-buffer registry. Whole-bank copies are raw bytes, including NaN payloads.
export class TxStorageRow {
    readonly bufferId: number;
    readonly ownerId: number;
    readonly path: string;
    readonly bytes: number;
    readonly aliasOf: number;
    readonly width: number;
    readonly offset: number;
    readonly length: number;
    constructor(id: number, owner: number, path: string, bytes: number, alias: number, width: number, offset: number, length: number) { this.bufferId = id; this.ownerId = owner; this.path = path; this.bytes = bytes; this.aliasOf = alias; this.width = width; this.offset = offset; this.length = length; }
}
export class TxStorageRegistry {
    private readonly buffers: ArrayBuffer[] = [];
    private readonly byteViews: Uint8Array[] = [];
    readonly rows: TxStorageRow[] = [];
    private readonly banks: Uint8Array[][] = [];
    private readonly scalars: TxScalarBinding[] = [];
    frozen = false;
    copiedBytes = 0;
    readonly ownerId: number;
    constructor(owner: number, buffers: ArrayBuffer[]) { this.ownerId = owner; for (let i = 0; i < buffers.length; i++)
        this.register(owner, 'original.' + i, buffers[i], 1, 0, buffers[i].byteLength); }
    register(owner: number, path: string, buffer: ArrayBuffer, width: number, offset: number, length: number): number { if (this.frozen)
        throw new Error('TX_COLD_REGISTRY_FROZEN'); if (!Number.isSafeInteger(offset) || !Number.isSafeInteger(length) || offset < 0 || length < 0 || offset + width * length > buffer.byteLength || buffer.byteLength > 0xffffffff)
        throw new Error('TX_COLD_ADDRESS_RANGE'); let id = this.buffers.indexOf(buffer), alias = id; if (id < 0) {
        id = this.buffers.length;
        this.buffers.push(buffer);
        this.byteViews.push(new Uint8Array(buffer));
    } this.rows.push(new TxStorageRow(id, owner, path, length * width, alias, width, offset, length)); return id; }
    registerScalar(owner: number, path: string, read: () => number, write: (v: number) => void): void { const b = new TxScalarBinding(read, write); this.register(owner, path, b.bits.buffer, 8, 0, 1); this.scalars.push(b); }
    freeze(depth: number = 8): void { if (this.frozen)
        return; for (let d = 0; d < depth; d++) {
        const bank: Uint8Array[] = [];
        for (const b of this.buffers)
            bank.push(new Uint8Array(b.byteLength));
        this.banks.push(bank);
    } this.frozen = true; }
    mark(depth: number): void { for (const scalar of this.scalars)
        scalar.bits.setFloat64(0, scalar.read(), true); const b = this.banks[depth]; for (let i = 0; i < b.length; i++) {
        b[i].set(this.byteViews[i]);
        this.copiedBytes += b[i].byteLength;
    } }
    rollback(depth: number): void { const b = this.banks[depth]; for (let i = 0; i < b.length; i++) {
        this.byteViews[i].set(b[i]);
        this.copiedBytes += b[i].byteLength;
    } for (const scalar of this.scalars)
        scalar.write(scalar.bits.getFloat64(0, true)); }
    coldUndoForTests(capacity: number): TxWordUndo { return new TxWordUndo(this.buffers, capacity); }
    uniqueBytes(): number { let n = 0; for (const b of this.buffers)
        n += b.byteLength; return n; }
}
