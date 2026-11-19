// D1 never skips a numeric audit. Reserved D2 dependencies fail closed.
export class TxCertificates {
    readonly valid = new Uint8Array(64);
    readonly readMask = new Uint32Array(64 * 8);
    readonly domainRev = new Float64Array(256);
    visits = 0;
    invalidate(domain: number): void { this.domainRev[domain]++; for (let s = 0; s < 64; s++) {
        this.visits++;
        if ((this.readMask[s * 8 + (domain >>> 5)] & (1 << (domain & 31))) !== 0)
            this.valid[s] = 0;
    } }
    canReuseNumericAudit(): boolean { return false; }
}
