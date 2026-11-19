import worker, { MessageEvents } from '@ohos.worker';
import { G9RxColdArchiveCore } from '../physics/G9RxColdArchiveCore';
interface G9RxArchiveRequest { type: string; mode?: string; directory?: string; }
const archive = new G9RxColdArchiveCore();
worker.workerPort.onmessage = (event: MessageEvents): void => {
  const request = event.data as G9RxArchiveRequest;
  if (request.type === 'CANCEL') { archive.cancel(); return; }
  if (request.type !== 'BEGIN' || request.mode === undefined || request.directory === undefined) { worker.workerPort.postMessage({ type: 'FAIL', error: 'BAD_REQUEST' }); return; }
  archive.collect(request.mode, request.directory).then((path: string): void => { worker.workerPort.postMessage({ type: 'DONE', path: path }); }).catch((error: Error): void => { worker.workerPort.postMessage({ type: 'FAIL', error: error.message }); });
};
