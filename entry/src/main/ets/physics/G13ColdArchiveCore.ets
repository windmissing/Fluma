import native from 'libgrs_g13.so';
import fs from '@ohos.file.fs';
import crypto from '@ohos.security.cryptoFramework';
import util from '@ohos.util';
interface G13Begin { route: number; schema: number; state: string; id: string; rawBytes: string; segments: number; createdNativeNs: string; }
interface G13Page { route: number; schema: number; state: string; id: string; segment: number; wordOffset: string; rawByteOffset: string; words: number[]; }
export class G13ColdArchiveCore {
  private busy: boolean = false;
  private cancelled: boolean = false;
  cancel(): void { this.cancelled = true; }
  private hex(bytes: Uint8Array): string { let value: string = ''; for (const b of bytes) value += b.toString(16).padStart(2, '0'); return value; }
  async collect(commit: string, directory: string): Promise<string> {
    if (this.busy) throw new Error('WOULD_BLOCK'); this.busy = true; this.cancelled = false;
    let file: fs.File | null = null; let id: string = ''; let path: string = ''; let firstError: string = '';
    try {
      const receiptRaw: string = native.g13ColdBegin(commit, 'FULL'); const begin = JSON.parse(receiptRaw) as G13Begin;
      if (begin.route !== 13 || begin.schema !== 1 || (begin.state !== 'READY' && begin.state !== 'REQUESTED')) throw new Error(receiptRaw);
      id = begin.id; let firstRaw: string = ''; let firstPage: G13Page | null = null;
      for (let poll: number = 0; poll < 6000; poll++) {
        if (this.cancelled) throw new Error('CANCELLED');
        const raw: string = native.g13ColdPage(id, 0, 0); const response = JSON.parse(raw) as G13Page;
        if (response.state === 'PAGE') { firstRaw = raw; firstPage = response; break; }
        if (response.state !== 'BUILDING') throw new Error(raw);
        await new Promise<void>((resolve) => { setTimeout(resolve, 10); });
      }
      if (firstPage === null) throw new Error('COLD_WAIT_BOUND');
      const total: number = Number(begin.rawBytes); if (!Number.isSafeInteger(total) || total <= 65536 || total > 5755136) throw new Error('RAW_BOUND');
      path = directory + '/g13-' + id + '-' + begin.createdNativeNs + '.part.jsonl';
      if (await fs.access(path)) throw new Error('ARCHIVE_ALREADY_EXISTS');
      file = await fs.open(path, fs.OpenMode.CREATE | fs.OpenMode.TRUNC | fs.OpenMode.WRITE_ONLY);
      const pagesHash = crypto.createMd('SHA256'); const fileHash = crypto.createMd('SHA256'); const payloadHash = crypto.createMd('SHA256'); const prefixHash = crypto.createMd('SHA256'); const canonicalHash = crypto.createMd('SHA256');
      let expectedPrefix: string = ''; const lengths: number[] = []; let offset: number = 0; let count: number = 0; let fileBytes: number = 0;
      const pageMax: number = 1 + Math.ceil(total / 4096) + 32; const limit: number = (pageMax + 1) * 16384;
      for (let segment: number = 0; segment < begin.segments; segment++) {
        let local: number = 0;
        do {
          if (this.cancelled) throw new Error('CANCELLED');
          const raw: string = count === 0 ? firstRaw : native.g13ColdPage(id, segment, local); const line = new util.TextEncoder().encodeInto(raw + '\n');
          if (line.length > 16384 || ++count > pageMax || fileBytes + line.length > limit) throw new Error('PAGE_BOUND');
          const page = JSON.parse(raw) as G13Page; if (count === 1) { firstRaw = ''; firstPage = null; }
          if (page.route !== 13 || page.schema !== 1 || page.state !== 'PAGE' || page.id !== id || page.segment !== segment || page.wordOffset !== String(local) || page.rawByteOffset !== String(offset) || page.words.length === 0 || page.words.length > 1024) throw new Error('PAGE_IDENTITY');
          const bytes = new Uint8Array(page.words.length * 4); const view = new DataView(bytes.buffer);
          for (let i: number = 0; i < page.words.length; i++) { const value: number = page.words[i]; if (!Number.isInteger(value) || value < 0 || value > 4294967295) throw new Error('WORD_RANGE'); view.setUint32(i * 4, value, true); }
          if (offset === 0) { const digest = new Uint8Array(32); const digestView = new DataView(digest.buffer); for (let i: number = 0; i < 8; i++) digestView.setUint32(i * 4, page.words[248 + i], true); expectedPrefix = this.hex(digest); for (let i: number = 0; i < begin.segments; i++) { const at: number = 256 + 32 * i; const length: number = (page.words[at + 8] + page.words[at + 9] * 4294967296) / 4; if (!Number.isSafeInteger(length) || length <= 0) throw new Error('DIRECTORY_LENGTH'); lengths.push(length); } }
          await payloadHash.update({ data: bytes });
          if (offset < 65536) { await prefixHash.update({ data: bytes }); const mask = bytes.slice(); if (offset === 0) mask.fill(0, 248 * 4, 256 * 4); await canonicalHash.update({ data: mask }); }
          await pagesHash.update({ data: line }); await fileHash.update({ data: line });
          let written: number = 0; while (written < line.length) { const n: number = await fs.write(file.fd, line.buffer.slice(written)); if (n <= 0) throw new Error('SHORT_WRITE'); written += n; }
          offset += bytes.length; local += page.words.length; fileBytes += line.length;
          if (local > lengths[segment]) throw new Error('SEGMENT_OVERFLOW');
        } while (local < lengths[segment]);
      }
      if (offset !== total) throw new Error('PAYLOAD_LENGTH');
      const canonical: string = this.hex((await canonicalHash.digest()).data); if (canonical !== expectedPrefix) throw new Error('CANONICAL_PREFIX_SHA');
      const manifest: string = JSON.stringify({ schema: 'G13_ARCHIVE_V1', state: 'ARCHIVED_RAW_NOT_PHYSICS_PASS', id: id, beginRaw: receiptRaw, rawBytes: total, pages: count, rawPrefixSHA: this.hex((await prefixHash.digest()).data), canonicalPrefixSHA: canonical, payloadRawSHA: this.hex((await payloadHash.digest()).data), pagesFileSHA: this.hex((await pagesHash.digest()).data) }) + '\n';
      const last = new util.TextEncoder().encodeInto(manifest); if (last.length > 16384 || fileBytes + last.length > limit) throw new Error('MANIFEST_BOUND');
      let written: number = 0; while (written < last.length) { const n: number = await fs.write(file.fd, last.buffer.slice(written)); if (n <= 0) throw new Error('SHORT_MANIFEST_WRITE'); written += n; }
      await fileHash.update({ data: last }); await fs.fsync(file.fd); await fs.close(file); file = null;
      const final: string = path.replace('.part.jsonl', '.jsonl'); await fs.rename(path, final);
      const receipt = new util.TextEncoder().encodeInto(JSON.stringify({ schema: 'G13_FILE_RECEIPT_V1', file: final, sha256: this.hex((await fileHash.digest()).data) }) + '\n');
      if (await fs.access(final + '.receipt.json')) throw new Error('RECEIPT_ALREADY_EXISTS');
      const receiptFile = await fs.open(final + '.receipt.json', fs.OpenMode.CREATE | fs.OpenMode.TRUNC | fs.OpenMode.WRITE_ONLY);
      try { let at: number = 0; while (at < receipt.length) { const n: number = await fs.write(receiptFile.fd, receipt.buffer.slice(at)); if (n <= 0) throw new Error('SHORT_RECEIPT_WRITE'); at += n; } await fs.fsync(receiptFile.fd); } finally { await fs.close(receiptFile); }
      return final;
    } catch (e) {
      firstError = String(e);
      if (path !== '') {
        try { const incomplete = await fs.open(path + '.incomplete.json', fs.OpenMode.CREATE | fs.OpenMode.TRUNC | fs.OpenMode.WRITE_ONLY); try { const bytes = new util.TextEncoder().encodeInto(JSON.stringify({ schema: 'G13_ARCHIVE_FAILURE_V1', state: 'INCOMPLETE', id: id, firstError: firstError.slice(0, 1024) }) + '\n'); let at: number = 0; while (at < bytes.length) { const n: number = await fs.write(incomplete.fd, bytes.buffer.slice(at)); if (n <= 0) throw new Error('SHORT_FAILURE_WRITE'); at += n; } await fs.fsync(incomplete.fd); } finally { await fs.close(incomplete); } } catch (ignored) { /* Preserve the original IO/identity failure. */ }
      }
      throw new Error('INCOMPLETE:' + firstError);
    }
    finally { try { if (file !== null) await fs.close(file); } finally { if (id !== '') native.g13ColdEnd(id); this.busy = false; } }
  }
}
