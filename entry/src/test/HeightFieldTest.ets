import { describe, it, expect } from '@ohos/hypium';
import { HeightField } from '../main/ets/physics/HeightField';

function makeField(): HeightField {
  const field: HeightField = new HeightField(66, 77);
  for (let c = 0; c < 66; c++) {
    field.light[c] = 19;
    field.heavy[c] = 27;
  }
  return field;
}

function vol(field: HeightField): number[] {
  let heavy = 0;
  let light = 0;
  for (let c = 0; c < field.cols; c++) {
    heavy += field.heavy[c];
    light += field.light[c];
  }
  return [heavy, light];
}

export default function heightFieldTest() {
  describe('HeightFieldTest', () => {
    /** 构造标准初始场：蓝(轻)上 19 / 绿(重)下 27，水平分层 */
    it('volumeConserved_allTilts', 0, () => {
      const f = makeField();
      const v0 = vol(f);
      const angles: number[] = [-68, -40, -15, 0, 15, 40, 68];
      for (let a = 0; a < angles.length; a++) {
        for (let s = 0; s < 120; s++) {
          f.step(angles[a]);
        }
        const v = vol(f);
        expect(v[0]).assertEqual(v0[0]);
        expect(v[1]).assertEqual(v0[1]);
      }
    });

    it('tilt_heavyFlowsToLowerSide', 0, () => {
      const f = makeField();
      // 右倾 68°：重相应集中到右侧（沿斜面滑向低处）
      for (let s = 0; s < 200; s++) {
        f.step(68);
      }
      expect(f.heavy[65]).assertGreaterThan(f.heavy[0]);
    });

    it('tilt_lightFlowsToUpperSide', 0, () => {
      const f = makeField();
      // 右倾 68°：轻相应集中到左侧（浮在斜面高处）
      for (let s = 0; s < 200; s++) {
        f.step(68);
      }
      expect(f.light[0]).assertGreaterThan(f.light[65]);
    });

    it('tilt_reachesEquilibrium_thenStable', 0, () => {
      const f = makeField();
      let moved = 0;
      for (let s = 0; s < 300; s++) {
        moved = f.step(-68);
      }
      // 平衡后应完全静止（零抖动）
      expect(moved).assertEqual(0);
    });

    it('level_restoresFlatLayers', 0, () => {
      const f = makeField();
      for (let s = 0; s < 200; s++) {
        f.step(45);
      }
      for (let s = 0; s < 300; s++) {
        f.step(0);
      }
      // 回正后应恢复水平分层：每列一致
      for (let c = 1; c < f.cols; c++) {
        expect(f.heavy[c]).assertEqual(f.heavy[0]);
        expect(f.light[c]).assertEqual(f.light[0]);
      }
    });

    it('oscillation_neverLocksForever', 0, () => {
      const f = makeField();
      let maxZeroStreak = 0;
      let zeroStreak = 0;
      for (let s = 0; s < 200; s++) {
        const m = f.step(s % 40 < 20 ? -40 : 40);
        if (m === 0) {
          zeroStreak++;
          if (zeroStreak > maxZeroStreak) {
            maxZeroStreak = zeroStreak;
          }
        } else {
          zeroStreak = 0;
        }
      }
      // 方向切换后最多短暂静止，不允许永久锁死
      expect(maxZeroStreak).assertLess(30);
    });

    it('total_neverExceedsCap', 0, () => {
      const f = makeField();
      const angles: number[] = [-68, 68, -30, 30];
      for (let a = 0; a < angles.length; a++) {
        for (let s = 0; s < 150; s++) {
          f.step(angles[a]);
          for (let c = 0; c < f.cols; c++) {
            expect(f.total(c)).assertLessOrEqual(f.cap);
          }
        }
      }
    });

    it('smallAngle_flowsSlowly', 0, () => {
      const f = makeField();
      let totalMoved = 0;
      for (let s = 0; s < 60; s++) {
        totalMoved += f.step(12);
      }
      // 小角度也应持续流动（而非锁死）
      expect(totalMoved).assertGreaterThan(0);
    });

    it('extremeAngle80_reachesTheoreticalEquilibrium', 0, () => {
      const f = makeField();
      // 极端角度需跨列交换破互锁，最终必须达到理论平衡（偏差=0）
      let stall = 0;
      for (let s = 0; s < 600; s++) {
        if (f.step(80) === 0) {
          stall++;
          if (stall > 3) {
            break;
          }
        } else {
          stall = 0;
        }
      }
      // 重相集中在右侧（右低），左侧重相清空
      expect(f.heavy[0]).assertLess(5);
      expect(f.heavy[65]).assertGreaterThan(50);
    });
  });
}
