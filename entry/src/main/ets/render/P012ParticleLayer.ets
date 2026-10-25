import { P012ParticleRenderSnapshot, P012FrameContext } from './P012RenderTypes';
export interface P012ParticleSampler { sandTopAtPx(x: number): number; waterTopAtPx(x: number): number; fogBandHalfPx: number; }
export interface P012ParticleCanvas {
  fillStyle: string; globalAlpha: number;
  fillRect(x: number, y: number, width: number, height: number): void;
}
/** Read-only physical positions. Uses existing tracer's candidate colors/2px size, no age or integration. */
export class P012ParticleLayer {
  drawnCount: number = 0; skippedCount: number = 0; skipReason: string = '';
  draw(s: P012ParticleRenderSnapshot, f: P012FrameContext, sampler: P012ParticleSampler,
    canvas: P012ParticleCanvas, materialCapacity: number): number {
    this.drawnCount = 0; this.skippedCount = s.count; this.skipReason = '';
    if (!s.active || !f.particleLayerEnabled) { this.skipReason = 'DISABLED'; return 0; }
    if (s.physicsCommitId !== f.physicsCommitId) { this.skipReason = 'PHYSICS_COMMIT_MISMATCH'; return 0; }
    for (let i = 0; i < s.count; i++) {
      if (!Number.isFinite(s.x[i]) || !Number.isFinite(s.y[i]) || s.mass[i] <= 0 ||
        (s.state[i] !== 1 && s.state[i] !== 2)) { this.skipReason = 'INVALID_PARTICLE'; continue; }
      const x = s.x[i] * f.boxW / s.cols;
      // CONTACT_WAIT remains visible at the current surface, including exact contact.
      if (s.state[i] === 1 && s.y[i] < sampler.sandTopAtPx(x)) { this.skipReason = 'SAND_MASK'; continue; }
      if (s.y[i] > sampler.waterTopAtPx(x)) { this.skipReason = 'WATER_MASK'; continue; }
      const px = f.boxX + x;
      const py = f.boxY + (1 - s.y[i] / materialCapacity) * f.boxH;
      if (px < f.boxX || px > f.boxX + f.boxW || py < f.boxY || py > f.boxY + f.boxH) {
        this.skipReason = 'BOX_MASK'; continue;
      }
      canvas.fillStyle = s.phase[i] === 0 ? 'rgba(196,168,120,1.0)' : 'rgba(226,208,168,1.0)';
      const distancePx = (s.y[i] - sampler.sandTopAtPx(x)) * f.boxH / materialCapacity;
      const fog = s.state[i] === 1 && sampler.fogBandHalfPx > 0
        ? Math.max(0, Math.min(1, distancePx / sampler.fogBandHalfPx)) : 1;
      canvas.globalAlpha = fog * fog * (3 - 2 * fog);
      canvas.fillRect(px - 1, py - 1, 2, 2); this.drawnCount++;
    }
    canvas.globalAlpha = 1; this.skippedCount = s.count - this.drawnCount;
    return this.drawnCount;
  }
}
