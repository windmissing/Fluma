import { DerivedViewV2 } from './P011DerivedViewV2';
import { DerivedStatus, DerivedCountersV2 } from './P011DerivedTypesV2';
export interface DerivedFieldV2 {
    capacity: number;
    waterTop: Float64Array;
    bh: Float64Array;
    bl: Float64Array;
    mh: Float64Array;
    ml: Float64Array;
    suspH: Float64Array;
    suspL: Float64Array;
    surface: Float64Array;
}
export class DryDomainV2 {
    readonly field: DerivedFieldV2;
    readonly current: DerivedViewV2;
    readonly dirty = new Uint8Array(128);
    readonly counters: DerivedCountersV2;
    revision = 0;
    constructor(field: DerivedFieldV2, current: DerivedViewV2, counters: DerivedCountersV2) { this.field = field; this.current = current; this.counters = counters; this.dirty.fill(1); }
    mark(column: number = -1): void {
        if (column < 0)
            this.dirty.fill(1);
        else if (column < 128)
            this.dirty[column] = 1;
        this.revision++;
    }
    flushBeforeRead(column: number = -1): DerivedStatus {
        const f = this.field, v = this.current;
        for (let c = column < 0 ? 0 : column; c < (column < 0 ? 128 : column + 1); c++) {
            if (!this.dirty[c] && !v.dirty[c])
                continue;
            this.counters.add(6);
            this.counters.add(7);
            const bh = f.bh[c], bl = f.bl[c], mh = f.mh[c], ml = f.ml[c], h = v.cacheH[c], l = v.cacheL[c];
            if (!Number.isFinite(bh) || !Number.isFinite(bl) || !Number.isFinite(mh) || !Number.isFinite(ml) || bh < 0 || bl < 0 || mh < 0 || ml < 0 || f.suspH[c] !== 0 || f.suspL[c] !== 0 || !Number.isFinite(h) || !Number.isFinite(l) || h < 0 || l < 0)
                return DerivedStatus.DRY_DOMAIN_INFEASIBLE;
            const mix = bh + bl, top = (mix + mh) + ml, P = h + l, envelope = ((top + f.suspH[c]) + f.suspL[c]) + P;
            if (!Number.isFinite(envelope) || envelope > f.capacity || !(bh <= mix && mix <= top && top <= envelope))
                return DerivedStatus.DRY_DOMAIN_INFEASIBLE;
            f.surface[c] = top;
            f.waterTop[c] = envelope;
            this.dirty[c] = 0;
            v.dirty[c] = 0;
        }
        return DerivedStatus.OK;
    }
    verifyStrict(): DerivedStatus {
        const f = this.field, v = this.current;
        for (let c = 0; c < 128; c++) {
            this.counters.add(7);
            const bh = f.bh[c], bl = f.bl[c], mh = f.mh[c], ml = f.ml[c];
            if (!Number.isFinite(bh) || !Number.isFinite(bl) || !Number.isFinite(mh) || !Number.isFinite(ml) || bh < 0 || bl < 0 || mh < 0 || ml < 0 || f.suspH[c] !== 0 || f.suspL[c] !== 0)
                return DerivedStatus.DRY_DOMAIN_INFEASIBLE;
            const mix = bh + bl, top = (mix + mh) + ml, envelope = ((top + f.suspH[c]) + f.suspL[c]) + (v.cacheH[c] + v.cacheL[c]);
            if (!Number.isFinite(envelope) || envelope > f.capacity || f.surface[c] !== top || f.waterTop[c] !== envelope || !(bh <= mix && mix <= top && top <= envelope))
                return DerivedStatus.DRY_DOMAIN_INFEASIBLE;
        }
        return DerivedStatus.OK;
    }
}
