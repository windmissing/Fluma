'use strict';
// T346 evidence: second-process log (hilog_v2.txt, 13 injects) fps timeline.
const fs = require('fs');
const lines = fs.readFileSync('test/reports/t343/hilog_v2.txt', 'utf8').split('\n');
const injects = lines.filter(l => l.includes('T218DBG'));
console.log('v2 log T218DBG:', injects.length);
const re = /(\d\d:\d\d:\d\d\.\d+).*"commit":(\d+),"frame":(\d+)/;
const scene = lines.filter(l => l.includes('p013_scene')).map(l => re.exec(l)).filter(Boolean).map(m => ({ t: m[1], c: +m[2], f: +m[3] }));
console.log('v2 log scene rows:', scene.length);
let prev = null;
for (const r of scene) {
  if (prev) {
    const t0 = Date.parse('2026-10-31T' + prev.t + '+08:00'), t1 = Date.parse('2026-10-31T' + r.t + '+08:00');
    const dt = (t1 - t0) / 1000, df = r.f - prev.f;
    console.log(r.t, 'dframe', df, 'in', dt.toFixed(1) + 's =>', (df / dt).toFixed(1), 'fps');
  }
  prev = r;
}
