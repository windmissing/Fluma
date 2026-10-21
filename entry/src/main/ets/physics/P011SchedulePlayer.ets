/**
 * P-011 observation-3 v3 schedule player (§26.4, EXECUTION_CHANNEL_BLOCKED
 * remediation, option 1).
 *
 * The player is a COMMAND GENERATOR, not an execution mode: it replays the
 * pre-registered nine-phase-30-v3 command list on a monotonic wall clock
 * (100 ms command step / 5100 ms platform holds), and every dispatched
 * command flows through the EXISTING input path — setTilt target override
 * plus the normal planner+guard substep pipeline. No direct-drive bypass:
 * clamped/held/BLOCKED outcomes appear and land in the ledger exactly like
 * any other input (§26.4 attached confirmation).
 *
 * Two frozen interface decisions (§26.4):
 *  1. Mutual exclusion with the single-command want channel — while the
 *     player is active a fresh single command is NOT consumed and does NOT
 *     advance its sequence; the want parse layer emits
 *     SCHEDULE_ACTIVE_COMMAND_REJECTED (hilog only, never the physics
 *     ledger — a rejected command never became an input). Symmetric the
 *     other way: a schedule want arriving during a single-command session
 *     is rejected the same way.
 *  2. Drift is recorded, never chased — each command records
 *     scheduledOffsetMs and actualDispatchMs; the attempt layer carries
 *     scheduleDriftMs = actual − scheduled (no schema bump). Drift > 50 ms
 *     additionally flags SCHEDULE_DRIFT_EXCEEDED (a marker, not an error).
 *     Catch-up would fabricate the beat the protocol §3.2 gates exist to
 *     protect; the checker consumes the drift field under the existing
 *     input-timing gates (drift judgement belongs to the tester domain).
 *
 * Lifecycle (risk 1): the activation flag MUST be cleared on reset/exit so
 * a crashed player can never permanently reject single commands. The
 * reset()/forceStop() path and the "player crashed then single command
 * recovers" case are unit-covered.
 */

import { P011ScheduleV3, P011ScheduleV3Command, p011BuildScheduleV3,
  P011_V3_RATE_PRIMARY_DEG_PER_S, P011_V3_RATE_SENSITIVITY_DEG_PER_S
} from './P011ScheduleV3';
import { P011RelaxHysteresisParamError } from './P011RelaxHysteresis';
import { validateP011RunMetadataGroup } from './P011TiltWant';

/** Drift marker threshold (§26.4 decision 2): > 50 ms flags the attempt. */
export const P011_SCHEDULE_DRIFT_MARKER_MS = 50;

export type P011ScheduleWantWarning =
  | 'INVALID_P011SCHEDULE_FORMAT'
  | 'INVALID_P011SCHEDULE_PEAK'
  | 'INVALID_P011SCHEDULE_RATE'
  | 'SCHEDULE_BUILD_FAILED'
  // §26.5 fail-closed metadata arming: a partial or invalid four-parameter
  // group rejects the whole schedule want.
  | 'MISSING_SCHEDULE_METADATA'
  | 'INVALID_SCHEDULE_METADATA';

export interface P011ScheduleWantParseResult {
  /** peakADeg:rate binding; null when the parameter is absent. */
  peakADeg: number | null;
  rateDegPerSec: number | null;
  warnings: P011ScheduleWantWarning[];
  // §26.5 metadata arming: the validated four-parameter group (all-null on
  // pure smoke, i.e. a schedule want without the metadata parameters).
  metadata: P011RunMetadataArming | null;
}

/** §26.5: validated run metadata carried by the schedule want. */
export interface P011RunMetadataArming {
  runRole: 'calibration' | 'holdout';
  thresholdCalibrationId: string;
  thetaStartDeg: number;
  thetaStopDeg: number;
}

/**
 * Parse the p011schedule want parameter ("peakADeg:rate", e.g. "20:0.5").
 * Absent parameter -> nulls (no schedule run). Illegal format / out-of-range
 * A / non-frozen rate -> warnings; the caller must not start a player on a
 * warned parse (same three-state style as the p011route parameter).
 */
