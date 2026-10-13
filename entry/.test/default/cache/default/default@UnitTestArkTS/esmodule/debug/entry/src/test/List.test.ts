import { describe, it, expect } from "@package:pkg_modules/.ohpm/@ohos+hypium@1.0.6/pkg_modules/@ohos/hypium/index";
import particleTest from "@bundle:com.example.helloworld/entry/src/test/ParticleTest";
import physicsConfigTest from "@bundle:com.example.helloworld/entry/src/test/PhysicsConfigTest";
import spatialHashTest from "@bundle:com.example.helloworld/entry/src/test/SpatialHashTest";
import boundsTest from "@bundle:com.example.helloworld/entry/src/test/BoundsTest";
import collisionSolverTest from "@bundle:com.example.helloworld/entry/src/test/CollisionSolverTest";
export default function abilityTest() {
    describe('ActsAbilityTest', () => {
        it('assertContain', 0, () => {
            let a: string = 'abc';
            let b: string = 'b';
            expect(a).assertContain(b);
        });
    });
    particleTest();
    physicsConfigTest();
    spatialHashTest();
    boundsTest();
    collisionSolverTest();
}
