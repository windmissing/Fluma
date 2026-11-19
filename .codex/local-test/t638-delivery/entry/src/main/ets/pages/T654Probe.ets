// T654 tester probe: full g13Status + g13InitDiagnostics(attempt,page0..3) raw
// capture per architecture-T649-contract-v1.json (8192B x4 pages, decode/stages/
// logs/control segments, no new configure/active/pulse during read, <=3 rounds,
// <=16KiB per JSON). Button-triggered single-shot; output to on-screen log for
// tester copy. G13 attempt id passed via AppStorage from launch want.
import native from 'libgrs_g13.so';
import { router } from '@kit.ArkUI';
import { G13ResearchConfig } from '../physics/G13ResearchConfig';

@Entry
@Component
struct T654Probe {
  @State log: string = 'ready';
  private rounds: number = 0;

  aboutToAppear(): void {
    const attemptId = AppStorage.get<string>('t654Attempt') ?? '209';
    this.log = 'attempt=' + attemptId;
  }

  private newAttempt(): void {
    try {
      const reply: string = native.g13Configure(G13ResearchConfig.words(), G13ResearchConfig.seed()) as string;
      this.log += '\n[NEW ATTEMPT] ' + reply.slice(0, 200);
      // 读回 attemptId：
      try { const st: string = native.g13Status() as string; const m: RegExpMatchArray | null = st.match(/"attemptId":"(\d+)"/); if (m !== null && m.length > 1) { AppStorage.setOrCreate('t654Attempt', m[1]); this.log += '\n[ATTEMPT] ' + m[1]; } } catch (e) { }
    } catch (e) { this.log += '\n[NEW ATTEMPT] THROW:' + String(e); }
  }

  private capture(): void {
    if (this.rounds >= 3) { this.log += '\n[ROUNDS] CAPACITY_REACHED'; return; }
    this.rounds++;
    let status = '';
    try { status = native.g13Status(); } catch (e) { status = 'THROW:' + String(e); }
    this.log += '\n[STATUS R' + this.rounds + '] ' + status.slice(0, 600);
    // 4 pages raw:
    for (let page = 0; page < 4; page++) {
      let pageJson = '';
      try { pageJson = native.g13InitDiagnostics(AppStorage.get<string>('t654Attempt') ?? '209', page); }
      catch (e) { pageJson = 'THROW:' + String(e); }
      let decoded: Record<string, Object> | null = null;
      try { const v: Object = JSON.parse(pageJson) as Object; if (v !== null && typeof v === 'object' && !Array.isArray(v)) decoded = v as Record<string, Object>; } catch (e) { decoded = null; }
      let head = '';
      if (decoded && typeof decoded === 'object') {
        const keys = Object.keys(decoded);
        const rawB = decoded['rawBytes']; const seq = decoded['sampleSeq'];
        const stArr = decoded['stages'];
        const stLen = (stArr !== null && stArr !== undefined && Array.isArray(stArr)) ? (stArr as Array<Object>).length : -1;
        head = 'keys=' + keys.length + ' rawBytes=' + String(rawB) + ' sampleSeq=' + String(seq) + ' stages=' + String(stLen);
      } else { head = pageJson.slice(0, 120); }
      this.log += '\n[P' + page + ' R' + this.rounds + '] len=' + pageJson.length + ' ' + head;
      // Full body kept for page 0 and page 3 (control anchor):
      if (page === 0 || page === 3) this.log += '\n[FULL P' + page + '] ' + pageJson.slice(0, 2400);
    }
  }

  build() {
    Column({ space: 6 }) {
      Text('T654 g13 diagnostics raw capture').fontSize(16)
      Scroll() { Text(this.log).fontSize(9).width('96%') }.height('72%')
      Row({ space: 6 }) {
        Button('去G13配置').onClick((): void => { try { router.pushUrl({ url: 'pages/GpuResidentG13' }); } catch (e) { this.log += '\n[ROUTER] ' + String(e); } })
        Button('新attempt').onClick((): void => this.newAttempt())
      Button('capture(1轮)').onClick((): void => this.capture())
        Button('清显示').onClick((): void => { this.log = ''; })
      }
    }.width('100%').padding(6)
  }
}
