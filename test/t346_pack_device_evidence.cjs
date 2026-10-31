'use strict';
// T346 evidence packer: device-log scene rows + fps timeline + simulated-time
// reconstruction for both T343 processes, written into test/reports/t346/.
const fs = require('fs');
const mk = (src, out, admissions) => {
  const lines = fs.readFileSync(src, 'utf8').split('\n');
  const re = new RegExp('(\\d\\d:\\d\\d:\\d\\d\\.\\d+).*"commit":(\\d+),"frame":(\\d+)');
  const secRe = new RegExp('(\\d\\d:\\d\\d:\\d\\d\\.\\d+)');
  const scene = lines.filter(l => l.includes('p013_scene')).map(l => re.exec(l)).filter(Boolean)
    .map(m => ({ t: m[1], c: +m[2], f: +m[3] }));
  const injectTimes = lines.filter(l => l.includes('T218DBG'))
    .map(l => Date.parse('2026-10-31T' + secRe.exec(l)[1] + '+08:00'));
  const rows = [];
  let prev = null;
  for (const r of scene) {
    const t = Date.parse('2026-10-31T' + r.t + '+08:00');
    if (prev) {
      const dt = (t - prev.t) / 1000, df = r.f - prev.f;
      rows.push({ t: r.t, commit: r.c, frame: r.f, dFrame: df, wallDt: +dt.toFixed(3), fps: +(df / dt).toFixed(2) });
    }
    prev = { t };
  }
  let recon = null;
  if (injectTimes.length) {
    const firstInject = Math.min(...injectTimes);
    const last = scene[scene.length - 1];
    const base = scene.filter(r => Date.parse('2026-10-31T' + r.t + '+08:00') < firstInject).pop() || scene[0];
    const df = last.f - base.f, dc = last.c - base.c;
    const adm = injectTimes.filter(t => t > Date.parse('2026-10-31T' + base.t + '+08:00') && t <= Date.parse('2026-10-31T' + last.t + '+08:00')).length;
    recon = { firstInject: new Date(firstInject).toISOString().slice(11, 19),
      lastSceneRow: last.t, frameDelta: df, commitDelta: dc, admissions: adm,
      physicsSubsteps: dc - adm, substepsPerFrame: +((dc - adm) / df).toFixed(2),
      simulatedSecondsUpperBound: +(df / 30).toFixed(2),
      wallSecondsAfterFirstInject: +((Date.parse('2026-10-31T' + last.t + '+08:00') - firstInject) / 1000).toFixed(0),
      fallTimeNeeded220atV64: 3.44 };
  }
  fs.writeFileSync(out, JSON.stringify({ sceneRows: rows, reconstruction: recon }, null, 1));
  console.log(out, 'rows', rows.length, 'recon', JSON.stringify(recon));
};
mk('test/reports/t343/hilog_full.txt', 'test/reports/t346/device-log1-full.json', 15);
mk('test/reports/t343/hilog_v2.txt', 'test/reports/t346/device-log2-v2.json', 13);
