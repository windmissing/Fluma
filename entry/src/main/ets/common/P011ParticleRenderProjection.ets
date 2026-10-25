// Structural, read-only consumer projection. No renderer dependency.
export interface P011ParticleRenderProjection {
  physicsCommitId: number; particleRevision: number; routeId: string | null;
  active: boolean; count: number; capacity: number; cols: number;
  x: Float64Array; y: Float64Array; vx: Float32Array; vy: Float32Array;
  mass: Float64Array; phase: Uint8Array; state: Uint8Array; particleId: Uint32Array;
}
