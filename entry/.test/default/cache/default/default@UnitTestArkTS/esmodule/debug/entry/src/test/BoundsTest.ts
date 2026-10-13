import { describe, it, expect } from "@package:pkg_modules/.ohpm/@ohos+hypium@1.0.6/pkg_modules/@ohos/hypium/index";
import { Bounds } from "@bundle:com.example.helloworld/entry/ets/physics/Bounds";
import { Particle } from "@bundle:com.example.helloworld/entry/ets/physics/Particle";
export default function boundsTest() {
    describe('BoundsTest', () => {
        it('reflect_leftBoundaryReversesVelocity', 0, () => {
            // 粒子穿透左边界：x - r < 0
            const p: Particle = new Particle(2, 50, 5, 'red');
            p.vx = -10;
            p.vy = 0;
            Bounds.reflect(p, 100, 100, 0.5);
            // x = r = 5, vx = -(-10) * 0.5 = 5
            expect(p.x).assertEqual(5);
            expect(p.vx).assertEqual(5);
        });
        it('reflect_rightBoundaryReversesVelocity', 0, () => {
            // 粒子穿透右边界：x + r > w
            const p: Particle = new Particle(98, 50, 5, 'red');
            p.vx = 10;
            p.vy = 0;
            Bounds.reflect(p, 100, 100, 0.5);
            // x = w - r = 95, vx = -10 * 0.5 = -5
            expect(p.x).assertEqual(95);
            expect(p.vx).assertEqual(-5);
        });
        it('reflect_topBoundaryReversesVelocity', 0, () => {
            // 粒子穿透上边界：y - r < 0
            const p: Particle = new Particle(50, 2, 5, 'red');
            p.vx = 0;
            p.vy = -10;
            Bounds.reflect(p, 100, 100, 0.5);
            // y = r = 5, vy = -(-10) * 0.5 = 5
            expect(p.y).assertEqual(5);
            expect(p.vy).assertEqual(5);
        });
        it('reflect_bottomBoundaryReversesVelocity', 0, () => {
            // 粒子穿透下边界：y + r > h
            const p: Particle = new Particle(50, 98, 5, 'red');
            p.vx = 0;
            p.vy = 10;
            Bounds.reflect(p, 100, 100, 0.5);
            // y = h - r = 95, vy = -10 * 0.5 = -5
            expect(p.y).assertEqual(95);
            expect(p.vy).assertEqual(-5);
        });
        it('reflect_cornerReflectsBothAxes', 0, () => {
            // 粒子同时在左和上边界外（角落）
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
            // |vx| < 1 或 |vy| < 1 时归零
            const p: Particle = new Particle(50, 50, 5, 'red');
            p.vx = 0.5;
            p.vy = -0.3;
            Bounds.reflect(p, 100, 100, 0.5);
            expect(p.vx).assertEqual(0);
            expect(p.vy).assertEqual(0);
        });
        it('reflect_velocityExactlyOneNotClamped', 0, () => {
            // |vx| == 1 时不归零（条件是 < 1）
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
            // e=0: 完全非弹性，碰撞后速度为0（然后被归零逻辑处理）
            const p: Particle = new Particle(2, 50, 5, 'red');
            p.vx = -10;
            p.vy = 0;
            Bounds.reflect(p, 100, 100, 0);
            // vx = -(-10) * 0 = 0, |0| < 1 → vx = 0
            expect(p.x).assertEqual(5);
            expect(p.vx).assertEqual(0);
        });
        it('reflect_fullRestorationPreservesSpeed', 0, () => {
            // e=1: 完全弹性，速度大小不变
            const p: Particle = new Particle(2, 50, 5, 'red');
            p.vx = -10;
            p.vy = 0;
            Bounds.reflect(p, 100, 100, 1);
            // vx = -(-10) * 1 = 10
            expect(p.x).assertEqual(5);
            expect(p.vx).assertEqual(10);
        });
        it('reflect_boundaryExactlyAtEdgeNoCollision', 0, () => {
            // x - r == 0 时不触发左边界（条件是 < 0）
            const p: Particle = new Particle(5, 50, 5, 'red');
            p.vx = -10;
            p.vy = 0;
            Bounds.reflect(p, 100, 100, 0.5);
            expect(p.x).assertEqual(5);
            expect(p.vx).assertEqual(-10);
        });
        it('reflect_afterBounceSmallVelocityClamped', 0, () => {
            // 碰撞后速度变小，如果 < 1 则归零
            const p: Particle = new Particle(2, 50, 5, 'red');
            p.vx = -3;
            p.vy = 0;
            Bounds.reflect(p, 100, 100, 0.1);
            // vx = -(-3) * 0.1 = 0.3, |0.3| < 1 → vx = 0
            expect(p.x).assertEqual(5);
            expect(p.vx).assertEqual(0);
        });
    });
}
