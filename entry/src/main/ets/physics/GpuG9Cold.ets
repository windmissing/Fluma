import native from 'libgrsm1.so';

interface G9Receipt {
  revision?: string;
  payloadRevision?: string;
  state?: string;
  requestId?: string;
  traceId?: string;
  page?: number;
  pages?: number;
  wordOffset?: number;
  capacityBytes?: string;
  fullTrialTrace?: boolean;
  K?: number;
  words?: number[];
  clockProofState?: string;
  currentClockProofSha256?: string;
  baseClockProofSha256?: string;
  currentPlatformSourceSha256?: string;
  basePlatformSourceSha256?: string;
  rawPrefixSha256?: string;
  currentTouchSha256?: string;
  baseTouchSha256?: string;
  currentTouchPresent?: boolean;
  baseTouchPresent?: boolean;
}

// Bounded one-page collector. Native owner seals hashes in the prefix; the
// offline reader verifies them. Never assemble the complete trace in ArkTS.
export class GpuG9Cold {
  private identity: number[] = [];
  private trace: string = '0';
  private request: string = '0';
  private page: number = 0;
  private pages: number = 0;
  private offset: number = 0;
  private totalWords: number = 0;
  private remaining: number = 0;
  private state: string = 'IDLE';
  private payload: string = 'G9_SPF_COLD_V1';
  private summary: G9Receipt | null = null;

  private args(id: string, page: number = -1): Uint32Array {
    if (!/^[1-9][0-9]{0,19}$/.test(id) && id !== '0') throw new Error('G9_TRACE_ID');
    let lo: number = 0;
    let hi: number = 0;
    for (let i: number = 0; i < id.length; i++) {
      const expanded: number = lo * 10 + Number(id.charAt(i));
      hi = hi * 10 + Math.floor(expanded / 4294967296);
      lo = expanded % 4294967296;
      if (hi > 4294967295) throw new Error('G9_TRACE_ID_OVERFLOW');
    }
    const words = new Uint32Array(page < 0 ? 20 : 21);
    words.set(this.identity);
    words[18] = lo;
    words[19] = hi;
    if (page >= 0) words[20] = page;
    return words;
  }

  begin(identity: number[]): string {
    if (this.trace !== '0') return 'WOULD_BLOCK';
    if (identity.length !== 18) return 'G9_STALE_SCOPE';
    this.identity = identity.slice();
    const capabilities = JSON.parse(native.g9ColdCapabilities()) as G9Receipt;
    if (capabilities.revision !== 'G9_SPF_COLD_V1' && capabilities.revision !== 'G9_SPF_COLD_V2') return 'INCOMPLETE_SCHEMA';
    this.payload = capabilities.revision; this.summary = null;
    const receipt = JSON.parse(native.g9ColdBegin(this.args('0'))) as G9Receipt;
    if (receipt.revision !== this.payload || receipt.state !== 'REQUESTED' ||
      receipt.traceId === undefined || receipt.requestId === undefined) return receipt.state ?? 'INCOMPLETE';
    this.trace = receipt.traceId;
    this.request = receipt.requestId;
    this.page = this.pages = this.offset = this.totalWords = 0;
    this.remaining = 4 * 1024 * 1024 * 1024;
    this.state = 'REQUESTED';
    return this.state;
  }

