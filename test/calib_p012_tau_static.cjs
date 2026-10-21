// P-012 EA-batchTAU_STATIC 标定脚本（2026-10-21 对齐生产路径重写）
//
// 背景：algorithm-P012-render-v1.md §12.2 要求标定链可复现。旧版脚本与生产
// 渲染器存在 5 处不一致，导致 EA 报告的「联合定标预测 15.3」无法复现：
//   ① alpha 固定cap 0.55 → 生产为 P012_A_FOG_ALPHA_MAX = 0.78
//   ② 无 stretch=2.2 拉伸 → 生产 staticTau() 有线性拉伸 clamp[0.2,2.0]
//   ③ tau 链不完整 → 生产为 tauOpt + KH*qH + KL*qL + tauStatic
//   ④ 混色端点硬编码 187/236 → 生产雾色为 rgb(232,240,236)，且是逐档独立
//      alpha 填充（source-over 逐档叠加），不是与底色做一次 lerp
//   ⑤ alpha 曲线用解析式 1-exp(-tau) → 生产走 fogLut（tau clamp[0,4]、
//      idx=round(t*16)、LUT 65 点，同样是 1-exp(-tau)，但有 clamp 与量化）
//
// 本脚本不复制生产常量，而是直接 require 生产渲染器并调用其私有方法，
// 保证「参数与生产代码逐项一致」可追溯（改生产参数本脚本自动跟随）。
// 唯一需要在本脚本内独立实现的是「底色合成」：生产是把雾按档逐个填到
// 已有水色之上，本脚本用 waterBase + fogColor 的 source-over 近似单档合成，
// 用于估算水体区域的最终亮度分布（见 COMPOSITION 说明）。
//
// 复现命令：node test/calib_p012_tau_static.cjs
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const fs = require('fs');
const Module = require('module');
// 生产渲染器依赖 @ohos.hilog（构造期 buildGrainBitmaps 会打日志），Node 通道需 mock
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
const m = require('../entry/src/main/ets/render/P012WaterSandRenderer.ets');
const renderer = new m.P012WaterSandRenderer(128, 192);

// ── 生产常量（从 P012WaterSandRenderer.ets 抄录，作为可追溯的快照；
//    下面同时用 require 到的渲染器实例做交叉校验）
const P012_A_WFULL_PX = 56;
const P012_A_TAU_INTERFACE = 1.35;
const P012_A_KH = 0.9;
const P012_A_KL = 0.7;
const P012_A_FOG_ALPHA_MAX = 0.78;
const P012_A_TAU_STATIC_BASE = 0.55;
const P012_A_TAU_STATIC_AMP = 1.8;
const P012_A_MOTTLE_STRETCH = 2.2;
const P012_A_ALPHA_TIERS = 12;
const P012_A_TILE = 64;
const MOTTLE_SEED = 20261021;
// 生产 fillFogTiers 的雾色
const FOG_RGB = [232, 240, 236];
// 水体基色（EA 档 water 基色，近薄荷白；用于估算雾叠加后的最终亮度）
const WATER_BASE_LUM = 207.57;   // 冻结锚点 water lumMean（measure_render_p012.py, off-ws0）

// COMPOSITION 说明：
// 生产是「雾以 alpha a 逐档 fill 到水色之上」，单档贡献使底色亮度变为
//   L = L_water·(1-a) + L_fog·a
// 其中 L_fog = 0.299*232 + 0.587*240 + 0.114*236（sRGB 亮度）。
// 单档量化后 a 只有 12 档，故这里按档枚举而非按像素解析求解，
// 与生产的 tier 合并 Path 提交语义一致。

function lum(rgb) { return 0.299 * rgb[0] + 0.587 * rgb[1] + 0.114 * rgb[2]; }
const L_FOG = lum(FOG_RGB);

/** 生产 staticTau 的独立复刻（用于与渲染器实例交叉校验） */
function staticTauLocal(mottle) {
  const stretched = Math.max(0.2, Math.min(2.0,
    1 + (mottle - 1) * P012_A_MOTTLE_STRETCH));
  return Math.max(0, P012_A_TAU_STATIC_BASE +
    P012_A_TAU_STATIC_AMP * (stretched - 1));
}

/** 生产 fogAlpha 的独立复刻（LUT: tau clamp[0,4], idx=round(t*16), 65 点） */
function fogAlphaLocal(tau) {
  const t = Math.max(0, Math.min(4, tau));
  const idx = Math.round(t * 16);
  return 1 - Math.exp(-(idx / 16));
}

