import { DerivedStatus, DerivedCountersV2 } from './P011DerivedTypesV2';
// T214 RN70: production code has no BigInt, decimal conversion or hot allocation.
export class Exact70 {
    readonly counters: DerivedCountersV2;
    constructor(counters: DerivedCountersV2 = new DerivedCountersV2()) { this.counters = counters; }
    encodeMassBits(lo: number, hi: number, out: Uint16Array, offset: number = 0): DerivedStatus {
        if (!Number.isInteger(lo) || lo < 0 || lo > 0xffffffff || !Number.isInteger(hi) || hi < 0 || hi > 0xffffffff || !Number.isInteger(offset) || offset < 0 || offset + 70 > out.length)
            return DerivedStatus.INPUT_INVALID;
        out.fill(0, offset, offset + 70);
        const sign = hi >>> 31, E = (hi >>> 20) & 2047, fractionHi = hi & 0xfffff;
        if (E === 2047 || (sign !== 0 && (lo !== 0 || fractionHi !== 0 || E !== 0)))
            return DerivedStatus.INPUT_INVALID;
        if (E > 1031 || (E === 1031 && (fractionHi !== 0 || lo !== 0)))
            return DerivedStatus.INPUT_INVALID;
        const shift = E === 0 ? 0 : E - 1, word = Math.floor(shift / 16), rem = shift % 16;
        let carry = 0;
        for (let j = 0; j < 4; j++) {
            const digit = j === 0 ? lo & 65535 : j === 1 ? lo >>> 16 : j === 2 ? fractionHi & 65535 : Math.floor(fractionHi / 65536) + (E === 0 ? 0 : 16);
            const n = digit * Math.pow(2, rem) + carry, k = word + j;
            this.counters.add(3);
            if (k < 70)
                out[offset + k] = n % 65536;
            else if (n !== 0)
                return DerivedStatus.ACC_OVERFLOW;
            carry = Math.floor(n / 65536);
        }
        if (carry !== 0) {
            if (word + 4 >= 70)
                return DerivedStatus.ACC_OVERFLOW;
            out[offset + word + 4] = carry;
        }
        return DerivedStatus.OK;
    }
    addAt(acc: Uint16Array, offset: number, encoded: Uint16Array): DerivedStatus {
        if (!Number.isInteger(offset) || offset < 0 || offset + 70 > acc.length || encoded.length !== 70)
            return DerivedStatus.INPUT_INVALID;
        let carry = 0;
        for (let j = 0; j < 70; j++) {
            this.counters.add(3);
            carry = Math.floor((acc[offset + j] + encoded[j] + carry) / 65536);
        }
        if (carry !== 0)
            return DerivedStatus.ACC_OVERFLOW;
        carry = 0;
        for (let j = 0; j < 70; j++) {
            this.counters.add(3);
            const n = acc[offset + j] + encoded[j] + carry;
            acc[offset + j] = n % 65536;
            carry = Math.floor(n / 65536);
        }
        return DerivedStatus.OK;
    }
    subtractAt(acc: Uint16Array, offset: number, encoded: Uint16Array): DerivedStatus {
        if (!Number.isInteger(offset) || offset < 0 || offset + 70 > acc.length || encoded.length !== 70)
            return DerivedStatus.INPUT_INVALID;
        let cmp = 0;
        for (let j = 69; j >= 0; j--) {
            this.counters.add(3);
            if (acc[offset + j] !== encoded[j]) {
                cmp = acc[offset + j] < encoded[j] ? -1 : 1;
                break;
            }
        }
        if (cmp < 0)
            return DerivedStatus.ACC_NEGATIVE;
        let borrow = 0;
        for (let j = 0; j < 70; j++) {
            this.counters.add(3);
            let n = acc[offset + j] - encoded[j] - borrow;
            borrow = n < 0 ? 1 : 0;
            if (n < 0)
                n += 65536;
            acc[offset + j] = n;
        }
        return DerivedStatus.OK;
    }
    private bit(a: Uint16Array, o: number, b: number): number { this.counters.add(3); return Math.floor(a[o + Math.floor(b / 16)] / Math.pow(2, b % 16)) % 2; }
    roundAt(a: Uint16Array, o: number, bits: DataView): DerivedStatus {
        if (!Number.isInteger(o) || o < 0 || o + 70 > a.length || bits.byteLength < 8)
            return DerivedStatus.INPUT_INVALID;
        this.counters.add(4);
        let top = 69;
        while (top >= 0) {
            this.counters.add(3);
            if (a[o + top] !== 0)
                break;
            top--;
        }
        if (top < 0) {
            bits.setUint32(0, 0, true);
            bits.setUint32(4, 0, true);
            return DerivedStatus.OK;
        }
        let digit = a[o + top], b = top * 16;
        while (digit > 0) {
            b++;
            digit = Math.floor(digit / 2);
        }
        let shift = Math.max(0, b - 53), M = 0;
        for (let k = b - 1; k >= shift; k--)
            M = M * 2 + this.bit(a, o, k);
        if (shift > 0) {
            const guard = this.bit(a, o, shift - 1);
            let sticky = false;
            for (let k = shift - 2; k >= 0; k--)
                if (this.bit(a, o, k) !== 0) {
                    sticky = true;
                    break;
                }
            if (guard !== 0 && (sticky || M % 2 !== 0))
                M++;
        }
        if (M === 9007199254740992) {
            M /= 2;
            shift++;
        }
        if (b <= 52) {
            bits.setUint32(0, M % 4294967296, true);
            bits.setUint32(4, Math.floor(M / 4294967296), true);
            return DerivedStatus.OK;
        }
        const E = shift + 1;
        if (E >= 2047)
            return DerivedStatus.ACC_OVERFLOW;
        const fraction = M - 4503599627370496;
        bits.setUint32(0, fraction % 4294967296, true);
        bits.setUint32(4, E * 1048576 + Math.floor(fraction / 4294967296), true);
        return Number.isFinite(bits.getFloat64(0, true)) ? DerivedStatus.OK : DerivedStatus.ACC_OVERFLOW;
    }
    static probe(): boolean { const d = new DataView(new ArrayBuffer(8)); d.setUint32(0, 1, true); d.setUint32(4, 0, true); const tiny = d.getFloat64(0, true); d.setFloat64(0, tiny, true); return tiny === Number.MIN_VALUE && d.getUint32(0, true) === 1 && d.getUint32(4, true) === 0 && tiny + tiny === 2 * Number.MIN_VALUE && 1 + Math.pow(2, -53) === 1; }
}
