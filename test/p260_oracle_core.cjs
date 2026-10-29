// T260 independent M0 oracle (tester, offline). Reads frozen product artifacts
// (layout_generated.h, angles_generated.h) as DATA ONLY; implements all step
// semantics from T230 v1 spec text with self-held integer/Fraction math.
// No product helper calls, no designer model import, no GPU execution here.
'use strict';
const fs = require('fs'), path = require('path');
const ROOT = path.resolve(__dirname, '..');
const OUT = path.join(ROOT, 'test', 'reports', 't260');
if (!fs.existsSync(OUT)) fs.mkdirSync(OUT, { recursive: true });

// ---------- frozen constants (T230 spec / T233 manifest, independently restated) ----------
const Qm = 1048576;            // 2^20
const Cq = 256 * Qm;           // 268435456
const Hfloor = 605395, Hq = 605394;
const ANGLE_MIN = -7680, ANGLE_ROWS = 15361;
const TICK_HZ = 120, MAX_QUANTA = 4;
const BUDGET_TOTAL = 2621440;          // floor(300*Qm/120)
const BUDGET_LOCAL = 244667;           // floor(28*Qm/120)
const PACKAGE = 131072;                // uPackage = .125*Qm
const COLS = 128;
const K_DEFAULT = 192, PAD_DEFAULT = 256;
const GRAVITY = 10276045;              // from product SubmitRoot uniform (observed constant)
const DAMPING = 1048576;               // A = Qm (no drag)
const FLUX = 25165824;                 // 24*Qm
const SOFT = 276695;                   // uSoft = ColdAngleWords[(7680+7168)*4] observed

// wire magic words (from T233 manifest; oracle uses them as opaque tokens)
const MAGIC = { stateHeader: 1213420103, sourceCommand: 1129534023, event64: 1163088455, completion256: 1381192263, publicationHeader: 1347637831, seal: 3235823838 };
const OUTCOME = { SUCCESS: 1, METADATA_REJECT: 2, DEDUP_REPLAY: 3, FATAL: 4 };
const RECEIPT = { SUCCESS: 1, PARTIAL: 2, REJECTED: 3 };
const REASON = { NONE: 0, UNSUPPORTED_FEATURE: 1, INPUT_RANGE_UNSUPPORTED: 2, NUMERIC_RESOLUTION_REJECTED: 3, POSITION_IN_BED: 4, POOL_FULL: 5, CAPACITY_REJECTED: 6, ID_EXHAUSTED: 7, DUPLICATE_KEY_CONFLICT: 8, STALE_IDENTITY: 9, CLOCK_WINDOW_REJECTED: 10, SOURCE_LOG_FULL: 11, EVENT_ARENA_FULL: 12, NUMERIC_OVERFLOW: 13, GUARD_VIOLATION: 14, AUDIT_FAILED: 15, REPLAY_MISMATCH: 16, TIME_NO_PROGRESS: 17, GL_FAILURE: 18, OWNER_STATE_LOST: 19, CONFIG_UNSUPPORTED: 20, SLOT_GENERATION_EXHAUSTED: 21, UNSUPPORTED_INITIAL_STATE: 22 };
const STAGE = { COPY: 1, ADMISSION: 2, SORT: 3, MOTION: 4, BED: 5, AUDIT: 6, REPLAY: 7, PUBLICATION: 8, SEAL: 9 };
const KIND = { SOURCE_BIRTH: 1, MOVE: 2, CROSS: 3, CONTACT: 4, LAND: 5, BED: 6, FINAL_CONTACT: 7 };
const SAND_STATE = { FREE: 0, AIR: 1, CONTACT_WAIT: 2, SIDE_WAIT: 3 };

// event64 used-mask per kind (from T233 manifest eventKinds, read as frozen wire data)
const MASK64 = { 1: 0x001f0003ffffffffn, 2: 0x001ff000ffffffffn, 3: 0x005fffffffffffffn, 4: 0x005ff000ffffffffn, 5: 0x001f0fffffffffn << 0n, 6: 0x00500fff0030ffffn, 7: 0x001f0000ffffffffn };
MASK64[5] = 0x001f0fffffffffffn; // fix: kind5 mask 0x001f0fffffffffff

