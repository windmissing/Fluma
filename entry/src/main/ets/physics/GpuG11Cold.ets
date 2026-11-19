import worker, { MessageEvents } from '@ohos.worker';
interface G11ArchiveMessage { type: string; kind?: string; token?: string; mode?: string; directory?: string; path?: string; error?: string; }
export class GpuG11Cold {
 private current: worker.ThreadWorker | null = null;
 cancel(): void { this.current?.postMessage({ type: 'CANCEL' } as G11ArchiveMessage); }
 collect(kind: string, token: string, mode: string, directory: string): Promise<string> {
  if (this.current !== null) return Promise.reject(new Error('WOULD_BLOCK'));
  return new Promise<string>((resolve, reject) => {
   const instance = new worker.ThreadWorker('entry/ets/workers/G11ColdWorker.ets'); this.current = instance;
   instance.onmessage = (event: MessageEvents): void => {
    if (this.current !== instance) return;
    const reply = event.data as G11ArchiveMessage; this.current = null; instance.terminate();
    if (reply.type === 'DONE' && reply.path !== undefined) resolve(reply.path); else reject(new Error(reply.error ?? 'INCOMPLETE'));
   };
   instance.onerror = (): void => { if (this.current === instance) { this.current = null; instance.terminate(); reject(new Error('ARCHIVE_WORKER_ERROR')); } };
   instance.postMessage({ type: 'BEGIN', kind: kind, token: token, mode: mode, directory: directory } as G11ArchiveMessage);
  });
 }
}
