import { TxWorkTrace } from './TxWorkTrace';
// Cold developer export, not a replacement endpoint/renderer protocol.
export class TxCapture {
    readonly revision = 'P011_TX_CAPTURE_V1';
    readonly traceRevision = 'P011_TX_WORK_TRACE_V1';
    readonly counters: Float64Array;
    readonly scopes: Float64Array;
    readonly wallInclusive: Float64Array;
    readonly wallExclusive: Float64Array;
    readonly wallCalls: Float64Array;
    readonly clockDomain: string;
    readonly detailStatus: string;
    readonly incomplete: boolean;
    readonly maxDepth: number;
    readonly physicalStatus: string;
    readonly archiveStatus: string;
    constructor(t: TxWorkTrace, physical: string, archive: string) { this.counters = t.counts.slice(); this.wallInclusive = t.wallInclusive.slice(); this.wallExclusive = t.wallExclusive.slice(); this.wallCalls = t.wallCalls.slice(); this.clockDomain = t.clockDomain; this.scopes = t.rows.slice(0, t.count * 32); this.detailStatus = t.detail ? 'ENABLED' : 'DISABLED'; this.incomplete = t.incomplete; this.maxDepth = t.maxDepth; this.physicalStatus = physical; this.archiveStatus = archive; }
}
