import worker, { MessageEvents } from '@ohos.worker';
import { G11ColdArchiveCore } from '../physics/G11ColdArchiveCore';
interface G11ArchiveRequest { type: string; kind?: string; token?: string; mode?: string; directory?: string; }
const archive = new G11ColdArchiveCore();
worker.workerPort.onmessage = (event: MessageEvents): void => {
 const request = event.data as G11ArchiveRequest;
 if (request.type === 'CANCEL') { archive.cancel(); return; }
 if (request.type !== 'BEGIN' || request.kind === undefined || request.token === undefined || request.mode === undefined || request.directory === undefined) { worker.workerPort.postMessage({ type: 'FAIL', error: 'BAD_REQUEST' }); return; }
 archive.collect(request.kind, request.token, request.mode, request.directory).then((path: string): void => { worker.workerPort.postMessage({ type: 'DONE', path: path }); }).catch((error: Error): void => { worker.workerPort.postMessage({ type: 'FAIL', error: error.message }); });
};
