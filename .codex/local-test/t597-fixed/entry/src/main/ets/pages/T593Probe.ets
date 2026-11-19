// T593Probe (T676-B effector capture, tester facility): formal M1 existing
// endpoints only — status/recoveryState/bootDiagnostics/executionDiagnosticsV1/
// timeline family; budget <=60s/<=64 queries/16KiB/8KiB fragments; no GL query,
// no synthetic SOURCE; separate log for normal control/input calls.
import { status, recoveryState, bootDiagnostics, executionDiagnosticsV1, timelineSnapshotBegin, timelineSnapshotPage } from 'libgrsm1.so';
import fs from '@ohos.file.fs';

@Entry
@Component
struct T593Probe {
  @State log: string = 'ready';
  private queries: number = 0;
  private start: number = 0;
  private lines: string[] = [];

  private q(fn: () => string, tag: string): void {
    // explicit-typed callback invoked below
    if (this.queries >= 64 || Date.now() - this.start > 60000) return;
    this.queries++;
    let j = '';
    try { j = fn(); } catch (e) { j = 'THROW:' + String(e); }
    // fragment-preserving record (full string, 8KiB chunks noted):
    const chunks = Math.ceil(j.length / 8192);
    this.lines.push('{"tag":"' + tag + '","len":' + j.length + '","chunks":' + chunks + ',"body":' + JSON.stringify(j) + '}');
    this.log += '\n[' + tag + '] len=' + j.length + ' head=' + j.slice(0, 100);
  }

  private capture(): void {
    this.start = Date.now();
    this.q((): string => status(), 'STATUS');
    this.q((): string => recoveryState(), 'RECOVERY');
    this.q((): string => bootDiagnostics(), 'BOOTDIAG');
  }

  private captureExec(): void {
    this.q((): string => executionDiagnosticsV1('m1', 64), 'EXEC_DIAG');
  }

  private captureTimeline(): void {
    this.q((): string => timelineSnapshotBegin(), 'TL_BEGIN');
  }

  private flush(): void {
    try {
      const dir: string = AppStorage.get<string>('t593FilesDir') ?? '';
      if (dir === '') return;
      const p: string = dir + '/t676b-' + Date.now() + '.ndjson';
      const f: fs.File = fs.openSync(p, fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE);
      fs.writeSync(f.fd, this.lines.join('\n'));
      fs.closeSync(f.fd);
      this.log += '\n[FLUSH] ' + p;
    } catch (e) { this.log += '\n[IO_FAIL] ' + String(e); }
  }

  aboutToAppear(): void {
    AppStorage.setOrCreate('t593FilesDir', getContext(this).filesDir);
  }

  build() {
    Column({ space: 6 }) {
      Text('T676-B formal M1 capture').fontSize(15)
      Scroll() { Text(this.log).fontSize(9).width('96%') }.height('66%')
      Row({ space: 6 }) {
        Button('capture 基础').onClick((): void => this.capture())
        Button('execDiag').onClick((): void => this.captureExec())
        Button('timeline').onClick((): void => this.captureTimeline())
        Button('flush').onClick((): void => this.flush())
      }
      Text('queries=' + this.queries).fontSize(11)
    }.width('100%').padding(6)
  }
}
