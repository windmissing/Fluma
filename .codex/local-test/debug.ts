import { WaterSandField, defaultWaterSandParams } from './WaterSandField';

const params = defaultWaterSandParams();
params.heavyDiffusion = 0;
params.lightDiffusion = 0;
params.waterLevelRate = 0.001;
params.vSandTopMax = 1;
params.totalMotionBudget = 1;
const field = new WaterSandField(2, 192);
field.configure(params);
field.initialize({
  waterRegions: [{ startCol: 0, endColInclusive: 1, bottom: 144, height: 38 }],
  heavyDepositedRegions: [{ startCol: 0, endColInclusive: 1, bottom: 0, height: 72 }],
  lightDepositedRegions: [{ startCol: 0, endColInclusive: 1, bottom: 72, height: 72 }],
  heavySuspendedRegions: [],
  lightSuspendedRegions: []
});
field.setTilt(20);
const result = field.step(1 / 60);
console.log(result);
console.log(field.getP011Metric());
