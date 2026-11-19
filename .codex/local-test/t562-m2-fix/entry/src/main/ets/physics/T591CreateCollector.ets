// T591CreateCollector: tester-owned in-page collector (architecture-T651-v1 §2).
// Arms BEFORE first XComponent mount; reads m2CreateDiagnostics("0") at <=1s tick
// to DISCOVER a NEW attempt only; locks bootSerial as full decimal string; then
// reads only that serial; <=64 native queries total (diagnostics + object logs);
// <=60s window; bounded JSON lines to app files dir (<=16KiB each, <=1MiB total
// incl. index; CAPACITY_REACHED stops without overwriting). No App-mutex work,
// no GL task enqueue — Global.Copy serialized by Store's own mutex only.
import { m2CreateDiagnostics, m2CreateLog } from 'libgrsm1.so';
import fs from '@ohos.file.fs';

export class T591CreateCollector {
  private armed: boolean = false;
  private timerId: number = -1;
  private tick: number = 0;
  private queryBudget: number = 64;
  private windowStart: number = 0;
  private lockedSerial: string = '';
  private bootId: string = '';
  private lines: string[] = [];
  private bytes: number = 0;
  private capReached: boolean = false;
  private objectsRead: boolean = false;
  private seq: number = 0;

  arm(): void {
    if (this.armed) return;
    this.armed = true;
    this.windowStart = Date.now();
    // initial read receipt BEFORE any XComponent creation:
    this.sample('ARMED_INITIAL', '0');
    this.timerId = setInterval((): void => this.tickOnce(), 1000);
  }

  private budgetLeft(): boolean { return this.queryBudget > 0 && !this.capReached; }

  private sample(tag: string, serial: string): void {
    if (!this.budgetLeft()) return;
    this.queryBudget--;
    let json = '';
    try { json = m2CreateDiagnostics(serial); } catch (e) { json = 'THROW:' + String(e); }
    this.record(tag, json);
    if (this.lockedSerial === '') {
      const m: RegExpMatchArray | null = json.match(/"bootSerial":"(\d+)"/);
      if (m !== null && m.length > 1 && m[1] !== '0') {
        this.lockedSerial = m[1];
        this.record('SERIAL_LOCKED', m[1]);
      }
    } else {
      // terminal semantics: sealed / nativeEndNs presence, or THROW, else keep
      if (json.indexOf('"sealed"') >= 0 || json.indexOf('nativeEndNs') >= 0) this.record('TERMINAL_SEEN', this.lockedSerial);
      if (json.indexOf('THROW') === 0) this.record('PROCESS_LOST_OR_THROW', json.slice(0, 120));
    }
  }

  private tickOnce(): void {
    if (!this.armed || this.capReached) { this.stopIf(); return; }
    this.tick++;
    if (Date.now() - this.windowStart > 60000) { this.record('WINDOW_END', this.lockedSerial || 'NONE'); this.stopIf(); return; }
    if (this.lockedSerial === '') { this.sample('DISCOVER_TICK', '0'); return; }
    this.sample('LOCKED_TICK', this.lockedSerial);
    // object logs on change/terminal (objects 0..2, budget-guarded):
    if (!this.objectsRead && (this.tick % 5 === 0 || this.queryBudget <= 6)) {
      for (let obj = 0; obj < 3 && this.queryBudget > 3; obj++) {
        this.queryBudget--;
        let j = '';
        try { j = m2CreateLog(this.lockedSerial, obj); } catch (e) { j = 'THROW:' + String(e); }
        if (j.length > 4 && j !== '{}') this.record('OBJ' + obj, j);
      }
      if (this.queryBudget <= 3) this.objectsRead = true;
    }
    this.stopIf();
  }

  private record(tag: string, body: string): void {
    if (this.capReached) return;
    const line = '{"seq":' + (this.seq++) + ',"tick":' + this.tick + ',"tag":"' + tag + '","lockedSerial":"' + this.lockedSerial + '","body":' + JSON.stringify(body.slice(0, 16384)) + '}';
    if (line.length > 16384) return;
    if (this.bytes + line.length > 1048576) { this.capReached = true; this.lines.push('{"tag":"CAPACITY_REACHED"}'); this.stopIf(); return; }
    this.lines.push(line);
    this.bytes += line.length;
  }

  private stopIf(): void {
    if (this.timerId >= 0 && (this.capReached || this.queryBudget <= 0 || Date.now() - this.windowStart > 61000)) {
      clearInterval(this.timerId);
      this.timerId = -1;
      this.flush();
    }
  }

  flush(): void {
    try {
      const filesDir: string = AppStorage.get<string>('t591FilesDir') ?? '';
      if (filesDir === '') return;
      const path: string = filesDir + '/t591-collector-' + Date.now() + '.ndjson';
      const f: fs.File = fs.openSync(path, fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE);
      fs.writeSync(f.fd, this.lines.join('\n'));
      fs.closeSync(f.fd);
    } catch (e) { /* DIAG_IO_FAILED kept in lines; original preserved in memory */ }
  }

  summary(): string {
    return 'armed=' + this.armed + ' tick=' + this.tick + ' budget=' + this.queryBudget +
      ' lockedSerial=' + (this.lockedSerial || 'NONE') + ' lines=' + this.lines.length +
      ' bytes=' + this.bytes + ' capReached=' + this.capReached;
  }

  lastBody(): string { return this.lines.length > 0 ? this.lines[this.lines.length - 1] : ''; }
}