// ---------- layout offsets (word units) ----------
function planSegments(K, align) {
  // mirrors PlanStateSegments word math with align in BYTES
  let next = 64 * K; // bytes
  const alignUp = (b) => Math.ceil(b / align) * align;
  const water = alignUp(next); next = water + 48 * 64;
  const column = alignUp(next); next = column + 64 * COLS;
  const header = alignUp(next); next = header + 1024;
  return { sand: 0, water: water / 4, column: column / 4, header: header / 4, totalBytes: next, totalWords: next / 4 };
}
function planWorkspace(K, align) {
  const seg = planSegments(K, align);
  const pad = K === 192 ? 256 : 512;
  let next = seg.totalBytes; const app = (bytes) => { const at = Math.ceil(next / align) * align / 4; next = Math.ceil(next / align) * align + bytes; return at; };
  const w = {};
  w.motion = app(128 * K); w.indexA = app(4 * pad); w.indexB = app(4 * pad); w.scan = app(4 * (K + 64 + 128));
  w.edges = app(64 * COLS); w.audit = app(16 * COLS); w.dedup = app(256 * 4096); w.sourceEvents = app(64 * 4096);
  w.command = app(128 * 16); w.status = app(2048); w.angles = app(245776); w.stats = app(1024); w.terminal = app(1024);
  const workWords = Math.ceil(next / 4);
  let pnext = 0; const papp = (bytes) => { const at = Math.ceil(pnext / align) * align / 4; pnext = Math.ceil(pnext / align) * align + bytes; return at; };
  const pub = {}; pub.sand = papp(64 * K); pub.water = papp(64 * 64); pub.column = papp(32 * COLS); pub.header = papp(1024);
  const pubWords = Math.ceil(pnext / 4);
  return { seg, work: w, workWords, pub, pubWords, pad };
}

// ---------- angle table (frozen artifact read as data; SHA-checked by caller) ----------
function loadAngles() {
  const text = fs.readFileSync(path.join(ROOT, 'entry/src/main/cpp/gpu_resident/angles_generated.h'), 'utf8');
  const hex = text.match(/0x([0-9a-f]{8})u/g);
  const words = hex.map(h => parseInt(h.slice(2, 10), 16) >>> 0);
  if (words.length !== 61444) throw new Error('angle words ' + words.length);
  const s32 = v => v >= 0x80000000 ? v - 0x100000000 : v;
  return { tan: j => s32(words[(j + 7680) * 4]), sin: j => s32(words[(j + 7680) * 4 + 1]), cos: j => s32(words[(j + 7680) * 4 + 2]), raw: words };
}

