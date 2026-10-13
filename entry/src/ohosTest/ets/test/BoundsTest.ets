import { describe, it, expect } from '@ohos/hypium';
import { Bounds } from '../../../main/ets/physics/Bounds';
import { Particle } from '../../../main/ets/physics/Particle';

export default function boundsTest() {
  describe('BoundsTest', () => {
    it('reflect_leftBoundaryReversesVelocity', 0, () => {
      const p: Particle = new Particle(2, 50, 5, 'red');
      p.vx = -10;
      p.vy = 0;
      Bounds.reflect(p, 100, 100, 0.5);
      expect(p.x).assertEqual(5);
      expect(p.vx).assertEqual(5);
    });

    it('reflect_rightBoundaryReversesVelocity', 0, () => {
      const p: Particle = new Particle(98, 50, 5, 'red');
      p.vx = 10;
      p.vy = 0;
      Bounds.reflect(p, 100, 100, 0.5);
      expect(p.x).assertEqual(95);
      expect(p.vx).assertEqual(-5);
    });

    it('reflect_topBoundaryReversesVelocity', 0, () => {
      const p: Particle = new Particle(50, 2, 5, 'red');
      p.vx = 0;
      p.vy = -10;
      Bounds.reflect(p, 100, 100, 0.5);
      expect(p.y).assertEqual(5);
      expect(p.vy).assertEqual(5);
    });

    it('reflect_bottomBoundaryReversesVelocity', 0, () => {
      const p: Particle = new Particle(50, 98, 5, 'red');
      p.vx = 0;
      p.vy = 10;
      Bounds.reflect(p, 100, 100, 0.5);
      expect(p.y).assertEqual(95);
      expect(p.vy).assertEqual(-5);
    });

    it('reflect_cornerReflectsBothAxes', 0, () => {
      const p: Particle = new Particle(2, 2, 5, 'red');
      p.vx = -10;
      p.vy = -10;
      Bounds.reflect(p, 100, 100, 0.5);
      expect(p.x).assertEqual(5);
      expect(p.y).assertEqual(5);
      expect(p.vx).assertEqual(5);
      expect(p.vy).assertEqual(5);
    });

    it('reflect_smallVelocityClampedToZero', 0, () => {
      const p: Particle = new Particle(50, 50, 5, 'red');
      p.vx = 0.5;
      p.vy = -0.3;
      Bounds.reflect(p, 100, 100, 0.5);
      expect(p.vx).assertEqual(0);
      expect(p.vy).assertEqual(0);
    });

    it('reflect_velocityExactlyOneNotClamped', 0, () => {
      const p: Particle = new Particle(50, 50, 5, 'red');
      p.vx = 1;
      p.vy = -1;
      Bounds.reflect(p, 100, 100, 0.5);
      expect(p.vx).assertEqual(1);
      expect(p.vy).assertEqual(-1);
    });

    it('reflect_noCollisionPreservesState', 0, () => {
      const p: Particle = new Particle(50, 50, 5, 'red');
      p.vx = 10;
      p.vy = -10;
      Bounds.reflect(p, 100, 100, 0.5);
      expect(p.x).assertEqual(50);
      expect(p.y).assertEqual(50);
      expect(p.vx).assertEqual(10);
      expect(p.vy).assertEqual(-10);
    });

    it('reflect_zeroRestitutionStopsVelocity', 0, () => {
      const p: Particle = new Particle(2, 50, 5, 'red');
      p.vx = -10;
      p.vy = 0;
      Bounds.reflect(p, 100, 100, 0);
      expect(p.x).assertEqual(5);
      expect(p.vx).assertEqual(0);
    });

    it('reflect_fullRestorationPreservesSpeed', 0, () => {
      const p: Particle = new Particle(2, 50, 5, 'red');
      p.vx = -10;
      p.vy = 0;
      Bounds.reflect(p, 100, 100, 1);
      expect(p.x).assertEqual(5);
      expect(p.vx).assertEqual(10);
    });

    it('reflect_boundaryExactlyAtEdgeNoCollision', 0, () => {
      const p: Particle = new Particle(5, 50, 5, 'red');
      p.vx = -10;
      p.vy = 0;
      Bounds.reflect(p, 100, 100, 0.5);
      expect(p.x).assertEqual(5);
      expect(p.vx).assertEqual(-10);
    });

    it('reflect_afterBounceSmallVelocityClamped', 0, () => {
      const p: Particle = new Particle(2, 50, 5, 'red');
      p.vx = -3;
      p.vy = 0;
      Bounds.reflect(p, 100, 100, 0.1);
      expect(p.x).assertEqual(5);
      expect(p.vx).assertEqual(0);
    });
  });
}
