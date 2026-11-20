import fs from '@ohos.file.fs';
import util from '@ohos.util';
import crypto from '@ohos.security.cryptoFramework';
import { DiagnosticColdFiles } from './DiagnosticColdFiles';

class T676Part {
  file: string = '';
  offset: number = 0;
}
class T676Record {
  sequence: number = 0;
  tag: string = '';
  scope: string = '';
  enterMs: number = 0;
  returnMs: number = 0;
  bytes: number = 0;
  sha256: string = '';
  observeSideEffect: boolean = false;
  parts: T676Part[] = [];
}
class T676Index {
  schema: string = 'T676_COLLECTOR_V1';
  scope: string = '';
  elapsedMs: number = 0;
  queries: T676Record[] = [];
  state: string = 'INCOMPLETE';
}
export class T676Collector {
  private start: number = 0;
  private calls: number = 0;
  private total: number = 0;
  private directory: string = '';
  private scope: string = '';
  private records: T676Record[] = [];
  private encoder: util.TextEncoder = new util.TextEncoder();

  begin(filesDir: string): void {
    this.start = Date.now();
    this.directory = filesDir + '/t676-' + String(this.start);
    fs.mkdirSync(this.directory);
  }
  bind(scope: string): void { this.scope = scope; }
  elapsed(): number { return Date.now() - this.start; }
  remaining(): number { return 1048576 - 32768 - this.total; }
  private write(name: string, bytes: Uint8Array): void {
    if (bytes.byteLength > 8192 || bytes.byteLength > this.remaining()) throw new Error('NOT_CAPTURED_CAPACITY');
    const file = fs.openSync(this.directory + '/' + name, fs.OpenMode.CREATE | fs.OpenMode.WRITE_ONLY);
    try {
      let done: number = 0;
      while (done < bytes.byteLength) {
        const written: number = fs.writeSync(file.fd, bytes.slice(done).buffer);
        if (written <= 0) throw new Error('SHORT_WRITE');
        done += written;
      }
      fs.fsyncSync(file.fd);
    } finally { fs.closeSync(file.fd); }
    this.total += bytes.byteLength;
  }
  async query(tag: string, read: () => string): Promise<string> {
    if (this.calls >= 64 || Date.now() - this.start > 60000) throw new Error('NOT_CAPTURED_QUERY_BUDGET');
    this.calls++;
    const enter: number = Date.now();
    let raw: string = '';
    try { raw = read(); } catch (error) { raw = 'THROW:' + String(error); }
    const returned: number = Date.now();
    const bytes: Uint8Array = this.encoder.encodeInto(raw);
    if (bytes.length > this.remaining()) throw new Error('NOT_CAPTURED_CAPACITY');
    const md = crypto.createMd('SHA256');
    await md.update({ data: bytes });
    const digest = await md.digest();
    const record = new T676Record();
    record.enterMs = enter;
    record.returnMs = returned;
    record.sequence = this.calls;
    record.tag = tag;
    record.scope = this.scope;
    record.bytes = bytes.length;
    record.observeSideEffect = tag === 'STATUS';
    for (const byte of digest.data) record.sha256 += byte.toString(16).padStart(2, '0');
    for (let at: number = 0; at < bytes.length; at += 8192) {
      const part = new T676Part();
      part.offset = at;
      part.file = 'q' + String(this.calls) + '-' + String(at) + '.raw';
      this.write(part.file, bytes.slice(at, at + 8192));
      record.parts.push(part);
    }
    this.records.push(record);
    return raw;
  }
  async handoff(raw: string): Promise<void> {
    const bytes = this.encoder.encodeInto(raw);
    if (bytes.length > 8192 || bytes.length > this.remaining()) throw new Error('NOT_CAPTURED_CAPACITY');
    this.write('page-witness.json', bytes);
  }
  async finish(state: string): Promise<string> {
    const index = new T676Index();
    index.scope = this.scope;
    index.elapsedMs = Date.now() - this.start;
    index.queries = this.records;
    index.state = state;

    const raw: string = JSON.stringify(index);
    if (this.encoder.encodeInto(raw).length > 32768) throw new Error('INDEX_BUDGET');
    await new DiagnosticColdFiles(this.directory).save('index.json', raw);
    return this.directory;
  }
}