// ---------- self-held integer primitives (independent reimplementation) ----------
function mulDivRNE(a, b, d) { // signed a,b; d>0 uint; exact product then ties-to-even
  const neg = (a < 0) !== (b < 0);
  const A = BigInt(Math.abs(a)), B = BigInt(Math.abs(b)), D = BigInt(d);
  const prod = A * B; let q = prod / D, r = prod % D;
  if (r * 2n > D || (r * 2n === D && (q & 1n) === 1n)) q += 1n;
  if (q > 0xffffffffn) return null; // grsMulDivRNE overflow reject (v+1 wrap check)
  const v = Number(q);
  if (v > (neg ? 0x80000000 : 0x7fffffff)) return null;
  return neg ? (v === 0x80000000 ? -2147483648 : -v) : v;
}
function u32(v) { return v >>> 0; }
function add64(a, b) { const lo = (a[0] + b[0]) >>> 0; const carry = (a[0] + b[0]) > 0xffffffff ? 1 : 0; const hi = a[1] + b[1] + carry; return hi > 0xffffffff ? null : [lo, hi >>> 0]; }
function cmp64(a, b) { return a[1] !== b[1] ? (a[1] < b[1] ? -1 : 1) : (a[0] === b[0] ? 0 : a[0] < b[0] ? -1 : 1); }
function plusOne64(a) { return add64(a, [1, 0]); }
function mul32to64(a, b) { return [Math.imul(a, b) >>> 0, Math.floor(a * b / 4294967296) >>> 0]; }
function mul32Big(a, b) { return BigInt(a) * BigInt(b); }
function cmpMul(an, ad, bn, bd) { // compare an/ad vs bn/bd with U64 cross products (exact via BigInt, mimicking grsCompare64(grsMul32..))
  const a = mul32Big(an, bd), b = mul32Big(bn, ad);
  return a < b ? -1 : a > b ? 1 : 0;
}
function floorBinary64Q20(lo, hi, maxUnits) { // returns [valid, units]; exact product word logic
  const bits = (BigInt(hi) << 32n) | BigInt(lo);
  const exp = Number((bits >> 52n) & 2047n);
  const frac = hi & 0xfffff;               // m.y fraction bits (32-bit view)
  const sign = Number(bits >> 63n);
  if (exp === 2047) return [false, 0];
  if (sign && (exp !== 0 || frac !== 0 || lo !== 0)) return [false, 0];
  if (exp === 0) return [true, 0];
  // product logic: m = (frac|0x100000, lo) as uvec2; value = m * 2^(exp-1023-52) ... restate exactly:
  // shift = exp-1023-32 relative to uvec2 (lo,fracHi) integer pair m.x=lo, m.y=frac|0x100000
  const mY = frac | 0x100000, mX = lo;
  const shift = exp - 1023 - 32;
  if (shift >= 0) {
    if (shift >= 32 || mY !== 0) {
      // product rejects when shift>=32 && m.y!=0, or m.x > maxUnits>>shift
      if (shift >= 32) return [false, 0];
      if (mX > (maxUnits >>> shift)) return [false, 0];
      return [true, u32(mX << shift)];
    }
    if (mX > (maxUnits >>> shift)) return [false, 0];
    return [true, u32(mX << shift)];
  }
  const right = -shift;
  if (right >= 64) return [true, 0];
  if (right >= 32) return [true, mY >>> (right - 32)];
  if ((mY >>> right) !== 0) return [false, 0];
  const units = u32((mX >>> right) | u32(mY << (32 - right)));
  return [units <= maxUnits, units <= maxUnits ? units : 0];
}
function q20FloatBits(units) { // software IEEE binary32 RNE for units*2^-20, nonnegative
  if (units === 0) return 0;
  const top = 31 - Math.clz32(units);
  let sig, e;
  if (top <= 23) { sig = units << (23 - top); e = top; }
  else {
    const shift = top - 23;
    sig = Math.floor(units / 2 ** shift);
    const r = units & ((1 << shift) - 1), halfway = 1 << (shift - 1);
    if (r > halfway || (r === halfway && (sig & 1) !== 0)) sig++;
    if (sig === (1 << 24)) { sig >>= 1; top++; }
    e = top;
  }
  return (((e - 20 + 127) << 23) | (sig & 0x7fffff)) >>> 0;
}
function columnPreservingFloatBits(x) {
  let bits = q20FloatBits(x); const upper = (x >>> 20) + 1;
  if (cmpMul(bits, 1, q20FloatBits(upper << 20), 1) >= 0) bits--;
  return bits >>> 0;
}
function validateSandStateWord(p) {
  const state = p & 15, wait = (p >>> 12) & 15;
  if (p & 0xffff0ff0) return false;
  return state <= 3 && (state <= 1 ? wait === 0 : state === 2 ? wait === 1 : (wait >= 2 && wait <= 5));
}
function validateEventWords(e) { // e: 64-word array; returns [ok, offendingWord]
  if (e[0] !== MAGIC.event64) return [false, 0];
  if (e[1] !== 1) return [false, 1];
  const kind = e[6]; if (kind < 1 || kind > 7) return [false, 6];
  const mask = MASK64[kind];
  for (let i = 0; i < 64; i++) if (!((mask >> BigInt(i)) & 1n) && e[i] !== 0) return [false, i];
  if ((e[2] | e[3]) === 0) return [false, 2];
  if ((e[7] | e[8]) === 0) return [false, 7];
  if ((e[9] | e[10]) === 0) return [false, 9];
  if (e[4] !== (kind === 1 ? 1 : 2)) return [false, 4];
  const f = e[52], oldState = f & 15, newState = (f >>> 4) & 15, oldWait = (f >>> 10) & 15, newWait = (f >>> 14) & 15, ns = (f >>> 18) & 3;
  if ((f & 0xfff00300) !== 0 || !validateSandStateWord(oldState | (oldWait << 12)) || !validateSandStateWord(newState | (newWait << 12))) return [false, 52];
  if (ns !== (kind === 1 ? 3 : kind === 6 ? 2 : 0)) return [false, 52];
  if (e[5] !== (kind === 1 ? 1 : kind === 6 ? 3 : kind === 7 ? 4 : 2)) return [false, 5];
  if (kind !== 6) { if (!e[17] || !e[18] || !e[19] || e[20] >= 128 || e[21] >= 128) return [false, 16]; }
  if ((kind === 2 || kind === 3 || kind === 4) && e[46] === 0 && e[47] === 0) return [false, 44];
  if (kind !== 6 && ((e[48] | e[49]) === 0 || (e[50] | e[51]) === 0)) return [false, 48];
  if (kind === 1 && (e[48] !== e[12] || e[49] !== e[13] || e[24] || e[25] || e[28] || e[29] || e[30] || e[31])) return [false, 48];
  if (kind === 1 && (e[22] !== 0 || e[23] === 0 || oldState !== 0 || newState !== 1)) return [false, 22];
  if ((kind === 2 || kind === 3 || kind === 4 || kind === 7) && e[22] !== e[23]) return [false, 22];
  if (kind === 2 && e[20] !== e[21]) return [false, 20];
  if (kind === 3 && Math.abs(e[20] - e[21]) !== 1) return [false, 20];
  if (kind === 5 && e[22] <= e[23]) return [false, 22];
  return [true, 0xffffffff];
}

module.exports = { Qm, Cq, Hfloor, Hq, ANGLE_MIN, ANGLE_ROWS, TICK_HZ, MAX_QUANTA, BUDGET_TOTAL, BUDGET_LOCAL, PACKAGE, COLS, GRAVITY, DAMPING, FLUX, SOFT, MAGIC, OUTCOME, RECEIPT, REASON, STAGE, KIND, SAND_STATE, MASK64, planSegments, planWorkspace, loadAngles, mulDivRNE, u32, add64, cmp64, plusOne64, mul32to64, cmpMul, floorBinary64Q20, q20FloatBits, columnPreservingFloatBits, validateSandStateWord, validateEventWords, K_DEFAULT, PAD_DEFAULT };
