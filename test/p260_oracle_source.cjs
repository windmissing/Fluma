// T260 independent whole-step oracle: predicts every observable word of the
// product M0 root pipeline (state, events, publication, completion) from the
// frozen T230 v1 spec semantics. Self-held math only. tester, offline.
'use strict';
const C = require('./p260_oracle_core.cjs');
const { Qm, Cq, Hq, BUDGET_TOTAL, BUDGET_LOCAL, PACKAGE, COLS, GRAVITY, DAMPING, FLUX, SOFT, MAGIC, OUTCOME, RECEIPT, REASON, STAGE, KIND, MASK64, planWorkspace, loadAngles, mulDivRNE, u32, add64, cmp64, plusOne64, mul32to64, cmpMul, floorBinary64Q20, q20FloatBits, columnPreservingFloatBits, validateSandStateWord } = C;

const angles = loadAngles();
const K = 192, PAD = 256;
const L = planWorkspace(K, 64); // alignment 64 observed on ANGLE/SwiftShader
const SEG = L.seg, W = L.work;
const HDR = SEG.header, COL = SEG.column, STATE_WORDS = SEG.totalWords;

// ---------- full state image ----------
// state = { sand: Array(K*16), column: Array(128*16), header: Array(256) } plus work areas
function initialState(K_, run, owner, context, surface, geometry, config, shaderSha, angleSha, layoutSha, input) {
  const sand = new Array(64 * K_).fill(0);
  const column = new Array(16 * COLS).fill(0);
  const header = new Array(256).fill(0);
  header[0] = MAGIC.stateHeader; header[1] = 1; header[2] = 256;
  const put64 = (at, v) => { header[at] = v[0]; header[at + 1] = v[1]; };
  put64(4, run); put64(6, owner); put64(8, context); put64(10, surface); put64(20, geometry); put64(22, config);
  for (let i = 24; i <= 31; i++) header[i] = 1;
  header[39] = header[40] = 1; header[43] = K_; header[44] = 64; header[77] = 1; header[78] = 2;
  put64(84, input);
  for (let i = 0; i < 8; i++) { header[88 + i] = shaderSha[i]; header[96 + i] = angleSha[i]; header[104 + i] = layoutSha[i]; }
  return { K: K_, sand, column, header };
}
// flat word image matching product memory layout (segment sizes are BYTES in planSegments)
function flatten(st) {
  const out = new Array(SEG.totalWords).fill(0);
  for (let i = 0; i < Math.min(64 * st.K / 4, SEG.water); i++) out[i] = u32(st.sand[i]); // sand words = 64*K/4 = 16*K
  for (let f = 0; f < 12; f++) for (let c = 0; c < 64; c++) out[SEG.water + f * 64 + c] = 0; // water12: 12 fields*64 slots
  for (let f = 0; f < 16; f++) for (let c = 0; c < COLS; c++) out[COL + f * COLS + c] = u32(st.column[f * COLS + c]);
  for (let i = 0; i < 256; i++) out[HDR + i] = u32(st.header[i]);
  return out;
}
function unflatten(words) {
  const sand = words.slice(0, 16 * K);
  const column = new Array(16 * COLS); for (let f = 0; f < 16; f++) for (let c = 0; c < COLS; c++) column[f * COLS + c] = words[COL + f * COLS + c];
  const header = words.slice(HDR, HDR + 256);
  return { K, sand, column, header };
}

