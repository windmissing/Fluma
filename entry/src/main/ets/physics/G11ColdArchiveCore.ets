import native from 'libgrs_g11.so';
import fs from '@ohos.file.fs';
import crypto from '@ohos.security.cryptoFramework';
import util from '@ohos.util';
interface G11Segment { id: number; name: string; byteOffset: number; bytes: number; strideWords: number; sha256: string; }
interface G11ColdManifest { revision: string; state: string; traceId: string; requestId: string; createdNs: string; bytes: string; headerWords: number; dataSHA256: string; segments: G11Segment[]; }
interface G11SeedSegment { kind: number; preimageOffset: number; bytes: number; sha256: string; }
interface G11SeedManifest { schema: string; state: number; id: string; rawBytes: string; actualVisualSeedSHA: string; segments: G11SeedSegment[]; }
interface G11WitnessManifest { revision: string; state: string; id: string; rootNonce: string; bytes: number; sha256: string; }
interface G11Page { revision: string; state: string; traceId: string; id: string; segment: number; wordOffset: number; byteOffset: number; bytes: number; stride: number; words: number[]; }
// Streaming reader only. No physics oracle, GPU mutation, full payload mirror or UI file IO.
export class G11ColdArchiveCore {
 private busy: boolean = false;
 private cancelled: boolean = false;
 cancel(): void { this.cancelled = true; }
 private hex(bytes: Uint8Array): string { let s: string = ''; for (const b of bytes) s += b.toString(16).padStart(2, '0'); return s; }
 private rawWords(raw: string): G11Page {
  if (new util.TextEncoder().encodeInto(raw).length > 16384) throw new Error('PAGE_UTF8_BOUND');
  const page = JSON.parse(raw) as G11Page;
  if (page.words === undefined || page.words.length === 0 || page.words.length > 1024) throw new Error('PAGE_WORD_BOUND');
  for (const w of page.words) if (!Number.isInteger(w) || w < 0 || w > 4294967295) throw new Error('WORD_RANGE');
  return page;
 }
 private bytes(words: number[]): Uint8Array { const b = new Uint8Array(words.length * 4); const v = new DataView(b.buffer); for (let i: number = 0; i < words.length; i++) v.setUint32(4 * i, words[i], true); return b; }
 private async write(file: fs.File, bytes: Uint8Array): Promise<void> { let at: number = 0; while (at < bytes.length) { const n: number = await fs.write(file.fd, bytes.buffer.slice(at)); if (n <= 0) throw new Error('SHORT_WRITE'); at += n; } }
 private async smallFile(path: string, text: string): Promise<void> {
  if (await fs.access(path)) throw new Error('ARCHIVE_ALREADY_EXISTS');
  const f = await fs.open(path, fs.OpenMode.CREATE | fs.OpenMode.WRITE_ONLY);
  try { await this.write(f, new util.TextEncoder().encodeInto(text)); await fs.fsync(f.fd); } finally { await fs.close(f); }
 }
 async collect(kind: string, token: string, mode: string, directory: string): Promise<string> {
  if (this.busy || (kind !== 'COLD' && kind !== 'BOOTSTRAP' && kind !== 'WITNESS')) throw new Error('INVALID_OR_BUSY');
  this.busy = true; this.cancelled = false; let file: fs.File | null = null; let id: string = ''; let path: string = '';
  try {
   const statusRaw: string = native.g11Status();
   let manifestRaw: string = ''; let total: number = 0; let expected: string = '';
   const segments: G11Segment[] = [];
   if (kind === 'COLD') {
    manifestRaw = native.g11ColdBegin(token, mode); let m = JSON.parse(manifestRaw) as G11ColdManifest;
    id = m.traceId; if (id === undefined || !/^[1-9][0-9]*$/.test(id)) throw new Error(manifestRaw);
    for (let poll: number = 0; poll < 6000 && m.state !== 'TRACE_READY'; poll++) {
     if (this.cancelled) throw new Error('CANCELLED');
     if (m.state !== 'BUILDING' && m.state !== 'HASHING') throw new Error(manifestRaw);
     await new Promise<void>((resolve) => { setTimeout(resolve, 10); });
     manifestRaw = native.g11ColdState(id); m = JSON.parse(manifestRaw) as G11ColdManifest;
    }
    if (m.state !== 'TRACE_READY' || m.revision !== 'G11_VIS_COLD_V2' || m.headerWords !== 8192 || (m.segments.length !== 11 && m.segments.length !== 17)) throw new Error('COLD_DIRECTORY');
    total = Number(m.bytes); expected = m.dataSHA256;
    segments.push({ id: 0, name: 'HEADER', byteOffset: 0, bytes: 32768, strideWords: 1, sha256: '' });
    let at: number = 32768;
    for (let i: number = 0; i < m.segments.length; i++) {
     const s = m.segments[i]; if (s.id !== i + 1 || s.byteOffset !== at || !Number.isSafeInteger(s.bytes) || s.bytes <= 0 || s.bytes % (4 * s.strideWords) !== 0) throw new Error('SEGMENT_DIRECTORY');
     if (s.strideWords !== 1 && s.strideWords !== 4 && s.strideWords !== 8 && s.strideWords !== 32 && s.strideWords !== 64) throw new Error('STRIDE');
     at += s.bytes; segments.push(s);
    }
    if (at !== total || total > 194154496) throw new Error('COLD_RESOURCE_BOUND');
   } else if (kind === 'BOOTSTRAP') {
    manifestRaw = native.g11BootstrapState(); const m = JSON.parse(manifestRaw) as G11SeedManifest;
    if (m.schema !== 'G11_VIS_SEED_RAW_V1' || m.state !== 3 || m.segments.length !== 4) throw new Error('BOOTSTRAP_NOT_RAW_READY');
    id = m.id; total = Number(m.rawBytes); expected = m.actualVisualSeedSHA; let at: number = 0;
    for (let i: number = 0; i < 4; i++) { const s = m.segments[i]; if (s.kind !== i + 1 || s.preimageOffset !== at) throw new Error('SEED_DIRECTORY'); segments.push({ id: i, name: 'SEED', byteOffset: at, bytes: s.bytes, strideWords: 1, sha256: s.sha256 }); at += s.bytes; }
    if (at !== total || (total !== 271104 && total !== 1057536)) throw new Error('SEED_BYTES');
   } else {
    manifestRaw = native.g11WitnessState(); const m = JSON.parse(manifestRaw) as G11WitnessManifest;
    if (m.revision !== 'G11_PHASE_WITNESS_V2' || m.state !== 'READY' || (m.bytes !== 3149824 && m.bytes !== 4722688)) throw new Error('WITNESS_NOT_READY');
    id = m.id; total = m.bytes; expected = m.sha256; segments.push({ id: 0, name: 'WITNESS', byteOffset: 0, bytes: total, strideWords: 1, sha256: expected });
   }
   if (!Number.isSafeInteger(total) || total <= 0 || !/^[1-9][0-9]*$/.test(id) || !/^[0-9a-f]{64}$/.test(expected)) throw new Error('RAW_IDENTITY');
   path = directory + '/g11-' + kind.toLowerCase() + '-' + id + '.part.u32le';
   if (await fs.access(path)) throw new Error('ARCHIVE_ALREADY_EXISTS');
   file = await fs.open(path, fs.OpenMode.CREATE | fs.OpenMode.WRITE_ONLY);
   const aggregate = crypto.createMd('SHA256'); let done: number = 0; let pages: number = 0;
   for (const s of segments) {
    const hash = crypto.createMd('SHA256'); let local: number = 0;
    while (local < s.bytes) {
     if (this.cancelled) throw new Error('CANCELLED');
     const raw: string = kind === 'COLD' ? native.g11ColdPage(id, s.id, local / 4) : kind === 'BOOTSTRAP' ? native.g11BootstrapPage(s.id, local) : native.g11WitnessPage(local / 4);
     const p = this.rawWords(raw);
     if (kind === 'COLD' && (p.revision !== 'G11_VIS_SEGMENT_PAGE_V2' || p.state !== 'TRACE_READY' || p.traceId !== id || p.segment !== s.id || p.wordOffset !== local / 4 || p.stride !== s.strideWords)) throw new Error('COLD_PAGE_IDENTITY');
     if (kind === 'BOOTSTRAP' && (p.revision !== 'G11_VIS_SEED_RAW_V1' || p.id !== id || p.segment !== s.id || p.byteOffset !== local || p.bytes !== p.words.length * 4)) throw new Error('SEED_PAGE_IDENTITY');
     if (kind === 'WITNESS' && (p.id !== id || p.wordOffset !== local / 4)) throw new Error('WITNESS_PAGE_IDENTITY');
     if (p.words.length % s.strideWords !== 0 || local + p.words.length * 4 > s.bytes) throw new Error('WHOLE_ROW_BOUND');
     const b = this.bytes(p.words); await hash.update({ data: b }); await aggregate.update({ data: b }); await this.write(file, b);
     local += b.length; done += b.length; pages++;
    }
    const observed: string = this.hex((await hash.digest()).data); if (s.sha256 !== '' && s.sha256 !== observed) throw new Error('SEGMENT_SHA_MISMATCH');
   }
   const observed: string = this.hex((await aggregate.digest()).data); if (done !== total || observed !== expected) throw new Error('RAW_SHA_MISMATCH');
   await fs.fsync(file.fd); await fs.close(file); file = null;
   const final: string = path.replace('.part.u32le', '.u32le'); if (await fs.access(final)) throw new Error('FINAL_EXISTS'); await fs.rename(path, final);
   await this.smallFile(final + '.manifest.json', JSON.stringify({ revision: 'G11_STREAM_ARCHIVE_V2', state: 'ARCHIVED_RAW_NOT_PRODUCT_PASS', kind: kind, id: id, bytes: done, sha256: observed, pages: pages, nativeManifestRaw: manifestRaw, nativeStatusRaw: statusRaw }) + '\n');
   return final;
  } catch (error) {
   if (path !== '') { try { await this.smallFile(path + '.incomplete.json', JSON.stringify({ state: 'INCOMPLETE', kind: kind, id: id, error: String(error).slice(0, 1024) }) + '\n'); } catch (ignored) { /* Preserve the first failure. */ } }
   throw new Error('INCOMPLETE:' + String(error));
  } finally {
   try { if (file !== null) await fs.close(file); } finally { if (kind === 'COLD' && id !== '') native.g11ColdEnd(id); if (kind === 'WITNESS') native.g11WitnessEnd(); this.busy = false; }
  }
 }
}
