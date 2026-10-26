import { P011ParticleParams } from './P011ParticleField';
export const P051_DRY_REVISION: string = 'P051_DRY_AIR_SOURCE_V1';
export const P051_DRY_MEDIUM: string = 'DRY_AIR_V1';
export const P051_DRY_GRAVITY: string = 'DRY_AIR_GRAVITY_V1';
export interface P051DryRunConfig { revision: string; sourceId: number; particleParams: P011ParticleParams; }
export interface P051DryRunResult { ok: boolean; code: string; }
export interface P051DryAdmissionRequest {
 revision: string; sourceId: number; sequence: number; inputTimeMs: number; seed: number;
 x: number; y: number; requestedH: number; requestedL: number;
}
export interface P051DryAdmissionReceipt {
 revision: string; runId: string; sourceId: number; sequence: number;
 requestedH: number; requestedL: number; admittedH: number; admittedL: number; lambda: number;
 x: number; y: number; rejectedReason: string | null; admissionTransactionId: number | null;
 sourceEpoch: number; physicsCommitId: number;
}
export function p051DryRequestError(r: P051DryAdmissionRequest, source: number, sequence: number, time: number, cols: number, capacity: number): string | null {
 if(r.revision!==P051_DRY_REVISION||r.sourceId!==source)return 'MODE_CONFLICT';
 if(!Number.isSafeInteger(r.sequence)||r.sequence<=sequence||!Number.isFinite(r.inputTimeMs)||r.inputTimeMs<time||r.inputTimeMs<0||!Number.isSafeInteger(r.seed))return 'INVALID_IDENTITY';
 if(!Number.isFinite(r.x)||r.x<0||r.x>=cols||!Number.isFinite(r.y)||r.y<0||r.y>capacity)return 'INVALID_POSITION';
 if(!Number.isFinite(r.requestedH)||!Number.isFinite(r.requestedL)||r.requestedH<0||r.requestedL<0||!Number.isFinite(r.requestedH+r.requestedL))return 'INVALID_MASS';
 return r.requestedH+r.requestedL===0?'NO_OP':null;
}
