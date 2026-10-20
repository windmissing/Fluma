/**
 * P-011 observation-3 v3 schedule channel (GAP-2).
 *
 * Deterministic command generation for the v3 pre-registration protocol
 * (docs/observation-protocol-P011-v3-preregistration-v1.md §3): the platform
 * sequence 0 -> +15 -> +A -> +15 -> 0 -> -15 -> -A -> -15 -> 0 (H0..H8) with
 * eight ramp legs L1..L8, 5100 ms platform holds, a 100 ms command step and
 * the frozen rates 0.5 deg/s (primary, 0.05 deg per step) and 1.0 deg/s
 * (sensitivity, 0.10 deg per step). A is a schedule parameter (15.1..29.9
 * grid, bound from the shadow audit); nothing here defaults A to 30 or 29.9.
 *
 * The generator is pure math: it produces the command list (leg identity,
 * phase kind, target angle, offset ms) that the runner replays on device.
 * CSV serialization follows protocol §9: UTF-8 no BOM, LF endings, one LF
 * after the last row, columns commandSequence,offsetMs,segmentId,phaseKind,
 * legId,targetTiltDeg,rateDegPerSec — hold rows have empty legId and rate 0,
 * ramp rows carry their leg and the positive rate magnitude.
 */

import { P011RelaxHysteresisParamError } from './P011RelaxHysteresis';

export const P011_SCHEDULE_NINE_PHASE_30_V3 = 'nine-phase-30-v3';

/** Protocol §3.2 frozen constants. */
export const P011_V3_PLATFORM_HOLD_MS = 5100;
export const P011_V3_COMMAND_STEP_MS = 100;
export const P011_V3_RATE_PRIMARY_DEG_PER_S = 0.5;
export const P011_V3_RATE_SENSITIVITY_DEG_PER_S = 1.0;

/** One generated command (ramp or hold row). */
export interface P011ScheduleV3Command {
  commandSequence: number;
  offsetMs: number;
  /** Segment identity: H0..H8 platform or L1..L8 leg. */
  segmentId: string;
  phaseKind: 'platform' | 'ramp';
  /** Empty for platform rows, L1..L8 for ramp rows. */
  legId: string;
  targetTiltDeg: number;
  /** 0 for platform rows; positive rate magnitude for ramp rows. */
  rateDegPerSec: number;
}

/** Generated schedule plus derived identity fields. */
export interface P011ScheduleV3 {
  scheduleId: string;
  /** Bound A value (15.1..29.9 exclusive of 30). */
  peakADeg: number;
  rateDegPerSec: number;
  /** Angular step per command (0.05 primary / 0.10 sensitivity). */
  angularStepDeg: number;
  /** Ideal total duration: 9*5100 + 4*A/v ms. */
  totalDurationMs: number;
  commands: P011ScheduleV3Command[];
}

/**
 * Build the v3 schedule. Validations (protocol §3.2, all constructor-time):
 *   - rate must be exactly 0.5 or 1.0 (frozen rates; no other scan speed)
 *   - A must be finite in (0, 30): the §4 grid is 15.1..29.9 but the
 *     generator only needs A < 30; the grid membership check belongs to the
 *     caller (manifest binding) so audit-time grids cannot drift in here
 *   - every leg length n = |T1-T0| / (stepDeg * stepRate) must be an exact
 *     integer at the given rate: with stepMs=100, v=0.5 -> 0.05 deg/command,
 *     v=1.0 -> 0.10 deg/command; A on the 0.1 grid guarantees divisibility
 *     for both 15-degree legs (300/50 steps at 0.5 deg/s) and A legs.
 */