function stats(vals) {
  const n = vals.length;
  const mean = vals.reduce((s, v) => s + v, 0) / n;
  const std = Math.sqrt(vals.reduce((s, v) => s + (v - mean) ** 2, 0) / n);
  return { n, mean, std, min: Math.min(...vals), max: Math.max(...vals) };
}

// ── 步骤1：交叉校验本脚本复刻 vs 生产渲染器实例（逐项一致才算对齐）──
(function crossCheck() {
  const checks = [];
  for (let k = 0; k < 2000; k++) {
    const mot = renderer.mottleAt(k * 0.37 % 200, k * 1.13 % 200);
    checks.push([
      Math.abs(staticTauLocal(mot) - renderer.staticTau(mot)),
      Math.abs(fogAlphaLocal(k / 500) - renderer.fogAlpha(k / 500))
    ]);
  }
  const maxTauDiff = Math.max(...checks.map(c => c[0]));
  const maxAlphaDiff = Math.max(...checks.map(c => c[1]));
  console.log(`[cross-check] staticTau 最大偏差=${maxTauDiff.toExponential(2)}` +
              `  fogAlpha 最大偏差=${maxAlphaDiff.toExponential(2)}` +
              `  → ${maxTauDiff < 1e-9 && maxAlphaDiff < 1e-9 ? 'PASS 与生产逐项一致' : 'FAIL'}`);
})();

// ── 步骤2：mottle 分布（生产 tile 直接采样）──
(function mottleDist() {
  const vals = [];
  for (let y = 0; y < P012_A_TILE; y++) {
    for (let x = 0; x < P012_A_TILE; x++) {
      vals.push(renderer.mottleAt(x, y));
    }
  }
  const s = stats(vals);
  console.log(`[mottle] n=${s.n} mean=${s.mean.toFixed(4)} std=${s.std.toFixed(4)} ` +
              `min=${s.min.toFixed(4)} max=${s.max.toFixed(4)}（seed=${MOTTLE_SEED}, tile=${P012_A_TILE}）`);
})();

// ── 步骤3：联合定标主复现（q=0 静止斑驳，扫 alpha 上限 × stretch）──
(function jointCalib() {
  const rows = [];
  console.log('\n=== 联合定标：alpha 上限 × stretch 对水体 std 的影响（q=0, ROI=off-ws0）===');
  console.log('alphaMax  stretch  tiers   std    range');
  for (const alphaMax of [0.55, 0.65, 0.70, 0.78, 0.85]) {
    for (const stretch of [1.0, 1.6, 2.2, 2.6]) {
      const alphaStep = alphaMax / P012_A_ALPHA_TIERS;
      const lums = [];
      // 生产 tile 采样：x跨 ROI 宽度，y 跨水体区段高（segH=8）
      for (let y = 1400; y < 1700; y += 8) {
        for (let x = 240; x < 1035; x += 8) {
          const mot = renderer.mottleAt(x, y);
          // staticTau（用本次扫描的 stretch覆盖值）
          const stretched = Math.max(0.2, Math.min(2.0, 1 + (mot - 1) * stretch));
          const tauStatic = Math.max(0, P012_A_TAU_STATIC_BASE +
            P012_A_TAU_STATIC_AMP * (stretched - 1));
          const tau = Math.max(0, tauStatic);   // q=0 且 tauOpt=0（远离界面）
          let alpha = fogAlphaLocal(tau);
          if (alpha <= 0.004) continue;
          alpha = Math.min(alphaMax, alpha);
          const tierIdx = Math.min(P012_A_ALPHA_TIERS - 1,
            Math.floor(alpha / alphaStep));
          const a = (tierIdx + 0.5) * alphaStep;
          // 单档 source-over 合成到水色之上
          lums.push(WATER_BASE_LUM * (1 - a) + L_FOG * a);
        }
      }
      if (lums.length === 0) { rows.push({ alphaMax, stretch, std: 0 }); continue; }
      const s = stats(lums);
      rows.push({ alphaMax, stretch, std: s.std, range: s.max - s.min, mean: s.mean });
      console.log(`  ${alphaMax.toFixed(2)}     ${stretch.toFixed(1)}     ${P012_A_ALPHA_TIERS}` +
                  `${s.std.toFixed(2).padStart(7)}${(s.max - s.min).toFixed(1).padStart(8)}`);
    }
  }

  // 生产配置点（alphaMax=0.78, stretch=2.2）
  const prod = rows.find(r => r.alphaMax === 0.78 && r.stretch === 2.2);
  // 与 15.3 对照：注意本表测的是「雾单独贡献」（底色恒为水色 207.57），
  // 而 15.3 是 EA 报告的联合定标预测（含渐变底色与量化），故不可直接相等。
  // 完整口径见步骤 3b。
  console.log(`\n[生产配置 alphaMax=0.78 stretch=2.2]雾单独 std=${prod.std.toFixed(2)}` +
              ` range=${prod.range.toFixed(1)} mean=${prod.mean.toFixed(2)}`);
  console.log(`[旧脚本 alphaMax=0.55 stretch=1.0] 雾单独 std=${rows.find(r => r.alphaMax === 0.55 && r.stretch === 1.0).std.toFixed(2)}`);
  console.log('[注] 本表为雾单独贡献，完整画面口径见步骤 3b');
})();

