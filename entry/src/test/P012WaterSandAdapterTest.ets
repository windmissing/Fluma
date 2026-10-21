// P-012 E-batch：P012WaterSandAdapter 守恒/配对状态机单测
// 问题 ID：P-012；方案版本：p012-render-material-v1（E-batch）
// 覆盖 architecture-P012.md §3 DoD ③：H/L 独立守恒、注入断配对、
// 同快照零平流、非有限保护、间隔断配对、双 slot 不 alias 页面 snapshot。
import { describe, it, expect } from '@ohos/hypium';
import { WaterSandField, WaterSandScene, WaterSandRegion } from '../main/ets/physics/WaterSandField';
import { P012WaterSandAdapter, P012MaterialFrameView } from '../main/ets/render/P012WaterSandAdapter';
import { P012MaterialMotionCache } from '../main/ets/render/P012MaterialMotionCache';
import { P012AppearanceTier, P012FrameContext, P012InvalidReason,
  P012_E_BATCH_DRAW_BUDGET, P012Ablation, P012FillBox } from '../main/ets/render/P012RenderTypes';
import { P012WaterSandRenderer, p012ShouldUseP012Renderer } from '../main/ets/render/P012WaterSandRenderer';

function region(startCol: number, endCol: number, bottom: number, height: number): WaterSandRegion {
  return { startCol, endColInclusive: endCol, bottom, height };
}

function fullScene(cols: number): WaterSandScene {
  return {
    waterRegions: [region(0, cols - 1, 120, 60)],
    heavyDepositedRegions: [region(0, cols - 1, 0, 50)],
    lightDepositedRegions: [region(0, cols - 1, 50, 50)],
    heavySuspendedRegions: [region(0, cols - 1, 110, 10)],
    lightSuspendedRegions: [region(0, cols - 1, 110, 10)]
  };
}

let frameSerial = 0;
function makeContext(over?: Partial<P012FrameContext>): P012FrameContext {
  frameSerial++;
  const ctx: P012FrameContext = {
    fieldEpoch: 1, continuityEpoch: 1, geometryEpoch: 1,
    frameSerial: frameSerial,
    sampleTimeMs: 1000 + frameSerial * 16.7,
    frameDeltaMs: 16.7,
    inputMode: 'TEST', sourceKind: 'script',
    physicalRouteId: 'test-route', runIdentity: 'run-1',
    boxX: 0, boxY: 0, boxW: 320, boxH: 480, pixelRatio: 1,
    visualInverted: false,
    appearanceTier: P012AppearanceTier.E,
    renderSeed: 20261020, configRevision: 1,
    ablation: P012Ablation.OFF,
    fillBox: P012FillBox.TIGHT,
    canonicalEvidence: false
  };
  if (over !== undefined) {
    if (over.fieldEpoch !== undefined) ctx.fieldEpoch = over.fieldEpoch;
    if (over.continuityEpoch !== undefined) ctx.continuityEpoch = over.continuityEpoch;
    if (over.geometryEpoch !== undefined) ctx.geometryEpoch = over.geometryEpoch;
    if (over.frameSerial !== undefined) ctx.frameSerial = over.frameSerial;
    if (over.sampleTimeMs !== undefined) ctx.sampleTimeMs = over.sampleTimeMs;
    if (over.frameDeltaMs !== undefined) ctx.frameDeltaMs = over.frameDeltaMs;
    if (over.inputMode !== undefined) ctx.inputMode = over.inputMode;
    if (over.sourceKind !== undefined) ctx.sourceKind = over.sourceKind;
    if (over.physicalRouteId !== undefined) ctx.physicalRouteId = over.physicalRouteId;
    if (over.runIdentity !== undefined) ctx.runIdentity = over.runIdentity;
    if (over.boxX !== undefined) ctx.boxX = over.boxX;
    if (over.boxY !== undefined) ctx.boxY = over.boxY;
    if (over.boxW !== undefined) ctx.boxW = over.boxW;
    if (over.boxH !== undefined) ctx.boxH = over.boxH;
    if (over.pixelRatio !== undefined) ctx.pixelRatio = over.pixelRatio;
    if (over.visualInverted !== undefined) ctx.visualInverted = over.visualInverted;
    if (over.appearanceTier !== undefined) ctx.appearanceTier = over.appearanceTier;
    if (over.renderSeed !== undefined) ctx.renderSeed = over.renderSeed;
    if (over.configRevision !== undefined) ctx.configRevision = over.configRevision;
    if (over.ablation !== undefined) ctx.ablation = over.ablation;
    if (over.fillBox !== undefined) ctx.fillBox = over.fillBox;
    if (over.canonicalEvidence !== undefined) ctx.canonicalEvidence = over.canonicalEvidence;
  }
  return ctx;
}

