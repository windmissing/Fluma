import native from 'libgrs_g13.so';
import fs from '@ohos.file.fs';
import util from '@ohos.util';
interface InitFields { firstFailureAttempt?: string; currentConfigTicket?: string; }
interface FullStatus { initialization?: InitFields; }
interface ConfigureReply { attempt?: string; }
interface PageReply { schema?: string; state?: string; page?: number; rawBytes?: number; base64?: string; }
// Test-only same-page collector. No GL calls, workers, routes or activation.
export class T672Collector {
  private directory: string = '';
  private total: number = 0;
  private records: string[] = [];
  private statusCalls: number = 0;
  private rounds: number = 0;
  private ticket: string = '0';
  private configuredOnce: boolean = false;
  private encoder: util.TextEncoder = new util.TextEncoder();
  private save(name: string, raw: string): void {
    const bytes: Uint8Array = this.encoder.encodeInto(raw);
    if (bytes.byteLength > 16384 || this.total + bytes.byteLength + 16384 > 1048576) throw new Error('T672_EVIDENCE_BUDGET');
    if (fs.accessSync(this.directory + '/' + name)) throw new Error('T672_FILE_EXISTS');
    const file = fs.openSync(this.directory + '/' + name, fs.OpenMode.CREATE | fs.OpenMode.WRITE_ONLY);
    try { let at: number = 0; while (at < bytes.byteLength) { const n: number = fs.writeSync(file.fd, bytes.slice(at).buffer); if (n <= 0) throw new Error('T672_SHORT_WRITE'); at += n; } } finally { fs.closeSync(file); }
    this.total += bytes.byteLength; this.records.push(name);
  }
  private status(name: string): FullStatus { if (++this.statusCalls > 4) throw new Error('T672_STATUS_BUDGET'); const raw: string = native.g13Status(); this.save(name, raw); return JSON.parse(raw) as FullStatus; }
  arm(filesDir: string): void {
    if (this.directory.length > 0) throw new Error('T672_ALREADY_ARMED');
    this.directory = filesDir + '/t672-' + String(Date.now()); fs.mkdirSync(this.directory);
    this.status('status-before.json');
    this.save('armed.json', JSON.stringify({ schema: 'T672_SAME_PAGE_V1', page: 'GpuResidentG13', ability: 'G13ResearchAbility', control: 'ARMED_BEFORE_CONFIGURE', deviceResult: 'NOT_EVALUATED' }));
  }
  configured(raw: string): void { if (this.configuredOnce) throw new Error('T672_CONFIGURE_COUNT'); this.configuredOnce = true; this.save('configure.json', raw); const reply = JSON.parse(raw) as ConfigureReply; if (reply.attempt !== undefined && /^(0|[1-9][0-9]{0,19})$/.test(reply.attempt)) this.ticket = reply.attempt; }
  private readPage(name: string, ticket: string, page: number): void { const raw: string = native.g13InitDiagnostics(ticket, page); this.save(name, raw); const r = JSON.parse(raw) as PageReply; if (r.schema !== 'G13InitDiagnosticV1' || r.state !== 'SNAPSHOT' || r.page !== page || r.rawBytes !== 8192 || r.base64 === undefined || new util.Base64Helper().decodeSync(r.base64).byteLength !== 8192) throw new Error('T672_DIAGNOSTIC_SCHEMA'); }
  capture(): string {
    if (!this.configuredOnce || ++this.rounds > 3) throw new Error('T672_ROUND_BUDGET');
    const prefix: string = 'r' + String(this.rounds);
    try {
      const status = this.status(prefix + '-status.json');
      const failure: string = status.initialization?.firstFailureAttempt ?? '0';
      if (failure !== '0' && /^(0|[1-9][0-9]{0,19})$/.test(failure)) this.ticket = failure;
      // "0" is the existing readonly discovery contract, never a status call ID.
      this.readPage(prefix + '-anchor-before.json', this.ticket, 3);
      for (let page: number = 0; page < 4; page++) this.readPage(prefix + '-page' + String(page) + '.json', this.ticket, page);
      this.readPage(prefix + '-anchor-after.json', this.ticket, 3);
      this.save(prefix + '-index.json', JSON.stringify({ schema: 'T672_RAW_CAPTURE_V1', ticket: this.ticket, round: this.rounds, statusCalls: this.statusCalls, files: this.records.slice(), totalRawBytes: this.total, consistency: 'OFFLINE_CHECK_REQUIRED', module: 'libgrs_g13.so', page: 'GpuResidentG13' }));
      return this.directory;
    } catch (error) { this.save(prefix + '-failure.json', JSON.stringify({ state: 'INCOMPLETE', error: String(error), ticket: this.ticket })); throw new Error(String(error)); }
  }
}
