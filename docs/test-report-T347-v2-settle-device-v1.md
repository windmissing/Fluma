# T347 V2 粒子沉降真机验证（E1-E4，T346 时延假说证实）（真机，完成——E1-E4 全 PASS）

2026-10-31，tester 真机（VYG-AL00 62T0225B10010020）。验证对象：T346 修复方向验证（不修代码，给足模拟时间）——commit `41c5351`（T342 8de02c9 链 + T347 观测 wiring）。

## 0. 包身份与取证 wiring（T151 三件套）

- **T347-signed.hap SHA256 `0195c9359f6cf397cd5bf76afc17a7702e459da93a23bc47b862ff3252df445d`**（localSign + verify-app PASS）。
- **取证 wiring（0bb30f0 + 41c5351，如实登记）**：`P051DerivedV2Demo.ets` 页面侧每 120 帧经 `exportPayloadCold()` 打 `T347STEP` 行（pYmin/pYmean/live/motion/mh/ml——T345 §3 真机接点）；观测语义零物理/审计改动。ArkTS 10605999 null-safe 修复一并登记。
- 启动：`--ps p051v2 1`（V2 显式入口）；注入 = uitest pan swipe 1.5s×23 笔（`T218DBG inject ok=true adm>0`）。

## 1. E1 观测窗延长至模拟时间 ≥7s：**PASS**

hilog 实时重建（T346 方法，p013_scene commit/frame 差分）：**首注入后累计模拟时间 11.93s ≥ 7s**（358 帧 × dt=1/30 cap——1.3fps 下 tick 间隔 0.77s 远超 1/30，cap 恒生效，重建精确非近似）。fps 坍塌曲线（附带采集）：空场 12.2fps → 注入后 5.1→2.8→2.0→稳定 1.3-1.4fps（供 V2 包装层性能卡定量输入）。

## 2. E2 pY 真机下降：**PASS**

`T347STEP` 轨迹（对照 harness 基线 215→0.577 形态）：

| frame | live | pYmin | pYmean | motion | mh |
|---|---|---|---|---|---|
| 120 | 0 | — | — | 0 | 0 |
| 240 | 107 | 105.885 | 124.315 | 107 | 0 |
| 360 | 96 | **1.155** | **22.247** | 65 | **1.1547** |
| 480 | 96 | 1.155 | 22.232 | 63 | 1.1547 |

pYmean 124.3→22.2 单调下降，pYmin 触底 1.155（床面 y≈0.577 附近）——**粒子真机慢速下落证实，T346 时延假说成立**。

## 3. E3 床量非零 + LAND：**PASS**

mh 0 → **1.1547**（frame 240→360），首注入到首个 LAND 的模拟时间 ≈4s > 3.44s 落差时延——**V2 物理链真机完整走通：注入→沉降→着陆→床增长**。frame 360→480 mh 恒定（注入停止后床稳定，静置保持 ✓）。

## 4. E4 双态同帧可见：**PASS**

- 同 FrameToken：p013_scene 全程 `state=CURRENT`（170/170）、request==actual 逐行相等、零 code100/STALE/FAULT。
- 截图 s1：右下沙粒束（下落中 live 粒子 + 束底已着陆/床沙）可见；床层视觉极薄（mh=1.15 单粒量级，与 harness mh=0.577 同量级），随持续注入可增厚。

## 5. 结论

**E1-E4 全 PASS**：T346 判定的设备层根因（帧率坍塌致模拟时间不足）真机证实——模拟时间给足后 V2 全链（沉降/LAND/床增长/双态渲染）真机完整工作，无代码修改达成。**T343 的 D2/D3 FAIL 撤销**（其观测窗 3.27s < 3.44s 时延，粒子物理上尚未落完）。

移交：
- T218 DD12 判据复活且达成——V1→V2 生产切换的门条件（T342 S1-S4 + 本卡 D 系/ E 系）已齐，**切换决策交 manager**。
- fps 坍塌（12→1.3fps）独立问题按 T346 §4 移交 V2 包装层性能卡（barrier verifyPublicBoundary 256×70 对拍 + matchesAuthority 全量比较为定量输入）。

证据：`test/reports/t347/`（hilog_full.txt 23323 行 + hilog_p1.txt + step_all.txt + s1.jpeg）；签包 `.codex/local-test/t318-sign/T347-signed.hap`。
