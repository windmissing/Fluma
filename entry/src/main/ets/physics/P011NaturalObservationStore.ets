import worker, { ErrorEvent, MessageEvents } from '@ohos.worker';
import common from '@ohos.app.ability.common';
import hilog from '@ohos.hilog';
import { P011NaturalObservationRecord } from './P011NaturalObservation';

export interface P011NaturalObservationStore {
  readonly filePath: string;
  append(record: P011NaturalObservationRecord): void;
  close(): Promise<void>;
}

interface StoreRequest {
  type: string;
  filePath?: string;
  record?: P011NaturalObservationRecord;
}

interface StoreResponse {
  type: string;
  sequence?: number;
  message?: string;
}

class FileStore implements P011NaturalObservationStore {
  readonly filePath: string;
  private readonly workerInstance: worker.ThreadWorker;
  private closeResolve: (() => void) | null = null;
  private closeReject: ((error: Error) => void) | null = null;
  private failed = false;

  constructor(filePath: string) {
    this.filePath = filePath;
    this.workerInstance = new worker.ThreadWorker('entry/ets/physics/P011NaturalObservationWorker.ets');
    this.workerInstance.onmessage = (event: MessageEvents): void => {
      const response = event.data as StoreResponse;
      if (response.type === 'closed') {
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
    this.post({ type: 'init', filePath: filePath });
  }

  append(record: P011NaturalObservationRecord): void {
    if (this.failed) {
      return;
    }
    this.post({ type: 'append', record: record });
  }

  close(): Promise<void> {
    if (this.failed) {
      return Promise.reject(new Error('OBS_STORE_FAILED'));
    }
    return new Promise<void>((resolve, reject) => {
      this.closeResolve = resolve;
      this.closeReject = reject;
      this.post({ type: 'close' });
    });
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
  return new FileStore(filePath);
}