export function p011BuildScheduleV3(peakADeg: number,
  rateDegPerSec: number): P011ScheduleV3 {
  if (rateDegPerSec !== P011_V3_RATE_PRIMARY_DEG_PER_S &&
    rateDegPerSec !== P011_V3_RATE_SENSITIVITY_DEG_PER_S) {
    throw new P011RelaxHysteresisParamError(
      'v3 schedule rate must be 0.5 or 1.0 deg/s (frozen protocol)');
  }
  if (!Number.isFinite(peakADeg) || peakADeg <= 0 || peakADeg >= 30) {
    throw new P011RelaxHysteresisParamError(
      'v3 schedule A must satisfy 0 < A < 30 (no 30/29.9 default)');
  }
  const angularStepDeg = rateDegPerSec * P011_V3_COMMAND_STEP_MS / 1000;
  const platformTilts = [0, 15, peakADeg, 15, 0, -15, -peakADeg, -15, 0];
  // Legs L1..L8: (fromPlatformIndex, toPlatformIndex) — §3.1 table.
  const legPairs: number[][] = [
    [0, 1], [1, 2], [2, 3], [3, 4], [4, 5], [5, 6], [6, 7], [7, 8]
  ];
  const commands: P011ScheduleV3Command[] = [];
  let sequence = 0;
  let offsetMs = 0;
  // H0 platform at t=0: the initial 0-degree command.
  commands.push({
    commandSequence: ++sequence,
    offsetMs: offsetMs,
    segmentId: 'H0',
    phaseKind: 'platform',
    legId: '',
    targetTiltDeg: platformTilts[0],
    rateDegPerSec: 0
  });
  offsetMs += P011_V3_PLATFORM_HOLD_MS;
  for (let legIndex = 0; legIndex < legPairs.length; legIndex++) {
    const fromIndex = legPairs[legIndex][0];
    const toIndex = legPairs[legIndex][1];
    const t0 = platformTilts[fromIndex];
    const t1 = platformTilts[toIndex];
    const legId = `L${legIndex + 1}`;
    const spanDeg = Math.abs(t1 - t0);
    const exactN = spanDeg / angularStepDeg;
    const n = Math.round(exactN);
    if (Math.abs(exactN - n) > 1e-9) {
      throw new P011RelaxHysteresisParamError(
        `v3 schedule leg ${legId} length not integral: n=${exactN}`);
    }
    // Leg-start marker command still at T0 (protocol §3.2), then one
    // incremental command every 100 ms; the n-th endpoint command T1 closes
    // the leg and opens the next platform hold (half-open [legStart,
    // endpointCommandTime)).
    commands.push({
      commandSequence: ++sequence,
      offsetMs: offsetMs,
      segmentId: legId,
      phaseKind: 'ramp',
      legId: legId,
      targetTiltDeg: t0,
      rateDegPerSec: rateDegPerSec
    });
    offsetMs += P011_V3_COMMAND_STEP_MS;
    for (let step = 1; step <= n; step++) {
      const progress = step / n;
      commands.push({
        commandSequence: ++sequence,
        offsetMs: offsetMs,
        segmentId: legId,
        phaseKind: 'ramp',
        legId: legId,
        targetTiltDeg: t0 + (t1 - t0) * progress,
        rateDegPerSec: rateDegPerSec
      });
      offsetMs += P011_V3_COMMAND_STEP_MS;
    }
    // Endpoint command time opens the target platform hold. The platform's
    // first command IS the leg's endpoint command (same commandSequence);
    // platform rows are the hold evidence rows emitted after each leg. Here
    // the platform segment rows for H1..H8 are represented implicitly by the
    // hold duration — the schedule's command list carries one explicit
    // platform marker per platform for the runner (target tilt unchanged).
    const platformId = `H${toIndex}`;
    commands.push({
      commandSequence: ++sequence,
      offsetMs: offsetMs,
      segmentId: platformId,
      phaseKind: 'platform',
      legId: '',
      targetTiltDeg: t1,
      rateDegPerSec: 0
    });
    offsetMs += P011_V3_PLATFORM_HOLD_MS;
  }
  // Ideal total duration: 9 platform holds + ramp time. The ramp algebra is
  // 4*15deg legs + 4*(A-15)deg legs = 4*A, which assumes A >= 15. A < 15 is
  // outside the §4 audit grid (15.1..29.9); grid membership is the caller's
  // (manifest binding) responsibility, so an off-grid A would over-report
  // the total here. Leg-length integrality above is enforced per leg, so
  // on-grid inputs are exact.
  const totalDurationMs = 9 * P011_V3_PLATFORM_HOLD_MS +
    4 * peakADeg / rateDegPerSec * 1000;
  return {
    scheduleId: P011_SCHEDULE_NINE_PHASE_30_V3,
    peakADeg: peakADeg,
    rateDegPerSec: rateDegPerSec,
    angularStepDeg: angularStepDeg,
    totalDurationMs: totalDurationMs,
    commands: commands
  };
}

/**
 * Protocol §9 CSV: UTF-8 no BOM, LF, trailing LF, fixed column order, no
 * scientific notation (integers and finite decimals via toFixed where
 * needed). Hold rows: legId empty, rate 0.
 */
export function p011ScheduleV3ToCsv(schedule: P011ScheduleV3): string {
  const lines: string[] = [
    'commandSequence,offsetMs,segmentId,phaseKind,legId,targetTiltDeg,rateDegPerSec'
  ];
  for (const command of schedule.commands) {
    lines.push(`${command.commandSequence},${command.offsetMs},${command.segmentId},` +
      `${command.phaseKind},${command.legId},${formatAngle(command.targetTiltDeg)},` +
      `${formatRate(command.rateDegPerSec)}`);
  }
  return lines.join('\n') + '\n';
}

function formatAngle(value: number): string {
  // 0.05/0.10 grid values render exactly at 2 decimals; A on the 0.1 grid
  // too. More precision would still be a finite decimal but the protocol
  // examples use plain decimals — 2 places covers the frozen steps.
  return (Math.round(value * 100) / 100).toFixed(2);
}

function formatRate(value: number): string {
  return value === 0 ? '0' : (Math.round(value * 100) / 100).toFixed(2);
}

/**
 * Bind a command to its (legId, phaseKind) for ledger context: ramp commands
 * carry their leg; platform commands carry the platform id. Used by
 * WaterSandField's observation context so the attempt table's segment
 * identity comes from the schedule, never re-segmented from transitions.
 */
export interface P011ScheduleV3Binding {
  legId: string | null;
  segmentId: string | null;
  phaseKind: 'platform' | 'ramp' | null;
}

export function p011ScheduleV3BindCommand(schedule: P011ScheduleV3,
  commandSequence: number): P011ScheduleV3Binding {
  for (const command of schedule.commands) {
    if (command.commandSequence === commandSequence) {
      return {
        legId: command.phaseKind === 'ramp' ? command.legId : null,
        segmentId: command.segmentId,
        phaseKind: command.phaseKind
      };
    }
  }
  return { legId: null, segmentId: null, phaseKind: null };
}
