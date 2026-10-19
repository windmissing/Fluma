import { WaterSandField, defaultWaterSandParams } from './WaterSandField';

const params = defaultWaterSandParams();
params.heavyDiffusion = 0;
params.lightDiffusion = 0;
params.waterLevelRate = 0.001;
params.totalMotionBudget = 1;
params.vSandTopMax = 30;
params.heavySettleSpeed = 100;
params.lightSettleSpeed = 50;
params.percolationRate = 1000;
params.angleDeg = 1;
params.bottomCreepDeadZone = 0.5;
params.maxAvalancheFlux = 100;
params.heavyLiftRate = 1000;
params.lightLiftRate = 1000;
const field = new WaterSandField(8, 192);
field.configure(params);
field.initialize({
  waterRegions: [{ startCol: 0, endColInclusive: 7, bottom: 120, height: 60 }],
  heavyDepositedRegions: [{ startCol: 0, endColInclusive: 7, bottom: 0, height: 50 }],
  lightDepositedRegions: [{ startCol: 0, endColInclusive: 7, bottom: 50, height: 50 }],
  heavySuspendedRegions: [{ startCol: 0, endColInclusive: 7, bottom: 0, height: 10 }],
  lightSuspendedRegions: [{ startCol: 0, endColInclusive: 7, bottom: 0, height: 10 }]
});
const before = field.snapshot();
field.setTilt(20);
field.setShakeInput(1);
console.log(field.step(1 / 60));
const after = field.snapshot();
let beforeSuspension = 0;
let afterSuspension = 0;
for (let c = 0; c < after.cols; c++) {
  beforeSuspension += before.suspH[c] + before.suspL[c];
  afterSuspension += after.suspH[c] + after.suspL[c];
}
console.log({ beforeSuspension, afterSuspension, delta: afterSuspension - beforeSuspension });
