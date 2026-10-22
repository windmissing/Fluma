// P-012 E3 N1 噪声场定标前置报告（§12.4 红线：幅值系数不直接沿用）
// 多种子实测：均值/std/clamp 占比 + 与旧 mottle（实测 std 0.5494）对照。
// 运行：node test/calib_p012_e3_noise.cjs
const fs = require('fs');
const Module = require('module');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const _load = Module._load;
Module._load = function (name, parent, main) {
  if (name === '@ohos.hilog' || name === '@ohos/hilog') {
    return { default: { info: () => {}, error: () => {}, warn: () => {}, debug: () => {} } };
  }
  return _load.call(this, name, parent, main);
};
require.extensions['.ets'] = (mod, filename) => mod._compile(ts.transpileModule(
  fs.readFileSync(filename, 'utf8'), { compilerOptions: {
    target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS
  }}).outputText, filename);
const m = require('../entry/src/main/ets/render/P012E3Noise.ets');

console.log('=== N1 定标前置报告（多种子实测，§12.4）===');
console.log('采样域：921x1099（=显示盒）+ 余量 64 → 985x1163，步长 1px');
console.log('');
const W = 985, H = 1163;
const seeds = [20261020, 20261021, 20261022, 20261023];
for (const seed of seeds) {
  const f = new m.P012E3NoiseField(seed);
  f.bakeCoverage(0, 0, W, H);
  let sum = 0, sum2 = 0, clampHi = 0, clampLo = 0;
  const N = W * H;
  for (let j = 0; j < H; j += 3) {          // 步长 3 抽样（覆盖率 11%）
    for (let i = 0; i < W; i += 3) {
      const v = f.sample(i, j);
      sum += v;
      sum2 += v * v;
      if (v >= 0.999) clampHi++;
      if (v <= -0.999) clampLo++;
    }
  }
  const n = Math.ceil(W / 3) * Math.ceil(H / 3);
  const mean = sum / n;
  const std = Math.sqrt(sum2 / n - mean * mean);
  console.log(`seed=${seed}: mean=${mean.toFixed(4)} std=${std.toFixed(4)} ` +
    `clampHi=${(clampHi / n * 100).toFixed(3)}% clampLo=${(clampLo / n * 100).toFixed(3)}%`);
}
// 与旧 mottle 对照：mottle = 1 + a1·n1 + a2·n2（std 0.5494 实测），
// N1 输出为加权噪声本体（零均值），staticTau 链需要 mottle 语义 →
// renderer 侧用 mottleEquiv = 1 + A1·n1field.sample(...)，A1 按下式重定标：
// 旧链 mottle std 0.5494（a1=1.3/a2=1.0 时）→ 等效总幅值 k = 0.5494/1.63
// ≈ 0.337（1.63=sqrt(1.3²+1.0²) 归一化权重和）；N1 的 w1/w2 已按 1.3/1.0
// 归一，等效 A1 = 1.63，即 mottleEquiv std ≈ N1_std × 1.63。
console.log('');
console.log('=== 对照与重定标 ===');
const f = new m.P012E3NoiseField(20261020);
let sum = 0, sum2 = 0, n = 0;
for (let j = 0; j < H; j += 3) {
  for (let i = 0; i < W; i += 3) {
    const v = f.sample(i, j);
    sum += v; sum2 += v * v; n++;
  }
}
const std = Math.sqrt(sum2 / n - (sum / n) ** 2);
console.log(`N1(原种子) std=${std.toFixed(4)}`);
console.log(`旧 mottle 实测 std=0.5494（EA 标定链，calib-tau-static 报告）`);
const equiv = std * 1.63;
console.log(`等效 mottle（1+1.63·N1）std=${equiv.toFixed(4)} vs 旧 0.5494 → 比例 ${(equiv / 0.5494).toFixed(3)}`);
