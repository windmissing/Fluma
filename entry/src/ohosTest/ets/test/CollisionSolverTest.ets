import { describe, it, expect } from '@ohos/hypium';
import { CollisionSolver } from '../../../main/ets/physics/CollisionSolver';
import { SpatialHash } from '../../../main/ets/physics/SpatialHash';
import { Particle } from '../../../main/ets/physics/Particle';
import { PhysicsConfig } from '../../../main/ets/physics/PhysicsConfig';

export default function collisionSolverTest() {
  describe('CollisionSolverTest', () => {
    const approxEqual = (a: number, b: number, eps: number = 1e-9): boolean => {
      return Math.abs(a - b) < eps;
    };

    it('samePhase_usesESame', 0, () => {
      const r: number = 5;
      const particles: Particle[] = [
        new Particle(0, 0, r, 'red', 1, 0),
        new Particle(8, 0, r, 'blue', 1, 0),
      ];
      particles[0].vx = 1;
      particles[1].vx = -1;

      const hash: SpatialHash = new SpatialHash(20);
      hash.rebuild(particles);
      CollisionSolver.solve(particles, hash, r, PhysicsConfig.E_SAME, PhysicsConfig.E_CROSS);

      expect(approxEqual(particles[0].x, -1)).assertTrue();
      expect(approxEqual(particles[1].x, 9)).assertTrue();
      expect(approxEqual(particles[0].vx, -0.25)).assertTrue();
      expect(approxEqual(particles[1].vx, 0.25)).assertTrue();
    });

    it('differentPhase_usesECross', 0, () => {
      const r: number = 5;
      const particles: Particle[] = [
        new Particle(0, 0, r, 'red', 1, 0),
        new Particle(8, 0, r, 'blue', 1, 1),
      ];
      particles[0].vx = 1;
      particles[1].vx = -1;

      const hash: SpatialHash = new SpatialHash(20);
      hash.rebuild(particles);
      CollisionSolver.solve(particles, hash, r, PhysicsConfig.E_SAME, PhysicsConfig.E_CROSS);

      expect(approxEqual(particles[0].x, -1)).assertTrue();
      expect(approxEqual(particles[1].x, 9)).assertTrue();
      expect(approxEqual(particles[0].vx, -0.15)).assertTrue();
      expect(approxEqual(particles[1].vx, 0.15)).assertTrue();
    });

    it('samePhase_differentFromCrossPhase', 0, () => {
      const r: number = 5;
      const eSame: number = PhysicsConfig.E_SAME;
      const eCross: number = PhysicsConfig.E_CROSS;

      const pSame: Particle[] = [
        new Particle(0, 0, r, 'red', 1, 0),
        new Particle(8, 0, r, 'blue', 1, 0),
      ];
      pSame[0].vx = 1;
      pSame[1].vx = -1;
      const hashSame: SpatialHash = new SpatialHash(20);
      hashSame.rebuild(pSame);
      CollisionSolver.solve(pSame, hashSame, r, eSame, eCross);

      const pCross: Particle[] = [
        new Particle(0, 0, r, 'red', 1, 0),
        new Particle(8, 0, r, 'blue', 1, 1),
      ];
      pCross[0].vx = 1;
      pCross[1].vx = -1;
      const hashCross: SpatialHash = new SpatialHash(20);
      hashCross.rebuild(pCross);
      CollisionSolver.solve(pCross, hashCross, r, eSame, eCross);

      expect(Math.abs(pSame[0].vx) > Math.abs(pCross[0].vx)).assertTrue();
      expect(Math.abs(pSame[1].vx) > Math.abs(pCross[1].vx)).assertTrue();
    });

    it('noCollision_preservesState', 0, () => {
      const r: number = 5;
      const particles: Particle[] = [
        new Particle(0, 0, r, 'red', 1, 0),
        new Particle(20, 0, r, 'blue', 1, 0),
      ];
      particles[0].vx = 1;
      particles[1].vx = -1;

      const hash: SpatialHash = new SpatialHash(20);
      hash.rebuild(particles);
      CollisionSolver.solve(particles, hash, r, 0.25, 0.15);

      expect(particles[0].x).assertEqual(0);
      expect(particles[1].x).assertEqual(20);
      expect(particles[0].vx).assertEqual(1);
      expect(particles[1].vx).assertEqual(-1);
    });

    it('separatingVelocity_noImpulseApplied', 0, () => {
      const r: number = 5;
      const particles: Particle[] = [
        new Particle(0, 0, r, 'red', 1, 0),
        new Particle(8, 0, r, 'blue', 1, 0),
      ];
      particles[0].vx = -1;
      particles[1].vx = 1;

      const hash: SpatialHash = new SpatialHash(20);
      hash.rebuild(particles);
      CollisionSolver.solve(particles, hash, r, 0.25, 0.15);

      expect(approxEqual(particles[0].x, -1)).assertTrue();
      expect(approxEqual(particles[1].x, 9)).assertTrue();
      expect(particles[0].vx).assertEqual(-1);
      expect(particles[1].vx).assertEqual(1);
    });

    it('massWeightedPositionCorrection', 0, () => {
      const r: number = 5;
      const particles: Particle[] = [
        new Particle(0, 0, r, 'red', 1, 0),
        new Particle(8, 0, r, 'blue', 3, 0),
      ];
      const hash: SpatialHash = new SpatialHash(20);
      hash.rebuild(particles);
      CollisionSolver.solve(particles, hash, r, 0.25, 0.15);

      expect(approxEqual(particles[0].x, -1.5)).assertTrue();
      expect(approxEqual(particles[1].x, 8.5)).assertTrue();
      expect(Math.abs(particles[0].x - 0) > Math.abs(particles[1].x - 8)).assertTrue();
    });

    it('momentumConserved_equalMass', 0, () => {
      const r: number = 5;
      const particles: Particle[] = [
        new Particle(0, 0, r, 'red', 1, 0),
        new Particle(8, 0, r, 'blue', 1, 0),
      ];
      particles[0].vx = 3;
      particles[1].vx = -2;
      const initialMomentum: number = 1 * 3 + 1 * (-2);

      const hash: SpatialHash = new SpatialHash(20);
      hash.rebuild(particles);
      CollisionSolver.solve(particles, hash, r, 0.25, 0.15);

      const finalMomentum: number = 1 * particles[0].vx + 1 * particles[1].vx;
      expect(approxEqual(finalMomentum, initialMomentum)).assertTrue();
    });

    it('momentumConserved_differentMass', 0, () => {
      const r: number = 5;
      const particles: Particle[] = [
        new Particle(0, 0, r, 'red', 1, 0),
        new Particle(8, 0, r, 'blue', 3, 0),
      ];
      particles[0].vx = 2;
      particles[1].vx = -1;
      const initialMomentum: number = 1 * 2 + 3 * (-1);

      const hash: SpatialHash = new SpatialHash(20);
      hash.rebuild(particles);
      CollisionSolver.solve(particles, hash, r, 0.25, 0.15);

      const finalMomentum: number = 1 * particles[0].vx + 3 * particles[1].vx;
      expect(approxEqual(finalMomentum, initialMomentum)).assertTrue();
    });

    it('collisionWithYAxis', 0, () => {
      const r: number = 5;
      const particles: Particle[] = [
        new Particle(0, 0, r, 'red', 1, 0),
        new Particle(0, 8, r, 'blue', 1, 0),
      ];
      particles[0].vy = 1;
      particles[1].vy = -1;

      const hash: SpatialHash = new SpatialHash(20);
      hash.rebuild(particles);
      CollisionSolver.solve(particles, hash, r, 0.25, 0.15);

      expect(approxEqual(particles[0].y, -1)).assertTrue();
      expect(approxEqual(particles[1].y, 9)).assertTrue();
      expect(approxEqual(particles[0].vy, -0.25)).assertTrue();
      expect(approxEqual(particles[1].vy, 0.25)).assertTrue();
      expect(particles[0].x).assertEqual(0);
      expect(particles[1].x).assertEqual(0);
    });

    it('diagonalCollision', 0, () => {
      const r: number = 5;
      const dist: number = 8;
      const particles: Particle[] = [
        new Particle(0, 0, r, 'red', 1, 0),
        new Particle(dist / Math.sqrt(2), dist / Math.sqrt(2), r, 'blue', 1, 0),
      ];
      const speed: number = 1;
      particles[0].vx = speed / Math.sqrt(2);
      particles[0].vy = speed / Math.sqrt(2);
      particles[1].vx = -speed / Math.sqrt(2);
      particles[1].vy = -speed / Math.sqrt(2);

      const hash: SpatialHash = new SpatialHash(20);
      hash.rebuild(particles);
      CollisionSolver.solve(particles, hash, r, 0.25, 0.15);

      const v0Normal: number = particles[0].vx / Math.sqrt(2) + particles[0].vy / Math.sqrt(2);
      expect(approxEqual(v0Normal, -0.25)).assertTrue();
    });

    it('zeroDistance_noCollision', 0, () => {
      const r: number = 5;
      const particles: Particle[] = [
        new Particle(5, 5, r, 'red', 1, 0),
        new Particle(5, 5, r, 'blue', 1, 0),
      ];
      particles[0].vx = 1;
      particles[1].vx = -1;

      const hash: SpatialHash = new SpatialHash(20);
      hash.rebuild(particles);
      CollisionSolver.solve(particles, hash, r, 0.25, 0.15);

      expect(particles[0].x).assertEqual(5);
      expect(particles[1].x).assertEqual(5);
      expect(particles[0].vx).assertEqual(1);
      expect(particles[1].vx).assertEqual(-1);
    });

    it('multipleParticles_pairwise', 0, () => {
      const r: number = 5;
      const particles: Particle[] = [
        new Particle(0, 0, r, 'red', 1, 0),
        new Particle(8, 0, r, 'blue', 1, 0),
        new Particle(50, 0, r, 'green', 1, 0),
      ];
      particles[0].vx = 1;
      particles[1].vx = -1;
      particles[2].vx = 0;

      const hash: SpatialHash = new SpatialHash(20);
      hash.rebuild(particles);
      CollisionSolver.solve(particles, hash, r, 0.25, 0.15);

      expect(approxEqual(particles[0].vx, -0.25)).assertTrue();
      expect(approxEqual(particles[1].vx, 0.25)).assertTrue();
      expect(particles[2].vx).assertEqual(0);
      expect(particles[2].x).assertEqual(50);
    });

    it('customRestitutionValues', 0, () => {
      const r: number = 5;
      const particles: Particle[] = [
        new Particle(0, 0, r, 'red', 1, 0),
        new Particle(8, 0, r, 'blue', 1, 0),
      ];
      particles[0].vx = 1;
      particles[1].vx = -1;

      const hash: SpatialHash = new SpatialHash(20);
      hash.rebuild(particles);
      CollisionSolver.solve(particles, hash, r, 0.5, 0.1);

      expect(approxEqual(particles[0].vx, -0.5)).assertTrue();
      expect(approxEqual(particles[1].vx, 0.5)).assertTrue();
    });
  });
}