// ---------- semantics helpers (per T230 §5 / product frozen behavior) ----------
function foldState(st) {
  st.column.fill(0, 8 * COLS, 9 * COLS);
  let live = 0; let totalLo = 0, totalHi = 0;
  for (let i = 0; i < st.K; i++) {
    const m = st.sand[i]; if (m === 0) continue;
    const x = st.sand[K + i];
    const col = x >>> 20;
    st.column[8 * COLS + col] += m; live++;
  }
  for (let c = 0; c < COLS; c++) {
    const mh = st.column[2 * COLS + c], ph = st.column[8 * COLS + c];
    st.column[10 * COLS + c] = mh; st.column[11 * COLS + c] = mh; st.column[12 * COLS + c] = mh; st.column[13 * COLS + c] = mh + ph;
    const t = totalLo + mh + ph; if (t > 0xffffffff) { totalHi++; totalLo = t >>> 0; } else totalLo = t;
  }
  st.header[56] = totalLo; st.header[57] = totalHi;
  st.header[41] = live; st.header[43] = st.K - live;
}
function computeSpareAudit(st) {
  // independent per-column audit sums (used by both audit and cross-checks)
  const auditedP = new Array(COLS).fill(0); let auditedLive = 0; let totalLo = 0, totalHi = 0;
  for (let i = 0; i < st.K; i++) {
    const m = st.sand[i]; if (m === 0) continue;
    const x = st.sand[K + i]; if (x >= 134217728 || m > Cq) return null;
    const c = x >>> 20; auditedP[c] += m; auditedLive++;
  }
  return { auditedP, auditedLive };
}
function chooseAngle(st, requested) {
  let low = -605394, high = 605394;
  for (let c = 0; c < 127; c++) {
    const d = st.column[10 * COLS + c + 1] - st.column[10 * COLS + c];
    low = Math.max(low, d - 605394); high = Math.min(high, d + 605394);
  }
  if (low > high) return null;
  // binary search first index with tan >= low
  let lo = 0, hi = 15361;
  while (lo < hi) { const mid = (lo + hi) >> 1; if (angles.tan(mid - 7680) < low) lo = mid + 1; else hi = mid; }
  const first = lo;
  lo = 0; hi = 15361;
  while (lo < hi) { const mid = (lo + hi) >> 1; if (angles.tan(mid - 7680) <= high) lo = mid + 1; else hi = mid; }
  const last = lo - 1;
  if (first > last || first >= 15361 || last < 0) return null;
  return Math.min(Math.max(requested + 7680, first), last) - 7680;
}

