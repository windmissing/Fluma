import worker, { ErrorEvent, MessageEvents } from '@ohos.worker';
import { G11ArchiveMessage } from './G11WorkerProtocol';

type WorkerReceipt = (stage: string, raw: string) => void;
const STARTUP_MS: number = 5000;

export class GpuG11Cold {
 private current: worker.ThreadWorker | null = null;
 private sequence: number = 0;
 private cancelCurrent: (() => void) | null = null;

 cancel(): void { this.cancelCurrent?.(); }

 probe(observe: WorkerReceipt): Promise<string> {
  return this.start(null, observe);
 }

 collect(kind: string, token: string, mode: string, directory: string, observe: WorkerReceipt): Promise<string> {
  const request: G11ArchiveMessage = { type: 'BEGIN', requestId: '', kind: kind, token: token,
   mode: mode, directory: directory };
  return this.start(request, observe);
 }

 private start(request: G11ArchiveMessage | null, observe: WorkerReceipt): Promise<string> {
  if (this.current !== null) return Promise.reject(new Error('WOULD_BLOCK'));
  if (this.sequence >= Number.MAX_SAFE_INTEGER) return Promise.reject(new Error('REQUEST_ID_EXHAUSTED'));
  const id: string = String(++this.sequence);
  return new Promise<string>((resolve, reject) => {
   const emit = (stage: string, raw: string): void => { observe(id + ':' + stage, raw); };
   let instance: worker.ThreadWorker;
   try {
    instance = new worker.ThreadWorker('entry/ets/workers/G11ColdWorker.ets');
   } catch (error) {
    emit('CONSTRUCT_ERROR', String(error));
    reject(new Error('CONSTRUCT_ERROR:' + String(error)));
    return;
   }
   this.current = instance;
   let timer: number = -1;
   let acked: boolean = false;
   let begun: boolean = false;
   let beginSeen: boolean = false;
   const finish = (stage: string, raw: string, result: string, exited: boolean = false): void => {
    if (this.current !== instance) return;
    clearTimeout(timer);
    this.current = null;
    this.cancelCurrent = null;
    emit(stage, raw);
    if (!exited) {
     try { instance.terminate(); } catch (error) { emit('TERMINATE_ERROR', String(error)); }
    }
    if (result !== '') resolve(result); else reject(new Error(stage + ':' + raw));
   };
   const post = (value: G11ArchiveMessage): boolean => {
    try {
     instance.postMessage(value);
     return true;
    } catch (error) {
     if (value.type === 'CANCEL') emit('CANCEL_POST_ERROR', String(error));
     else finish(value.type + '_POST_ERROR', String(error), '');
     return false;
    }
   };
   this.cancelCurrent = (): void => {
    if (!begun) { finish('CANCELLED_BEFORE_BEGIN', 'CANCELLED', ''); return; }
    if (post({ type: 'CANCEL', requestId: id })) emit('CANCEL_POSTED', '');
   };
   instance.onmessage = (event: MessageEvents): void => {
    if (this.current !== instance) return;
    const reply = event.data as G11ArchiveMessage;
    if (reply.requestId !== id) return;
    if (reply.type === 'ENTRY_ACK' && !acked) {
     acked = true;
     clearTimeout(timer);
     if (request === null) { finish('ENTRY_ACK', '', 'ENTRY_ACK'); return; }
     emit('ENTRY_ACK', '');
     request.requestId = id;
     begun = post(request);
    } else if (reply.type === 'BEGIN_SEEN' && begun) {
     beginSeen = true;
     emit('BEGIN_SEEN', '');
    } else if (reply.type === 'NATIVE_REJECTED' && beginSeen) {
     emit('NATIVE_REJECTED', reply.error ?? 'UNKNOWN');
    } else if (reply.type === 'DONE' && beginSeen && reply.path !== undefined) {
     finish('ARCHIVED', reply.path, reply.path);
    } else if (reply.type === 'FAIL') {
     finish('ARCHIVE_FAILED', reply.error ?? 'UNKNOWN', '');
    }
   };
   instance.onerror = (error: ErrorEvent): void => {
    finish('WORKER_ERROR', JSON.stringify({ message: error.message, filename: error.filename,
     line: error.lineno, column: error.colno }), '');
   };
   instance.onexit = (code: number): void => { finish('WORKER_EXIT', String(code), '', true); };
   timer = setTimeout(() => { finish('ENTRY_TIMEOUT', 'UNKNOWN', ''); }, STARTUP_MS);
   if (post({ type: 'HELLO', requestId: id })) emit('HELLO_POSTED', '');
  });
 }
}
