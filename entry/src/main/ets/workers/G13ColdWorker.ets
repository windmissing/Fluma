import worker, { MessageEvents } from '@ohos.worker';
import { G13ColdArchiveCore } from '../physics/G13ColdArchiveCore';
interface G13ArchiveRequest { type: string; token?: string; directory?: string; }
const archive = new G13ColdArchiveCore();
worker.workerPort.onmessage = (event: MessageEvents): void => {
  const request = event.data as G13ArchiveRequest;
  if (request.type === 'CANCEL') { archive.cancel(); return; }
  if (request.type !== 'BEGIN' || request.token === undefined || request.directory === undefined) { worker.workerPort.postMessage({ type: 'FAIL', error: 'BAD_REQUEST' }); return; }
  archive.collect(request.token, request.directory).then((path: string): void => { worker.workerPort.postMessage({ type: 'DONE', path: path }); }).catch((error: Error): void => { worker.workerPort.postMessage({ type: 'FAIL', error: error.message }); });
};
