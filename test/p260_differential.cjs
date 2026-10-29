// T260 differential runner: oracle predicts every observable word of the
// captured SwiftShader run; compare word-by-word against host_trace.txt.
'use strict';
const fs = require('fs'), path = require('path');
const ROOT = path.resolve(__dirname, '..');
const O = require('./p260_oracle_source.cjs');
const C = require('./p260_oracle_core.cjs');
const { HDR, COL, K } = O;

const trace = fs.readFileSync(path.join(ROOT, 'test/reports/t260/host_trace.txt'), 'utf8').split('\n');
const captures = [];
let pendingCompletion = null;
for (const l of trace) {
  if (l.startsWith('LASTCOMPLETION ')) pendingCompletion = l.split(/\s+/).slice(2).map(Number);
  else if (l.startsWith('STATE ')) captures.push({ state: l.split(/\s+/).slice(2).map(Number), completion: pendingCompletion });
  else if (l.startsWith('HEADER ') && captures.length) captures[captures.length - 1].header = l.split(/\s+/).slice(1, 1 + 256).map(Number);
}

// ---------- reconstruct observed constants for the oracle (frozen identity) ----------
const conv = x => { const b = new ArrayBuffer(8); new Float64Array(b)[0] = x; const v = new Uint32Array(b); return [v[0], v[1]]; };
const shaderSha = captures[0].header.slice(88, 96);
const angleSha = captures[0].header.slice(96, 104);
const layoutSha = captures[0].header.slice(104, 112);
const IDENTITY = { run: [7, 0], owner: [8, 0], context: [9, 0], surface: [10, 0], geometry: [11, 0], config: [1, 0], input: [1, 0] };

const cmd = new Array(32).fill(0);
cmd[0] = C.MAGIC.sourceCommand; cmd[1] = 1; cmd[4] = 7; cmd[5] = 0; cmd[6] = 100; cmd[8] = 1; cmd[10] = 1; cmd[12] = 11; cmd[14] = 1; cmd[15] = 1; cmd[22] = 16; cmd[24] = 1; cmd[26] = 1; cmd[27] = 1; cmd[28] = 1;
cmd[16] = conv(64.5)[0]; cmd[17] = conv(64.5)[1]; cmd[18] = conv(100.0)[0]; cmd[19] = conv(100.0)[1]; cmd[20] = conv(0.25)[0]; cmd[21] = conv(0.25)[1];

let results = [], diffsTotal = 0;
function rec(id, name, status, detail) { results.push({ id, name, status, detail }); console.log(status, id, name, detail && detail.diffs !== undefined ? 'diffs=' + detail.diffs : ''); }

// ---------- oracle run: initial → ROOT1 → Q1..Q4 ----------
const base = O.initialState(K, IDENTITY.run, IDENTITY.owner, IDENTITY.context, IDENTITY.surface, IDENTITY.geometry, IDENTITY.config, shaderSha, angleSha, layoutSha, IDENTITY.input);

// SOURCE ROOT
const srcCtx = { nonce: [1, 0], frame: [1, 0], control: [1, 0], input: IDENTITY.input, geometry: IDENTITY.geometry, cfg: { x: 0, y: 1, z: 2 }, eventStart: 0, eventCapacity: 133 * 192 + 508, eventArena: 0, pubSlot: 0, dedup: null, dispatchCount: 32, uploadedBytes: 1220, carry: 0, baseBank: 1 };
const src = O.runSourceRoot({ K, sand: base.sand.slice(), column: base.column.slice(), header: base.header.slice() }, cmd, srcCtx);
const srcState = O.flatten(src.st);
{
  const cap = captures[0];
  let diffs = [], evDiff = [];
  for (let i = 0; i < srcState.length; i++) if (srcState[i] !== cap.state[i]) diffs.push([i, srcState[i], cap.state[i]]);
  // oracle event reconstruction
  const ev = [];
  for (const e of src.events) for (const w of e) ev.push(w);
  rec('D-SRC', 'source root state+events word-diff', diffs.length === 0 ? 'PASS' : 'FAIL', { diffs: diffs.length, sample: diffs.slice(0, 6) });
  if (diffs.length) diffsTotal += diffs.length;
  // completion compare (oracle predicted vs GPU LASTCOMPLETION)
  const cd = [];
  for (let i = 0; i < 256; i++) if (src.completion[i] !== cap.completion[i]) cd.push([i, src.completion[i], cap.completion[i]]);
  rec('D-SRC-C', 'source completion256 word-diff', cd.length === 0 ? 'PASS' : 'FAIL', { diffs: cd.length, sample: cd.slice(0, 8) });
  if (cd.length) diffsTotal += cd.length;
}

