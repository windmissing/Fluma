import waterSandFieldTest from './WaterSandFieldTest';
import { selectedTests } from './hypium';

waterSandFieldTest();
let failed = 0;
for (const test of selectedTests) {
  try {
    test.run();
    console.log(`PASS ${test.name}`);
  } catch (error) {
    failed++;
    console.error(`FAIL ${test.name}: ${(error as Error).message}`);
  }
}
if (failed > 0 || selectedTests.length !== 3) process.exitCode = 1;
