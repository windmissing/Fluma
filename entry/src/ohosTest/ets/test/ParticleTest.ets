import { describe, it, expect } from '@ohos/hypium';
import { Particle } from '../../../main/ets/physics/Particle';

export default function particleTest() {
  describe('ParticleTest', () => {
    // 辅助函数：浮点数近似比较
    const approxEqual = (a: number, b: number, eps: number = 1e-9): boolean => {
      return Math.abs(a - b) < eps;
    };

    it('constructor_defaultMassAndPhase', 0, () => {
      const p: Particle = new Particle(10, 20, 5, 'red');
      expect(p.x).assertEqual(10);
      expect(p.y).assertEqual(20);
      expect(p.r).assertEqual(5);
      expect(p.color).assertEqual('red');
      expect(p.mass).assertEqual(1);
      expect(p.phase).assertEqual(0);
      expect(p.vx).assertEqual(0);
      expect(p.vy).assertEqual(0);
    });

    it('constructor_customMassAndPhase', 0, () => {
      const p: Particle = new Particle(30, 40, 8, 'blue', 2.5, 1);
      expect(p.x).assertEqual(30);
      expect(p.y).assertEqual(40);
      expect(p.r).assertEqual(8);
      expect(p.color).assertEqual('blue');
      expect(p.mass).assertEqual(2.5);
      expect(p.phase).assertEqual(1);
    });

    it('integrate_gravityUpdatesVelocity', 0, () => {
      const p: Particle = new Particle(0, 0, 5, 'red');
      p.integrate(1, 10, 20);
      expect(p.vx).assertEqual(10);
      expect(p.vy).assertEqual(20);
    });

    it('integrate_gravityUpdatesPositionWithNewVelocity', 0, () => {
      const p: Particle = new Particle(100, 200, 5, 'red');
      p.integrate(1, 10, 0);
      expect(p.x).assertEqual(110);
      expect(p.y).assertEqual(200);
    });

    it('integrate_withInitialVelocity', 0, () => {
      const p: Particle = new Particle(100, 200, 5, 'red');
      p.vx = 5;
      p.vy = 3;
      p.integrate(0.5, 20, 40);
      expect(approxEqual(p.vx, 15)).assertTrue();
      expect(approxEqual(p.vy, 23)).assertTrue();
      expect(approxEqual(p.x, 107.5)).assertTrue();
      expect(approxEqual(p.y, 211.5)).assertTrue();
    });

    it('integrate_multipleStepsAccumulate', 0, () => {
      const p: Particle = new Particle(0, 0, 5, 'red');
      const dt: number = 1 / 60;
      const g: number = 2500;
      p.integrate(dt, 0, g);
      const vx1: number = p.vx;
      const x1: number = p.x;
      p.integrate(dt, 0, g);
      expect(approxEqual(p.vy, 2 * g * dt)).assertTrue();
      expect(approxEqual(p.y, 3 * g * dt * dt)).assertTrue();
    });

    it('integrate_zeroGravityPreservesVelocity', 0, () => {
      const p: Particle = new Particle(50, 50, 5, 'red');
      p.vx = 10;
      p.vy = -5;
      p.integrate(2, 0, 0);
      expect(p.vx).assertEqual(10);
      expect(p.vy).assertEqual(-5);
      expect(p.x).assertEqual(70);
      expect(p.y).assertEqual(40);
    });

    it('integrate_zeroTimeStepNoChange', 0, () => {
      const p: Particle = new Particle(10, 20, 5, 'red');
      p.vx = 5;
      p.vy = 7;
      p.integrate(0, 100, 200);
      expect(p.vx).assertEqual(5);
      expect(p.vy).assertEqual(7);
      expect(p.x).assertEqual(10);
      expect(p.y).assertEqual(20);
    });

    it('integrate_negativeGravityDecelerates', 0, () => {
      const p: Particle = new Particle(0, 0, 5, 'red');
      p.vy = 100;
      p.integrate(1, 0, -50);
      expect(p.vy).assertEqual(50);
      expect(p.y).assertEqual(50);
    });

    it('integrate_semiImplicitEulerDiffersFromExplicit', 0, () => {
      const p: Particle = new Particle(0, 0, 5, 'red');
      p.integrate(1, 10, 0);
      expect(p.x).assertEqual(10);
    });
  });
}
