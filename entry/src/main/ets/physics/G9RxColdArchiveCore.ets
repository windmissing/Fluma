import native from 'libgrs_g9rx.so';
import fs from '@ohos.file.fs';
import crypto from '@ohos.security.cryptoFramework';
import util from '@ohos.util';
interface RxColdBegin { state: string; request: string; trace: string; }
interface RxColdState { state: string; trace: string; pages: number; words: number; capability: string; journal: number[]; }
interface RxColdPage { page: string; trace: string; index: number; words: number[]; }
export class G9RxColdArchiveCore {
  private cancelled: boolean = false;
  private busy: boolean = false;
  cancel(): void { this.cancelled = true; }
  private hex(bytes: Uint8Array): string { let out: string = ''; for (const b of bytes) out += b.toString(16).padStart(2, '0'); return out; }
  async collect(mode: string, directory: string): Promise<string> {
    if (this.busy) throw new Error('WOULD_BLOCK'); this.busy = true; this.cancelled = false;
    let file: fs.File | null = null; let trace: string = '';
    try {
      const initialStatusRaw: string = native.g9RxStatus();
      const beginRaw: string = native.g9RxColdBegin('0', mode); const begin = JSON.parse(beginRaw) as RxColdBegin;
      if (begin.state !== 'QUEUED') throw new Error(beginRaw); trace = begin.trace;
      let state: RxColdState | null = null;
      for (let i: number = 0; i < 6000; i++) {
        if (this.cancelled) throw new Error('CANCELLED');
        const raw: string = native.g9RxColdState(begin.request); const current = JSON.parse(raw) as RxColdState;
        if (current.state === 'READY') { state = current; break; }
        if (current.state !== 'PENDING') throw new Error(raw);
        await new Promise<void>((resolve) => { setTimeout(resolve, 10); });
      }
      if (state === null || state.capability !== 'G9_DM_RX_COLD_V1' || state.trace !== trace || !Number.isSafeInteger(state.words) || state.words < 8192 || state.words > 47014912 || state.pages !== Math.ceil(state.words / 1024)) throw new Error('COLD_LAYOUT');
      const path: string = directory + '/g9rx-' + trace + '.part.jsonl';
      if (await fs.access(path)) throw new Error('ARCHIVE_EXISTS');
      file = await fs.open(path, fs.OpenMode.CREATE | fs.OpenMode.TRUNC | fs.OpenMode.WRITE_ONLY);
      const pagesHash = crypto.createMd('SHA256'); const payloadHash = crypto.createMd('SHA256'); const prefixHash = crypto.createMd('SHA256');
      let offset: number = 0;
      for (let i: number = 0; i < state.pages; i++) {
        if (this.cancelled) throw new Error('CANCELLED');
        const raw: string = native.g9RxColdPage(trace, i); const line = new util.TextEncoder().encodeInto(raw + '\n');
        if (line.length > 16384) throw new Error('PAGE_UTF8_LIMIT');
        const page = JSON.parse(raw) as RxColdPage;
        if (page.page !== 'G9_DM_RX_SEGMENT_PAGE_V1' || page.trace !== trace || page.index !== i || page.words.length !== Math.min(1024, state.words - offset)) throw new Error('PAGE_IDENTITY');
        const bytes = new Uint8Array(page.words.length * 4); const view = new DataView(bytes.buffer);
        for (let w: number = 0; w < page.words.length; w++) { const value: number = page.words[w]; if (!Number.isInteger(value) || value < 0 || value > 4294967295) throw new Error('U32_RANGE'); view.setUint32(w * 4, value, true); }
        await payloadHash.update({ data: bytes }); if (offset < 8192) await prefixHash.update({ data: bytes }); await pagesHash.update({ data: line });
        let written: number = 0; while (written < line.length) { const n: number = await fs.write(file.fd, line.buffer.slice(written)); if (n <= 0) throw new Error('SHORT_WRITE'); written += n; }
        offset += page.words.length;
      }
      const manifest: string = JSON.stringify({ schema: 'G9_RX_ARCHIVE_V1', state: 'RAW_ARCHIVED_NOT_PHYSICS_PASS', beginRaw: beginRaw, initialStatusRaw: initialStatusRaw, finalStatusRaw: native.g9RxStatus(), journal: state.journal, loadedIdentityRaw: native.g9RxLoadedIdentity(), trace: trace, rawBytes: offset * 4, pages: state.pages, rawPrefixSHA: this.hex((await prefixHash.digest()).data), payloadRawSHA: this.hex((await payloadHash.digest()).data), pagesFileSHA: this.hex((await pagesHash.digest()).data) }) + '\n';
      const last = new util.TextEncoder().encodeInto(manifest); if (last.length > 16384) throw new Error('MANIFEST_LIMIT'); let written: number = 0;
      while (written < last.length) { const n: number = await fs.write(file.fd, last.buffer.slice(written)); if (n <= 0) throw new Error('SHORT_MANIFEST'); written += n; }
      await fs.fsync(file.fd); await fs.close(file); file = null;
      const final: string = path.replace('.part.jsonl', '.jsonl'); await fs.rename(path, final); return final;
    } finally { try { if (file !== null) await fs.close(file); } finally { if (trace.length > 0) native.g9RxColdEnd(trace); this.busy = false; } }
  }
}
