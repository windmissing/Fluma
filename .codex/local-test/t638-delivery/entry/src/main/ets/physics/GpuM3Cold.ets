import native from 'libgrsm1.so';

interface M3ColdReceipt {
  revision?: string;
  payloadRevision?: string;
  state?: string;
  requestId?: string;
  traceId?: string;
  page?: number;
  pages?: number;
  actualWords?: number;
  physicalStateWords?: number;
  quotaWordOffset?: number;
  controlWordOffset?: number;
  inputWordOffset?: number;
  eventsWordOffset?: number;
  branchMetaWordOffset?: number;
  branchControlWordOffset?: number;
  branchEventsWordOffset?: number;
  branchScopeRows?: number;
  words?: number[];
}

// One page at a time; caller writes each part before requesting another.
// The existing 4MiB serialized output credit is retained. A larger V4 trace
// returns WOULD_BLOCK rather than silently growing the collector or truncating.
export class GpuM3Cold {
  state: string = 'IDLE';
  private identity: number[] = [];
  private full: boolean = false;
  private request: string = '0';
  private trace: string = '0';
  private page: number = 0;
  private pages: number = 0;
  private actualWords: number = 0;
  private remaining: number = 4 * 1024 * 1024;

  private limbs(decimal: string): number[] {
    if (!/^(0|[1-9][0-9]{0,19})$/.test(decimal)) return [];
    let lo: number = 0;
    let hi: number = 0;
    for (let i: number = 0; i < decimal.length; i++) {
      const expanded: number = lo * 10 + Number(decimal.charAt(i));
      hi = hi * 10 + Math.floor(expanded / 4294967296);
      if (hi > 4294967295) return [];
      lo = expanded % 4294967296;
    }
    return [lo, hi];
  }

  private args(id: string, page: number = -1): Uint32Array {
    const pair: number[] = this.limbs(id);
    if (this.identity.length !== 18 || pair.length !== 2) throw new Error('M3_COLD_IDENTITY');
    const words = new Uint32Array(page < 0 ? 20 : 22);
    words.set(this.identity);
    words[18] = pair[0];
    words[19] = pair[1];
    if (page >= 0) words[20] = page;
    return words;
  }

  begin(identity: number[], full: boolean = false): string {
    if (this.state === 'REQUESTED' || this.state === 'READING') return 'WOULD_BLOCK';
    this.identity = identity.slice();
    this.full = full;
    this.remaining = full ? 4 * 1024 * 1024 * 1024 : 4 * 1024 * 1024;
    this.page = this.pages = this.actualWords = 0;
    const raw: string = native.m3ColdBegin(this.args('0'));
    const receipt = JSON.parse(raw) as M3ColdReceipt;
    if (receipt.revision !== 'GRS_M3_BOUNDED_COLD_V1' || receipt.state !== 'REQUESTED' ||
      receipt.requestId === undefined || receipt.traceId === undefined) {
      this.state = receipt.state ?? 'M3_COLD_SCHEMA_REJECTED';
      return this.state;
    }
    this.request = receipt.requestId;
    this.trace = receipt.traceId;
    this.state = 'REQUESTED';
    return this.state;
  }

  next(write: (part: string) => void): string {
    if (this.state === 'REQUESTED') {
      const raw: string = native.m3ColdState(this.args(this.request));
      const receipt = JSON.parse(raw) as M3ColdReceipt;
      if (receipt.revision !== 'GRS_M3_BOUNDED_COLD_V1') return this.finish('M3_COLD_SCHEMA_REJECTED');
      if (receipt.state === 'REQUESTED' || receipt.state === 'BUILDING') return this.state;
      if (receipt.state !== 'TRACE_READY') return this.finish(receipt.state ?? 'INCOMPLETE');
      this.state = 'READING';
    }
    if (this.state !== 'READING') return this.state;
    const raw: string = native.m3ColdPage(this.args(this.trace, this.page));
    if (raw.length > 16384 || raw.length > this.remaining) return this.finish('WOULD_BLOCK');
    for (let i: number = 0; i < raw.length; i++) if (raw.charCodeAt(i) > 127) return this.finish('M3_COLD_ASCII_REJECTED');
    const receipt = JSON.parse(raw) as M3ColdReceipt;
    if (receipt.revision !== 'GRS_M3_BOUNDED_COLD_V1' ||
      receipt.payloadRevision !== (this.full ? 'GRS_M3_FULL_BRANCH_V1' : 'GRS_M3_TRACE_V4_QUOTA3_CONTROL128') ||
      receipt.state !== 'TRACE_READY' || receipt.traceId !== this.trace || receipt.page !== this.page ||
      receipt.pages === undefined || !Number.isSafeInteger(receipt.pages) || receipt.pages < 1 ||
      receipt.actualWords === undefined || !Number.isSafeInteger(receipt.actualWords) ||
      receipt.words === undefined) return this.finish('M3_COLD_SCHEMA_REJECTED');
    if (this.page === 0) {
      if ((receipt.pages + 1) * 16384 > this.remaining) return this.finish('WOULD_BLOCK');
      this.pages = receipt.pages;
      this.actualWords = receipt.actualWords;
      const physical: number = receipt.physicalStateWords ?? 0;
      const rows: number = receipt.branchScopeRows ?? 0;
      const extra: number = this.full ? 8224 + 20 * rows * 64 : 0;
      if (this.full && (rows !== 10622 && rows !== 11646 && rows !== 25982 && rows !== 27006)) return this.finish('M3_COLD_LAYOUT_REJECTED');
      if (this.full && (receipt.branchMetaWordOffset !== physical + 3104 || receipt.branchControlWordOffset !== physical + 3136 || receipt.branchEventsWordOffset !== physical + 11328)) return this.finish('M3_COLD_LAYOUT_REJECTED');
      if (physical <= 0 || receipt.quotaWordOffset !== 256 + physical ||
        receipt.controlWordOffset !== 512 + physical || receipt.inputWordOffset !== 2560 + physical ||
        receipt.eventsWordOffset !== 3104 + physical + extra || receipt.words.length !== 0 ||
        receipt.actualWords < 3104 + physical || receipt.actualWords > (this.full ? 41537856 : 6961952) ||
        receipt.pages !== Math.ceil(receipt.actualWords / 1024)) return this.finish('M3_COLD_LAYOUT_REJECTED');
    } else {
      const expected: number = Math.min(1024, this.actualWords - (this.page - 1) * 1024);
      if (receipt.pages !== this.pages || receipt.actualWords !== this.actualWords ||
        receipt.words.length !== expected) return this.finish('INCOMPLETE');
      for (const word of receipt.words) {
        if (!Number.isSafeInteger(word) || word < 0 || word > 4294967295) return this.finish('M3_COLD_WORD_REJECTED');
      }
    }
    write(raw + '\n');
    this.remaining -= raw.length + 1;
    if (this.page++ === this.pages) return this.finish('COMPLETE');
    return this.state;
  }

  cancel(): string { return this.finish('CANCELLED'); }

  private finish(state: string): string {
    if (this.trace !== '0') native.m3ColdEnd(this.args(this.trace));
    this.trace = this.request = '0';
    this.state = state;
    return state;
  }
}
