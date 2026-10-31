'use strict';
// Reconstruct simulated physics time in the SECOND process (hilog_v2, 13 injects).
const fs = require('fs');
const lines = fs.readFileSync('test/reports/t343/hilog_v2.txt', 'utf8').split('\n');
const re = new RegExp('(\\d\\d:\\d\\d:\\d\\d\\.\\d+).*"commit":(\\d+),"frame":(\\d+)');
const secRe = new RegExp('(\\d\\d:\\d\\d:\\d\\d\\.\\d+)');
const scene = lines.filter(l => l.includes('p013_scene')).map(l => re.exec(l)).filter(Boolean)
  .map(m => ({ t: Date.parse('2026-10-31T' + m[1] + '+08:00'), c: +m[2], f: +m[3] }));
const injectTimes = lines.filter(l => l.includes('T218DBG'))
  .map(l => Date.parse('2026-10-31T' + secRe.exec(l)[1] + '+08:00'));
const firstInject = Math.min(...injectTimes), lastInject = Math.max(...injectTimes);
const last = scene[scene.length - 1];
const base = scene.filter(r => r.t < firstInject).pop() || scene[0];
const df = last.f - base.f, dc = last.c - base.c;
const admissions = injectTimes.filter(t => t > base.t && t <= last.t).length;
console.log('first inject', new Date(firstInject).toISOString().slice(11, 19),
  'last scene row', new Date(last.t).toISOString().slice(11, 19),
  'frameDelta', df, 'commitDelta', dc, 'admissions', admissions,
  'substeps', dc - admissions, 'substeps/frame', ((dc - admissions) / df).toFixed(2));
console.log('simulated time after first inject <=', (df / 30).toFixed(2), 's over',
  ((last.t - firstInject) / 1000).toFixed(0), 's wall');
