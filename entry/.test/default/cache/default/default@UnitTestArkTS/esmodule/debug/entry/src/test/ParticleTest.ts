import { describe, it, expect } from "@package:pkg_modules/.ohpm/@ohos+hypium@1.0.6/pkg_modules/@ohos/hypium/index";
import { Particle } from "@bundle:com.example.helloworld/entry/ets/physics/Particle";
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
            // 半隐式欧拉：先更新速度，再用新速度更新位置
            const p: Particle = new Particle(0, 0, 5, 'red');
            p.integrate(1, 10, 20);
            // vx = 0 + 10*1 = 10, vy = 0 + 20*1 = 20
            expect(p.vx).assertEqual(10);
            expect(p.vy).assertEqual(20);
        });
        it('integrate_gravityUpdatesPositionWithNewVelocity', 0, () => {
            // 验证半隐式欧拉：位置使用更新后的速度
            const p: Particle = new Particle(100, 200, 5, 'red');
            p.integrate(1, 10, 0);
            // vx = 0 + 10*1 = 10
            // x = 100 + 10*1 = 110 (使用新速度，不是旧速度)
            expect(p.x).assertEqual(110);
            expect(p.y).assertEqual(200);
        });
        it('integrate_withInitialVelocity', 0, () => {
            const p: Particle = new Particle(100, 200, 5, 'red');
            p.vx = 5;
            p.vy = 3;
            p.integrate(0.5, 20, 40);
            // vx = 5 + 20*0.5 = 15
            // vy = 3 + 40*0.5 = 23
            // x = 100 + 15*0.5 = 107.5
            // y = 200 + 23*0.5 = 211.5
            expect(approxEqual(p.vx, 15)).assertTrue();
            expect(approxEqual(p.vy, 23)).assertTrue();
            expect(approxEqual(p.x, 107.5)).assertTrue();
            expect(approxEqual(p.y, 211.5)).assertTrue();
        });
        it('integrate_multipleStepsAccumulate', 0, () => {
            const p: Particle = new Particle(0, 0, 5, 'red');
            const dt: number = 1 / 60;
            const g: number = 2500;
            // 模拟两步积分
            p.integrate(dt, 0, g);
            const vx1: number = p.vx;
            const x1: number = p.x;
            p.integrate(dt, 0, g);
            // 第二步：vy = g*dt + g*dt = 2*g*dt
            expect(approxEqual(p.vy, 2 * g * dt)).assertTrue();
            // 位置：y1 = g*dt*dt, y2 = y1 + (2*g*dt)*dt = g*dt*dt + 2*g*dt*dt = 3*g*dt*dt
            expect(approxEqual(p.y, 3 * g * dt * dt)).assertTrue();
        });
        it('integrate_zeroGravityPreservesVelocity', 0, () => {
            const p: Particle = new Particle(50, 50, 5, 'red');
            p.vx = 10;
            p.vy = -5;
            p.integrate(2, 0, 0);
            // 无重力，速度不变
            expect(p.vx).assertEqual(10);
            expect(p.vy).assertEqual(-5);
            // 位置：x = 50 + 10*2 = 70, y = 50 + (-5)*2 = 40
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
            // vy = 100 + (-50)*1 = 50
            expect(p.vy).assertEqual(50);
            // y = 0 + 50*1 = 50
            expect(p.y).assertEqual(50);
        });
        it('integrate_semiImplicitEulerDiffersFromExplicit', 0, () => {
            // 半隐式欧拉与显式欧拉的区别：
            // 半隐式：x_new = x + v_new * dt
            // 显式：x_new = x + v_old * dt
            const p: Particle = new Particle(0, 0, 5, 'red');
            p.integrate(1, 10, 0);
            // 半隐式：vx=10, x=0+10*1=10
            // 显式会是：vx=10, x=0+0*1=0
            expect(p.x).assertEqual(10);
        });
    });
}