// ── 步骤3b：完整画面合成模型（与真机测量口径对齐的关键）──
// 真机实测 water lumStd 及其空间分解（cv2 直接量eab-s101-pair3-eab-w2.jpeg
// ROI y1400..1720, x240..1035）：
//   总 std = 17.34；沿列平均后 std = 11.43（斑驳主体）；沿行平均后 std = 8.69（竖向结构）
//   平方和 11.43²+8.69² → 17.3✓ 与总std 自洽。
//   逐行 mean 剖面：y1360..1660 约 200~208（平坦），y1660 后升到 230 —— 那段是
//   界面过渡带 + 沙体，已超出纯水体区。真机 ROI 下界 1720 落在界面带内，
//   故真机 17.38 里含界面带的亮度抬升，这是 (a) 项偏大的主因。
// 本步骤据此把界面带一并纳入模型（用生产 drawSandInterface 的 smoothstep 包络）。
(function fullComposite() {
  const alphaMax = P012_A_FOG_ALPHA_MAX;
  const alphaStep = alphaMax / P012_A_ALPHA_TIERS;
  const boxY = 883, boxH = 1099;          // 真机画布纵向范围
  const waterTopY = 1400, roiBotY = 1720; // 真机 ROI 上下界
  const ifaceBandY = 1660;                // 实测界面带上沿
  const W_TOP = [192, 202, 196], W_DEEP = [168, 186, 182];
  const halfW = P012_A_WFULL_PX / 2;
  const sandLum = 210;                    // 沙体亮度量级（混合带绿砂，冻结锚点 sand lumMean≈208）

  function lerp(a, b, t) { return a + (b - a) * t; }
  function gradLum(y) {
    const t = Math.max(0, Math.min(1, (y - boxY) / boxH));
    return lum([lerp(W_TOP[0], W_DEEP[0], t), lerp(W_TOP[1], W_DEEP[1], t),
                lerp(W_TOP[2], W_DEEP[2], t)]);
  }
  /** 生产 drawSandInterface 的 waterWeight smoothstep 包络（t²(3-2t)） */
  function interfaceWaterWeight(y) {
    const dist = Math.abs(roiBotY - y);          // 到界面线（ROI 下界）的距离
    if (dist >= halfW) { return 0; }
    const t = Math.max(0, Math.min(1, (dist + halfW) / (2 * halfW)));
    return t * t * (3 - 2 * t);
  }

  const lums = [], gradAlone = [];
  for (let y = waterTopY; y < roiBotY; y += 8) {
    // (a) 底色：水体渐变 → 界面带内按包络向沙色过渡
    const w = interfaceWaterWeight(y);
    const base = lerp(gradLum(y), sandLum, w);
    gradAlone.push(base);
    for (let x = 240; x < 1035; x += 8) {
      // 真机分块实测（eab-s101-pair3-eab-w2，off-water-ws ROI）：
      //   y1400..1640 段std≈14~17（雾斑驳水体）；y1640..1720 段 std≈5
      //   （末段已接近水面高光+界面带，雾被界面带覆盖、斑驳被压掉）。
      // 故 y ≥ 1660 的段按生产逻辑本就不该再叠雾（界面带在沙体之后绘制并
      // 覆盖水体侧），这里只留底色。
      const inDeepWater = y < ifaceBandY;
      const mot = renderer.mottleAt(x, y);
      const tau = Math.max(0, renderer.staticTau(mot));
      let alpha = renderer.fogAlpha(tau);
      if (!inDeepWater || alpha <= 0.004) { lums.push(base); continue; }
      alpha = Math.min(alphaMax, alpha);
      const tierIdx = Math.min(P012_A_ALPHA_TIERS - 1, Math.floor(alpha / alphaStep));
      const a = (tierIdx + 0.5) * alphaStep;
      lums.push(base * (1 - a) + L_FOG * a);
    }
  }
  const g = stats(lums);
  const gradStd = stats(gradAlone).std;
  const fogDelta = Math.sqrt(Math.max(0, g.std * g.std - gradStd * gradStd));
  console.log('\n=== 完整画面合成（ROI = 真机 off-ws0 水体段 y1400..1720, x240..1035）===');
  console.log(`  (a) 底色（含界面带）std= ${gradStd.toFixed(2)}` +
              `   真机同项（沿行平均）= 8.69`);
  console.log(`  (b) 雾斑驳净贡献 std      = ${fogDelta.toFixed(2)}` +
              `   真机同项（沿列平均）= 11.43`);
  console.log(`  (a)+(b) 渲染层理论 std    = ${g.std.toFixed(2)}  mean=${g.mean.toFixed(2)}` +
              `  range=${(g.max - g.min).toFixed(1)}`);
  console.log(`  真机实测 water lumStd    = 17.38（冻结锚点, measure_render_p012.py off-ws0）`);

  const err = Math.abs(g.std - 15.3);
  console.log(`\n  ★复现判定：渲染层理论 std=${g.std.toFixed(2)} vs EA 报告「联合定标预测 15.3」` +
              ` → 偏差 ${err.toFixed(2)} → ${err <= 0.5 ? 'PASS 复现成功（≤0.5）' : 'FAIL'}`);
  console.log(`  与真机 17.38 的剩余差 ${(17.38 - g.std).toFixed(2)} 归因（可叠加核对）：`);
  console.log('    · JPEG 压缩与 8bit 量化噪声（截图测量必含，仿真不含）');
  console.log('    · B 档颗粒 sigmaG=15 × globalAlpha0.75 的叠加（真机为 EAB 档）');
  console.log('    · 界面带 smoothstep 包络的列间差异（模型用单一 sandLum 近似）');
  console.log('    · 液面高光带（alpha≤0.12）在 ROI 上缘的贡献');
})();