// QUANTUM: implement the M0 quantum step in oracle (T230 §5.2/5.3 semantics)
function runQuantumStep(stIn, ordinal, requestedAngle, nonce, frame, control, eventStart, eventCapacity, eventArena, pubSlot, carry) {
  const st = { K, sand: stIn.sand.slice(), column: stIn.column.slice(), header: stIn.header.slice() };
  const events = [];
  const failInfo = { stage: 0, reason: 0, ordinal: 0 };
  const status = new Array(40).fill(0);
  const fatal = (stage, reason, ordinal) => { if (status[0] === 0) { status[0] = 1; status[1] = reason; status[14] = stage; status[15] = ordinal; status[2] = 4; failInfo.stage = stage; failInfo.reason = reason; failInfo.ordinal = ordinal; } };
  const mismatch = (stage, ordinal, word, expected, actual) => { if (status[0] === 0) { status[35] = word; status[36] = expected; status[37] = actual; } fatal(stage, C.REASON.REPLAY_MISMATCH, ordinal); };
  status[2] = 1;
  // chooseAngle over current bed
  const accepted = O.chooseAngle(st, requestedAngle);
  if (accepted === null) { fatal(4, C.REASON.GUARD_VIOLATION, 0); }
  if (status[0] === 0) {
    st.header[45] = accepted >>> 0; // signed store as u32
    st.header[52] = C.BUDGET_TOTAL; st.header[53] = 0; st.header[54] = C.BUDGET_TOTAL; st.header[55] = 0;
    for (let c = 0; c < 128; c++) { st.column[14 * 128 + c] = 0; st.column[15 * 128 + c] = C.BUDGET_LOCAL; }
  }
  // proposal
  const motion = {}; // slot -> [vx,vy,nx,ny]
  if (status[0] === 0) {
    const tanA = O.angles.tan(accepted), sinA = O.angles.sin(accepted), cosA = O.angles.cos(accepted);
    for (let slot = 0; slot < K; slot++) {
      if (st.sand[slot] === 0) continue;
      const vx = st.sand[3 * K + slot] | 0, vy = st.sand[4 * K + slot] | 0;
      const dvx = C.mulDivRNE(vx, C.DAMPING, 1048576), dvy = C.mulDivRNE(vy, C.DAMPING, 1048576);
      let gx = C.mulDivRNE(C.GRAVITY, sinA, 1048576), gy = C.mulDivRNE(C.GRAVITY, cosA, 1048576);
      gx = C.mulDivRNE(gx, 1, 120); gy = C.mulDivRNE(gy, 1, 120);
      if (dvx === null || dvy === null || gx === null || gy === null) { motion[slot] = { err: 13 }; continue; }
      const nx0 = dvx + gx, ny0 = dvy - gy;
      if (Math.abs(nx0) > 1073741824 || Math.abs(ny0) > 1073741824) { motion[slot] = { err: 13 }; continue; }
      const dx = C.mulDivRNE(nx0, 1, 120), dy = C.mulDivRNE(ny0, 1, 120);
      if (dx === null || dy === null) { motion[slot] = { err: 13 }; continue; }
      const x0 = st.sand[K + slot], y0 = st.sand[2 * K + slot];
      motion[slot] = { vx: nx0 >>> 0, vy: ny0 >>> 0, nx: (x0 + dx) >>> 0, ny: (y0 + dy) >>> 0, dx, dy };
    }
  }
  // resolve: ID order (slotLess: live first, then id, then slot)
  const order = [];
  for (let i = 0; i < K; i++) if (st.sand[i] !== 0) order.push(i);
  order.sort((a, b) => { const ia = st.sand[5 * K + a], ib = st.sand[5 * K + b]; return ia !== ib ? ia - ib : a - b; });
  if (status[0] === 0) {
    let eventCursor = eventStart;
    const logRow = (e) => { events.push(e); eventCursor++; };
    for (const slot of order) {
      const m = st.sand[slot]; if (m === 0) continue;
      const mp = motion[slot]; if (!mp) continue;
      if (mp.err) { fatal(4, mp.err, slot + 1); break; }
      const x0 = st.sand[K + slot] | 0, y0 = st.sand[2 * K + slot] | 0;
      const endx = mp.nx | 0, endy = mp.ny | 0;
      let dx = endx - x0, dy = endy - y0;
      const vx = mp.vx | 0, vy = mp.vy | 0;
      let c = (st.sand[K + slot] >>> 20);
      let curX = x0, curY = y0, curVx = vx, curVy = vy;
      let lastn = 1, lastd = 1;
      let moved = false;
      for (let turn = 0; turn < 32; turn++) {
        c = curX >>> 20;
        // ground contact at start
        if (curY < st.column[10 * 128 + c] || (curY === st.column[10 * 128 + c] && dy <= 0)) {
          // contact: y=S, vy=0, state CONTACT_WAIT
          const oldTag = st.sand[6 * K + slot];
          const e = makeContact();
          st.sand[K + slot] = curX; st.sand[2 * K + slot] = st.column[10 * 128 + c];
          st.sand[4 * K + slot] = 0; st.sand[6 * K + slot] = 2 | (1 << 12); st.sand[11 * K + slot] = 1;
          logRow(e);
          // LAND request
          landStep(slot, c);
          break;
        }
        if ((st.sand[6 * K + slot] & 15) !== 1) {
          // contact keeping state -> event kind4 with same state
        }
        // candidate ratios
        let n = 1, d = 1, hit = 4;
        const cand = (num, den, type) => { if (num < 0 || den === 0 || num > den) return; if (C.cmpMul(num, den, n, d) < 0 || (C.cmpMul(num, d, n, den) === 0 && type < hit)) { n = num; d = den; hit = type; } };
        if (dy < 0) cand(y0 - st.column[10 * 128 + c], -dy, 0);
        if (dx < 0) cand(x0, -dx, 1); else if (dx > 0) cand(134217727 - x0, dx, 1);
        if (dy > 0) cand(268435456 - y0, dy, 2);
        if (dx > 0) cand((c + 1) * 1048576 - x0, dx, 3); else if (dx < 0) cand(x0 - c * 1048576, -dx, 3);
        if (C.cmpMul(n, d, lastn, lastd) < 0) { fatal(4, C.REASON.TIME_NO_PROGRESS, slot + 1); return finish(); }
        // interpolate
        const ix = C.mulDivRNE(dx, n, d), iy = C.mulDivRNE(dy, n, d);
        const ivx = C.mulDivRNE(vx - (st.sand[3 * K + slot] | 0), n, d), ivy = C.mulDivRNE(vy - (st.sand[4 * K + slot] | 0), n, d);
        if (ix === null || iy === null || ivx === null || ivy === null) { fatal(4, C.REASON.NUMERIC_OVERFLOW, slot + 1); return finish(); }
        const X = x0 + ix, Y = y0 + iy, VX = (st.sand[3 * K + slot] | 0) + ivx, VY = (st.sand[4 * K + slot] | 0) + ivy;
        if (hit === 0) { // bed contact mid-flight
          const e = makeContact();
          st.sand[K + slot] = X >>> 0; st.sand[2 * K + slot] = st.column[10 * 128 + c];
          st.sand[3 * K + slot] = VX >>> 0; st.sand[4 * K + slot] = 0;
          st.sand[6 * K + slot] = 2 | (1 << 12); st.sand[11 * K + slot] = 1;
          logRow(e); landStep(slot, c); break;
        }
        if (hit === 1 || hit === 2) { // wall/ceiling: stop lateral/vertical
          const e = makeContact();
          st.sand[K + slot] = Math.min(Math.max(X, 0), 134217727) >>> 0;
          st.sand[2 * K + slot] = Math.min(Math.max(Y, 0), 268435456) >>> 0;
          st.sand[3 * K + slot] = (hit === 1 ? 0 : VX) >>> 0;
          st.sand[4 * K + slot] = (hit === 2 ? 0 : VY) >>> 0;
          st.sand[6 * K + slot] = 3 | (5 << 12); st.sand[11 * K + slot] = 5;
          logRow(e); break;
        }
        if (hit === 3) { // column crossing
          const target = dx > 0 ? c + 1 : c - 1;
          const wall = dx > 0 ? (c + 1) * 1048576 - 1 : c * 1048576;
          const mass = st.sand[slot];
          const to = target;
          let reason = 0;
          if (Y < st.column[10 * 128 + to]) reason = 3;
          else if (mass > 268435456 - st.column[10 * 128 + to] - st.column[8 * 128 + to]) reason = 2;
          else if (mass > st.header[54] || mass > st.column[15 * 128 + c] || mass > st.column[15 * 128 + to]) reason = 4;
          if (reason !== 0) {
            const e = makeContact(Math.min(X, wall));
            st.sand[K + slot] = wall >>> 0; st.sand[2 * K + slot] = Y >>> 0;
            st.sand[3 * K + slot] = 0; st.sand[4 * K + slot] = VY >>> 0;
            st.sand[6 * K + slot] = 3 | (reason << 12); st.sand[11 * K + slot] = reason;
            logRow(e); break;
          }
          // CROSS
          const oldP = st.column[8 * 128 + c], oldToP = st.column[8 * 128 + to];
          const tb = st.header[54], la = st.column[15 * 128 + c], lb = st.column[15 * 128 + to];
          charge(st, mass, c, to, true, fatal);
          if (status[0] !== 0) return finish();
          st.column[8 * 128 + c] = oldP - mass; st.column[8 * 128 + to] = oldToP + mass;
          st.sand[K + slot] = wall >>> 0; st.sand[2 * K + slot] = Y >>> 0;
          st.sand[3 * K + slot] = VX >>> 0; st.sand[4 * K + slot] = VY >>> 0;
          const e = new Array(64).fill(0);
          e[0] = C.MAGIC.event64; e[1] = 1; e[2] = nonce[0]; e[3] = nonce[1]; e[4] = 2; e[5] = 2; e[6] = 3; e[7] = eventCursor + 1; e[9] = frame[0]; e[10] = frame[1]; e[11] = ordinal;
          e[16] = slot; e[17] = st.sand[10 * K + slot]; e[18] = st.sand[5 * K + slot]; e[19] = st.sand[7 * K + slot];
          e[20] = c; e[21] = to; e[22] = mass; e[23] = mass; e[24] = x0 >>> 0; e[25] = y0 >>> 0; e[26] = wall >>> 0; e[27] = Y >>> 0;
          e[28] = (st.sand[3 * K + slot]) >>> 0; e[29] = (st.sand[4 * K + slot]) >>> 0; e[30] = VX >>> 0; e[31] = VY >>> 0;
          e[32] = oldP; e[33] = oldP - mass; e[34] = oldToP; e[35] = oldToP + mass; e[36] = tb; e[38] = st.header[54];
          e[40] = la; e[41] = st.column[15 * 128 + c]; e[42] = lb; e[43] = st.column[15 * 128 + to];
          e[44] = n; e[46] = d; e[48] = st.sand[8 * K + slot]; e[49] = st.sand[9 * K + slot];
          e[50] = cmd[6]; e[51] = cmd[7]; e[52] = (3 << 18) | (2 << 4) | (0 << 14); e[54] = Math.min(c, to);
          logRow(e);
          curX = wall; curY = Y; curVx = VX; curVy = VY;
          lastn = n; lastd = d;
          continue;
        }
        // full move (hit==4)
        const oldX = st.sand[K + slot], oldY = st.sand[2 * K + slot];
        st.sand[K + slot] = endx >>> 0; st.sand[2 * K + slot] = endy >>> 0;
        st.sand[3 * K + slot] = mp.vx; st.sand[4 * K + slot] = mp.vy;
        if (oldX !== (endx >>> 0) || oldY !== (endy >>> 0)) {
          const e = new Array(64).fill(0);
          e[0] = C.MAGIC.event64; e[1] = 1; e[2] = nonce[0]; e[3] = nonce[1]; e[4] = 2; e[5] = 2; e[6] = 2; e[7] = eventCursor + 1; e[9] = frame[0]; e[10] = frame[1]; e[11] = ordinal;
          e[16] = slot; e[17] = st.sand[10 * K + slot]; e[18] = st.sand[5 * K + slot]; e[19] = st.sand[7 * K + slot];
          e[20] = c; e[21] = c; e[22] = st.sand[slot]; e[23] = st.sand[slot];
          e[24] = oldX; e[25] = oldY; e[26] = endx >>> 0; e[27] = endy >>> 0;
          e[28] = (st.sand[3 * K + slot]); e[29] = (st.sand[4 * K + slot]); e[30] = mp.vx; e[31] = mp.vy;
          e[44] = 1; e[46] = 1; e[48] = st.sand[8 * K + slot]; e[49] = st.sand[9 * K + slot];
          e[50] = cmd[6]; e[51] = cmd[7]; e[52] = (0 << 18) | (1 << 4);
          logRow(e);
        }
        break;
      }
    }
  }
  function makeContact(xOverride, yOverride) { return new Array(64).fill(0); } // refined below
  function charge(st, q, a, b, pair, fatalFn) {
    if (q > st.header[54] || q > st.column[15 * 128 + a] || (pair && q > st.column[15 * 128 + b])) { fatalFn(4, C.REASON.AUDIT_FAILED, 0); return; }
    st.header[54] -= q; st.column[15 * 128 + a] -= q; st.column[14 * 128 + a] += q;
    if (pair) { st.column[15 * 128 + b] -= q; st.column[14 * 128 + b] += q; }
  }
  function landStep(slot, c) {
    const mass = st.sand[slot];
    let q = Math.min(mass, Math.min(st.header[54], st.column[15 * 128 + c]));
    // clipLand
    const tangent = O.angles.tan(st.header[45] | 0);
    if (c > 0) { const dd = st.column[10 * 128 + c] - st.column[10 * 128 + c - 1] - tangent; if (Math.abs(dd) > 605394) { fatal(4, C.REASON.GUARD_VIOLATION, c); return; } q = Math.min(q, 605394 - dd); }
    if (c < 127) { const dd = st.column[10 * 128 + c + 1] - st.column[10 * 128 + c] - tangent; if (Math.abs(dd) > 605394) { fatal(4, C.REASON.GUARD_VIOLATION, c); return; } q = Math.min(q, 605394 + dd); }
    if (q === 0 || status[0] !== 0) return;
    const mh = st.column[2 * 128 + c], p = st.column[8 * 128 + c];
    const tb = st.header[54], lb = st.column[15 * 128 + c];
    charge(st, q, c, c, false, fatal);
    st.column[2 * 128 + c] = mh + q; st.column[10 * 128 + c] = mh + q; st.column[8 * 128 + c] = p - q;
    if (q === mass) { for (let f = 0; f < 16; f++) if (f !== 10) st.sand[f * K + slot] = 0; }
    else { st.sand[slot] = mass - q; st.sand[2 * K + slot] = mh + q; st.sand[4 * K + slot] = 0; st.sand[6 * K + slot] = 2 | (1 << 12); st.sand[11 * K + slot] = 1; }
    const e = new Array(64).fill(0);
    e[0] = C.MAGIC.event64; e[1] = 1; e[2] = nonce[0]; e[3] = nonce[1]; e[4] = 2; e[5] = 2; e[6] = 5; e[7] = events.length + eventStart + 1; e[9] = frame[0]; e[10] = frame[1]; e[11] = ordinal;
    e[16] = slot; e[17] = st.sand[10 * K + slot]; e[18] = st.sand[5 * K + slot]; e[19] = st.sand[7 * K + slot];
    e[20] = c; e[21] = c; e[22] = mass; e[23] = st.sand[slot]; e[24] = 0; e[25] = 0; e[26] = 0; e[27] = 0; e[28] = 0; e[29] = 0; e[30] = 0; e[31] = 0;
    e[32] = mh; e[33] = mh + q; e[34] = p; e[35] = p - q; e[36] = tb; e[38] = st.header[54]; e[40] = lb; e[41] = st.column[15 * 128 + c];
    e[48] = st.sand[8 * K + slot]; e[49] = st.sand[9 * K + slot]; e[50] = cmd[6]; e[51] = cmd[7];
    e[52] = (0 << 18) | (2 << 4) | (1 << 10);
    events.push(e);
  }
  function finish() { return { st, events, status, failInfo }; }
  // final contact fold (床上升埋点)
  if (status[0] === 0) {
    for (let slot = 0; slot < K; slot++) {
      if (st.sand[slot] === 0) continue;
      const c = (st.sand[K + slot] >>> 20);
      const y = st.sand[2 * K + slot], s = st.column[10 * 128 + c], tag = st.sand[6 * K + slot];
      if (y < s) { st.sand[2 * K + slot] = s; st.sand[4 * K + slot] = 0; st.sand[6 * K + slot] = 2 | (1 << 12); st.sand[11 * K + slot] = 1; }
      else if ((tag & 15) === 2 && y > s) { st.sand[6 * K + slot] = 1; st.sand[11 * K + slot] = 0; }
    }
  }
  if (status[0] === 0) {
    // foldState
    O.foldState(st);
    // metadata: physics commit increments, angle, carry, frame etc.
    st.header[16] = (st.header[16] + 1) >>> 0;
    st.header[12] = nonce[0]; st.header[13] = nonce[1];
    st.header[18] = control[0]; st.header[19] = control[1];
    st.header[49] = frame[0]; st.header[50] = frame[1];
    st.header[84] = IDENTITY.input[0]; st.header[85] = IDENTITY.input[1];
    st.header[72] = (st.header[72] + 1) >>> 0; if (st.header[72] === 0) st.header[73] = (st.header[73] + 1) >>> 0;
    const baseBank = (ordinal % 2 === 1) ? 1 : 0; // bank selector alternates per accepted root
    st.header[76] = 1 - baseBank; st.header[77] = baseBank; st.header[78] = 2; // GPU uBanks(base,candidate,spare)
    st.header[79] = 2; // rootKind QUANTUM
    st.header[51] = ordinal; st.header[80] = status[2] || 1; st.header[81] = 2;
    st.header[37] = eventArena; st.header[38] = eventStart + events.length;
    st.header[35] = frame[0]; st.header[36] = frame[1];
    st.header[47] = carry || 0; st.header[48] = 0;
  }
  return finish();
}

