import native from 'libgrsm1.so';
import { utf8Bytes } from './GpuUtf8Budget';
import { DiagnosticColdFiles } from './DiagnosticColdFiles';
interface CallIdentity582 { route?: string; api?: string; bootStamp?: string; diagGeneration?: string; attemptId?: string; }
interface InputReply582 extends CallIdentity582 { revision?: string; ok?: boolean; state?: string; code?: string; inputCall?: CallIdentity582; firstFailure?: CallIdentity582 | null; error?: string; identity?: number[]; features?: number; }
interface AttemptRecord582 { revision: string; state: string; reason: string; request: string; reply: string; selected: string; first: string; }
class AttemptSlot582 {
  last: string = ''; first: string = ''; busy: boolean = false;
  pendingLast: string = ''; pendingFirst: string = '';
  directory: string = ''; notify: (state: string) => void = (): void => {};
}
// Only two retained records per route, including pending IO. No Promise history.
export class GpuInputAttempt {
  private static slots: AttemptSlot582[] = [new AttemptSlot582(), new AttemptSlot582(), new AttemptSlot582()];
  private static request: number = 0;
  private static bytes(text: string): number { return utf8Bytes(text); }
  private static object(raw: string, limit: number): InputReply582 {
    if (typeof raw !== 'string' || raw.length === 0 || raw.length > limit || GpuInputAttempt.bytes(raw) > limit) throw new Error('NATIVE_REPLY_UTF8_BOUND');
    let parsed: Object;
    try { parsed = JSON.parse(raw); } catch (e) { throw new Error('LEGACY_NON_JSON_REPLY'); }
    if (parsed === null || typeof parsed !== 'object' || Array.isArray(parsed)) throw new Error('NATIVE_REPLY_NOT_OBJECT');
    return parsed as InputReply582;
  }
  private static id(value: string | undefined): boolean { return typeof value === 'string' && /^[1-9][0-9]{0,19}$/.test(value) && (value.length < 20 || value <= '18446744073709551615'); }
  private static exact(found: CallIdentity582, expected: CallIdentity582): boolean { return found.route === expected.route && found.api === expected.api && found.bootStamp === expected.bootStamp && found.diagGeneration === expected.diagGeneration && found.attemptId === expected.attemptId; }
  static requested(raw: string, state: string): boolean { const value = GpuInputAttempt.object(raw, 4096); return value.revision === 'GPU_NAPI_SUCCESS_V1' && value.ok === true && value.state === state; }
  static call(route: string, api: string, words: Uint32Array, schema: string, directory: string, invoke: (value: Uint32Array) => string, notify: (state: string) => void): string {
    const immutable: number[] = []; for (let i: number = 0; i < words.length; i++) immutable.push(words[i]);
    if (GpuInputAttempt.request >= Number.MAX_SAFE_INTEGER) throw new Error('UI_REQUEST_SEQUENCE_EXHAUSTED');
    const request: string = JSON.stringify({ uiRequest: String(++GpuInputAttempt.request), route: route, api: api, schema: schema, type: 'Uint32Array', length: words.length, byteOffset: words.byteOffset, byteLength: words.byteLength, backingBytes: words.buffer.byteLength, words: immutable });
    if (GpuInputAttempt.bytes(request) > 2048) throw new Error('UI_WORDS_UTF8_BOUND');
    let callIdentity: CallIdentity582 = {};
    let reply: string = ''; let selected: string = ''; let first: string = ''; let reason: string = ''; let rawRecord: string = '';
    try {
      reply = invoke(words);
      const response = GpuInputAttempt.object(reply, 4096); const identity: CallIdentity582 = response.inputCall ?? response;
      callIdentity = { route: identity.route, api: identity.api, bootStamp: identity.bootStamp, diagGeneration: identity.diagGeneration, attemptId: identity.attemptId };
      if (identity.route !== route || identity.api !== api || !GpuInputAttempt.id(identity.bootStamp) || !GpuInputAttempt.id(identity.diagGeneration) || !GpuInputAttempt.id(identity.attemptId)) throw new Error('INPUT_CALL_IDENTITY_MISSING');
      selected = native.gpuInputDiagnosticsV1(route, identity.attemptId ?? '', identity.bootStamp ?? '', identity.diagGeneration ?? '');
      const diagnostic = GpuInputAttempt.object(selected, 4096);
      if ((diagnostic.revision !== 'GPU_INPUT_DIAGNOSTIC_V1' && diagnostic.revision !== 'GPU_INPUT_DIAGNOSTIC_V2') || !GpuInputAttempt.exact(diagnostic, identity)) throw new Error('INPUT_DIAGNOSTIC_IDENTITY_MISMATCH:' + (diagnostic.error ?? 'UNKNOWN'));
      const ref: CallIdentity582 | null | undefined = diagnostic.firstFailure;
      if (ref !== null && ref !== undefined) {
        if (ref.route !== route || ref.bootStamp !== identity.bootStamp || ref.diagGeneration !== identity.diagGeneration || !GpuInputAttempt.id(ref.attemptId)) throw new Error('FIRST_FAILURE_IDENTITY_INVALID');
        first = native.gpuInputDiagnosticsV1(route, ref.attemptId ?? '', ref.bootStamp ?? '', ref.diagGeneration ?? '');
        if (!GpuInputAttempt.exact(GpuInputAttempt.object(first, 4096), ref)) throw new Error('FIRST_FAILURE_SNAPSHOT_MISMATCH');
      }
    } catch (error) { reason = String(error); }
    const record: AttemptRecord582 = { revision: 'T582_INPUT_ATTEMPT_FILE_V1', state: reason === '' ? 'COMPLETE' : 'INCOMPLETE', reason: reason, request: request, reply: reply.length <= 4096 ? reply : 'REPLY_OVER_BOUND', selected: selected, first: first };
    rawRecord = JSON.stringify(record);
    if (GpuInputAttempt.bytes(rawRecord) > 16384) rawRecord = JSON.stringify({ revision: 'T582_INPUT_ATTEMPT_FILE_V1', state: 'INCOMPLETE', reason: 'ATTEMPT_ENCODING_UTF8_BOUND', request: '', reply: '', selected: '', first: '' });
    const index: number = route === 'M3A' ? 0 : route === 'M3B' ? 1 : 2;
    const slot: AttemptSlot582 = GpuInputAttempt.slots[index]; slot.directory = directory; slot.notify = notify;
    if (!DiagnosticColdFiles.retain(index * 2, GpuInputAttempt.bytes(rawRecord))) throw new Error('INPUT_RETAIN_CREDIT');
    slot.last = rawRecord; slot.pendingLast = rawRecord;
    slot.pendingFirst = JSON.stringify({ revision: 'T582_FIRST_FAILURE_FILE_V1', route: route, currentCall: callIdentity, state: first === '' ? 'NO_FIRST_FAILURE' : 'COMPLETE', snapshot: first }); slot.first = slot.pendingFirst;
    if (!DiagnosticColdFiles.retain(index * 2 + 1, GpuInputAttempt.bytes(slot.pendingFirst))) throw new Error('INPUT_RETAIN_CREDIT');
    if (!slot.busy) { slot.busy = true; GpuInputAttempt.flush(slot, route).catch((error: Error): void => { slot.busy = false; slot.notify('INCOMPLETE_INPUT_FILE:' + String(error)); }); }
    if (reason !== '') { notify('INCOMPLETE_INPUT_DIAGNOSTIC:' + reason); throw new Error('INCOMPLETE_INPUT_DIAGNOSTIC:' + reason); }
    return reply;
  }
  private static async flush(slot: AttemptSlot582, route: string): Promise<void> {
    try {
      while (slot.pendingLast !== '' || slot.pendingFirst !== '') {
        const text: string = slot.pendingFirst !== '' ? slot.pendingFirst : slot.pendingLast;
        const kind: string = slot.pendingFirst !== '' ? 'first' : 'last';
        if (kind === 'first') slot.pendingFirst = ''; else slot.pendingLast = '';
        const stored = await new DiagnosticColdFiles(slot.directory).save('gpu-input-' + route + '-' + kind + '.json', text);
        slot.notify(stored.name + ' sha256=' + stored.sha256);
      }
    } finally { slot.busy = false; }
  }
  static waitSession(read: () => string, revision: string, features: number, live: () => boolean, ready: (identity: number[], source: number[]) => void, fault: (message: string) => void): void {
    const poll = (): void => {
      if (!live()) return;
      try {
        const raw: string = read(); const value = GpuInputAttempt.object(raw, 4096);
        if (value.revision === 'GPU_NAPI_ERROR_V1' && (value.code ?? '').indexOf('NOT_READY') >= 0) { setTimeout(poll, 33); return; }
        // Numeric M3/M5 revisions are compared without coercing their payload.
        const parsed = JSON.parse(raw) as Record<string, Object>;
        const actualRevision: Object = parsed['revision'];
        const numericRevision: boolean = revision === '4' || revision === '5';
        if (numericRevision ? typeof actualRevision !== 'number' || actualRevision !== Number(revision) : typeof actualRevision !== 'string' || actualRevision !== revision) throw new Error('SESSION_REVISION_INVALID');
        if (features !== 0 && value.features !== features || !Array.isArray(value.identity) || value.identity.length !== 18) throw new Error('SESSION_SCHEMA_INVALID');
        const identity: number[] = value.identity;
        for (const word of identity) if (!Number.isSafeInteger(word) || word < 0 || word > 4294967295) throw new Error('SESSION_IDENTITY_WORD_INVALID');
        const sourceRaw: Object = parsed['sourceControlGeneration']; const source: number[] = [1, 0];
        if (Array.isArray(sourceRaw)) { if (sourceRaw.length !== 2) throw new Error('SOURCE_GENERATION_INVALID'); for (let i: number = 0; i < 2; i++) { const word: Object = sourceRaw[i]; if (typeof word !== 'number' || !Number.isSafeInteger(word) || word < 0 || word > 4294967295) throw new Error('SOURCE_GENERATION_INVALID'); source[i] = word; } }
        if (live()) ready(identity.slice(), source);
      } catch (error) { if (live()) fault('SESSION_FAULT:' + String(error)); }
    };
    poll();
  }
}
