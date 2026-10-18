// Executes the actual ArkTS product helper and Hypium tests using the bundled TS compiler.
// No rewritten model: .ets modules are transpiled in memory and use Node's module loader.
const fs = require('fs');
const path = require('path');
const assert = require('assert/strict');
const Module = require('module');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const results = [];
const hypium = {
  describe: (_name, fn) => fn(),
  it: (name, _flags, fn) => {
    try { fn(); results.push({name, status: 'PASS'}); }
    catch (error) { results.push({name, status: 'FAIL', error: error.message}); }
  },
  expect: value => ({
    assertEqual: expected => assert.strictEqual(value, expected),
    assertTrue: () => assert.strictEqual(value, true),
    assertFalse: () => assert.strictEqual(value, false),
    assertNull: () => assert.strictEqual(value, null),
    assertUndefined: () => assert.strictEqual(value, undefined),
    assertContain: expected => assert.ok(value.includes(expected)),
    assertLarger: expected => assert.ok(value > expected),
    assertLess: expected => assert.ok(value < expected),
    assertFail: () => assert.fail()
  })
};
const originalLoad = Module._load;
Module._load = function(name, parent, main) {
  return name === '@ohos/hypium' ? hypium : originalLoad.call(this, name, parent, main);
};
require.extensions['.ets'] = (mod, filename) => mod._compile(ts.transpileModule(
  fs.readFileSync(filename, 'utf8'), { compilerOptions: {
    target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS
  }}).outputText, filename);
require('../entry/src/test/P011InputModeTest.ets').default();
require('../entry/src/test/WaterSandFieldTest.ets').default();
const output = {execution: 'actual-product-ets-via-bundled-typescript', results,
  passed: results.filter(r => r.status === 'PASS').length,
  failed: results.filter(r => r.status === 'FAIL').length};
const outputPath = process.argv[2];
if (outputPath) {
  fs.mkdirSync(path.dirname(outputPath), {recursive: true});
  fs.writeFileSync(outputPath, JSON.stringify(output, null, 2));
}
console.log(JSON.stringify(output, null, 2));
process.exitCode = output.failed ? 1 : 0;
