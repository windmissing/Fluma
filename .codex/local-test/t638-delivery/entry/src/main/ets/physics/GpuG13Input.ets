import crypto from '@ohos.security.cryptoFramework';
import native from 'libgrs_g13.so';
import { G13ResearchConfig } from './G13ResearchConfig';
interface G13InputScope { run: string; context: string; surface: string; geometry: string; config: string; session: string; permission: string; control: string; }
interface G13InputState { commit: string; acceptedQuantum: string; consumedSourceSeq: string; motionBits: number[]; }
interface G13InputStatus { scope: G13InputScope; native: G13InputState; }
// Only packs explicit controls and their raw bytes. No field integration,
// pressure RHS, per-cell forcing or tracer trajectories run on the CPU.
export class GpuG13Input {
  private sequence: Uint32Array = new Uint32Array(2);
  private pair(words: Uint32Array, at: number, value: string): void {
    if (!/^(0|[1-9][0-9]{0,19})$/.test(value)) throw new Error('G13_U64');
    let lo: number = 0; let hi: number = 0;
    for (let i: number = 0; i < value.length; i++) { const n: number = lo * 10 + Number(value.charAt(i)); hi = hi * 10 + Math.floor(n / 4294967296); lo = n % 4294967296; if (hi > 4294967295) throw new Error('G13_U64_OVERFLOW'); }
    words[at] = lo; words[at + 1] = hi;
  }
  private add(words: Uint32Array, at: number, value: number): void { const n: number = words[at] + value; const hi: number = words[at + 1] + Math.floor(n / 4294967296); if (hi > 4294967295) throw new Error('G13_SEQUENCE_EXHAUSTED'); words[at] = n % 4294967296; words[at + 1] = hi; }
  private f32(words: Uint32Array, at: number, value: number): void { new DataView(words.buffer).setFloat32(at * 4, value, true); }
  async view(mode: number): Promise<string> {
    if (!Number.isInteger(mode) || mode < 0 || mode > 4) throw new Error('VIEW_MODE');
    const config = G13ResearchConfig.words(); const policy = new Uint32Array(64);
    policy[0] = 1462972743; policy[1] = 1; policy[2] = 64; policy[3] = mode;
    const values: number[] = [0, 2, -64, 64, -256, 256, -128, 128, 0.002, 0.025, 1];
    for (let i: number = 0; i < values.length; i++) this.f32(policy, 4 + i, values[i]);
    policy.set(config.slice(76, 84), 16); policy.set(config.slice(44, 52), 32); policy[63] = 0xc0de1301;
    const md = crypto.createMd('SHA256'); await md.update({ data: new Uint8Array(policy.buffer) }); const hash = (await md.digest()).data; const bytes = new DataView(hash.buffer);
    for (let i: number = 0; i < 8; i++) policy[24 + i] = bytes.getUint32(i * 4, true);
    const names: string[] = ['VELOCITY', 'TRACERS', 'VORTICITY', 'DYNAMIC_PRESSURE', 'TOTAL_PRESSURE']; return native.g13ViewMode(names[mode], policy);
  }
  async offer(opcode: number): Promise<string> {
    const raw: string = native.g13Status(); if (raw.length > 65536) throw new Error('G13_STATUS_BOUND');
    const status = JSON.parse(raw) as G13InputStatus; const s = status.scope; const state = status.native;
    if (state.commit === '0' || state.motionBits.length < 6) throw new Error('G13_NEEDS_ACCEPTED_FIELD');
    const words = new Uint32Array(128); words[0] = 1228091719; words[1] = 1; words[2] = 128; words[3] = 50;
    this.pair(words, 4, s.run); this.pair(words, 6, s.context); this.pair(words, 8, s.surface); this.pair(words, 10, s.geometry); this.pair(words, 12, s.config); this.pair(words, 14, s.session); this.pair(words, 16, s.permission); this.pair(words, 18, s.control);
    const consumed = new Uint32Array(2); this.pair(consumed, 0, state.consumedSourceSeq);
    if (consumed[1] > this.sequence[1] || (consumed[1] === this.sequence[1] && consumed[0] > this.sequence[0])) this.sequence.set(consumed);
    this.add(this.sequence, 0, 1); words.set(this.sequence, 20); words.set(this.sequence, 104); this.pair(words, 24, state.commit); this.pair(words, 26, state.acceptedQuantum);
    const config = G13ResearchConfig.words();
    words.set(config.slice(76, 84), 32); words.set(config.slice(36, 44), 40); words.set(config.slice(44, 52), 48); words.set(config.slice(52, 60), 56); words.set(config.slice(60, 68), 64); words.set(config.slice(68, 76), 72);
    words[88] = opcode;
    if (opcode === 1) { this.f32(words, 93, 0); this.f32(words, 94, -9.81); }
    else if (opcode === 2) { words[89] = 2; words[90] = state.motionBits[0]; this.f32(words, 91, 1); this.f32(words, 92, 4); }
    else if (opcode === 3) { words[95] = 1; this.f32(words, 96, 0.25); this.pair(words, 97, state.acceptedQuantum); this.add(words, 97, 4); words[99] = words[97]; words[100] = words[98]; this.add(words, 99, 120); }
    else throw new Error('G13_SOURCE_OPCODE');
    words[101] = config[32]; words[102] = config[33]; words[103] = 1; words[127] = 0xc0de1301;
    const md = crypto.createMd('SHA256'); await md.update({ data: new Uint8Array(words.buffer) }); const hash = (await md.digest()).data;
    const view = new DataView(hash.buffer); for (let i: number = 0; i < 8; i++) words[80 + i] = view.getUint32(i * 4, true);
    return native.g13Offer128(words);
  }
}
