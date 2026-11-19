// Endpoint-only acquisition binding. Existing page schedules remain unchanged.
// A capture driver opts in on a fresh field run and supplies manifest phase IDs.
import { WaterSandField } from './WaterSandField';
import { P011EndpointClock } from './P011EndpointMetrics';
import { P011SchedulePlayer, P011ScheduleDispatch, P011ConditioningSpec } from './P011SchedulePlayer';

export interface P011EndpointBoundary {
  caseId: string; endpointPhaseOccurrenceId: string;
  endpointBoundaryElapsedMs: number; windowStartElapsedMs: number; windowEndElapsedMs: number;
  shakeOffEffectiveElapsedMs: number | null;
}
export class P011EndpointCaptureSession {
  readonly clock: P011EndpointClock;
  readonly field: WaterSandField;
  constructor(field: WaterSandField, clock: P011EndpointClock, initialOccurrenceId: string,
    mode: 'TEST' | 'EXPERIENCE') {
    this.field = field; this.clock = clock;
    field.setEndpointSampling(clock, field.endpointCurrentPhaseIdentity(initialOccurrenceId));
    field.setEndpointCaptureMode(mode);
  }
  startSchedule(player: P011SchedulePlayer, peakADeg: number, rateDegPerSec: number,
    conditioning: P011ConditioningSpec | null = null): boolean {
    return player.start(peakADeg, rateDegPerSec, this.clock.nowElapsedMs(), conditioning);
  }
  tickSchedule(player: P011SchedulePlayer): P011ScheduleDispatch[] {
    return player.tick(this.clock.nowElapsedMs());
  }
  // Call immediately after the existing command path accepts a manifest
  // command. It reads actual field identity, never re-segments by phaseD.
  bindAcceptedOccurrence(occurrenceId: string): number {
    const identity = this.field.endpointCurrentPhaseIdentity(occurrenceId);
    this.field.bindEndpointOccurrence(occurrenceId, identity.commandSequence,
      identity.commandPhaseId, identity.segmentId);
    return this.clock.nowElapsedMs();
  }
  shakeOffBoundary(occurrenceId: string, acceptedEffectiveElapsedMs: number): P011EndpointBoundary {
    // The physical input path must already have accepted shakeInput=0. This
    // method only binds its measured effective time; it never changes physics.
    if (this.field.getShakeInput() > 1e-9 || !Number.isFinite(acceptedEffectiveElapsedMs) ||
      acceptedEffectiveElapsedMs < 0) throw new Error('SHAKE_OFF_NOT_ACCEPTED');
    this.bindAcceptedOccurrence(occurrenceId);
    const effective = acceptedEffectiveElapsedMs;
    return { caseId: 'TC-WS-08', endpointPhaseOccurrenceId: occurrenceId,
      shakeOffEffectiveElapsedMs: effective, endpointBoundaryElapsedMs: effective + 8000,
      windowStartElapsedMs: effective + 7000, windowEndElapsedMs: effective + 8000 };
  }
  fixedSlipBoundary(occurrenceId: string, predeclaredEndMs: number): P011EndpointBoundary {
    if (!Number.isFinite(predeclaredEndMs) || predeclaredEndMs < 1000) throw new Error('INVALID_ENDPOINT_BOUNDARY');
    return { caseId: 'TC-P011-SLIP-03', endpointPhaseOccurrenceId: occurrenceId,
      shakeOffEffectiveElapsedMs: null, endpointBoundaryElapsedMs: predeclaredEndMs,
      windowStartElapsedMs: predeclaredEndMs - 1000, windowEndElapsedMs: predeclaredEndMs };
  }
}
