import { P011ParticleHotWorkspace } from '../P011ParticleHot';
class DerivedProjectionRowV2 {
    readonly acc = new Uint16Array(256 * 70);
    readonly count = new Uint32Array(256);
    readonly cache = new Float64Array(256);
    readonly field = new Float64Array(8 * 128);
    readonly dirty = new Uint8Array(128);
    readonly header = new Float64Array(32);
}
export class DerivedProjectionV2 {
    private readonly rows: DerivedProjectionRowV2[] = [];
    private readonly scratch = new DerivedProjectionRowV2();
    constructor() {
        for (let i = 0; i < 8; i++)
            this.rows.push(new DerivedProjectionRowV2());
    }
    private write(row: DerivedProjectionRowV2, w: P011ParticleHotWorkspace): boolean {
        const d = w.derivedV2, f = w.field;
        if (d === null || f === null)
            return false;
        if (d.flush() !== 0)
            return false;
        const v = d.canonical;
        if (v === null || !v.bindingValid())
            return false;
        row.dirty.set(v.dirty);
        row.acc.set(v.acc);
        row.count.set(v.count);
        row.cache.set(v.cacheH);
        row.cache.set(v.cacheL, 128);
        row.field.set(f.waterTop);
        row.field.set(f.bh, 128);
        row.field.set(f.bl, 256);
        row.field.set(f.mh, 384);
        row.field.set(f.ml, 512);
        row.field.set(f.suspH, 640);
        row.field.set(f.suspL, 768);
        row.field.set(f.surface, 896);
        row.header[0] = w.ownerId;
        row.header[1] = w.runToken;
        row.header[2] = w.paramsRevision;
        row.header[3] = w.inputRevision;
        row.header[4] = v.branch;
        row.header[5] = d.contentSerial;
        row.header[6] = v.viewId;
        row.header[7] = 2;
        row.header[8] = d.root;
        row.header[9] = w.substepId;
        row.header[10] = w.attemptToken;
        row.header[11] = w.stepEpoch;
        d.counters.add(14, row.acc.byteLength + row.count.byteLength + row.cache.byteLength + row.field.byteLength + row.header.byteLength + row.dirty.byteLength);
        return true;
    }
    capture(i: number, w: P011ParticleHotWorkspace): void {
        if (!this.write(this.rows[i], w))
            throw new Error('DERIVED_PROJECTION_INVALID');
    }
    consistent(i: number, w: P011ParticleHotWorkspace): boolean {
        if (!this.write(this.scratch, w))
            return false;
        const a = this.rows[i], b = this.scratch;
        for (let k = 0; k < a.acc.length; k++)
            if (a.acc[k] !== b.acc[k])
                return false;
        for (let k = 0; k < a.count.length; k++)
            if (a.count[k] !== b.count[k])
                return false;
        for (let k = 0; k < a.cache.length; k++)
            if (!same(a.cache[k], b.cache[k]))
                return false;
        for (let k = 0; k < a.field.length; k++)
            if (!same(a.field[k], b.field[k]))
                return false;
        for (let k = 0; k < 4; k++)
            if (a.header[k] !== b.header[k])
                return false;
        for (let c = 0; c < 128; c++)
            if (a.dirty[c] !== 0 || b.dirty[c] !== 0)
                return false;
        return a.header[7] === 2 && b.header[7] === 2 && a.header[8] === b.header[8] && a.header[9] === b.header[9] && a.header[4] > 0 && b.header[4] > 0;
    }
}
function same(a: number, b: number): boolean { return a === b && (a !== 0 || 1 / a === 1 / b); }
