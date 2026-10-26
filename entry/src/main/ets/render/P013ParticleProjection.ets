// T62: render staging only; authoritative Float64 source must be validated first.
export const P013_COLUMN_PROJECTION: string = 'COLUMN_PRESERVING_F32_RENDER_V1';
export function p013PreviousFloat32(value: number): number {
  const f = new Float32Array(1); const bits = new Uint32Array(f.buffer);
  f[0] = value; bits[0]--; return f[0];
}
export function p013ProjectX(value: number): number {
  const q = Math.fround(value); const upper = Math.floor(value) + 1;
  return q >= upper ? p013PreviousFloat32(upper) : q;
}
export function p013ProjectY(value: number, capacity: number): number {
  const q = Math.fround(value);
  return q > capacity ? p013PreviousFloat32(q) : q;
}
export class P013ProjectionDiagnostic {
  revision: string = 'RAW_F32_V1'; correctedCount: number = 0;
  maxXError: number = 0; maxYError: number = 0;
  firstSlot: number = -1; sourceX: number = 0; roundedX: number = 0; projectedX: number = 0;
}
