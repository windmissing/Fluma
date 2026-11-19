import { Particle } from './Particle';
import { SpatialHash } from './SpatialHash';

export class CollisionSolver {
  static solve(particles: Particle[], hash: SpatialHash, r: number, eSame: number, eCross: number): void {
    const minDist = 2 * r;
    const minDist2 = minDist * minDist;

    for (let i = 0; i < particles.length; i++) {
      const pi = particles[i];
      const neighbors = hash.queryNearby(pi);
      for (let k = 0; k < neighbors.length; k++) {
        const j = neighbors[k];
        if (j <= i) continue;
        const pj = particles[j];
        const dx = pj.x - pi.x;
        const dy = pj.y - pi.y;
        const dist2 = dx * dx + dy * dy;
        if (dist2 >= minDist2 || dist2 === 0) continue;
        const e = pi.phase === pj.phase ? eSame : eCross;
        const dist = Math.sqrt(dist2);
        const nx = dx / dist;
        const ny = dy / dist;
        const overlap = minDist - dist;
        const mi = pi.mass;
        const mj = pj.mass;
        const invSum = 1 / (mi + mj);
        pi.x -= mj * invSum * overlap * nx;
        pi.y -= mj * invSum * overlap * ny;
        pj.x += mi * invSum * overlap * nx;
        pj.y += mi * invSum * overlap * ny;
        const vrel = (pi.vx - pj.vx) * nx + (pi.vy - pj.vy) * ny;
        if (vrel <= 0) continue;
        const J = -(1 + e) * vrel / (1 / mi + 1 / mj);
        pi.vx += J / mi * nx;
        pi.vy += J / mi * ny;
        pj.vx -= J / mj * nx;
        pj.vy -= J / mj * ny;
      }
    }
  }
}