import worker, { MessageEvents } from '@ohos.worker';
interface G13ArchiveMessage { type: string; token?: string; directory?: string; path?: string; error?: string; }
// UI only exchanges bounded commands and a final path. All raw pages, hashes
// and streaming IO stay in the worker; no complete field enters the UI heap.
export class GpuG13Cold {
  private current: worker.ThreadWorker | null = null;
  cancel(): void { this.current?.postMessage({ type: 'CANCEL' } as G13ArchiveMessage); }
  collect(token: string, directory: string): Promise<string> {
    if (this.current !== null) return Promise.reject(new Error('WOULD_BLOCK'));
    return new Promise<string>((resolve, reject) => {
      const instance = new worker.ThreadWorker('entry/ets/workers/G13ColdWorker.ets'); this.current = instance;
      instance.onmessage = (event: MessageEvents): void => {
        const reply = event.data as G13ArchiveMessage;
        if (this.current !== instance) return;
        this.current = null; instance.terminate();
        if (reply.type === 'DONE' && reply.path !== undefined) resolve(reply.path); else reject(new Error(reply.error ?? 'INCOMPLETE'));
      };
      instance.onerror = (): void => { if (this.current === instance) { this.current = null; instance.terminate(); reject(new Error('ARCHIVE_WORKER_ERROR')); } };
      instance.postMessage({ type: 'BEGIN', token: token, directory: directory } as G13ArchiveMessage);
    });
  }
}