  next(write: (part: string) => void): string {
    try {
      if (this.state === 'REQUESTED') {
        const receipt = JSON.parse(native.g9ColdState(this.args(this.request))) as G9Receipt;
        if (receipt.revision !== this.payload) return this.finish('INCOMPLETE_SCHEMA');
        if (receipt.state === 'REQUESTED' || receipt.state === 'BUILDING') return this.state;
        if (receipt.state !== 'TRACE_READY') return this.finish(receipt.state ?? 'INCOMPLETE');
        this.state = 'READING';
      }
      if (this.state !== 'READING') return this.state;
      const raw: string = native.g9ColdPage(this.args(this.trace, this.page));
      if (raw.length + 1 > 16384 || raw.length + 1 > this.remaining) return this.finish('WOULD_BLOCK');
      for (let i: number = 0; i < raw.length; i++) if (raw.charCodeAt(i) > 127) return this.finish('INCOMPLETE_UTF8');
      const receipt = JSON.parse(raw) as G9Receipt;
      if (receipt.revision !== (this.payload === 'G9_SPF_COLD_V2' ? 'G9_SPF_SEGMENT_PAGE_V2' : 'G9_SPF_SEGMENT_PAGE_V1') || receipt.payloadRevision !== this.payload ||
        receipt.state !== 'TRACE_READY' || receipt.traceId !== this.trace || receipt.page !== this.page ||
        receipt.pages === undefined || !Number.isSafeInteger(receipt.pages) || receipt.pages < 9 || receipt.words === undefined) return this.finish('INCOMPLETE_SCHEMA');
      if (this.page === 0) {
        const bytes: number = Number(receipt.capacityBytes);
        if (!Number.isSafeInteger(bytes) || bytes <= 0 || bytes > 179671040 || bytes % 4 !== 0 ||
          (receipt.K !== 192 && receipt.K !== 512) || receipt.words.length !== 0 ||
          (receipt.pages + 1) * 16384 > this.remaining) return this.finish('INCOMPLETE_LAYOUT');
        if (this.payload === 'G9_SPF_COLD_V2') {
          if (typeof receipt.rawPrefixSha256 !== 'string' || !/^[0-9a-f]{64}$/.test(receipt.rawPrefixSha256) ||
            typeof receipt.currentTouchSha256 !== 'string' || !/^[0-9a-f]{64}$/.test(receipt.currentTouchSha256) ||
            typeof receipt.baseTouchSha256 !== 'string' || !/^[0-9a-f]{64}$/.test(receipt.baseTouchSha256) ||
            typeof receipt.currentTouchPresent !== 'boolean' || typeof receipt.baseTouchPresent !== 'boolean') return this.finish('INCOMPLETE_TOUCH_MANIFEST');
          this.summary = receipt;
        }
        this.totalWords = bytes / 4;
        this.pages = receipt.pages;
      } else {
        if (receipt.pages !== this.pages || receipt.wordOffset !== this.offset || receipt.words.length < 1 ||
          receipt.words.length > 1024 || this.offset + receipt.words.length > this.totalWords) return this.finish('INCOMPLETE_ORDER');
        for (const word of receipt.words) if (!Number.isInteger(word) || word < 0 || word > 4294967295) return this.finish('INCOMPLETE_WORD');
        this.offset += receipt.words.length;
      }
      write(raw + '\n');
      this.remaining -= raw.length + 1;
      if (++this.page === this.pages) {
        if (this.offset !== this.totalWords) return this.finish('INCOMPLETE_LENGTH');
        if (this.payload === 'G9_SPF_COLD_V2') {
          if (this.summary === null) return this.finish('INCOMPLETE_TOUCH_MANIFEST');
          const summary: G9Receipt = this.summary;
          const manifest: string = JSON.stringify({ revision: 'G9_COLD_ARCHIVE_MANIFEST_V2', state: 'COMPLETE', traceId: this.trace, pages: this.pages, words: this.totalWords, rawPrefixSha256: summary.rawPrefixSha256, currentTouchSha256: summary.currentTouchSha256, baseTouchSha256: summary.baseTouchSha256, currentTouchPresent: summary.currentTouchPresent, baseTouchPresent: summary.baseTouchPresent, inputDomain: 'REAL_TOUCH_R2', clockProofState: summary.clockProofState, currentClockProofSha256: summary.currentClockProofSha256, baseClockProofSha256: summary.baseClockProofSha256, currentPlatformSourceSha256: summary.currentPlatformSourceSha256, basePlatformSourceSha256: summary.basePlatformSourceSha256 });
          if (manifest.length + 1 > 16384 || manifest.length + 1 > this.remaining) return this.finish('INCOMPLETE_MANIFEST_BOUND');
          write(manifest + '\n'); this.remaining -= manifest.length + 1;
        }
        return this.finish('COMPLETE');
      }
      return this.state;
    } catch (e) { return this.finish('INCOMPLETE_EXCEPTION'); }
  }

  cancel(): string { return this.finish('CANCELLED'); }
  private finish(state: string): string {
    try { if (this.trace !== '0') native.g9ColdEnd(this.args(this.trace)); }
    finally { this.trace = this.request = '0'; this.state = state; }
    return state;
  }
}
