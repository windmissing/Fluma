import { describe, it, expect } from '@ohos/hypium';
import { SpatialHash } from '../main/ets/physics/SpatialHash';
import { Particle } from '../main/ets/physics/Particle';

export default function spatialHashTest() {
  describe('SpatialHashTest', () => {
    // 辅助函数：数组包含某值
    const contains = (arr: number[], val: number): boolean => {
      for (let i = 0; i < arr.length; i++) {
        if (arr[i] === val) return true;
      }
      return false;
    };

    // 辅助函数：数组不包含某值
    const notContains = (arr: number[], val: number): boolean => {
      return !contains(arr, val);
    };

    it('rebuild_andQueryNearby_findsSameCellParticle', 0, () => {
      const hash: SpatialHash = new SpatialHash(10);
      const particles: Particle[] = [
        new Particle(5, 5, 1, 'red'),   // cell (0,0) index 0
        new Particle(7, 8, 1, 'blue'),  // cell (0,0) index 1
      ];
      hash.rebuild(particles);
      const nearby: number[] = hash.queryNearby(particles[0]);
      // 同桶的两个粒子都应被找到
      expect(contains(nearby, 0)).assertTrue();
      expect(contains(nearby, 1)).assertTrue();
    });

    it('queryNearby_findsAdjacentCellParticles', 0, () => {
      // 3x3 邻域应包含相邻 8 个桶中的粒子
      const hash: SpatialHash = new SpatialHash(10);
      const particles: Particle[] = [
        new Particle(5, 5, 1, 'red'),    // cell (0,0) index 0
        new Particle(15, 5, 1, 'blue'),  // cell (1,0) index 1 - 右邻
        new Particle(5, 15, 1, 'green'), // cell (0,1) index 2 - 下邻
        new Particle(15, 15, 1, 'yellow') // cell (1,1) index 3 - 右下邻
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
        new Particle(5, 5, 1, 'red'),     // cell (0,0) index 0
        new Particle(25, 25, 1, 'blue'),  // cell (2,2) index 1 - 超出3x3邻域
      ];
      hash.rebuild(particles);
      const nearby: number[] = hash.queryNearby(particles[0]);
      expect(contains(nearby, 0)).assertTrue();
      // (2,2) 相对 (0,0) 的 dx=2, dy=2，超出 [-1,1] 范围
      expect(notContains(nearby, 1)).assertTrue();
    });

    it('queryNearby_includesAll3x3Neighbors', 0, () => {
      // 在 3x3 邻域边界上的粒子应被包含
      const hash: SpatialHash = new SpatialHash(10);
      const particles: Particle[] = [
        new Particle(5, 5, 1, 'red'),      // cell (0,0) index 0
        new Particle(-5, -5, 1, 'p1'),     // cell (-1,-1) index 1 - 左上角
        new Particle(15, -5, 1, 'p2'),     // cell (1,-1) index 2 - 右上角
        new Particle(-5, 15, 1, 'p3'),     // cell (-1,1) index 3 - 左下角
        new Particle(15, 15, 1, 'p4'),     // cell (1,1) index 4 - 右下角
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
        new Particle(5, 5, 1, 'red'),    // index 0
        new Particle(15, 5, 1, 'blue'),  // index 1
      ];
      // cellSize=10: p0在(0,0), p1在(1,0) - 相邻
      hash.rebuild(particles);
      let nearby: number[] = hash.queryNearby(particles[0]);
      expect(contains(nearby, 1)).assertTrue();

      // cellSize=5: p0在(1,1), p1在(3,1) - dx=2，不相邻
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

      // 重建为只有1个粒子
      const particles2: Particle[] = [
        new Particle(5, 5, 1, 'red'),
      ];
      hash.rebuild(particles2);
      const nearby: number[] = hash.queryNearby(particles2[0]);
      // 旧桶应被清除，只包含新粒子
      expect(nearby.length).assertEqual(1);
      expect(nearby[0]).assertEqual(0);
    });

    it('multipleParticlesInSameBucket', 0, () => {
      const hash: SpatialHash = new SpatialHash(100);
      const particles: Particle[] = [
        new Particle(10, 10, 1, 'red'),   // index 0, cell (0,0)
        new Particle(20, 20, 1, 'blue'),  // index 1, cell (0,0)
        new Particle(30, 30, 1, 'green'), // index 2, cell (0,0)
      ];
      hash.rebuild(particles);
      const nearby: number[] = hash.queryNearby(particles[0]);
      // 三个粒子都在同一桶
      expect(nearby.length).assertEqual(3);
      expect(contains(nearby, 0)).assertTrue();
      expect(contains(nearby, 1)).assertTrue();
      expect(contains(nearby, 2)).assertTrue();
    });

    it('particleOnCellBoundary', 0, () => {
      // x=10 在 cellSize=10 时属于 cell (1,0)（Math.floor(10/10)=1）
      const hash: SpatialHash = new SpatialHash(10);
      const particles: Particle[] = [
        new Particle(5, 5, 1, 'red'),    // cell (0,0) index 0
        new Particle(10, 5, 1, 'blue'),  // cell (1,0) index 1
      ];
      hash.rebuild(particles);
      const nearby: number[] = hash.queryNearby(particles[0]);
      expect(contains(nearby, 1)).assertTrue();
    });

    it('negativeCoordinates', 0, () => {
      const hash: SpatialHash = new SpatialHash(10);
      const particles: Particle[] = [
        new Particle(-5, -5, 1, 'red'),   // cell (-1,-1) index 0
        new Particle(-15, -5, 1, 'blue'), // cell (-2,-1) index 1 - 左邻
        new Particle(-35, -5, 1, 'green'),// cell (-4,-1) index 2 - 远处
      ];
      hash.rebuild(particles);
      const nearby: number[] = hash.queryNearby(particles[0]);
      expect(contains(nearby, 0)).assertTrue();
      expect(contains(nearby, 1)).assertTrue();
      expect(notContains(nearby, 2)).assertTrue();
    });
  });
}