// T590 cursor collector (T676-A): arms before XComponent, samples
// m2CreateDiagnostics to lock new attempt serial, then reads apiCursor region
// across compile/link/status/getError/log calls; <=64 queries/60s/16KiB/1MiB.
import { m2CreateDiagnostics, m2CreateLog } from 'libgrsm1.so';
import fs from '@ohos.file.fs';

@Entry
@Component
struct T590Cursor {
  @State log: string = 'ready';
  private timerId: number = -1;
  private queries: number = 0;
  private locked: string = '';
  private lines: string[] = [];
  private start: number = 0;

  aboutToAppear(): void {
    AppStorage.setOrCreate('t590FilesDir', getContext(this).filesDir);
    this.start = Date.now();
    this.sample('ARMED', '0');
    this.timerId = setInterval((): void => this.tick(), 1000);
  }

  private sample(tag: string, serial: string): void {
    if (this.queries >= 64) return;
    this.queries++;
    let j = '';
    try { j = m2CreateDiagnostics(serial); } catch (e) { j = 'THROW:' + String(e); }
    this.lines.push('{"tag":"' + tag + '","body":' + JSON.stringify(j) + '}');
    this.log += '\n[' + tag + '] len=' + j.length + ' ' + j.slice(0, 90);
    if (this.locked === '') {
      const m: RegExpMatchArray | null = j.match(/"bootSerial":"(\d+)"/);
      if (m !== null && m.length > 1 && m[1] !== '0') { this.locked = m[1]; this.lines.push('{"tag":"LOCKED","body":"' + m[1] + '"}'); }
    }
  }

  private tick(): void {
    if (this.queries >= 64 || Date.now() - this.start > 60000) { this.stop(); return; }
    if (this.locked === '') { this.sample('DISCOVER', '0'); return; }
    this.sample('CURSOR_TICK', this.locked);
    // apiCursor 追加在诊断 JSON 内——每 tick 全读。
  }

  private stop(): void {
    if (this.timerId >= 0) { clearInterval(this.timerId); this.timerId = -1; this.flush(); }
  }

  private flush(): void {
    try {
      const dir: string = AppStorage.get<string>('t590FilesDir') ?? '';
      if (dir === '') return;
      const p: string = dir + '/t676a-' + Date.now() + '.ndjson';
      const f: fs.File = fs.openSync(p, fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE);
      fs.writeSync(f.fd, this.lines.join('\n'));
      fs.closeSync(f.fd);
      this.log += '\n[FLUSH] ' + p;
    } catch (e) { this.log += '\n[IO_FAIL] ' + String(e); }
  }

  build() {
    Column({ space: 6 }) {
      Text('T676-A M2 cursor capture').fontSize(15)
      Scroll() { Text(this.log).fontSize(9).width('96%') }.height('70%')
      Button('flush').onClick((): void => this.flush())
    }.width('100%').padding(6)
  }
}
