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
  P011_V3_RATE_PRIMARY_DEG_PER_S, P011_V3_RATE_SENSITIVITY_DEG_PER_S,
  P011_V3_COMMAND_STEP_MS
} from './P011ScheduleV3';
import { P011RelaxHysteresisParamError } from './P011RelaxHysteresis';
import { validateP011RunMetadataGroup, validateP011ExecIdentityGroup } from './P011TiltWant';

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
  | 'INVALID_SCHEDULE_METADATA'
  // §26.6 fail-closed execution-identity arming: the identity triple is a
  // second optional group, and the two groups must appear together or not
  // at all (group XOR binding).
  | 'MISSING_SCHEDULE_EXEC_IDENTITY'
  | 'INVALID_SCHEDULE_EXEC_IDENTITY'
  | 'SCHEDULE_GROUP_BINDING_MISMATCH';

export interface P011ScheduleWantParseResult {
  /** peakADeg:rate binding; null when the parameter is absent. */
  peakADeg: number | null;
  rateDegPerSec: number | null;
  warnings: P011ScheduleWantWarning[];
  // §26.5 metadata arming: the validated four-parameter group (all-null on
  // pure smoke, i.e. a schedule want without the metadata parameters).
  metadata: P011RunMetadataArming | null;
  // §26.6 execution identity: the validated triple (non-null iff metadata
  // is non-null — the XOR binding rejects any other combination).
  execIdentity: P011ExecIdentityArming | null;
}

/** §26.5: validated run metadata carried by the schedule want. */
export interface P011RunMetadataArming {
  runRole: 'calibration' | 'holdout';
  thresholdCalibrationId: string;
  thetaStartDeg: number;
  thetaStopDeg: number;
}

/** §26.6: validated execution identity carried by the schedule want. */
export interface P011ExecIdentityArming {
  execSourceCommit: string;
  execSourceFileSha256: string;
  execBuildSha256: string;
}

/**
 * preregistration-P011-correction-matrix-v1.md §4 (G1/G2/G0 strata): a
 * CONDITIONING PREFIX injected before the formal schedule — a short
 * 0 -> +15 -> 0 -> -15 -> 0 tilt sequence (platform holds) that drives
 * the field from the reset state toward the stratum's working initial
 * state before the formal run's first command. The formal schedule then
 * starts seamlessly (its H0 command follows the prefix's final 0 hold).
 * 'none' (default) = no prefix — the pilot's same-initial-state degraded
 * path is unaffected.
 */
export type P011ConditioningPrefix = 'none' | 'G1' | 'G2' | 'G0';

/** Conditioning prefix spec: injected commands + total duration. */
export interface P011ConditioningSpec {
  kind: P011ConditioningPrefix;
  commands: P011ScheduleV3Command[];
  durationMs: number;
}

/**
 * Build the conditioning prefix commands: 0(hold) -> +15(hold) -> 0(hold)
 * -> -15(hold) -> 0(hold), 1000 ms per hold, 1 deg/s ramps between (the
 * ramp steps ride the same 100 ms command grid). The sequence tilts the
 * field in both directions once — enough to develop the competitive /
 * hot-spot working state before the formal run starts.
 */
