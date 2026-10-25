// T43 assembly only: existing route/clock/sampler/input APIs own all physics.
import { WaterSandField } from './WaterSandField';
import { P011EndpointClock } from './P011EndpointMetrics';
import { P011EndpointCaptureSession, P011EndpointBoundary } from './P011EndpointCaptureSession';
import { defaultP011ParticleParams, P011ParticleRouteResult } from './P011ParticleM2Field';
import { P011_PARTICLE_ROUTE_ID } from './P011ParticleField';
export function p011DeviceOccurrence(field: WaterSandField): string {
  const phase = field.endpointCurrentPhaseIdentity('');
  return `${field.getP011RunId()}:input:${phase.commandSequence}:phase:${phase.commandPhaseId}:segment:${phase.segmentId ?? 'none'}`;
}
export function p011ArmDeviceParticleRoute(field: WaterSandField): P011ParticleRouteResult {
  if (field.particleRouteActive()) return {ok: true, code: 'OK', active: true};
  const result = field.setParticleRoute(defaultP011ParticleParams());
  if (result.ok) field.setNaturalObservationRouteId(P011_PARTICLE_ROUTE_ID);
  return {ok: result.ok && field.particleRouteActive(), code: result.code, active: field.particleRouteActive()};
}
export function p011AssembleDeviceCapture(field: WaterSandField, clock: P011EndpointClock,
  mode: 'TEST' | 'EXPERIENCE'): P011EndpointCaptureSession {
  return new P011EndpointCaptureSession(field, clock, p011DeviceOccurrence(field), mode);
}
export function p011BindDevicePhase(field: WaterSandField, session: P011EndpointCaptureSession): void {
  session.bindAcceptedOccurrence(p011DeviceOccurrence(field));
}
export function p011ApplyDeviceShake(field: WaterSandField, session: P011EndpointCaptureSession | null,
  value: number): P011EndpointBoundary | null {
  if (session !== null) p011BindDevicePhase(field, session);
  field.setShakeInput(value); // same single physical input as the existing panel/Want
  if (session === null || value !== 0) return null;
  const events = field.getPeakShakeInputEvents();
  const event = events.length > 0 ? events[events.length - 1] : null;
  return event !== null && event.kind === 'SHAKE_OFF'
    ? session.shakeOffBoundary(event.caseOccurrenceId, event.effectiveElapsedMs) : null;
}
export function p011ShakeOnlyWant(parameters: Record<string, Object> | undefined): number | null {
  if (parameters === undefined || parameters.shake === undefined || Object.keys(parameters).some((key: string): boolean => key !== 'shake' && !key.startsWith('ohos.'))) return null;
  const text = `${parameters.shake}`.trim();
  const value = text === '' ? Number.NaN : Number(text);
  return Number.isFinite(value) && value >= 0 && value <= 1 ? value : Number.NaN;
}
