import worker, { MessageEvents } from '@ohos.worker';
import { G11ColdArchiveCore } from '../physics/G11ColdArchiveCore';
import { G11ArchiveMessage } from '../physics/G11WorkerProtocol';

let requestId: string = '';
let archive: G11ColdArchiveCore | null = null;
worker.workerPort.onmessage = (event: MessageEvents): void => {
 const request = event.data as G11ArchiveMessage;
 if (request.type === 'HELLO' && requestId === '' && request.requestId.length > 0) {
  requestId = request.requestId;
  worker.workerPort.postMessage({ type: 'ENTRY_ACK', requestId: requestId } as G11ArchiveMessage);
  return;
 }
 if (requestId === '' || request.requestId !== requestId) return;
 if (request.type === 'CANCEL') { archive?.cancel(); return; }
 if (request.type !== 'BEGIN' || archive !== null) return;
 worker.workerPort.postMessage({ type: 'BEGIN_SEEN', requestId: requestId } as G11ArchiveMessage);
 if (request.kind === undefined || request.token === undefined || request.mode === undefined || request.directory === undefined) {
  worker.workerPort.postMessage({ type: 'FAIL', requestId: requestId, error: 'BAD_REQUEST' } as G11ArchiveMessage);
  return;
 }
 archive = new G11ColdArchiveCore();
 archive.collect(request.kind, request.token, request.mode, request.directory, (raw: string): void => {
  worker.workerPort.postMessage({ type: 'NATIVE_REJECTED', requestId: requestId, error: raw } as G11ArchiveMessage);
 }).then((path: string): void => {
  worker.workerPort.postMessage({ type: 'DONE', requestId: requestId, path: path } as G11ArchiveMessage);
 }).catch((error: Error): void => {
  worker.workerPort.postMessage({ type: 'FAIL', requestId: requestId, error: error.message } as G11ArchiveMessage);
 });
};
