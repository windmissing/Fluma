import worker, { MessageEvents } from '@ohos.worker';
import fs from '@ohos.file.fs';
import { P011NaturalObservationRecord } from './P011NaturalObservation';

interface ObservationWorkerRequest {
  type: string;
  filePath?: string;
  record?: P011NaturalObservationRecord;
}

interface ObservationWorkerResponse {
  type: string;
  sequence?: number;
  message?: string;
}

const workerPort = worker.workerPort;
let ledgerFile: fs.File | null = null;
let writtenCount = 0;

function post(response: ObservationWorkerResponse): void {
  workerPort.postMessage(response);
}

workerPort.onmessage = (event: MessageEvents): void => {
  const request = event.data as ObservationWorkerRequest;
  try {
    if (request.type === 'init') {
      if (typeof request.filePath !== 'string' || request.filePath.length === 0) {
        post({ type: 'error', message: 'OBS_STORE_INVALID_PATH' });
        return;
      }
      ledgerFile = fs.openSync(request.filePath,
        fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE | fs.OpenMode.TRUNC);
      post({ type: 'ready' });
      return;
    }
    if (request.type === 'append') {
      if (ledgerFile === null || request.record === undefined) {
        post({ type: 'error', message: 'OBS_STORE_NOT_READY' });
        return;
      }
      const line = JSON.stringify(request.record) + '\n';
      fs.writeSync(ledgerFile.fd, line);
      fs.fsyncSync(ledgerFile.fd);
      writtenCount += 1;
      post({ type: 'written', sequence: writtenCount });
      return;
    }
    if (request.type === 'close') {
      if (ledgerFile !== null) {
        fs.fsyncSync(ledgerFile.fd);
        fs.closeSync(ledgerFile);
        ledgerFile = null;
      }
      post({ type: 'closed', sequence: writtenCount });
      return;
    }
    post({ type: 'error', message: 'OBS_STORE_UNKNOWN_REQUEST' });
  } catch (error) {
    post({ type: 'error', message: `OBS_STORE_WRITE_FAILED:${error}` });
  }
};
