// Endpoint-only clock adapter. Never falls back to Date.now or fabricates ticks.
import systemDateTime from '@ohos.systemDateTime';
import { P011EndpointClock } from './P011EndpointMetrics';
export class P011EndpointStartupClock implements P011EndpointClock {
  readonly clockId: string;
  readonly runStartClockMs: number;
  readonly endpointClockApi = 'systemDateTime.getUptime(STARTUP,false)';
  readonly endpointClockUnit = 'ms';
  readonly endpointClockResolutionMs: number | null;
  readonly endpointClockOrigin = 'DEVICE_STARTUP_MINUS_RUN_START_V1';
  constructor(runId: string, measuredResolutionMs: number | null = null) {
    if (measuredResolutionMs !== null && (!Number.isFinite(measuredResolutionMs) || measuredResolutionMs <= 0)) {
      throw new Error('INVALID_ENDPOINT_CLOCK_RESOLUTION');
    }
    this.endpointClockResolutionMs = measuredResolutionMs;
    this.clockId = runId + ':startup-elapsed-v1';
    this.runStartClockMs = systemDateTime.getUptime(systemDateTime.TimeType.STARTUP, false);
  }
  nowElapsedMs(): number {
    return systemDateTime.getUptime(systemDateTime.TimeType.STARTUP, false) - this.runStartClockMs;
  }
}
