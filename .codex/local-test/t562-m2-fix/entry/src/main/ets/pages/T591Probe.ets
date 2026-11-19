// T591 tester probe page: external collector for m2CreateDiagnostics/m2CreateLog
// per architecture-T622 (<=1s sampling, <=64 queries, <=16KiB per sample,
// serial-locked identity, no old-attempt splicing). Polls in-page and keeps a
// bounded on-screen log; samples are copied from the log text by the tester.
import native from 'libgrsm1.so';

@Entry
@Component
struct T591Probe {
  @State log: string = 'ready';
  private samples: number = 0;
  private serial: number = 0;
  private timerId: number = -1;

  aboutToAppear(): void {
    this.timerId = setInterval((): void => {
      if (this.samples >= 64) { clearInterval(this.timerId); this.log += '\n[COLLECTOR] CAPACITY_REACHED'; return; }
      this.samples++;
      const stamp = new Date().toISOString();
      let diag = '';
      try { diag = native.m2CreateDiagnostics(this.serial > 0 ? String(this.serial) : '0'); } catch (e) { diag = 'THROW:' + String(e); }
      let clog = '';
      try { clog = native.m2CreateLog(this.serial > 0 ? String(this.serial) : '0', 0); } catch (e) { clog = 'THROW:' + String(e); }
      const head = diag.slice(0, 120) + '|' + clog.slice(0, 80);
      this.log += `\n[S${this.samples} ${stamp}] ${head}`;
      const m = diag.match(/"serial":(\d+)/);
      if (m && this.serial === 0 && Number(m[1]) > 0) {
        this.serial = Number(m[1]);
        this.log += `\n[SERIAL LOCKED] ${this.serial}`;
      }
      if (diag.indexOf('"sealed":true') >= 0 || diag.indexOf('"code":0,"endNs"') >= 0) {
        this.log += `\n[FULL SNAPSHOT attempt=${this.serial}]\n${diag.slice(0, 1400)}\n[LOG]\n${clog.slice(0, 1400)}`;
      }
    }, 1000);
  }

  build() {
    Column({ space: 6 }) {
      Text('T591 m2Create collector probe').fontSize(16)
      Scroll() { Text(this.log).fontSize(10).width('96%') }.height('70%')
      Row({ space: 6 }) {
        Button('触发新空场(Create)').onClick((): void => { try { native.newRun(); this.log += '\n[TRIGGER] newRun sent'; } catch (e) { this.log += '\n[TRIGGER] THROW:' + String(e); } })
        Button('清显示').onClick((): void => { this.log = ''; })
      }
    }.width('100%').padding(6)
  }
}