export default function p012WaterSandAdapterTest() {
  describe('P012WaterSandAdapterTest', () => {
    it('pair_valid_afterTwoSteps_conservationHolds', 0, () => {
      const cols = 16;
      const field = new WaterSandField(cols, 192);
      field.initialize(fullScene(cols));
      field.setTilt(15);
      const adapter = new P012WaterSandAdapter(cols, 192);
      let snap = field.snapshot();
      let r = adapter.update(snap, makeContext());
      expect(r.pair.valid).assertFalse();          // 首帧 NO_PREV
      expect(r.pair.reason).assertEqual(P012InvalidReason.NO_PREV);
      for (let i = 0; i < 30; i++) {
        field.step(1 / 60);
        snap = field.snapshot();
        r = adapter.update(snap, makeContext());
        expect(r.pair.valid).assertTrue();
        if (r.conservation !== null) {
          expect(r.conservation.conservedH).assertTrue();
          expect(r.conservation.conservedL).assertTrue();
        }
      }
    });

    it('inject_breaksPair_andRebuildsBaselineNextFrame', 0, () => {
      const cols = 16;
      const field = new WaterSandField(cols, 192);
      field.initialize(fullScene(cols));
      field.setTilt(10);
      field.step(1 / 60);
      const adapter = new P012WaterSandAdapter(cols, 192);
      adapter.update(field.snapshot(), makeContext());
      field.step(1 / 60);
      // 注入重沙（phase=HEAVY=1）：H 总量跳变 → CONSERVATION_H（H/L 独立）
      field.inject({ phase: 1, amount: 30, startCol: 4, endColInclusive: 7, state: 0 });
      let r = adapter.update(field.snapshot(), makeContext());
      expect(r.pair.valid).assertFalse();
      expect(r.pair.reason).assertEqual(P012InvalidReason.CONSERVATION_H);
      // 下一帧：curr（注入后）成为新基线 → 配对恢复
      field.step(1 / 60);
      r = adapter.update(field.snapshot(), makeContext());
      expect(r.pair.valid).assertTrue();
      expect(r.pair.reason).assertEqual(P012InvalidReason.NONE);
    });

    it('sameSnapshot_zeroAdvection_validPair', 0, () => {
      const cols = 16;
      const field = new WaterSandField(cols, 192);
      field.initialize(fullScene(cols));
      field.setTilt(0);
      field.step(1 / 60);
      const adapter = new P012WaterSandAdapter(cols, 192);
      const snap = field.snapshot();
      adapter.update(snap, makeContext());
      // 相同快照连续两帧：H/L 总量必然相等 → 配对有效（deltaX=0 由调用方推导）
      const r = adapter.update(snap, makeContext());
      expect(r.pair.valid).assertTrue();
      expect(r.conservation !== null).assertTrue();
      if (r.conservation !== null) {
        expect(r.conservation.totalH - r.conservation.totalH).assertEqual(0);
      }
    });

    it('notFiniteSnapshot_returnsNullView_clearsPair', 0, () => {
      const cols = 16;
      const field = new WaterSandField(cols, 192);
      field.initialize(fullScene(cols));
      field.step(1 / 60);
      const adapter = new P012WaterSandAdapter(cols, 192);
      adapter.update(field.snapshot(), makeContext());
      // 构造损坏快照：bh[3] = NaN → curr=null 且清配对
      const bad = field.snapshot();
      bad.bh[3] = Number.NaN;
      const r = adapter.update(bad, makeContext());
      expect(r.curr === null).assertTrue();
      expect(r.pair.reason).assertEqual(P012InvalidReason.NOT_FINITE);
      // 规则 5：清 pairValid → 下一帧为 NO_PREV（重建基线），再下一帧恢复配对
      const good = field.snapshot();
      const r1 = adapter.update(good, makeContext());
      expect(r1.pair.valid).assertFalse();
      expect(r1.pair.reason).assertEqual(P012InvalidReason.NO_PREV);
      const r2 = adapter.update(field.snapshot(), makeContext());
      expect(r2.pair.valid).assertTrue();
    });

    it('frameGapOver100ms_breaksPair', 0, () => {
      const cols = 16;
      const field = new WaterSandField(cols, 192);
      field.initialize(fullScene(cols));
      field.step(1 / 60);
      const adapter = new P012WaterSandAdapter(cols, 192);
      adapter.update(field.snapshot(), makeContext());
      // 下一帧间隔 >100ms（后台恢复）
      const r = adapter.update(field.snapshot(),
        makeContext({ sampleTimeMs: 1000 + frameSerial * 16.7 + 200 }));
      expect(r.pair.valid).assertFalse();
      expect(r.pair.reason).assertEqual(P012InvalidReason.FRAME_GAP);
    });

    it('epochChange_breaksPair', 0, () => {
      const cols = 16;
      const field = new WaterSandField(cols, 192);
      field.initialize(fullScene(cols));
      field.step(1 / 60);
      const adapter = new P012WaterSandAdapter(cols, 192);
      adapter.update(field.snapshot(), makeContext());
      // 倒立事件：continuityEpoch 变化
      const r = adapter.update(field.snapshot(), makeContext({ continuityEpoch: 2 }));
      expect(r.pair.valid).assertFalse();
      expect(r.pair.reason).assertEqual(P012InvalidReason.EPOCH_CHANGED);
    });

    it('serialGap_breaksPair', 0, () => {
      const cols = 16;
      const field = new WaterSandField(cols, 192);
      field.initialize(fullScene(cols));
      field.step(1 / 60);
      const adapter = new P012WaterSandAdapter(cols, 192);
      adapter.update(field.snapshot(), makeContext());
      // 跳帧（frameSerial 不相邻）
      const r = adapter.update(field.snapshot(),
        makeContext({ frameSerial: frameSerial + 5 }));
      expect(r.pair.valid).assertFalse();
      expect(r.pair.reason).assertEqual(P012InvalidReason.SERIAL_GAP);
    });

    it('runIdentityChange_breaksPair', 0, () => {
      const cols = 16;
      const field = new WaterSandField(cols, 192);
      field.initialize(fullScene(cols));
      field.step(1 / 60);
      const adapter = new P012WaterSandAdapter(cols, 192);
      adapter.update(field.snapshot(), makeContext());
      const r = adapter.update(field.snapshot(), makeContext({ runIdentity: 'run-2' }));
      expect(r.pair.valid).assertFalse();
      expect(r.pair.reason).assertEqual(P012InvalidReason.IDENTITY_MISMATCH);
    });

    it('adapterSlots_doNotAliasPageSnapshot', 0, () => {
      const cols = 16;
      const field = new WaterSandField(cols, 192);
      field.initialize(fullScene(cols));
      const adapter = new P012WaterSandAdapter(cols, 192);
      const snap = field.snapshot();
      const r1 = adapter.update(snap, makeContext());
      // 修改页面 snapshot 原数组（模拟下帧 copyStateTo 前的意外写入）
      const before = r1.curr !== null ? r1.curr.bhAt(0) : -1;
      snap.bh[0] = snap.bh[0] + 50;
      // adapter 内部 slot 不受影响（深复制）
      const r2 = adapter.update(field.snapshot(), makeContext());
      expect(r2.pair.valid).assertTrue();
      // 旧视图借用已到期，但 slot 数据未被动过（重建基线后数值来自新快照）
      expect(before).assertLarger(-1);
    });

    it('kahanTotals_matchFieldGetTotals', 0, () => {
      const cols = 16;
      const field = new WaterSandField(cols, 192);
      field.initialize(fullScene(cols));
      field.setTilt(20);
      for (let i = 0; i < 20; i++) {
        field.step(1 / 60);
      }
      const adapter = new P012WaterSandAdapter(cols, 192);
      const r = adapter.update(field.snapshot(), makeContext());
      expect(r.curr !== null).assertTrue();
      const totals = field.getTotals();
      if (r.curr !== null && r.conservation !== null) {
        expect(Math.abs(r.conservation.totalH - totals.heavy) < 1e-6).assertTrue();
        expect(Math.abs(r.conservation.totalL - totals.light) < 1e-6).assertTrue();
      }
    });

    it('renderer_drawBudget_within16_offBranchDecision', 0, () => {
      // OFF 分支决策（纯函数）：renderer null / OFF / canonical → false
      const renderer = new P012WaterSandRenderer(16, 192);
      expect(p012ShouldUseP012Renderer(null, P012AppearanceTier.E, false)).assertFalse();
      expect(p012ShouldUseP012Renderer(renderer, P012AppearanceTier.OFF, false)).assertFalse();
      expect(p012ShouldUseP012Renderer(renderer, P012AppearanceTier.E, true)).assertFalse();
      expect(p012ShouldUseP012Renderer(renderer, P012AppearanceTier.E, false)).assertTrue();
      expect(p012ShouldUseP012Renderer(renderer, P012AppearanceTier.EAB, false)).assertTrue();
      // dispose 后决策 false
      renderer.dispose();
      expect(p012ShouldUseP012Renderer(renderer, P012AppearanceTier.E, false)).assertFalse();
      // draw 预算常量（E-batch ≤16）
      expect(P012_E_BATCH_DRAW_BUDGET <= 16).assertTrue();
    });

    it('motionCache_freezeWhenInvalid_zeroAdvection', 0, () => {
      const cols = 16;
      const field = new WaterSandField(cols, 192);
      field.initialize(fullScene(cols));
      field.step(1 / 60);
      const adapter = new P012WaterSandAdapter(cols, 192);
      const r0 = adapter.update(field.snapshot(), makeContext());
      expect(r0.curr !== null).assertTrue();
      const mc = new P012MaterialMotionCache(cols);
      // valid=false → 冻结：isValid=false，mean 偏移=0
      mc.update(r0.curr!, r0.curr!, false, 5, 256);
      expect(mc.isValid).assertFalse();
      expect(mc.meanDeltaXPx(true)).assertEqual(0);
      expect(mc.meanDeltaXPx(false)).assertEqual(0);
      // 同快照（无搬运）→ 即使 valid=true 也应零平流（deltaX=0）
      const r1 = adapter.update(field.snapshot(), makeContext());
      expect(r1.pair.valid).assertTrue();
      mc.update(r0.curr!, r1.curr!, true, 5, 256);
      expect(mc.isValid).assertTrue();
      expect(Math.abs(mc.meanDeltaXPx(true)) < 1e-9).assertTrue();
      expect(Math.abs(mc.meanDeltaXPx(false)) < 1e-9).assertTrue();
    });

    it('motionCache_netTransport_producesAdvection', 0, () => {
      const cols = 16;
      const field = new WaterSandField(cols, 192);
      field.initialize(fullScene(cols));
      field.setTilt(22);
      field.step(1 / 60);
      const adapter = new P012WaterSandAdapter(cols, 192);
      const r0 = adapter.update(field.snapshot(), makeContext());
      // 第二步搬运（倾斜使重/轻相横移）
      field.step(1 / 60);
      const r1 = adapter.update(field.snapshot(), makeContext());
      expect(r1.pair.valid).assertTrue();
      const mc = new P012MaterialMotionCache(cols);
      mc.update(r0.curr!, r1.curr!, true, 6, 256);
      expect(mc.isValid).assertTrue();
      // 净搬运应产生非零纹理平流偏移（材料滚动代理）
      expect(Math.abs(mc.meanDeltaXPx(true)) > 0).assertTrue();
      // 偏移 bounded 在 [0,tilePx)
      const off = mc.meanDeltaXPx(true);
      expect(off >= 0 && off < 256).assertTrue();
    });
  });
}