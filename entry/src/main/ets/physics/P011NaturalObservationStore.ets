import worker, { ErrorEvent, MessageEvents } from '@ohos.worker';
import common from '@ohos.app.ability.common';
import hilog from '@ohos.hilog';
import { P011ExternalAdmissionRecord } from './P011ExternalAdmissions';
import { P011NaturalObservationRecord } from './P011NaturalObservation';

export interface P011NaturalObservationStore {
  readonly filePath: string;
  readonly admissionFilePath: string;
  appendAdmission(record: P011ExternalAdmissionRecord): void;
  append(record: P011NaturalObservationRecord): void;
  close(): Promise<void>;
}

interface StoreRequest {
  type: string;
  filePath?: string;
  record?: P011NaturalObservationRecord;
  admissionFilePath?: string;
  admission?: P011ExternalAdmissionRecord;
}

interface StoreResponse {
  type: string;
  sequence?: number;
  admissions?: number;
  message?: string;
}

class FileStore implements P011NaturalObservationStore {
  readonly filePath: string;
  readonly admissionFilePath: string;
  private readyResolve: (() => void) | null = null;
  private readyReject: ((error: Error) => void) | null = null;
  readonly ready: Promise<void>;
  private sentRecords = 0;
  private sentAdmissions = 0;
  private closing = false;
  private closePromise: Promise<void> | null = null;
  private readonly workerInstance: worker.ThreadWorker;
  private closeResolve: (() => void) | null = null;
  private closeReject: ((error: Error) => void) | null = null;
  private failed = false;

  constructor(filePath: string) {
    this.filePath = filePath;
    this.admissionFilePath = filePath.replace(/\.ndjson$/, '-admissions.ndjson');
    this.ready = new Promise<void>((resolve, reject) => { this.readyResolve = resolve; this.readyReject = reject; });
    this.workerInstance = new worker.ThreadWorker('entry/ets/workers/P011NaturalObservationWorker.ets');
    this.workerInstance.onmessage = (event: MessageEvents): void => {
      const response = event.data as StoreResponse;
      if (response.type === 'ready') { this.readyResolve?.(); this.readyResolve = null; this.readyReject = null; }
      else if (response.type === 'closed') {
        if (response.sequence !== this.sentRecords || response.admissions !== this.sentAdmissions) {
          this.fail('OBS_STORE_INCOMPLETE'); return;
        }
        hilog.info(0x0000, 'WaterSand', 'P011_OBS_STORE_CLOSED %{public}s', JSON.stringify({
          filePath: this.filePath, admissionFilePath: this.admissionFilePath,
          records: response.sequence, admissions: response.admissions, status: 'COMPLETE'
        }));
        this.closeResolve?.();
        this.closeResolve = null;
        this.closeReject = null;
        this.workerInstance.terminate();
      } else if (response.type === 'error') {
        this.fail(response.message ?? 'OBS_STORE_ERROR');
      }
    };
    this.workerInstance.onerror = (error: ErrorEvent): void => {
      this.fail(`OBS_STORE_WORKER_FAILED:${error.message}`);
    };
    this.workerInstance.onmessageerror = (): void => {
      this.fail('OBS_STORE_MESSAGE_ERROR');
    };
    this.post({ type: 'init', filePath: filePath, admissionFilePath: this.admissionFilePath });
  }

  append(record: P011NaturalObservationRecord): void {
    // Observability failures invalidate the artifact, never the physical step.
    if (this.failed) return;
    if (this.closing) { this.fail('OBS_STORE_APPEND_AFTER_CLOSE'); return; }
    this.sentRecords++;
    this.post({ type: 'append', record: record });
  }

  appendAdmission(record: P011ExternalAdmissionRecord): void {
    if (this.failed || this.closing) throw new Error('OBS_STORE_UNAVAILABLE');
    this.sentAdmissions++;
    this.post({ type: 'admission', admission: record });
    if (this.failed) throw new Error('OBS_STORE_POST_FAILED');
  }
  close(): Promise<void> {
    if (this.failed) {
      return Promise.reject(new Error('OBS_STORE_FAILED'));
    }
    if (this.closePromise !== null) return this.closePromise;
    this.closing = true;
    this.closePromise = new Promise<void>((resolve, reject) => {
      this.closeResolve = resolve;
      this.closeReject = reject;
      this.post({ type: 'close' });
    });
    return this.closePromise;
  }

  private post(request: StoreRequest): void {
    try {
      this.workerInstance.postMessage(request);
    } catch (error) {
      this.fail(`OBS_STORE_POST_FAILED:${error}`);
    }
  }

  private fail(message: string): void {
    if (this.failed) {
      return;
    }
    this.failed = true;
    this.readyReject?.(new Error(message)); this.readyReject = null; this.readyResolve = null;
    hilog.error(0x0000, 'WaterSand', 'P011_OBS_ERROR %{public}s', message);
    this.closeReject?.(new Error(message));
    this.closeResolve = null;
    this.closeReject = null;
    try {
      this.workerInstance.terminate();
    } catch (error) {
    }
  }
}

export async function createP011NaturalObservationStore(
  context: common.UIAbilityContext,
  runId: string,
  artifactPrefix: string = 'p011-v212-relax-hysteresis'
): Promise<P011NaturalObservationStore> {
  if (!/^[A-Za-z0-9._-]+$/.test(runId)) {
    throw new Error('OBS_STORE_INVALID_RUN_ID');
  }
  if (!/^[A-Za-z0-9._-]+$/.test(artifactPrefix)) {
    throw new Error('OBS_STORE_INVALID_ARTIFACT_PREFIX');
  }
  const filePath = `${context.cacheDir}/${artifactPrefix}-${runId}.ndjson`;
  const store = new FileStore(filePath);
  await store.ready;
  return store;
}