export function parseP011ScheduleWant(
  parameters: Record<string, Object> | undefined): P011ScheduleWantParseResult {
  const warnings: P011ScheduleWantWarning[] = [];
  if (parameters === undefined || parameters.p011schedule === undefined) {
    return { peakADeg: null, rateDegPerSec: null, warnings, metadata: null };
  }
  const text = `${parameters.p011schedule}`.trim();
  const match = /^([+-]?(?:\d+(?:\.\d*)?|\.\d+)):([+-]?(?:\d+(?:\.\d*)?|\.\d+))$/
    .exec(text);
  if (match === null) {
    warnings.push('INVALID_P011SCHEDULE_FORMAT');
    return { peakADeg: null, rateDegPerSec: null, warnings, metadata: null };
  }
  const peakADeg = parseFloat(match[1]);
  const rateDegPerSec = parseFloat(match[2]);
  if (!Number.isFinite(peakADeg) || peakADeg <= 0 || peakADeg >= 30) {
    warnings.push('INVALID_P011SCHEDULE_PEAK');
  }
  if (rateDegPerSec !== P011_V3_RATE_PRIMARY_DEG_PER_S &&
    rateDegPerSec !== P011_V3_RATE_SENSITIVITY_DEG_PER_S) {
    warnings.push('INVALID_P011SCHEDULE_RATE');
  }
  // §26.5 fail-closed metadata arming: the four optional parameters are
  // validated as a GROUP through the shared validator (same rules as the
  // wstilt path — no drift). Partial/invalid group rejects the whole want.
  const runRoleText = parameters.p011runrole === undefined
    ? '' : `${parameters.p011runrole}`.trim();
  const calibrationId = parameters.thresholdcalibrationid === undefined
    ? null : `${parameters.thresholdcalibrationid}`.trim();
  const startText = parameters.thetastartdeg === undefined
    ? null : `${parameters.thetastartdeg}`.trim();
  const stopText = parameters.thetastopdeg === undefined
    ? null : `${parameters.thetastopdeg}`.trim();
  const numeric = (t: string | null): number | null => {
    if (t === null || !/^[+-]?(?:\d+(?:\.\d*)?|\.\d+)$/.test(t)) {
      return null;
    }
    const v = parseFloat(t);
    return Number.isFinite(v) ? v : null;
  };
  const group = validateP011RunMetadataGroup(runRoleText, calibrationId,
    numeric(startText), numeric(stopText));
  let metadata: P011RunMetadataArming | null = null;
  if (group.warning !== null) {
    warnings.push(group.warning);
  } else if (!group.allAbsent) {
    metadata = {
      runRole: group.runRole as 'calibration' | 'holdout',
      thresholdCalibrationId: group.thresholdCalibrationId as string,
      thetaStartDeg: group.thetaStartDeg as number,
      thetaStopDeg: group.thetaStopDeg as number
    };
  }
  return { peakADeg, rateDegPerSec, warnings, metadata };
}

/** One dispatched schedule command with its drift evidence. */
export interface P011ScheduleDispatch {
  command: P011ScheduleV3Command;
  /** The schedule's own offset for this command (from t0 of the run). */
  scheduledOffsetMs: number;
  /** Monotonic wall clock at dispatch (same clock the player started on). */
  actualDispatchMs: number;
  /** actualDispatch − scheduledOffset (§26.4 decision 2; >= 0 by design). */
  driftMs: number;
  driftExceeded: boolean;
}

export type P011SchedulePlayerState = 'inactive' | 'playing' | 'finished';

/**
 * Replay driver over one built schedule. tick(nowMs) is called from the
 * frame callback; it dispatches every command whose offsetMs has come due
 * (a slow frame may dispatch several — §26.4 risk 3: setTilt is
 * last-write-wins target semantics, the natural extension of the existing
 * want behavior; drift records the window truthfully).
 */
export class P011SchedulePlayer {
  private schedule: P011ScheduleV3 | null = null;
  private startWallMs = 0;
  private nextIndex = 0;
  private stateValue: P011SchedulePlayerState = 'inactive';

  getState(): P011SchedulePlayerState {
    return this.stateValue;
  }

  isActive(): boolean {
    return this.stateValue === 'playing';
  }

  /** Build and start. Returns false (and stays inactive) on invalid A/rate. */
  start(peakADeg: number, rateDegPerSec: number, startWallMs: number): boolean {
    try {
      this.schedule = p011BuildScheduleV3(peakADeg, rateDegPerSec);
    } catch (error) {
      if (error instanceof P011RelaxHysteresisParamError) {
        this.schedule = null;
        this.stateValue = 'inactive';
        return false;
      }
      const message = error instanceof Error ? error.message : 'unknown';
      throw new P011RelaxHysteresisParamError(
        `v3 schedule build failed unexpectedly: ${message}`);
    }
    this.startWallMs = startWallMs;
    this.nextIndex = 0;
    this.stateValue = 'playing';
    return true;
  }

  /**
   * Lifecycle cleanup (§26.4 risk 1): clear the activation flag so single
   * commands can never be permanently rejected by a crashed player. Called
   * on reset / page exit / any teardown path.
   */
  forceStop(): void {
    this.schedule = null;
    this.nextIndex = 0;
    this.stateValue = 'inactive';
  }

  /**
   * Dispatch all due commands. Returns the dispatches (with drift evidence)
   * in schedule order; empty when nothing is due or the player is inactive.
   * When the last command dispatches the player transitions to 'finished'
   * (no longer active — mutual exclusion lifts).
   */
  tick(nowMs: number): P011ScheduleDispatch[] {
    const schedule = this.schedule;
    if (schedule === null || this.stateValue !== 'playing') {
      return [];
    }
    const dispatches: P011ScheduleDispatch[] = [];
    while (this.nextIndex < schedule.commands.length) {
      const command = schedule.commands[this.nextIndex];
      const dueAt = this.startWallMs + command.offsetMs;
      if (nowMs < dueAt) {
        break;
      }
      const driftMs = nowMs - dueAt;
      dispatches.push({
        command,
        scheduledOffsetMs: command.offsetMs,
        actualDispatchMs: nowMs,
        driftMs,
        driftExceeded: driftMs > P011_SCHEDULE_DRIFT_MARKER_MS
      });
      this.nextIndex += 1;
    }
    if (this.nextIndex >= schedule.commands.length) {
      this.stateValue = 'finished';
    }
    return dispatches;
  }

  /** The command currently holding the target (last dispatched; test hook). */
  currentCommand(): P011ScheduleV3Command | null {
    if (this.schedule === null || this.nextIndex === 0) {
      return null;
    }
    return this.schedule.commands[this.nextIndex - 1];
  }
}
