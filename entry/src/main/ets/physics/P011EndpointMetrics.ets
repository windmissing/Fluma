// P-011 endpoint field rev2 / protocol v1, 2026-10-24. Pure telemetry.
export const P011_ENDPOINT_FIELD_REVISION = 'p011-endpoint-fields-v1';
export const P011_ENDPOINT_METRIC_REVISION = 'terminal-nondeposited-v1.1';
export const P011_ENDPOINT_PROTOCOL_ID = 'p011-endpoint-terminal-nondeposited';
export const P011_ENDPOINT_PROTOCOL_REVISION = 'endpoint-terminal-nondeposited-v1';
export interface P011EndpointClock { clockId: string; nowElapsedMs(): number; }
export interface P011EndpointCommitKey {
  runId: string; sequence: number; frameId: number; substepId: number;
  attemptId: string | null; committedOrdinal: number;
}
export interface P011EndpointPhaseIdentity {
  occurrenceId: string; commandSequence: number; commandPhaseId: number; segmentId: string | null;
}
export class P011EndpointParticleStateMass {
  count: number;
  massH: number;
  massL: number;
  constructor(count: number, massH: number, massL: number) {
    this.count = count; this.massH = massH; this.massL = massL;
  }
}
export class P011EndpointByStateValue {
  FLYING: P011EndpointParticleStateMass;
  CONTACT_WAIT: P011EndpointParticleStateMass;
  constructor(flying: P011EndpointParticleStateMass, contactWait: P011EndpointParticleStateMass) {
    this.FLYING = flying; this.CONTACT_WAIT = contactWait;
  }
}
export class P011EndpointParticleSourceMass {
  count: number; massH: number; massL: number;
  constructor(count: number = 0, massH: number = 0, massL: number = 0) {
    this.count = count; this.massH = massH; this.massL = massL;
  }
}
export class P011EndpointParticleSourceKind {
  GESTURE_EXTERNAL: P011EndpointParticleSourceMass; SHAKE_LIFT: P011EndpointParticleSourceMass;
  constructor(gesture: P011EndpointParticleSourceMass = new P011EndpointParticleSourceMass(),
    shake: P011EndpointParticleSourceMass = new P011EndpointParticleSourceMass()) {
    this.GESTURE_EXTERNAL = gesture; this.SHAKE_LIFT = shake;
  }
}
export class P011EndpointParticleSummary {
  count: number;
  massH: number;
  massL: number;
  byState: P011EndpointByStateValue;
  bySourceKind: P011EndpointParticleSourceKind;
  capacityUsed: number; capacityLimit: number; degraded: P011EndpointParticleSourceMass;
  constructor(count: number, massH: number, massL: number, byState: P011EndpointByStateValue,
    bySourceKind: P011EndpointParticleSourceKind = new P011EndpointParticleSourceKind(),
    capacityUsed: number = 0, capacityLimit: number = 0,
    degraded: P011EndpointParticleSourceMass = new P011EndpointParticleSourceMass()) {
    this.count = count; this.massH = massH; this.massL = massL; this.byState = byState;
    this.bySourceKind = bySourceKind; this.capacityUsed = capacityUsed;
    this.capacityLimit = capacityLimit; this.degraded = degraded;
  }
}
export class P011EndpointLedgerCapture {
  routeId: string | null;
  particleRouteActive: boolean;
  particles: P011EndpointParticleSummary | null;
  particleEvents: string[];
  poolSnapshot: string | null;
  committedSubstepId: number;
  constructor(routeId: string | null, particleRouteActive: boolean,
    particles: P011EndpointParticleSummary | null, particleEvents: string[],
    poolSnapshot: string | null, committedSubstepId: number) {
    this.routeId = routeId; this.particleRouteActive = particleRouteActive;
    this.particles = particles; this.particleEvents = particleEvents;
    this.poolSnapshot = poolSnapshot; this.committedSubstepId = committedSubstepId;
  }
}
export interface P011EndpointPostCommitStamp {
  key: P011EndpointCommitKey; endpointSampleElapsedMs: number;
  integratedPhysicsSeconds: number; endpointClockId: string; phase: P011EndpointPhaseIdentity;
}
export interface P011EndpointMassFractions {
  depositedMass: number; suspendedMass: number; particleMass: number | null;
  expectedMass: number; actualMass: number | null;
  depositedFraction: number | null; suspFraction: number | null; airborneFraction: number | null;
  particleEvidenceKind: 'CANONICAL' | 'STRUCTURAL_ZERO_OFF' | 'NOT_IMPLEMENTED';
}
export function p011EndpointMassFractions(deposited: number, suspended: number,
  particle: number | null, expected: number,
  evidence: 'CANONICAL' | 'STRUCTURAL_ZERO_OFF' | 'NOT_IMPLEMENTED'): P011EndpointMassFractions {
  if (!Number.isFinite(deposited) || deposited < 0 || !Number.isFinite(suspended) || suspended < 0 ||
    !Number.isFinite(expected) || (particle !== null && (!Number.isFinite(particle) || particle < 0))) {
    throw new Error('INVALID_LEDGER');
  }
  return { depositedMass: deposited, suspendedMass: suspended, particleMass: particle,
    expectedMass: expected, actualMass: particle === null ? null : deposited + suspended + particle,
    depositedFraction: expected > 0 ? deposited / expected : null,
    suspFraction: expected > 0 ? suspended / expected : null,
    airborneFraction: expected > 0 && particle !== null ? particle / expected : null,
    particleEvidenceKind: evidence };
}
export class P011EndpointSampler {
  readonly runId: string; readonly clock: P011EndpointClock;
  integratedPhysicsSeconds = 0; committedOrdinal = 0;
  private lastElapsed = -1;
  private lastSequence = -1;
  private lastSubstep = -1;
  private lastFrame = -1;
  constructor(runId: string, clock: P011EndpointClock) {
    if (!runId || !clock.clockId) throw new Error('INVALID_ENDPOINT_IDENTITY');
    this.runId = runId; this.clock = clock;
  }
  stampEndpointPostCommit(key: P011EndpointCommitKey, physicsDt: number,
    phase: P011EndpointPhaseIdentity, clock: P011EndpointClock): P011EndpointPostCommitStamp {
    if (clock !== this.clock) throw new Error('INVALID_ENDPOINT_CLOCK');
    if (!phase.occurrenceId || key.runId !== this.runId ||
      key.committedOrdinal !== this.committedOrdinal + 1 || !Number.isSafeInteger(key.sequence) ||
      key.sequence < this.lastSequence || !Number.isSafeInteger(key.frameId) ||
      !Number.isSafeInteger(key.substepId) ||
      (key.frameId === this.lastFrame ? key.substepId <= this.lastSubstep : key.frameId < this.lastFrame) ||
      !Number.isFinite(physicsDt) || physicsDt <= 0) {
      throw new Error('INVALID_ENDPOINT_COMMIT');
    }
    const elapsed = clock.nowElapsedMs(); // exactly one actual-clock read
    if (!Number.isFinite(elapsed) || elapsed < 0 || elapsed < this.lastElapsed) throw new Error('INVALID_ENDPOINT_CLOCK');
    this.lastElapsed = elapsed;
    this.lastFrame = key.frameId;
    this.recordUnstampedCommit(key, physicsDt);
    const frozenKey: P011EndpointCommitKey = { runId: key.runId, sequence: key.sequence,
      frameId: key.frameId, substepId: key.substepId, attemptId: key.attemptId,
      committedOrdinal: key.committedOrdinal };
    const frozenPhase: P011EndpointPhaseIdentity = { occurrenceId: phase.occurrenceId,
      commandSequence: phase.commandSequence, commandPhaseId: phase.commandPhaseId, segmentId: phase.segmentId };
    return { key: frozenKey, endpointSampleElapsedMs: elapsed,
      integratedPhysicsSeconds: this.integratedPhysicsSeconds, endpointClockId: clock.clockId, phase: frozenPhase };
  }
  stampRecordedPostCommit(key: P011EndpointCommitKey, elapsed: number,
    phase: P011EndpointPhaseIdentity): P011EndpointPostCommitStamp {
    if (!Number.isFinite(elapsed) || elapsed < 0 || elapsed < this.lastElapsed ||
      key.runId !== this.runId || key.committedOrdinal !== this.committedOrdinal ||
      !phase.occurrenceId) throw new Error('INVALID_ENDPOINT_STAMP');
    this.lastElapsed = elapsed;
    const frozenKey: P011EndpointCommitKey = { runId: key.runId, sequence: key.sequence,
      frameId: key.frameId, substepId: key.substepId, attemptId: key.attemptId,
      committedOrdinal: key.committedOrdinal };
    const frozenPhase: P011EndpointPhaseIdentity = { occurrenceId: phase.occurrenceId,
      commandSequence: phase.commandSequence, commandPhaseId: phase.commandPhaseId, segmentId: phase.segmentId };
    return { key: frozenKey, endpointSampleElapsedMs: elapsed,
      integratedPhysicsSeconds: this.integratedPhysicsSeconds, endpointClockId: this.clock.clockId, phase: frozenPhase };
  }
  recordSuccessfulCommitWithoutStamp(key: P011EndpointCommitKey, physicsDt: number): void {
    this.recordUnstampedCommit(key, physicsDt);
  }
  // A successful physical commit still advances integrated time when its
  // diagnostic clock/phase is unavailable. It must never gain a false stamp.
  recordUnstampedCommit(key: P011EndpointCommitKey, physicsDt: number): void {
    if (key.runId !== this.runId || key.committedOrdinal !== this.committedOrdinal + 1 ||
      !Number.isSafeInteger(key.sequence) || key.sequence < this.lastSequence ||
      !Number.isSafeInteger(key.frameId) || !Number.isSafeInteger(key.substepId) ||
      (key.frameId === this.lastFrame ? key.substepId <= this.lastSubstep : key.frameId < this.lastFrame) ||
      !Number.isFinite(physicsDt) || physicsDt <= 0) {
      throw new Error('INVALID_ENDPOINT_COMMIT');
    }
    this.lastSequence = key.sequence; this.lastFrame = key.frameId; this.lastSubstep = key.substepId;
    this.integratedPhysicsSeconds += physicsDt; this.committedOrdinal++;
  }
}