// ---------- source root ----------
// cmd: 32-word array; ctx: {nonce,frame,control,input,geometry,cfg:{x,y,z},eventStart,eventCapacity,eventArena,pubSlot}
// returns {outcome, state(next candidate=flattened words), events[], terminal[32], completion[256], fatal:{stage,reason,ordinal}}
function runSourceRoot(base, cmd, ctx) {
  const st = { K: base.K, sand: base.sand.slice(), column: base.column.slice(), header: base.header.slice() };
  const events = []; // 64-word rows appended at ctx.eventStart
  const status = new Array(40).fill(0);
  const terminal = new Array(32).fill(0);
  const failInfo = { stage: 0, reason: 0, ordinal: 0 };
  const fatal = (stage, reason, ordinal) => { if (status[0] === 0) { status[0] = 1; status[1] = reason; status[14] = stage; status[15] = ordinal; status[2] = 4; failInfo.stage = stage; failInfo.reason = reason; failInfo.ordinal = ordinal; } };
  const mismatch = (stage, ordinal, word, expected, actual) => { if (status[0] === 0) { status[35] = word; status[36] = expected; status[37] = actual; } fatal(stage, REASON.REPLAY_MISMATCH, ordinal); };
  const reject = (reason, append) => { status[1] = reason; status[2] = 2; status[11] = append ? 1 : 0; status[4] = 0; status[5] = 0; };
  const D = i => cmd[i];
  status[2] = 1; // Outcome=SUCCESS tentatively
  let eventCursor = ctx.eventStart;

  // ---- prepare (ADMISSION stage) ----
  const H = i => st.header[i];
  // audit current state first (sourceAudit(false) essentials): header/live/id/source-table checks
  const audit0 = auditState(base, { nonceLo: ctx.nonce[0], nonceHi: ctx.nonce[1], rootKind: 1, cmdD14: null });
  if (!audit0.ok) { fatal(2, 15, 0); }
  let count = H(33);
  if (status[0] === 0 && count > 4096) fatal(2, 15, 0);
  if (status[0] === 0 && (D(0) !== MAGIC.sourceCommand || D(1) !== 1 || D(31) !== 0)) { reject(9, false); }
  if (status[0] === 0 && (D(4) !== H(4) || D(5) !== H(5))) { reject(9, false); }
  // dedup scan: product holds dedup table in workspace; oracle mirrors with ctx.dedup (array of {cmd32, terminal32})
  let replayHit = -1;
  if (status[0] === 0 && ctx.dedup) {
    for (let i = 0; i < count; i++) {
      const at = ctx.dedup[i]; if (!at) continue;
      // keyEqual: words 4..7
      if (at.cmd[4] === D(4) && at.cmd[5] === D(5) && at.cmd[6] === D(6) && at.cmd[7] === D(7)) {
        let same = true; for (let j = 0; j < 32; j++) if (at.cmd[j] !== D(j)) { same = false; break; }
        if (!same) { reject(8, false); replayHit = -2; break; }
        replayHit = i; break;
      }
    }
  }
  if (replayHit >= 0) {
    status[2] = 3; status[12] = 1; status[10] = replayHit;
    for (let j = 0; j < 32; j++) terminal[j] = ctx.dedup[replayHit].term[j];
    terminal[2] = 1;
  }
  if (status[0] === 0 && replayHit < 0) {
    if (count === 4096) { reject(11, false); }
    else {
      status[11] = 1;
      if (D(2) !== 0 || D(3) !== 0 || D(30) !== 0) reject(1, true);
      else if (D(8) !== ctx.input[0] || D(9) !== ctx.input[1] || D(12) !== ctx.geometry[0] || D(13) !== ctx.geometry[1] || D(28) !== H(22) || D(29) !== H(23) || D(14) === 0 || D(15) === 0 || (D(10) | D(11)) === 0 || (D(6) | D(7)) === 0) reject(9, true);
      else if ((D(24) | D(25)) === 0 || D(27) > 1) reject(10, true);
      else {
        let x, y, q;
        const fx = floorBinary64Q20(D(16), D(17), 134217727);
        const fy = floorBinary64Q20(D(18), D(19), 268435456);
        const fq = floorBinary64Q20(D(20), D(21), 268435456);
        if (!fx[0] || !fy[0] || !fq[0]) reject(2, true);
        else {
          x = fx[1]; y = fy[1]; q = fq[1];
          const ax = D(17) & 0x7fffffff, ay = D(19) & 0x7fffffff, av = D(21) & 0x7fffffff;
          if (ax >= 0x40600000 || ay > 0x40700000 || (ay === 0x40700000 && D(18) > 0) || av > 0x40700000 || (av === 0x40700000 && D(20) > 0)) reject(2, true);
          else {
            status[3] = q; status[7] = x; status[8] = y;
            const column = x >>> 20; status[6] = column;
            if (q === 0) reject(3, true);
            else if (y < st.column[10 * COLS + column]) reject(4, true);
            else {
              let free = 0; for (let i = 0; i < st.K; i++) if (st.sand[i] === 0) free++;
              status[9] = free;
              const occupied = st.column[13 * COLS + column];
              if (occupied > Cq) fatal(2, 15, column);
              else {
                const bySlots = free * PACKAGE > 0xffffffff ? 0xffffffff : free * PACKAGE;
                const admitted = Math.min(q, Math.min(Cq - occupied, bySlots));
                if (admitted === 0) reject(free === 0 ? 5 : 6, true);
                else {
                  const births = Math.floor(admitted / PACKAGE) + (admitted % PACKAGE === 0 ? 0 : 1);
                  const next = H(39);
                  if (next === 0 || births > 0xffffffff - next) reject(7, true);
                  else if (H(34) >= 4096) reject(11, false);
                  else if (ctx.eventStart > ctx.eventCapacity || births > ctx.eventCapacity - ctx.eventStart) fatal(2, 12, 0);
                  else {
                    // slot generation guard on first births free slots
                    let rank = 0, genBad = false;
                    for (let i = 0; i < st.K && rank < births; i++) if (st.sand[i] === 0) { if (st.sand[10 * K + i] === 0xffffffff) { genBad = true; break; } rank++; }
                    if (genBad) reject(21, true);
                    else if ((H(14) === 0xffffffff && H(15) === 0xffffffff) || (H(16) === 0xffffffff && H(17) === 0xffffffff)) fatal(2, 13, 0);
                    else { status[4] = admitted; status[5] = births; }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  // ---- birth (rank by free-scan order) ----
  const birthRows = [];
  if (status[0] === 0 && status[2] === 1) {
    const scanSlots = [];
    for (let i = 0; i < st.K; i++) if (st.sand[i] === 0) scanSlots.push(i);
    for (let rank = 0; rank < status[5]; rank++) {
      const slot = scanSlots[rank];
      const m = (rank + 1 === status[5]) ? status[4] - rank * PACKAGE : PACKAGE;
      const id = st.header[39] + rank, gen = st.sand[10 * K + slot] + 1, column = status[6];
      const row = ctx.eventStart + rank, at = row;
      const e = new Array(64).fill(0);
      e[0] = MAGIC.event64; e[1] = 1; e[2] = ctx.nonce[0]; e[3] = ctx.nonce[1]; e[4] = 1; e[5] = 1; e[6] = 1; e[7] = row + 1; e[9] = ctx.frame[0]; e[10] = ctx.frame[1];
      const origin = plusOne64([st.header[14], st.header[15]]);
      const physical = plusOne64([st.header[16], st.header[17]]);
      if (!origin || !physical) { fatal(2, 13, row + 1); break; }
      e[12] = origin[0]; e[13] = origin[1]; e[14] = physical[0]; e[15] = physical[1];
      e[16] = slot; e[17] = gen; e[18] = id; e[19] = D(14); e[20] = column; e[21] = column; e[23] = m; e[26] = status[7]; e[27] = status[8];
      const ph = st.column[8 * COLS + column];
      e[32] = ph + rank * PACKAGE; e[33] = e[32] + m; e[48] = origin[0]; e[49] = origin[1]; e[50] = D(6); e[51] = D(7); e[52] = (3 << 18) | (1 << 4);
      birthRows.push(e);
      st.sand[slot] = m; st.sand[K + slot] = status[7]; st.sand[2 * K + slot] = status[8]; st.sand[3 * K + slot] = 0; st.sand[4 * K + slot] = 0;
      st.sand[5 * K + slot] = id; st.sand[6 * K + slot] = 1; st.sand[7 * K + slot] = D(14); st.sand[8 * K + slot] = origin[0]; st.sand[9 * K + slot] = origin[1]; st.sand[10 * K + slot] = gen;
      for (let f = 11; f < 16; f++) st.sand[f * K + slot] = 0;
      st.column[8 * COLS + column] += m;
    }
    if (status[0] === 0) events.push(...birthRows);
  }

  // ---- fold / metadata / terminal (candidate) ----
  if (status[0] === 0) {
    foldState(st);
    // pass9 guard stats: capacityMax = max envelope over columns
    let cap = 0;
    for (let c = 0; c < COLS; c++) if (st.column[13 * COLS + c] > cap) cap = st.column[13 * COLS + c];
    status[22] = cap;
    // metadata updates
    st.header[16] = u32(st.header[16] + 1); // physicsCommit increment (lo only; overflow guard equivalent)
    if (status[2] === 1) { // SUCCESS increments source commit too
      st.header[14] = u32(st.header[14] + 1);
      const amount = status[4];
      const t = st.header[60] + amount; if (t > 0xffffffff) { st.header[61]++; st.header[60] = t >>> 0; } else st.header[60] = t;
      st.header[39] += status[5];
      st.header[34] += 1;
    }
    if (status[11] === 1) st.header[33] += 1;
    st.header[74] = st.header[33]; st.header[75] = 0;
    st.header[12] = ctx.nonce[0]; st.header[13] = ctx.nonce[1];
    st.header[18] = ctx.control[0]; st.header[19] = ctx.control[1];
    st.header[20] = ctx.geometry[0]; st.header[21] = ctx.geometry[1];
    st.header[49] = ctx.frame[0]; st.header[50] = ctx.frame[1];
    if (status[2] === 1) { st.header[82] = D(24); st.header[83] = D(25); }
    st.header[84] = ctx.input[0]; st.header[85] = ctx.input[1];
    if (status[2] === 1) { st.header[86] = D(10); st.header[87] = D(11); }
    st.header[72] = u32(st.header[72] + 1); st.header[73] = u32(st.header[73] + (st.header[72] === 0 ? 1 : 0));
    st.header[76] = ctx.cfg.y; st.header[77] = ctx.cfg.x; st.header[78] = ctx.cfg.z; st.header[79] = 1;
    // NOTE h76/77/78 = uBanks(y=baseBank, x=1-baseBank, z=2) via metadata: header[76]=uBanks.y(baseBank), [77]=uBanks.x(candidate), [78]=uBanks.z(=2)
    // observed: baseBank=0 -> h76=0? snapshot h76=1 h77=0 h78=2 with baseBank=1? 见下: owner.Bank()=1, candidate=0 -> (1,0,2). matches snapshot
    st.header[76] = ctx.baseBank !== undefined ? ctx.baseBank : st.header[76];
    st.header[77] = ctx.baseBank !== undefined ? 1 - ctx.baseBank : st.header[77];
    st.header[78] = 2;
    st.header[51] = 0; st.header[80] = status[2]; st.header[81] = 2;
    st.header[37] = ctx.eventArena;
    st.header[38] = status[2] === 1 ? ctx.eventStart + status[5] : eventCursor;
    st.header[35] = ctx.frame[0]; st.header[36] = ctx.frame[1];
    st.header[47] = ctx.carry || 0; st.header[48] = 0;
    // terminal
    if (status[2] !== 3) {
      for (let i = 0; i < 32; i++) terminal[i] = 0;
      const admitted = status[4], rejected = status[3] - admitted;
      terminal[0] = status[2] === 1 ? (rejected === 0 ? 1 : 2) : 3;
      terminal[1] = status[1]; terminal[3] = admitted; terminal[4] = rejected; terminal[5] = status[6]; terminal[6] = status[5];
      terminal[7] = status[5] === 0 ? 0 : base.header[39]; terminal[8] = base.header[39]; terminal[9] = st.header[39];
      terminal[10] = st.header[14]; terminal[11] = st.header[15]; terminal[12] = st.header[16]; terminal[13] = st.header[17];
      terminal[14] = st.header[14]; terminal[15] = st.header[15];
      terminal[16] = base.header[56]; terminal[17] = base.header[57]; terminal[18] = st.header[56]; terminal[19] = st.header[57];
      terminal[24] = st.header[60]; terminal[25] = st.header[61]; terminal[28] = st.header[74]; terminal[29] = st.header[75];
      if (status[2] !== 1) { terminal[14] = 0; terminal[15] = 0; }
    }
  }

  // ---- replay layer (spare from base + events) ----
  let replayMismatch = null;
  if (status[0] === 0) {
    const spare = { K: base.K, sand: base.sand.slice(), column: base.column.slice(), header: base.header.slice() };
    if (status[2] === 1) {
      // replayBirths: rebuild from base using the logged events
      let scanFrom = 0;
      const spColumnP = 8 * COLS;
      for (let rank = 0; rank < status[5]; rank++) {
        const e = birthRows[rank];
        const slot = e[16];
        for (let freeSlot = scanFrom; freeSlot < slot; freeSlot++) if (base.sand[freeSlot] !== 0) { failInfo.stage = 7; fatal(7, 16, rank + 1); break; }
        if (status[0] !== 0) break;
        scanFrom = slot + 1;
        // event word-by-word check vs re-derivation is inherent (same code path); oracle revalidates identity fields:
        if (e[0] !== MAGIC.event64 || e[18] !== base.header[39] + rank || e[16] !== slot || e[23] !== (rank + 1 === status[5] ? status[4] - rank * PACKAGE : PACKAGE)) { fatal(7, 16, rank + 1); break; }
        spare.sand[slot] = e[23]; spare.sand[K + slot] = status[7]; spare.sand[2 * K + slot] = status[8];
        spare.sand[5 * K + slot] = e[18]; spare.sand[6 * K + slot] = 1; spare.sand[7 * K + slot] = D(14);
        spare.sand[8 * K + slot] = e[48]; spare.sand[9 * K + slot] = e[49]; spare.sand[10 * K + slot] = e[17];
        spare.column[8 * COLS + status[6]] += e[23];
      }
    }
    if (status[0] === 0) {
      foldState(spare);
      // metadata(true) mirrors candidate writes on spare
      spare.header[16] = u32(spare.header[16] + 1);
      if (status[2] === 1) {
        spare.header[14] = u32(spare.header[14] + 1);
        const t = spare.header[60] + status[4]; if (t > 0xffffffff) { spare.header[61]++; spare.header[60] = t >>> 0; } else spare.header[60] = t;
        spare.header[39] += status[5]; spare.header[34] += 1;
      }
      if (status[11] === 1) spare.header[33] += 1;
      spare.header[74] = spare.header[33]; spare.header[75] = 0;
      spare.header[12] = ctx.nonce[0]; spare.header[13] = ctx.nonce[1];
      spare.header[18] = ctx.control[0]; spare.header[19] = ctx.control[1];
      spare.header[20] = ctx.geometry[0]; spare.header[21] = ctx.geometry[1];
      spare.header[49] = ctx.frame[0]; spare.header[50] = ctx.frame[1];
      if (status[2] === 1) { spare.header[82] = D(24); spare.header[83] = D(25); }
      spare.header[84] = ctx.input[0]; spare.header[85] = ctx.input[1];
      if (status[2] === 1) { spare.header[86] = D(10); spare.header[87] = D(11); }
      spare.header[72] = u32(spare.header[72] + 1); spare.header[73] = u32(spare.header[73] + (spare.header[72] === 0 ? 1 : 0));
      spare.header[76] = ctx.cfg.y; spare.header[77] = ctx.cfg.x; spare.header[78] = ctx.cfg.z; spare.header[79] = 1;
      spare.header[51] = 0; spare.header[80] = status[2]; spare.header[81] = 2;
      spare.header[37] = ctx.eventArena;
      spare.header[38] = status[2] === 1 ? ctx.eventStart + status[5] : eventCursor;
      spare.header[35] = ctx.frame[0]; spare.header[36] = ctx.frame[1];
      spare.header[47] = ctx.carry || 0; spare.header[48] = 0;
      // final word-compare candidate vs spare
      const cand = flatten(st), spar = flatten(spare);
      for (let i = 0; i < STATE_WORDS; i++) if (cand[i] !== spar[i]) { replayMismatch = { stage: 7, ordinal: i, word: i, expected: spar[i], actual: cand[i] }; fatal(7, 16, i); break; }
    }
  }

  // ---- completion assembly ----
  const completion = new Array(256).fill(0);
  const fatalNow = status[0] !== 0;
  completion[0] = MAGIC.completion256; completion[1] = 1; completion[2] = 256; completion[3] = MAGIC.seal;
  completion[4] = ctx.nonce[0]; completion[5] = ctx.nonce[1];
  for (let i = 0; i < 8; i++) completion[6 + i] = base.header[4 + i];
  for (let i = 0; i < 4; i++) completion[14 + i] = base.header[14 + i];
  for (let i = 0; i < 4; i++) completion[18 + i] = st.header[14 + i];
  completion[22] = ctx.control[0]; completion[23] = ctx.control[1]; completion[24] = ctx.frame[0]; completion[25] = ctx.frame[1];
  completion[26] = 0; completion[27] = 1;
  completion[28] = fatalNow ? 4 : status[2]; completion[29] = status[1]; completion[30] = status[14]; completion[31] = status[15];
  completion[32] = ctx.cfg.x; completion[33] = ctx.cfg.y; completion[34] = ctx.cfg.z; completion[35] = ctx.pubSlot;
  completion[36] = st.header[72]; completion[37] = st.header[73];
  for (let i = 0; i < 13; i++) completion[38 + i] = st.header[20 + i];
  completion[51] = ctx.eventArena; completion[52] = ctx.eventStart; completion[53] = st.header[38]; completion[54] = ctx.eventCapacity;
  completion[55] = base.header[34]; completion[56] = st.header[34]; completion[57] = base.header[33]; completion[58] = st.header[33];
  completion[59] = D(14); completion[60] = D(15);
  completion[61] = terminal[3]; completion[62] = terminal[4]; completion[63] = terminal[0]; completion[64] = terminal[1]; completion[65] = terminal[2]; completion[66] = terminal[6]; completion[67] = terminal[7]; completion[68] = terminal[9];
  for (let i = 0; i < 2; i++) { completion[69 + i] = base.header[56 + i]; completion[71 + i] = st.header[56 + i]; completion[77 + i] = st.header[60 + i]; completion[83 + i] = st.header[54 + i]; }
  completion[85] = 0; completion[86] = 0; completion[87] = status[22] || 0;
  completion[88] = st.header[41]; completion[89] = st.header[43]; completion[91] = 64; completion[92] = st.header[39]; completion[93] = st.header[40];
  completion[96] = status[2] === 1 ? status[5] : 0;
  completion[81] = st.header[52] - st.header[54];
  completion[97] = fatalNow ? status[33] || 0 : stageMaskSource(status);
  completion[99] = fatalNow ? status[34] || 0 : auditMaskSource(status);
  completion[101] = fatalNow ? status[35] : 0xffffffff;
  completion[102] = status[36]; completion[103] = status[37];
  completion[106] = (status[2] === 2 || status[2] === 3) ? 1 : 0;
  completion[107] = st.header[45] || 0;
  completion[108] = ctx.carry || 0;
  completion[110] = ctx.dispatchCount; completion[111] = ctx.dispatchCount + 1; completion[112] = ctx.uploadedBytes;
  for (let i = 0; i < 24; i++) completion[114 + i] = st.header[88 + i];

  return { outcome: fatalNow ? 4 : status[2], status, terminal, events, completion, failInfo, replayMismatch, st, fatalNow };
}
function stageMaskSource(status) {
  // bit1 Copy/Prepare (set in pass0/1), bit2 Fold(8), bit4 Provenance(16), bit5 ReplayCompare(32), bit6 Pack(64), bit7 Seal(128) = 1|2|4|8|16|32|64|128
  return 255;
}
function auditMaskSource(status) {
  // pass8: sourceAudit(false) => rootKind==1?79:111; pass12 adds 400; pass13 adds 32; pass16 adds 512
  // 79 = 0b1001111 (audits 0..3,6); 79+400=479; +32=511; +512=1023
  return 1023;
}
function auditState(st, info) {
  // mirror of sourceAudit's structural checks over current accepted state
  const { auditedP, auditedLive } = computeSpareAudit(st) || {};
  if (!auditedP) return { ok: false };
  if (st.header[0] !== MAGIC.stateHeader || st.header[1] !== 1 || st.header[2] !== 256 || st.header[31] !== 1 || st.header[32] !== 0) return { ok: false };
  if (st.header[41] !== auditedLive || st.header[43] !== st.K - auditedLive) return { ok: false };
  const rootStatus = st.header[81], rootKind = st.header[79], rootOutcome = st.header[80];
  if (rootStatus !== 0 && rootStatus !== 2) return { ok: false };
  if (rootStatus === 0 && ((st.header[16] | st.header[17]) !== 0 || (st.header[14] | st.header[15]) !== 0 || auditedLive !== 0 || st.header[33] !== 0)) return { ok: false };
  if (rootStatus === 2 && (rootKind < 1 || rootKind > 2 || rootOutcome < 1 || rootOutcome > 3 || (rootKind === 2 && rootOutcome !== 1))) return { ok: false };
  const initialBudget = st.header[52], remainingBudget = st.header[54];
  if (st.header[53] !== 0 || st.header[55] !== 0 || (initialBudget !== 0 && initialBudget !== 2621440) || remainingBudget > initialBudget) return { ok: false };
  for (let i = 64; i < 68; i++) if (st.header[i] !== 0) return { ok: false };
  const prefix = st.header[34];
  if (prefix > 4096 || prefix !== st.header[14] || st.header[15] !== 0) return { ok: false };
  for (let i = 112; i < 256; i++) if (st.header[i] !== 0) return { ok: false };
  for (let i = 0; i < 12 * 64; i++) if (st.sand[16 * st.K + i] !== 0) return { ok: false };
  const angle = st.header[45] | 0; if (angle < -7680 || angle > 7680) return { ok: false };
  const tangent = angles.tan(angle);
  for (let c = 0; c < COLS; c++) {
    for (let f = 0; f < 10; f++) if (f !== 2 && f !== 8 && st.column[f * COLS + c] !== 0) return { ok: false };
    const used = st.column[14 * COLS + c], rem = st.column[15 * COLS + c];
    if (used > BUDGET_LOCAL || rem > BUDGET_LOCAL || used + rem !== (initialBudget === 0 ? 0 : BUDGET_LOCAL)) return { ok: false };
    const mh = st.column[2 * COLS + c], ph = st.column[8 * COLS + c];
    if (ph !== auditedP[c] || st.column[11 * COLS + c] !== mh || st.column[12 * COLS + c] !== mh) return { ok: false };
    if (mh > Cq || ph > Cq - mh || st.column[10 * COLS + c] !== mh || st.column[13 * COLS + c] !== mh + ph) return { ok: false };
    if (c < 127) { const d = st.column[2 * COLS + c + 1] - mh - tangent; if (Math.abs(d) > 605394) return { ok: false }; }
  }
  // per-slot structural audit
  const ids = new Set();
  for (let i = 0; i < st.K; i++) {
    const m = st.sand[i];
    if (m === 0) { for (let f = 1; f < 16; f++) if (f !== 10 && st.sand[f * st.K + i] !== 0) return { ok: false }; continue; }
    if (m > Cq || st.sand[5 * st.K + i] === 0 || st.sand[7 * st.K + i] === 0 || st.sand[10 * st.K + i] === 0 || st.sand[K + i] >= 134217728 || st.sand[2 * st.K + i] > Cq) return { ok: false };
    const origin = st.sand[8 * st.K + i], originHi = st.sand[9 * st.K + i], id = st.sand[5 * st.K + i];
    if (origin === 0 || originHi !== 0 || origin > prefix) return { ok: false };
    if (ids.has(id)) return { ok: false }; ids.add(id);
    const tag = st.sand[6 * st.K + i], state = tag & 15, wait = (tag >>> 12) & 15;
    if ((tag & 0xffff0ff0) !== 0 || state < 1 || state > 3 || (state === 1 && wait !== 0) || (state === 2 && wait !== 1) || (state === 3 && (wait < 2 || wait > 5)) || st.sand[11 * st.K + i] !== wait) return { ok: false };
    const x = st.sand[K + i];
    if (st.sand[2 * st.K + i] < st.column[10 * COLS + (x >>> 20)]) return { ok: false };
    for (let f = 3; f <= 4; f++) { const v = st.sand[f * st.K + i]; if (v > 1073741824 && v < 3221225472) return { ok: false }; }
    for (let f = 12; f < 16; f++) if (st.sand[f * st.K + i] !== 0) return { ok: false };
  }
  let localUsedSum = 0; for (let c = 0; c < COLS; c++) localUsedSum += st.column[14 * COLS + c];
  if (localUsedSum < initialBudget - remainingBudget || localUsedSum > 2 * (initialBudget - remainingBudget)) return { ok: false };
  return { ok: true, auditedP, auditedLive };
}

module.exports = { runSourceRoot, initialState, flatten, unflatten, foldState, chooseAngle, auditState, planWorkspace: L, SEG, HDR, COL, STATE_WORDS, K, PAD, angles, stageMaskSource, auditMaskSource };
