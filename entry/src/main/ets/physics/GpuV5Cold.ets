import native from 'libgrsm1.so';
import fs from '@ohos.file.fs';
import util from '@ohos.util';
import crypto from '@ohos.security.cryptoFramework';
interface V5ColdPage { revision: string; payloadRevision?: string; state: string; requestId: string; traceId: string; scope?: Object; mode?: number; K?: number; Kw?: number; page?: number; pages?: number; segmentIndex?: number; byteOffset?: number; stride?: number; firstRow?: number; rowCount?: number; words?: number[]; capacityBytes?: string; archiveUtf8Limit?: string; canonicalPrefixSha256?: string; rawPrefixSha256?: string; }
// Cold IO only. No whole-prefix/trace aggregation, no physics or frame loop.
export class GpuV5Cold {
  private busy: boolean = false;
  private cancelRequested: boolean = false;
  cancel(): void { this.cancelRequested = true; }
  private args(identity: number[], id: string, page: number = -1): Uint32Array {
    if (identity.length !== 18 || !/^(0|[1-9][0-9]{0,19})$/.test(id)) throw new Error('V5_COLD_IDENTITY');
    let lo: number = 0; let hi: number = 0;
    for (let i: number = 0; i < id.length; i++) { const next: number = lo * 10 + Number(id.charAt(i)); hi = hi * 10 + Math.floor(next / 4294967296); lo = next % 4294967296; if (hi > 4294967295) throw new Error('V5_TRACE_ID_OVERFLOW'); }
    const words = new Uint32Array(page < 0 ? 20 : 22); words.set(identity); words[18] = lo; words[19] = hi; if (page >= 0) words[20] = page; return words;
  }
  private parse(raw: string): V5ColdPage {
    if (!raw || raw.length + 1 > 16384) throw new Error('V5_PAGE_UTF8_BOUND');
    for (let i: number = 0; i < raw.length; i++) if (raw.charCodeAt(i) > 127) throw new Error('V5_PAGE_NOT_ASCII');
    const value: Object = JSON.parse(raw); if (value === null || typeof value !== 'object' || Array.isArray(value)) throw new Error('V5_REPLY_OBJECT'); return value as V5ColdPage;
  }
  private hex(data: Uint8Array): string { let out: string = ''; for (const byte of data) out += byte.toString(16).padStart(2, '0'); return out; }
  async collect(identity: number[], directory: string, notify: (message: string) => void): Promise<string> {
    if (this.busy) return 'WOULD_BLOCK'; this.busy = true; this.cancelRequested = false;
    const ids: number[] = identity.slice(); let trace: string = '0'; let file: fs.File | null = null; const path: string = directory + '/v5-cold-' + Date.now() + '.part.jsonl';
    let primary: string = ''; let result: string = 'INCOMPLETE'; let pendingRaw: string = ''; let createdFile: boolean = false;
    try {
      pendingRaw = native.m5ColdCapabilities(); const capability = this.parse(pendingRaw);
      if (capability.revision !== 'T527_V5_BOUNDED_COLD_V2') throw new Error('V5_CAPABILITY:' + capability.state);
      pendingRaw = native.m5ColdBegin(this.args(ids, '0')); const receipt = this.parse(pendingRaw);
      if (receipt.state !== 'REQUESTED' || receipt.revision !== capability.revision) throw new Error('V5_BEGIN:' + receipt.state); trace = receipt.traceId;
      let ready = receipt;
      while (ready.state === 'REQUESTED' || ready.state === 'BUILDING') { if (this.cancelRequested) throw new Error('CANCELLED'); await new Promise<void>((resolve: () => void): void => { setTimeout(resolve, 20); }); ready = this.parse(native.m5ColdState(this.args(ids, receipt.requestId))); }
      if (ready.state !== 'TRACE_READY') throw new Error('V5_STATE:' + ready.state);
      const summaryRaw: string = native.m5ColdPage(this.args(ids, trace, 0)); const summary = this.parse(summaryRaw);
      const pages: number = summary.pages ?? 0; const total: number = Number(summary.capacityBytes); const limit: number = (pages + 1) * 16384;
      if (!Number.isSafeInteger(pages) || pages < 2 || pages > 64076 || !Number.isSafeInteger(total) || total < 32768 || total > 262187776) throw new Error('V5_LAYOUT_BOUND');
      const scope: string = JSON.stringify(summary.scope); const prefixRaw = crypto.createMd('SHA256'); const canonical = crypto.createMd('SHA256'); const payload = crypto.createMd('SHA256'); const pagesHash = crypto.createMd('SHA256'); const fileHash = crypto.createMd('SHA256');
      file = await fs.open(path, fs.OpenMode.CREATE | fs.OpenMode.WRITE_ONLY | fs.OpenMode.TRUNC); createdFile = true;
      let offset: number = 0; let bytes: number = 0; let forensic: boolean = false;
      for (let page: number = 0; page < pages; page++) {
        if (this.cancelRequested) throw new Error('CANCELLED');
        const raw: string = page === 0 ? summaryRaw : native.m5ColdPage(this.args(ids, trace, page));
        pendingRaw = raw; const value: V5ColdPage = page === 0 ? summary : this.parse(raw);
        if (value.revision !== 'T527_SEGMENT_PAGE_V2' || value.payloadRevision !== 'GRS_M3B_LINEAGE_COLD_V5_2' || value.traceId !== trace || value.requestId !== receipt.requestId || value.mode !== summary.mode || value.K !== summary.K || value.Kw !== summary.Kw || value.pages !== pages || value.page !== page || JSON.stringify(value.scope) !== scope || !Array.isArray(value.words)) throw new Error('V5_PAGE_IDENTITY');
        if (page === 0) { if (value.words.length !== 0) throw new Error('V5_SUMMARY_WORDS'); }
        else {
          const count: number = value.words.length; const stride: number = value.stride ?? 0;
          if (count < 1 || count > 1024 || stride < 1 || count % stride !== 0 || value.rowCount !== count / stride || value.byteOffset !== offset * 4 || offset * 4 + count * 4 > total) throw new Error('V5_WHOLE_ROW_ORDER');
          const data = new Uint8Array(count * 4); const view = new DataView(data.buffer);
          for (let i: number = 0; i < count; i++) { const word: number = value.words[i]; if (!Number.isSafeInteger(word) || word < 0 || word > 4294967295) throw new Error('V5_WORD'); view.setUint32(4 * i, word, true); }
          if (offset === 0) { if (value.words[0] !== 826488150 || value.words[1] !== 2 || value.words[15] !== 1) throw new Error('UNSUPPORTED_V5_DIGEST_POLICY'); forensic = (value.words[12] & 1) !== 0; }
          await payload.update({ data: data });
          if (offset < 8192) { if (offset + count > 8192) throw new Error('V5_PREFIX_ROW'); await prefixRaw.update({ data: data }); const masked = new Uint8Array(data); for (let i: number = 0; i < count; i++) if (offset + i >= 279 && offset + i < 287) for (let j: number = 0; j < 4; j++) masked[4 * i + j] = 0; await canonical.update({ data: masked }); }
          offset += count;
        }
        const line = new util.TextEncoder().encodeInto(raw + '\n'); if (line.length > 16384 || bytes + line.length > limit) throw new Error('V5_ARCHIVE_UTF8_BOUND');
        let written: number = 0; while (written < line.length) { const n: number = await fs.write(file.fd, line.buffer.slice(written)); if (n <= 0) throw new Error('V5_SHORT_WRITE'); written += n; }
        await fs.fsync(file.fd); await pagesHash.update({ data: line }); await fileHash.update({ data: line }); pendingRaw = ''; bytes += line.length; if (page % 128 === 0) notify('V5 ' + page + '/' + pages);
      }
      if (offset * 4 !== total) throw new Error('V5_PAYLOAD_LENGTH');
      const rawPrefix: string = this.hex((await prefixRaw.digest()).data); const prefix: string = this.hex((await canonical.digest()).data);
      if (prefix !== summary.canonicalPrefixSha256 || rawPrefix !== summary.rawPrefixSha256) throw new Error('V5_PREFIX_SHA');
      const manifest: string = JSON.stringify({ schema: 'T535_V5_ARCHIVE_MANIFEST_V2', state: forensic ? 'FORENSIC_INCOMPLETE' : 'COMPLETE', requestId: receipt.requestId, traceId: trace, scope: summary.scope, mode: summary.mode, K: summary.K, Kw: summary.Kw, pages: pages, payloadBytes: total, pagesFileBytes: bytes, canonicalPrefixSha256: prefix, rawPrefixSha256: rawPrefix, payloadRawSha256: this.hex((await payload.digest()).data), pagesFileSha256: this.hex((await pagesHash.digest()).data), packageReceipt: 'EXTERNAL_SIGNED_RECEIPT_REQUIRED' }) + '\n';
      const last = new util.TextEncoder().encodeInto(manifest); if (last.length > 16384 || bytes + last.length > limit) throw new Error('V5_MANIFEST_BOUND');
      let done: number = 0; while (done < last.length) { const n: number = await fs.write(file.fd, last.buffer.slice(done)); if (n <= 0) throw new Error('V5_MANIFEST_SHORT_WRITE'); done += n; }
      await fs.fsync(file.fd); await fileHash.update({ data: last }); const finalSha: string = this.hex((await fileHash.digest()).data); await fs.close(file.fd); file = null; const destination: string = path.replace('.part.jsonl', '.jsonl'); await fs.rename(path, destination); result = (forensic ? 'FORENSIC_INCOMPLETE ' : '') + destination + ' sha256=' + finalSha;
    } catch (error) {
      primary = 'INCOMPLETE:' + String(error); result = primary;
      try {
        if (!createdFile) {
          const failed = new util.TextEncoder().encodeInto(JSON.stringify({ schema: 'T535_V5_ARCHIVE_MANIFEST_V2', state: 'INCOMPLETE', reason: primary, rejectedRaw: pendingRaw }) + '\n');
          if (failed.length <= 16384) { file = await fs.open(path, fs.OpenMode.CREATE | fs.OpenMode.WRITE_ONLY | fs.OpenMode.TRUNC); let at: number = 0; while (at < failed.length) { const n: number = await fs.write(file.fd, failed.buffer.slice(at)); if (n <= 0 || n > failed.length - at) throw new Error('V5_ERROR_SHORT_WRITE'); at += n; } await fs.fsync(file.fd); createdFile = true; }
        }
        if (createdFile) result += ' ' + path;
      } catch (io) { result += ';ERROR_STORE_FAILED:' + String(io); }
    }
    finally {
      try { if (trace !== '0') native.m5ColdEnd(this.args(ids, trace)); } catch (cleanup) { result += ';END_FAILED:' + String(cleanup); }
      try { if (file !== null) await fs.close(file.fd); } catch (cleanup) { result += ';CLOSE_FAILED:' + String(cleanup); }
      this.busy = false; notify(result);
    }
    return result;
  }
}
