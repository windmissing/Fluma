import { EvidenceArmConfig } from './GpuEvidenceCaptureCold';
import { T450_CAPTURE_LAUNCH_ENABLED } from './T450CaptureProfile';

export class CaptureLaunchDeclaration {
  readonly reason: string;
  readonly sha: string;
  constructor(reason: string, sha: string = '') { this.reason = reason; this.sha = sha; }
}
let cold: CaptureLaunchDeclaration | null = null;

export function initializeCaptureLaunch(route: Object | undefined, raw: Object | undefined): void {
  if (cold !== null) return;
  if (!T450_CAPTURE_LAUNCH_ENABLED) cold = new CaptureLaunchDeclaration('CAPTURE_UNSUPPORTED');
  else if (route !== true) cold = new CaptureLaunchDeclaration('CAPTURE_WRONG_ROUTE');
  else if (raw === undefined || raw === '') cold = new CaptureLaunchDeclaration('CAPTURE_SHA_MISSING');
  else if (typeof raw !== 'string' || raw.length !== 64 || /[^0-9A-Fa-f]/.test(raw)) {
    cold = new CaptureLaunchDeclaration('CAPTURE_SHA_INVALID');
  } else cold = new CaptureLaunchDeclaration('SHA_DECLARED', raw.toLowerCase());
}

export function captureLaunch(): CaptureLaunchDeclaration {
  return cold ?? new CaptureLaunchDeclaration('CAPTURE_SHA_MISSING');
}

export interface CaptureScope {
  epoch: number;
  readyEpoch: number;
  runLo: number;
  runHi: number;
  progressClockRevision?: string;
  pageSession?: string;
}

function word(n: number): boolean {
  return Number.isSafeInteger(n) && n >= 0 && n <= 4294967295;
}

export class CaptureArmAttempt {
  readonly schema: string = 'T450_CAPTURE_LAUNCH_V1';
  readonly source: string = 'HOST_DECLARED_COLD_WANT';
  validation: string = '';
  requestId: string = '';
  nativeReason: string = '';
  invalidField: string = '';
  config: EvidenceArmConfig | null = null;
}

export function prepareCaptureArm(declaration: CaptureLaunchDeclaration, scope: CaptureScope | null,
  active: boolean, pageSession: string): CaptureArmAttempt {
  const attempt = new CaptureArmAttempt();
  attempt.validation = declaration.reason;
  if (declaration.reason !== 'SHA_DECLARED') { attempt.invalidField = 'packageSha256'; return attempt; }
  if (scope === null) { attempt.validation = 'STATUS_FAULT'; return attempt; }
  const revision = scope.progressClockRevision ?? '';
  if (!active || !/^[1-9][0-9]{0,19}$/.test(pageSession) || pageSession !== scope.pageSession) {
    attempt.validation = 'STALE_PAGE_SESSION'; attempt.invalidField = 'pageSession'; return attempt;
  }
  if (!Number.isSafeInteger(scope.epoch) || scope.epoch < 1 || scope.epoch !== scope.readyEpoch) {
    attempt.validation = 'STALE_OR_UNREADY_EPOCH'; attempt.invalidField = 'epoch'; return attempt;
  }
  if (!word(scope.runLo) || !word(scope.runHi) || (scope.runLo === 0 && scope.runHi === 0)) {
    attempt.validation = 'INVALID_RUN'; attempt.invalidField = 'runLo/runHi'; return attempt;
  }
  if (revision !== 'GRS_M1_PROGRESS_CLOCK_V1' && revision !== 'GRS_M1_PROGRESS_CLOCK_V2') {
    attempt.validation = 'UNSUPPORTED_PROGRESS_REVISION'; attempt.invalidField = 'expectedRevision'; return attempt;
  }
  attempt.validation = 'CONFIG_READY';
  attempt.config = { packageSha256: declaration.sha, expectedRevision: revision, epoch: String(scope.epoch),
    runLo: scope.runLo, runHi: scope.runHi, pageSession: pageSession,
    durationNs: '2000000000', maxRows: 4096, maxBytes: 1048576 };
  return attempt;
}
