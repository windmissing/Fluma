import math
import unittest


ANGLE_DEG = 30.0
EFFECTIVE_TILT_DEG = 15.0
TARGET_AMPLITUDE = 2.7
HALF_SUPPORT_COLUMNS = 2


def former_target_delta():
    width = 2 * HALF_SUPPORT_COLUMNS
    center = width // 2
    return [(-TARGET_AMPLITUDE if index < center else TARGET_AMPLITUDE)
            for index in range(width)]


def required_flux_profile(target):
    flux = []
    cumulative = 0.0
    for delta in target:
        cumulative += delta
        flux.append(-cumulative)
    return flux


class FormerCandidateAngleTests(unittest.TestCase):
    def test_central_step_exceeds_relax_angle_limit(self):
        limit = math.tan(math.radians(ANGLE_DEG))
        target_slope = math.tan(math.radians(EFFECTIVE_TILT_DEG))
        target = former_target_delta()
        deviations = [abs(target[index + 1] - target[index] - target_slope)
                      for index in range(len(target) - 1)]
        maximum = max(deviations)
        self.assertGreater(maximum, limit)
        self.assertGreater(maximum, 2 * TARGET_AMPLITUDE - target_slope - limit)


class FormerReleaseTests(unittest.TestCase):
    def test_flux_residual_cannot_observe_avalanche_shape_drift(self):
        target = former_target_delta()
        flux_target = required_flux_profile(target)
        cumulative_flux = list(flux_target)
        flux_residual = sum(abs(want - got) for want, got in
                            zip(flux_target, cumulative_flux))
        self.assertEqual(flux_residual, 0.0)

        actual_bh = list(target)
        avalanche_flux = 0.5
        donor = len(actual_bh) // 2
        receiver = donor - 1
        actual_bh[donor] -= avalanche_flux
        actual_bh[receiver] += avalanche_flux

        shape_residual = sum(abs(got - want) for got, want in
                             zip(actual_bh, target))
        self.assertEqual(flux_residual, 0.0)
        self.assertGreater(shape_residual, 0.0)
        self.assertEqual(shape_residual, 2 * avalanche_flux)


if __name__ == '__main__':
    unittest.main()
