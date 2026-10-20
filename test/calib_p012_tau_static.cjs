// EA-batch TAU_STATIC 标定脚本：真实 tile 采样 → 加性调制 → 档量化 → 水体 std
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const fs = require('fs');
require.extensions['.ets'] = (mod, filename) => mod._compile(ts.transpileModule(
  fs.readFileSync(filename, 'utf8'), { compilerOptions: {
    target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS
  }}).outputText, filename);
const m = require('../entry/src/main/ets/render/P012WaterSandRenderer.ets');
const r = new m.P012WaterSandRenderer(128, 192);

for (const [tiers, base, amp] of [[12, 0.55, 1.6], [12, 0.45, 2.0], [16, 0.5, 2.0], [16, 0.6, 2.4]]) {
  const step = 0.55 / tiers;
  const lums = [];
  for (let y = 1420; y < 1700; y += 8) {
    for (let x = 240; x < 1035; x += 8) {
      const mot = r['mottleAt'](x, y);
      const tau = Math.max(0, base + amp * (mot - 1));
      let alpha = 1 - Math.exp(-tau);
      alpha = Math.min(0.55, alpha);
      const tier = Math.min(tiers - 1, Math.floor(alpha / step));
      const a = (tier + 0.5) * step;
      lums.push(187 * (1 - a) + 236 * a);
    }
  }
  const mean = lums.reduce((s, v) => s + v, 0) / lums.length;
  const std = Math.sqrt(lums.reduce((s, v) => s + (v - mean) ** 2, 0) / lums.length);
  const mn = Math.min(...lums), mx = Math.max(...lums);
  console.log(`tiers=${tiers} base=${base} amp=${amp} → std=${std.toFixed(1)} range=${mn.toFixed(0)}..${mx.toFixed(0)}`);
}