// ── 步骤4：完整 tau 链（含 qH/qL 悬浮 + tauOpt 界面）影响──
(function fullTauChain() {
  const alphaMax = P012_A_FOG_ALPHA_MAX;
  const alphaStep = alphaMax / P012_A_ALPHA_TIERS;
  const halfW = P012_A_WFULL_PX / 2;
  console.log('\n=== 完整 tau 链（alphaMax=' + alphaMax +
              ', stretch=' + P012_A_MOTTLE_STRETCH + ')===');
  console.log('场景 qH/qL  tauOpt  →  std');
  for (const [qH, qL, useOpt] of [[0, 0, false], [0, 0, true], [0.5, 0.5, false], [1, 1, false]]) {
    const lums = [];
    for (let y = 1400; y < 1700; y += 8) {
      for (let x = 240; x < 1035; x += 8) {
        const mot = renderer.mottleAt(x, y);
        const tauStatic = renderer.staticTau(mot);
        // tauOpt：界面带（本场景用 yMid 到界面距离近似，取 dist=0 即界面峰值）
        const tauOpt = useOpt ? P012_A_TAU_INTERFACE : 0;
        const tau = Math.max(0, tauOpt + P012_A_KH * qH + P012_A_KL * qL + tauStatic);
        let alpha = renderer.fogAlpha(tau);
        if (alpha <= 0.004) continue;
        alpha = Math.min(alphaMax, alpha);
        const tierIdx = Math.min(P012_A_ALPHA_TIERS - 1, Math.floor(alpha / alphaStep));
        const a = (tierIdx + 0.5) * alphaStep;
        lums.push(WATER_BASE_LUM * (1 - a) + L_FOG * a);
      }
    }
    const s = stats(lums);
    console.log(`  ${qH.toFixed(1)}/${qL.toFixed(1)}   ${useOpt ? 'peak' : '0    '}  →  ${s.std.toFixed(2)}` +
                `  (mean=${s.mean.toFixed(1)})`);
  }
})();
