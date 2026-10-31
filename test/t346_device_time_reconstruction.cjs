'use strict';
// T346 device-log physics-time reconstruction: how much SIMULATED time elapsed
// on the device between the first inject and the last screenshot?
//
// tick(dt) computes dt = min(1/30, (ns-prev)/1e9). Each step() call with
// dt<=1/60 => 1 substep of dt; dt>1/60 => 2 substeps of dt/2. So simulated
// time per tick = dt (capped at 1/30). Frame counter f (p013_scene frame)
// increments per tick; commit increments per substep (+1 per admission).
// Reconstruct per-interval dt from commit-delta vs frame-delta:
//   commitDelta == frameDelta  => all ticks had dt<=1/60 (1 substep each)
//   commitDelta == 2*frameDelta => all ticks had dt>1/60 (2 substeps each)
//   (admissions add +1 commit each; 15 injects total in the window)
// Simulated seconds = sum over ticks of dt, and dt per tick is in
// (1/60 .. 1/30] for 2-substep ticks, exactly ~1/60 for 1-substep ticks.
const fs = require('fs');
const lines = fs.readFileSync('test/reports/t343/hilog_full.txt', 'utf8').split('\n');
const re = /(\d\d:\d\d:\d\d\.\d+).*"commit":(\d+),"frame":(\d+)/;
const scene = lines.filter(l => l.includes('p013_scene')).map(l => re.exec(l)).filter(Boolean)
  .map(m => ({ t: Date.parse('2026-10-31T' + m[1] + '+08:00'), c: +m[2], f: +m[3] }));

// First inject 15:23:50.535; last screenshot s5 at 15:25:39 (frame 588).
const firstInject = Date.parse('2026-10-31T15:23:50.535+08:00');
const lastShot = scene[scene.length - 1];

// Segment: from frame at/just-before first inject to the end.
let i0 = 0; for (let i = 0; i < scene.length; i++) if (scene[i].t <= firstInject) i0 = i;
const a = scene[i0], b = lastShot;
const df = b.f - a.f, dc = b.c - a.c;
// admissions between a and b: injects 1..15 all after 15:23:50.535, but a is
// the last scene row <= firstInject (15:23:50.316, commit 982 frame 490).
// All 15 admissions happened inside (a,b]. But admissions also advance frame?
// No: admission is a facade.inject() call (not a tick); the page tick that
// carries it does inject + step in the SAME frame. So commitDelta includes
// +15 admissions.
const substeps = dc - 15; // 15 admissions inside the window
console.log('window', new Date(a.t).toISOString(), '->', new Date(b.t).toISOString());
console.log('frameDelta', df, 'commitDelta', dc, 'admissions', 15, 'substeps', substeps);
console.log('substeps/frame =', (substeps / df).toFixed(2));
// Simulated seconds: substeps each of length substepDt = dt/substeps-per-tick.
// If 2 substeps per tick, dt = 2*substepDt and dt is capped at 1/30, substepDt
// = dt/2 <= 1/60. The per-tick dt upper bound: wall gap between ticks. The
// wall time is 108s for 44 ticks post-inject => gaps ~2s, so dt hits the
// 1/30 cap for every post-inject tick. Simulated = ticks * 1/30.
// Inject window (frame 490->544): fps 2-10, gaps 0.1-1.4s > 1/30 => capped.
const postTicks = 588 - 544;
const injectWindowTicks = 544 - 490;
console.log('inject-window ticks', injectWindowTicks, '=> simulated <=', (injectWindowTicks / 30).toFixed(2), 's');
console.log('post-inject ticks', postTicks, '=> simulated <=', (postTicks / 30).toFixed(2), 's');
console.log('TOTAL simulated after first inject <=', ((588 - 490) / 30).toFixed(2), 's over', ((lastShot.t - firstInject) / 1000).toFixed(0), 's wall');
// Particle fall time from y=220 at terminal speed 64 units/s: 220/64 = 3.4s.
console.log('fall time needed from 220 at v=64:', (220 / 64).toFixed(2), 's');