export function p011BuildConditioningPrefix(
  kind: P011ConditioningPrefix): P011ConditioningSpec | null {
  if (kind === 'none') {
    return null;
  }
  const commands: P011ScheduleV3Command[] = [];
  const holdMs = 1000;
  const rampRate = 1.0; // deg/s
  const stepMs = P011_V3_COMMAND_STEP_MS;
  let offset = 0;
  let sequence = 0;
  const push = (targetTiltDeg: number, phase: string): void => {
    sequence += 1;
    commands.push({
      commandSequence: sequence,
      offsetMs: offset,
      segmentId: 'CND_' + phase,
      phaseKind: 'platform',
      legId: '',
      targetTiltDeg: targetTiltDeg,
      rateDegPerSec: 0
    });
  };
  const rampTo = (target: number, from: number): void => {
    const steps = Math.max(1, Math.round(Math.abs(target - from) /
      (rampRate * stepMs / 1000)));
    for (let index = 1; index <= steps; index++) {
      sequence += 1;
      const value = from + (target - from) * index / steps;
      commands.push({
        commandSequence: sequence,
        offsetMs: offset,
        segmentId: 'CND_R',
        phaseKind: 'ramp',
        legId: 'CND_R',
        targetTiltDeg: Math.round(value * 10) / 10,
        rateDegPerSec: rampRate
      });
      offset += stepMs;
    }
  };
  const hold = (tilt: number, phase: string): void => {
    push(tilt, phase);
    offset += holdMs;
  };
  hold(0, 'H0');
  rampTo(15, 0);
  hold(15, 'H15');
  rampTo(0, 15);
  hold(0, 'HZ');
  rampTo(-15, 0);
  hold(-15, 'Hm15');
  rampTo(0, -15);
  hold(0, 'HE');
  return { kind: kind, commands: commands, durationMs: offset };
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
    return { peakADeg: null, rateDegPerSec: null, warnings, metadata: null,
      execIdentity: null };
  }
  const text = `${parameters.p011schedule}`.trim();
  const match = /^([+-]?(?:\d+(?:\.\d*)?|\.\d+)):([+-]?(?:\d+(?:\.\d*)?|\.\d+))$/
    .exec(text);
  if (match === null) {
    warnings.push('INVALID_P011SCHEDULE_FORMAT');
    return { peakADeg: null, rateDegPerSec: null, warnings, metadata: null,
      execIdentity: null };
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
  // §26.6 execution-identity group: independent second group, same shared
  // validator family (commit 6..64 hex, file/build SHA 64 hex — the wstilt
  // path uses the identical rules).
  const execText = (key: string): string | null => {
    const raw = parameters[key] === undefined
      ? null : `${parameters[key]}`.trim();
    return raw === null || raw.length === 0 ? null : raw;
  };
  const identityGroup = validateP011ExecIdentityGroup(
    execText('execsourcecommit'), execText('execsourcefilesha256'),
    execText('execbuildsha256'));
  let execIdentity: P011ExecIdentityArming | null = null;
  if (identityGroup.warning !== null) {
    warnings.push(identityGroup.warning);
  } else if (!identityGroup.allAbsent) {
    execIdentity = {
      execSourceCommit: identityGroup.execSourceCommit as string,
      execSourceFileSha256: identityGroup.execSourceFileSha256 as string,
      execBuildSha256: identityGroup.execBuildSha256 as string
    };
  }
  // §26.6 group XOR binding (point 3): the metadata group and the identity
  // group must appear TOGETHER or not at all. Formal run with a missing
  // identity would produce executedThresholds with empty identity strings
  // (EXECUTION_THRESHOLD_IDENTITY_MISMATCH); identity without metadata
  // would have nowhere to land (the machine is not armed). Both rejected
  // here — the third-stoppage state dies at the entry point.
  const metadataPresent = metadata !== null;
  const identityPresent = execIdentity !== null;
  if (metadataPresent !== identityPresent) {
    warnings.push('SCHEDULE_GROUP_BINDING_MISMATCH');
    metadata = null;
    execIdentity = null;
  }
  return { peakADeg, rateDegPerSec, warnings, metadata, execIdentity };
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
  // preregistration §4: conditioning prefix state (null = no prefix).
  private conditioning: P011ConditioningSpec | null = null;
  private conditioningDone = false;

  getState(): P011SchedulePlayerState {
    return this.stateValue;
  }

  isActive(): boolean {
    return this.stateValue === 'playing';
  }

  /**
   * Build and start. Returns false (and stays inactive) on invalid A/rate.
   * The optional conditioning prefix (preregistration §4) runs FIRST —
   * its commands dispatch on the same clock, and the formal schedule's
   * offsets are shifted by the prefix duration so H0 follows seamlessly.
   */
  start(peakADeg: number, rateDegPerSec: number, startWallMs: number,
    conditioning: P011ConditioningSpec | null = null): boolean {
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
    this.conditioning = conditioning;
    this.conditioningDone = conditioning === null;
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
    this.conditioning = null;
    this.conditioningDone = false;
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
    // Conditioning prefix first: its offsets start at t0; the formal
    // schedule's commands are all shifted by the prefix duration.
    if (this.conditioning !== null && !this.conditioningDone) {
      for (const command of this.conditioning.commands) {
        const dueAt = this.startWallMs + command.offsetMs;
        if (nowMs < dueAt) {
          // Prefix still running — the formal schedule has not started.
          return dispatches;
        }
        const driftMs = nowMs - dueAt;
        dispatches.push({
          command,
          scheduledOffsetMs: command.offsetMs,
          actualDispatchMs: nowMs,
          driftMs,
          driftExceeded: driftMs > P011_SCHEDULE_DRIFT_MARKER_MS
        });
      }
      this.conditioningDone = true;
      // Shift the formal schedule's clock origin to after the prefix.
      this.startWallMs += this.conditioning.durationMs;
      if (dispatches.length > 0) {
        return dispatches;
      }
    }
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
