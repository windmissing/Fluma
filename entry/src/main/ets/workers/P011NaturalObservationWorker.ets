import { P011ExternalAdmissionRecord } from '../physics/P011ExternalAdmissions';
import util from '@ohos.util';
import worker, { MessageEvents } from '@ohos.worker';
import fs from '@ohos.file.fs';
import { P011NaturalObservationRecord } from '../physics/P011NaturalObservation';

interface ObservationWorkerRequest {
  type: string;
  filePath?: string;
  record?: P011NaturalObservationRecord;
  admissionFilePath?: string;
  admission?: P011ExternalAdmissionRecord;
}

interface ObservationWorkerResponse {
  type: string;
  sequence?: number;
  admissions?: number;
  message?: string;
}

const workerPort = worker.workerPort;
let ledgerFile: fs.File | null = null;
let writtenCount = 0;
let admissionCount = 0;
let admissionFile: fs.File | null = null;
let failed = false;
const encoder = new util.TextEncoder();
function writeComplete(file: fs.File, value: string): void {
  const bytes = encoder.encodeInto(value);
  let offset = 0;
  while (offset < bytes.byteLength) {
    const written = fs.writeSync(file.fd, bytes.buffer.slice(offset, bytes.byteLength));
    if (!Number.isInteger(written) || written <= 0 || written > bytes.byteLength - offset) throw new Error('OBS_STORE_SHORT_WRITE');
    offset += written;
  }
  fs.fsyncSync(file.fd);
}

function post(response: ObservationWorkerResponse): void {
  workerPort.postMessage(response);
}

workerPort.onmessage = (event: MessageEvents): void => {
  const request = event.data as ObservationWorkerRequest;
  try {
    if (failed) throw new Error('OBS_STORE_ALREADY_FAILED');
    if (request.type === 'init') {
      if (ledgerFile !== null || request.filePath === request.admissionFilePath || typeof request.admissionFilePath !== 'string' || request.admissionFilePath.length === 0 || typeof request.filePath !== 'string' || request.filePath.length === 0) {
        post({ type: 'error', message: 'OBS_STORE_INVALID_PATH' });
        return;
      }
      ledgerFile = fs.openSync(request.filePath,
        fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE | fs.OpenMode.TRUNC);
      admissionFile = fs.openSync(request.admissionFilePath, fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE | fs.OpenMode.TRUNC);
      post({ type: 'ready' });
      return;
    }
    if (request.type === 'append') {
      if (ledgerFile === null || request.record === undefined) {
        post({ type: 'error', message: 'OBS_STORE_NOT_READY' });
        return;
      }
      const line = JSON.stringify(request.record) + '\n';
      writeComplete(ledgerFile, line);
      writtenCount += 1;
      post({ type: 'written', sequence: writtenCount });
      return;
    }
    if (request.type === 'admission') {
      if (admissionFile === null || request.admission === undefined) throw new Error('OBS_STORE_NOT_READY');
      writeComplete(admissionFile, JSON.stringify(request.admission) + '\n');
      admissionCount++;
      post({ type: 'written', admissions: admissionCount });
      return;
    }
    if (request.type === 'close') {
      if (ledgerFile !== null) {
        fs.fsyncSync(ledgerFile.fd);
        fs.closeSync(ledgerFile);
        ledgerFile = null;
      }
      if (admissionFile !== null) { fs.fsyncSync(admissionFile.fd); fs.closeSync(admissionFile); admissionFile = null; }
      post({ type: 'closed', sequence: writtenCount, admissions: admissionCount });
      return;
    }
    post({ type: 'error', message: 'OBS_STORE_UNKNOWN_REQUEST' });
  } catch (error) {
    failed = true;
    if (ledgerFile !== null) { try { fs.closeSync(ledgerFile); } catch (closeError) {} ledgerFile = null; }
    if (admissionFile !== null) { try { fs.closeSync(admissionFile); } catch (closeError) {} admissionFile = null; }
    post({ type: 'error', message: `OBS_STORE_WRITE_FAILED:${error}` });
  }
};
