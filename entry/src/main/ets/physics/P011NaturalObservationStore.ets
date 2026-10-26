import { P011EndpointCommitKey } from './P011EndpointMetrics';
import worker, { ErrorEvent, MessageEvents } from '@ohos.worker';
import common from '@ohos.app.ability.common';
import hilog from '@ohos.hilog';
import { P011ExternalAdmissionRecord } from './P011ExternalAdmissions';
import { P011NaturalObservationRecord } from './P011NaturalObservation';

export interface P011StoreProgress {
  runId: string; storeGeneration: number; requestId: string; state: string;
  sentRecords: number; sentAdmissions: number; writtenRecords: number; writtenAdmissions: number;
  lastCommitKey: P011EndpointCommitKey | null;
  identityRevision: string; routeId: string; lastSequence: number; lastSourceEpoch: number;
  filePath: string; admissionFilePath: string; lastFrameId: number; lastSubstepId: number; lastAttemptId: string | null; lastCommittedOrdinal: number | null;
}
let nextStoreGeneration: number = 0;
export interface P011NaturalObservationStore {
  inspect(): P011StoreProgress;
  finish(runId: string, requestId: string, storeGeneration: number): Promise<void>;
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
  private readonly runId: string;
  private readonly generation: number = ++nextStoreGeneration;
  private state: string = 'OPEN'; private requestId: string = '';
  private writtenRecords: number = 0; private writtenAdmissions: number = 0;
  private lastSequence: number = 0; private lastSourceEpoch: number = 0;
  private lastCommitKey: P011EndpointCommitKey | null = null;
  private routeId: string = '';
  private lastFrameId: number = 0; private lastSubstepId: number = 0;
  private lastAttemptId: string | null = null; private lastCommittedOrdinal: number | null = null;
  inspect(): P011StoreProgress { return {runId:this.runId,storeGeneration:this.generation,requestId:this.requestId,state:this.state,
    sentRecords:this.sentRecords,sentAdmissions:this.sentAdmissions,writtenRecords:this.writtenRecords,writtenAdmissions:this.writtenAdmissions,
    lastCommitKey:this.lastCommitKey,identityRevision:'P011_COMMIT_IDENTITY_V2',routeId:this.routeId,lastSequence:this.lastSequence,lastSourceEpoch:this.lastSourceEpoch,
    filePath:this.filePath,admissionFilePath:this.admissionFilePath,lastFrameId:this.lastFrameId,lastSubstepId:this.lastSubstepId,
    lastAttemptId:this.lastAttemptId,lastCommittedOrdinal:this.lastCommittedOrdinal}; }
  finish(runId: string, requestId: string, generation: number): Promise<void> {
    if(runId!==this.runId || generation!==this.generation || !requestId) return Promise.reject(new Error('OBS_STORE_FINISH_IDENTITY'));
    if(this.requestId && this.requestId!==requestId)return Promise.reject(new Error('OBS_STORE_FINISH_REQUEST_CONFLICT'));
    this.requestId=requestId;return this.close();
  }
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

  constructor(filePath: string, runId: string) {
    this.runId=runId;
    this.filePath = filePath;
    this.admissionFilePath = filePath.replace(/\.ndjson$/, '-admissions.ndjson');
    this.ready = new Promise<void>((resolve, reject) => { this.readyResolve = resolve; this.readyReject = reject; });
    this.workerInstance = new worker.ThreadWorker('entry/ets/workers/P011NaturalObservationWorker.ets');
    this.workerInstance.onmessage = (event: MessageEvents): void => {
      const response = event.data as StoreResponse;
      if (response.type === 'ready') { this.readyResolve?.(); this.readyResolve = null; this.readyReject = null; }
      else if (response.type === 'written') {
        if(response.sequence !== undefined) {
          if(response.sequence!==this.writtenRecords+1 || response.sequence>this.sentRecords){this.fail('OBS_STORE_WRITTEN_RECORDS');return;}
          this.writtenRecords=response.sequence;
        }
        if(response.admissions !== undefined) {
          if(response.admissions!==this.writtenAdmissions+1 || response.admissions>this.sentAdmissions){this.fail('OBS_STORE_WRITTEN_ADMISSIONS');return;}
          this.writtenAdmissions=response.admissions;
        }
        if((this.writtenRecords+this.writtenAdmissions)%64===0 || this.closing)
          hilog.info(0x0000,'WaterSand','P011_OBS_STORE_PROGRESS %{public}s',JSON.stringify(this.inspect()));
      }
      else if (response.type === 'closed') {
        if (response.sequence !== this.sentRecords || response.admissions !== this.sentAdmissions ||
          response.sequence !== this.writtenRecords || response.admissions !== this.writtenAdmissions || !this.closing) {
          this.fail('OBS_STORE_INCOMPLETE'); return;
        }
        this.state='CLOSED';
        hilog.info(0x0000,'WaterSand','P011_OBS_STORE_FENCE %{public}s',JSON.stringify(this.inspect()));
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
    this.lastSequence=record.context.sequence;this.lastSourceEpoch=record.context.sourceEpoch ?? this.lastSourceEpoch;this.routeId=record.context.observationRouteId ?? this.routeId;
    this.lastFrameId=record.context.frameId;this.lastSubstepId=record.context.substepId;
    this.lastAttemptId=record.context.attemptId ?? null;this.lastCommittedOrdinal=record.context.endpointCommittedOrdinal ?? null;
    if(record.context.commitStatus==='committed' && record.context.endpointCommittedOrdinal !== undefined && record.context.endpointCommittedOrdinal !== null) {
      this.lastCommitKey={identityRevision:record.context.identityRevision,runId:record.context.runId,
        sequence:record.context.sequence,frameId:record.context.frameId,substepId:record.context.substepId,
        attemptId:record.context.attemptId ?? null,committedOrdinal:record.context.endpointCommittedOrdinal};
    }
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
    this.closing = true;this.state='DRAINING';
    hilog.info(0x0000,'WaterSand','P011_OBS_STORE_CLOSE_REQUESTED %{public}s',JSON.stringify(this.inspect()));
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
    this.failed = true;this.state='FAILED';
    hilog.error(0x0000,'WaterSand','P011_OBS_STORE_FENCE_ERROR %{public}s',JSON.stringify(this.inspect()));
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
  const store = new FileStore(filePath,runId);
  await store.ready;
  return store;
}