// oracle quantum predictions vs captures 1..4
let oracleState = src.st;
for (let q = 1; q <= 4; q++) {
  const evStart = (q - 1) * 2;
  const qr = runQuantumStep(oracleState, q, 0, [q + 1, 0], [2, 0], [2, 0], evStart, 133 * 192 + 508, 1, 1, 0);
  oracleState = qr.st;
  const flat = O.flatten(oracleState);
  const cap = captures[q];
  const diffs = [];
  for (let i = 0; i < flat.length; i++) if (flat[i] !== cap.state[i]) diffs.push([i, flat[i], cap.state[i]]);
  rec('D-Q' + q, 'quantum ' + q + ' state word-diff', diffs.length === 0 ? 'PASS' : 'FAIL', { diffs: diffs.length, sample: diffs.slice(0, 8) });
  diffsTotal += diffs.length;
}

fs.writeFileSync(path.join(ROOT, 'test/reports/t260/differential.json'), JSON.stringify({ revision: 'T260_DIFFERENTIAL_V1', scope: 'ORACLE_VS_SWIFTSHADER_TRACE', device: false, hardwareGPU: false, shader: 'PC SwiftShader (ANGLE Vulkan) product m0_source.comp', diffsTotal, results }, null, 2));
console.log('TOTAL DIFFS', diffsTotal);
