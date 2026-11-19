import worker, { MessageEvents } from '@ohos.worker';
interface G9RxArchiveMessage { type: string; mode?: string; directory?: string; path?: string; error?: string; }
// UI only exchanges bounded commands and a final path. All raw pages, hashes
// and streaming IO stay in the worker; no complete field enters the UI heap.
export class GpuG9RxCold {
  private current: worker.ThreadWorker | null = null;
  cancel(): void { this.current?.postMessage({ type: 'CANCEL' } as G9RxArchiveMessage); }
  collect(mode: string, directory: string): Promise<string> {
    if (this.current !== null) return Promise.reject(new Error('WOULD_BLOCK'));
    return new Promise<string>((resolve, reject) => {
      const instance = new worker.ThreadWorker('entry/ets/workers/G9RxColdWorker.ets'); this.current = instance;
      instance.onmessage = (event: MessageEvents): void => {
        const reply = event.data as G9RxArchiveMessage;
        if (this.current !== instance) return;
        this.current = null; instance.terminate();
        if (reply.type === 'DONE' && reply.path !== undefined) resolve(reply.path); else reject(new Error(reply.error ?? 'INCOMPLETE'));
      };
      instance.onerror = (): void => { if (this.current === instance) { this.current = null; instance.terminate(); reject(new Error('ARCHIVE_WORKER_ERROR')); } };
      instance.postMessage({ type: 'BEGIN', mode: mode, directory: directory } as G9RxArchiveMessage);
    });
  }
}
