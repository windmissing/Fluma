import { describe, it, expect } from '@ohos/hypium';
import { PhysicsConfig } from '../../../main/ets/physics/PhysicsConfig';

export default function physicsConfigTest() {
  describe('PhysicsConfigTest', () => {
    it('G_is2500', 0, () => {
      expect(PhysicsConfig.G).assertEqual(2500);
    });

    it('E_is0_4', 0, () => {
      expect(PhysicsConfig.E).assertEqual(0.4);
    });

    it('R_is4', 0, () => {
      expect(PhysicsConfig.R).assertEqual(4);
    });

    it('DT_isOneOver60', 0, () => {
      expect(Math.abs(PhysicsConfig.DT - 1 / 60) < 1e-12).assertTrue();
    });

    it('E_SAME_is0_25', 0, () => {
      expect(PhysicsConfig.E_SAME).assertEqual(0.25);
    });

    it('E_CROSS_is0_15', 0, () => {
      expect(PhysicsConfig.E_CROSS).assertEqual(0.15);
    });

    it('LAMBDA_is1_6', 0, () => {
      expect(PhysicsConfig.LAMBDA).assertEqual(1.6);
    });

    it('E_SAME_greaterThanE_CROSS', 0, () => {
      expect(PhysicsConfig.E_SAME > PhysicsConfig.E_CROSS).assertTrue();
    });

    it('allConstantsPositive', 0, () => {
      expect(PhysicsConfig.G > 0).assertTrue();
      expect(PhysicsConfig.E > 0).assertTrue();
      expect(PhysicsConfig.R > 0).assertTrue();
      expect(PhysicsConfig.DT > 0).assertTrue();
      expect(PhysicsConfig.E_SAME > 0).assertTrue();
      expect(PhysicsConfig.E_CROSS > 0).assertTrue();
      expect(PhysicsConfig.LAMBDA > 0).assertTrue();
    });

    it('restitutionCoefficientsLessThanOne', 0, () => {
      expect(PhysicsConfig.E < 1).assertTrue();
      expect(PhysicsConfig.E_SAME < 1).assertTrue();
      expect(PhysicsConfig.E_CROSS < 1).assertTrue();
    });
  });
}
