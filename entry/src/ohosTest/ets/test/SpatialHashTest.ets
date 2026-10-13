import { describe, it, expect } from '@ohos/hypium';
import { SpatialHash } from '../../../main/ets/physics/SpatialHash';
import { Particle } from '../../../main/ets/physics/Particle';

export default function spatialHashTest() {
  describe('SpatialHashTest', () => {
    const contains = (arr: number[], val: number): boolean => {
      for (let i = 0; i < arr.length; i++) {
        if (arr[i] === val) return true;
      }
      return false;
    };

    const notContains = (arr: number[], val: number): boolean => {
      return !contains(arr, val);
    };

    it('rebuild_andQueryNearby_findsSameCellParticle', 0, () => {
      const hash: SpatialHash = new SpatialHash(10);
      const particles: Particle[] = [
        new Particle(5, 5, 1, 'red'),
        new Particle(7, 8, 1, 'blue'),
      ];
      hash.rebuild(particles);
      const nearby: number[] = hash.queryNearby(particles[0]);
      expect(contains(nearby, 0)).assertTrue();
      expect(contains(nearby, 1)).assertTrue();
    });

    it('queryNearby_findsAdjacentCellParticles', 0, () => {
      const hash: SpatialHash = new SpatialHash(10);
      const particles: Particle[] = [
        new Particle(5, 5, 1, 'red'),
        new Particle(15, 5, 1, 'blue'),
        new Particle(5, 15, 1, 'green'),
        new Particle(15, 15, 1, 'yellow')
      ];
      hash.rebuild(particles);
      const nearby: number[] = hash.queryNearby(particles[0]);
      expect(contains(nearby, 0)).assertTrue();
      expect(contains(nearby, 1)).assertTrue();
      expect(contains(nearby, 2)).assertTrue();
      expect(contains(nearby, 3)).assertTrue();
    });

    it('queryNearby_excludesDistantParticles', 0, () => {
      const hash: SpatialHash = new SpatialHash(10);
      const particles: Particle[] = [
        new Particle(5, 5, 1, 'red'),
        new Particle(25, 25, 1, 'blue'),
      ];
      hash.rebuild(particles);
      const nearby: number[] = hash.queryNearby(particles[0]);
      expect(contains(nearby, 0)).assertTrue();
      expect(notContains(nearby, 1)).assertTrue();
    });

    it('queryNearby_includesAll3x3Neighbors', 0, () => {
      const hash: SpatialHash = new SpatialHash(10);
      const particles: Particle[] = [
        new Particle(5, 5, 1, 'red'),
        new Particle(-5, -5, 1, 'p1'),
        new Particle(15, -5, 1, 'p2'),
        new Particle(-5, 15, 1, 'p3'),
        new Particle(15, 15, 1, 'p4'),
      ];
      hash.rebuild(particles);
      const nearby: number[] = hash.queryNearby(particles[0]);
      expect(contains(nearby, 0)).assertTrue();
      expect(contains(nearby, 1)).assertTrue();
      expect(contains(nearby, 2)).assertTrue();
      expect(contains(nearby, 3)).assertTrue();
      expect(contains(nearby, 4)).assertTrue();
    });

    it('setCellSize_changesBucketing', 0, () => {
      const hash: SpatialHash = new SpatialHash(10);
      const particles: Particle[] = [
        new Particle(5, 5, 1, 'red'),
        new Particle(15, 5, 1, 'blue'),
      ];
      hash.rebuild(particles);
      let nearby: number[] = hash.queryNearby(particles[0]);
      expect(contains(nearby, 1)).assertTrue();

      hash.setCellSize(5);
      hash.rebuild(particles);
      nearby = hash.queryNearby(particles[0]);
      expect(notContains(nearby, 1)).assertTrue();
    });

    it('rebuild_emptyParticleList', 0, () => {
      const hash: SpatialHash = new SpatialHash(10);
      const particles: Particle[] = [];
      hash.rebuild(particles);
      const p: Particle = new Particle(5, 5, 1, 'red');
      const nearby: number[] = hash.queryNearby(p);
      expect(nearby.length).assertEqual(0);
    });

    it('rebuild_clearsPreviousBuckets', 0, () => {
      const hash: SpatialHash = new SpatialHash(10);
      const particles1: Particle[] = [
        new Particle(5, 5, 1, 'red'),
        new Particle(105, 105, 1, 'blue'),
      ];
      hash.rebuild(particles1);

      const particles2: Particle[] = [
        new Particle(5, 5, 1, 'red'),
      ];
      hash.rebuild(particles2);
      const nearby: number[] = hash.queryNearby(particles2[0]);
      expect(nearby.length).assertEqual(1);
      expect(nearby[0]).assertEqual(0);
    });

    it('multipleParticlesInSameBucket', 0, () => {
      const hash: SpatialHash = new SpatialHash(100);
      const particles: Particle[] = [
        new Particle(10, 10, 1, 'red'),
        new Particle(20, 20, 1, 'blue'),
        new Particle(30, 30, 1, 'green'),
      ];
      hash.rebuild(particles);
      const nearby: number[] = hash.queryNearby(particles[0]);
      expect(nearby.length).assertEqual(3);
      expect(contains(nearby, 0)).assertTrue();
      expect(contains(nearby, 1)).assertTrue();
      expect(contains(nearby, 2)).assertTrue();
    });

    it('particleOnCellBoundary', 0, () => {
      const hash: SpatialHash = new SpatialHash(10);
      const particles: Particle[] = [
        new Particle(5, 5, 1, 'red'),
        new Particle(10, 5, 1, 'blue'),
      ];
      hash.rebuild(particles);
      const nearby: number[] = hash.queryNearby(particles[0]);
      expect(contains(nearby, 1)).assertTrue();
    });

    it('negativeCoordinates', 0, () => {
      const hash: SpatialHash = new SpatialHash(10);
      const particles: Particle[] = [
        new Particle(-5, -5, 1, 'red'),
        new Particle(-15, -5, 1, 'blue'),
        new Particle(-35, -5, 1, 'green'),
      ];
      hash.rebuild(particles);
      const nearby: number[] = hash.queryNearby(particles[0]);
      expect(contains(nearby, 0)).assertTrue();
      expect(contains(nearby, 1)).assertTrue();
      expect(notContains(nearby, 2)).assertTrue();
    });
  });
}
