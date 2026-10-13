import { describe, it, expect } from "@package:pkg_modules/.ohpm/@ohos+hypium@1.0.6/pkg_modules/@ohos/hypium/index";
import { CollisionSolver } from "@bundle:com.example.helloworld/entry/ets/physics/CollisionSolver";
import { SpatialHash } from "@bundle:com.example.helloworld/entry/ets/physics/SpatialHash";
import { Particle } from "@bundle:com.example.helloworld/entry/ets/physics/Particle";
import { PhysicsConfig } from "@bundle:com.example.helloworld/entry/ets/physics/PhysicsConfig";
export default function collisionSolverTest() {
    describe('CollisionSolverTest', () => {
        const approxEqual = (a: number, b: number, eps: number = 1e-9): boolean => {
            return Math.abs(a - b) < eps;
        };
        it('samePhase_usesESame', 0, () => {
            // 两个同相位粒子碰撞，应使用 eSame=0.25
            const r: number = 5;
            const particles: Particle[] = [
                new Particle(0, 0, r, 'red', 1, 0),
                new Particle(8, 0, r, 'blue', 1, 0), // phase 0, dist=8 < 2r=10
            ];
            particles[0].vx = 1;
            particles[1].vx = -1;
            const hash: SpatialHash = new SpatialHash(20);
            hash.rebuild(particles);
            CollisionSolver.solve(particles, hash, r, PhysicsConfig.E_SAME, PhysicsConfig.E_CROSS);
            // 位置修正：overlap=2, invSum=0.5
            // p0.x = 0 - 1*0.5*2*1 = -1
            // p1.x = 8 + 1*0.5*2*1 = 9
            expect(approxEqual(particles[0].x, -1)).assertTrue();
            expect(approxEqual(particles[1].x, 9)).assertTrue();
            // 冲量：e=eSame=0.25, J=-(1.25)*2/2=-1.25
            // p0.vx = 1 + (-1.25) = -0.25
            // p1.vx = -1 - (-1.25) = 0.25
            expect(approxEqual(particles[0].vx, -0.25)).assertTrue();
            expect(approxEqual(particles[1].vx, 0.25)).assertTrue();
        });
        it('differentPhase_usesECross', 0, () => {
            // 两个不同相位粒子碰撞，应使用 eCross=0.15
            const r: number = 5;
            const particles: Particle[] = [
                new Particle(0, 0, r, 'red', 1, 0),
                new Particle(8, 0, r, 'blue', 1, 1), // phase 1, dist=8 < 2r=10
            ];
            particles[0].vx = 1;
            particles[1].vx = -1;
            const hash: SpatialHash = new SpatialHash(20);
            hash.rebuild(particles);
            CollisionSolver.solve(particles, hash, r, PhysicsConfig.E_SAME, PhysicsConfig.E_CROSS);
            // 位置修正与相位无关
            expect(approxEqual(particles[0].x, -1)).assertTrue();
            expect(approxEqual(particles[1].x, 9)).assertTrue();
            // 冲量：e=eCross=0.15, J=-(1.15)*2/2=-1.15
            // p0.vx = 1 + (-1.15) = -0.15
            // p1.vx = -1 - (-1.15) = 0.15
            expect(approxEqual(particles[0].vx, -0.15)).assertTrue();
            expect(approxEqual(particles[1].vx, 0.15)).assertTrue();
        });
        it('samePhase_differentFromCrossPhase', 0, () => {
            // 验证同相位和不同相位产生不同的碰撞结果
            const r: number = 5;
            const eSame: number = PhysicsConfig.E_SAME;
            const eCross: number = PhysicsConfig.E_CROSS;
            // 同相位
            const pSame: Particle[] = [
                new Particle(0, 0, r, 'red', 1, 0),
                new Particle(8, 0, r, 'blue', 1, 0),
            ];
            pSame[0].vx = 1;
            pSame[1].vx = -1;
            const hashSame: SpatialHash = new SpatialHash(20);
            hashSame.rebuild(pSame);
            CollisionSolver.solve(pSame, hashSame, r, eSame, eCross);
            // 不同相位
            const pCross: Particle[] = [
                new Particle(0, 0, r, 'red', 1, 0),
                new Particle(8, 0, r, 'blue', 1, 1),
            ];
            pCross[0].vx = 1;
            pCross[1].vx = -1;
            const hashCross: SpatialHash = new SpatialHash(20);
            hashCross.rebuild(pCross);
            CollisionSolver.solve(pCross, hashCross, r, eSame, eCross);
            // 同相位速度变化更大（eSame > eCross）
            expect(Math.abs(pSame[0].vx) > Math.abs(pCross[0].vx)).assertTrue();
            expect(Math.abs(pSame[1].vx) > Math.abs(pCross[1].vx)).assertTrue();
        });
        it('noCollision_preservesState', 0, () => {
            // 距离 >= 2r，不碰撞
            const r: number = 5;
            const particles: Particle[] = [
                new Particle(0, 0, r, 'red', 1, 0),
                new Particle(20, 0, r, 'blue', 1, 0), // dist=20 >= 2r=10
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
            // vrel <= 0（粒子正在远离），不应用冲量，但位置仍修正
            const r: number = 5;
            const particles: Particle[] = [
                new Particle(0, 0, r, 'red', 1, 0),
                new Particle(8, 0, r, 'blue', 1, 0),
            ];
            particles[0].vx = -1; // 向左远离
            particles[1].vx = 1; // 向右远离
            const hash: SpatialHash = new SpatialHash(20);
            hash.rebuild(particles);
            CollisionSolver.solve(particles, hash, r, 0.25, 0.15);
            // 位置修正仍执行
            expect(approxEqual(particles[0].x, -1)).assertTrue();
            expect(approxEqual(particles[1].x, 9)).assertTrue();
            // 速度不变（无冲量）
            expect(particles[0].vx).assertEqual(-1);
            expect(particles[1].vx).assertEqual(1);
        });
        it('massWeightedPositionCorrection', 0, () => {
            // 不同质量的粒子，位置修正按质量加权
            const r: number = 5;
            const particles: Particle[] = [
                new Particle(0, 0, r, 'red', 1, 0),
                new Particle(8, 0, r, 'blue', 3, 0), // mass=3 (重)
            ];
            // 零速度，只测试位置修正
            const hash: SpatialHash = new SpatialHash(20);
            hash.rebuild(particles);
            CollisionSolver.solve(particles, hash, r, 0.25, 0.15);
            // overlap=2, invSum=1/4=0.25
            // p0.x -= 3 * 0.25 * 2 * 1 = 1.5 → p0.x = -1.5 (轻粒子移动多)
            // p1.x += 1 * 0.25 * 2 * 1 = 0.5 → p1.x = 8.5 (重粒子移动少)
            expect(approxEqual(particles[0].x, -1.5)).assertTrue();
            expect(approxEqual(particles[1].x, 8.5)).assertTrue();
            // 验证重粒子位移小于轻粒子
            expect(Math.abs(particles[0].x - 0) > Math.abs(particles[1].x - 8)).assertTrue();
        });
        it('momentumConserved_equalMass', 0, () => {
            // 等质量碰撞，动量应守恒
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
            // 不同质量碰撞，动量应守恒
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
            // y 轴方向的碰撞
            const r: number = 5;
            const particles: Particle[] = [
                new Particle(0, 0, r, 'red', 1, 0),
                new Particle(0, 8, r, 'blue', 1, 0), // y方向距离8
            ];
            particles[0].vy = 1;
            particles[1].vy = -1;
            const hash: SpatialHash = new SpatialHash(20);
            hash.rebuild(particles);
            CollisionSolver.solve(particles, hash, r, 0.25, 0.15);
            // ny=1, 位置修正和冲量在y方向
            expect(approxEqual(particles[0].y, -1)).assertTrue();
            expect(approxEqual(particles[1].y, 9)).assertTrue();
            expect(approxEqual(particles[0].vy, -0.25)).assertTrue();
            expect(approxEqual(particles[1].vy, 0.25)).assertTrue();
            // x方向不变
            expect(particles[0].x).assertEqual(0);
            expect(particles[1].x).assertEqual(0);
        });
        it('diagonalCollision', 0, () => {
            // 对角线方向碰撞
            const r: number = 5;
            const dist: number = 8;
            const particles: Particle[] = [
                new Particle(0, 0, r, 'red', 1, 0),
                new Particle(dist / Math.sqrt(2), dist / Math.sqrt(2), r, 'blue', 1, 0),
            ];
            // 沿对角线相向运动
            const speed: number = 1;
            particles[0].vx = speed / Math.sqrt(2);
            particles[0].vy = speed / Math.sqrt(2);
            particles[1].vx = -speed / Math.sqrt(2);
            particles[1].vy = -speed / Math.sqrt(2);
            const hash: SpatialHash = new SpatialHash(20);
            hash.rebuild(particles);
            CollisionSolver.solve(particles, hash, r, 0.25, 0.15);
            // 碰撞后应反向（沿法线方向）
            // 法线方向 = (1/√2, 1/√2)
            // 速度应从 +speed 变为 -speed*eSame = -0.25
            const v0Normal: number = particles[0].vx / Math.sqrt(2) + particles[0].vy / Math.sqrt(2);
            expect(approxEqual(v0Normal, -0.25)).assertTrue();
        });
        it('zeroDistance_noCollision', 0, () => {
            // dist === 0 时跳过（避免除以0）
            const r: number = 5;
            const particles: Particle[] = [
                new Particle(5, 5, r, 'red', 1, 0),
                new Particle(5, 5, r, 'blue', 1, 0), // 完全重叠
            ];
            particles[0].vx = 1;
            particles[1].vx = -1;
            const hash: SpatialHash = new SpatialHash(20);
            hash.rebuild(particles);
            CollisionSolver.solve(particles, hash, r, 0.25, 0.15);
            // dist2 === 0, 跳过，状态不变
            expect(particles[0].x).assertEqual(5);
            expect(particles[1].x).assertEqual(5);
            expect(particles[0].vx).assertEqual(1);
            expect(particles[1].vx).assertEqual(-1);
        });
        it('multipleParticles_pairwise', 0, () => {
            // 三个粒子，p0和p1碰撞，p0和p2不碰撞
            const r: number = 5;
            const particles: Particle[] = [
                new Particle(0, 0, r, 'red', 1, 0),
                new Particle(8, 0, r, 'blue', 1, 0),
                new Particle(50, 0, r, 'green', 1, 0), // index 2, 远离
            ];
            particles[0].vx = 1;
            particles[1].vx = -1;
            particles[2].vx = 0;
            const hash: SpatialHash = new SpatialHash(20);
            hash.rebuild(particles);
            CollisionSolver.solve(particles, hash, r, 0.25, 0.15);
            // p0和p1碰撞，p2不变
            expect(approxEqual(particles[0].vx, -0.25)).assertTrue();
            expect(approxEqual(particles[1].vx, 0.25)).assertTrue();
            expect(particles[2].vx).assertEqual(0);
            expect(particles[2].x).assertEqual(50);
        });
        it('customRestitutionValues', 0, () => {
            // 使用自定义恢复系数而非 PhysicsConfig
            const r: number = 5;
            const particles: Particle[] = [
                new Particle(0, 0, r, 'red', 1, 0),
                new Particle(8, 0, r, 'blue', 1, 0),
            ];
            particles[0].vx = 1;
            particles[1].vx = -1;
            const hash: SpatialHash = new SpatialHash(20);
            hash.rebuild(particles);
            // 自定义 eSame=0.5, eCross=0.1
            CollisionSolver.solve(particles, hash, r, 0.5, 0.1);
            // e=0.5, J=-(1.5)*2/2=-1.5
            // p0.vx = 1 - 1.5 = -0.5
            // p1.vx = -1 + 1.5 = 0.5
            expect(approxEqual(particles[0].vx, -0.5)).assertTrue();
            expect(approxEqual(particles[1].vx, 0.5)).assertTrue();
        });
    });
}
