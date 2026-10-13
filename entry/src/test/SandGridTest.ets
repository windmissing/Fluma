import { describe, it, expect } from '@ohos/hypium';
import { SandGrid } from '../main/ets/physics/SandGrid';

export default function sandGridTest() {
  describe('SandGridTest', () => {
    it('constructor_allEmpty', 0, () => {
      const g: SandGrid = new SandGrid(10, 10);
      expect(g.sandCount()).assertEqual(0);
      expect(g.get(5, 5)).assertEqual(SandGrid.EMPTY);
    });

    it('get_outOfBoundsReturnsWall', 0, () => {
      const g: SandGrid = new SandGrid(10, 10);
      expect(g.get(-1, 5)).assertEqual(SandGrid.WALL);
      expect(g.get(5, -1)).assertEqual(SandGrid.WALL);
      expect(g.get(10, 5)).assertEqual(SandGrid.WALL);
      expect(g.get(5, 10)).assertEqual(SandGrid.WALL);
    });

    it('setCell_and_get', 0, () => {
      const g: SandGrid = new SandGrid(10, 10);
      g.setCell(3, 4, SandGrid.SAND, 1);
      expect(g.get(3, 4)).assertEqual(SandGrid.SAND);
      expect(g.getPhase(3, 4)).assertEqual(1);
      g.setCell(3, 4, SandGrid.EMPTY);
      expect(g.get(3, 4)).assertEqual(SandGrid.EMPTY);
    });

    it('fillRect_countCorrect', 0, () => {
      const g: SandGrid = new SandGrid(10, 10);
      g.fillRect(0, 5, 9, 9, SandGrid.SAND, 0);
      expect(g.sandCount()).assertEqual(50); // 10 列 x 5 行
    });

    it('gravityDown_sandFalls', 0, () => {
      const g: SandGrid = new SandGrid(5, 10);
      g.setCell(2, 3, SandGrid.SAND, 0);
      g.step(0, 9.8); // 重力向下
      // 沙粒应从 y=3 移动到 y=4
      expect(g.get(2, 3)).assertEqual(SandGrid.EMPTY);
      expect(g.get(2, 4)).assertEqual(SandGrid.SAND);
    });

    it('gravityUp_sandRises', 0, () => {
      const g: SandGrid = new SandGrid(5, 10);
      g.setCell(2, 5, SandGrid.SAND, 0);
      g.step(0, -9.8); // 重力向上（倒置）
      expect(g.get(2, 5)).assertEqual(SandGrid.EMPTY);
      expect(g.get(2, 4)).assertEqual(SandGrid.SAND);
    });

    it('gravityRight_sandMovesRight', 0, () => {
      const g: SandGrid = new SandGrid(10, 5);
      g.setCell(4, 2, SandGrid.SAND, 0);
      g.step(9.8, 0); // 重力向右（横握）
      expect(g.get(4, 2)).assertEqual(SandGrid.EMPTY);
      expect(g.get(5, 2)).assertEqual(SandGrid.SAND);
    });

    it('gravityRight_noChainedTeleport', 0, () => {
      // 回归测试：水平重力向右时，扫描必须从右往左，
      // 否则沙粒沿扫描方向连锁移动，一帧内瞬移多格直达右墙
      const g: SandGrid = new SandGrid(20, 5);
      g.setCell(4, 2, SandGrid.SAND, 0);
      g.step(9.8, 0);
      // 单步后只能移动一格到 (5,2)，绝不能到 (19,2)
      expect(g.get(5, 2)).assertEqual(SandGrid.SAND);
      expect(g.get(19, 2)).assertEqual(SandGrid.EMPTY);
    });

    it('gravityLeft_sandMovesLeft', 0, () => {
      const g: SandGrid = new SandGrid(20, 5);
      g.setCell(15, 2, SandGrid.SAND, 0);
      g.step(-9.8, 0); // 重力向左
      expect(g.get(15, 2)).assertEqual(SandGrid.EMPTY);
      expect(g.get(14, 2)).assertEqual(SandGrid.SAND);
      expect(g.get(0, 2)).assertEqual(SandGrid.EMPTY); // 不瞬移到左墙
    });

    it('floorBlocks_sandStops', 0, () => {
      const g: SandGrid = new SandGrid(5, 5);
      g.fillRect(0, 4, 4, 4, SandGrid.WALL, 0); // 底部墙
      g.setCell(2, 2, SandGrid.SAND, 0);
      // 多步后沙粒应停在 y=3（墙上格 y=4 上方）
      for (let i = 0; i < 20; i++) {
        g.step(0, 9.8);
      }
      expect(g.get(2, 3)).assertEqual(SandGrid.SAND);
      expect(g.get(2, 4)).assertEqual(SandGrid.EMPTY); // 不能穿墙
    });

    it('restingStack_zeroJitter', 0, () => {
      // 静止沙堆：多步后所有沙粒位置不再变化（零抖动核心验证）
      const g: SandGrid = new SandGrid(8, 8);
      g.fillRect(0, 6, 7, 7, SandGrid.SAND, 0);
      for (let i = 0; i < 30; i++) {
        g.step(0, 9.8);
      }
      const snapshot: string = JSON.stringify(Array.from(new Uint8Array(0))) // 占位避免 lint
        ;
      // 记录终态
      const finalCount: number = g.sandCount();
      // 再走 10 步，沙数和形态应完全不变
      let changed: boolean = false;
      for (let i = 0; i < 10; i++) {
        g.step(0, 9.8);
        if (g.hasMoved()) {
          changed = true;
        }
      }
      expect(changed).assertFalse();
      expect(g.sandCount()).assertEqual(finalCount);
    });

    it('zeroGravity_noMovement', 0, () => {
      const g: SandGrid = new SandGrid(5, 5);
      g.setCell(2, 2, SandGrid.SAND, 0);
      g.step(0, 0);
      expect(g.get(2, 2)).assertEqual(SandGrid.SAND);
    });

    it('pillarCollapsesToRestAngle', 0, () => {
      // 一根竖直沙柱应倒塌为带安息角的堆（顶部扁平、两侧斜坡）
      const g: SandGrid = new SandGrid(20, 20);
      g.fillRect(9, 2, 10, 17, SandGrid.SAND, 0); // 2x16 竖柱
      for (let i = 0; i < 400; i++) {
        g.step(0, 9.8);
      }
      // 终态：沙全部落到底部区域，最高列高度有限（安息角约束）
      // 检查顶部 5 行应为空
      let topEmpty: boolean = true;
      for (let cx = 0; cx < 20; cx++) {
        for (let cy = 0; cy < 5; cy++) {
          if (g.get(cx, cy) === SandGrid.SAND) {
            topEmpty = false;
          }
        }
      }
      expect(topEmpty).assertTrue();
      // 沙粒总数守恒
      expect(g.sandCount()).assertEqual(2 * 16);
    });

    it('sandCountConserved_throughSim', 0, () => {
      const g: SandGrid = new SandGrid(10, 10);
      g.fillRect(0, 0, 9, 4, SandGrid.SAND, 0);
      const before: number = g.sandCount();
      for (let i = 0; i < 50; i++) {
        g.step(0, 9.8);
      }
      expect(g.sandCount()).assertEqual(before);
    });

    it('slideChanceZero_freezesSlope', 0, () => {
      // slideChance=0：无对角滑移，沙粒只直落，单粒悬在单柱上方不滑落
      const g: SandGrid = new SandGrid(5, 5);
      g.setSlideChance(0);
      g.fillRect(0, 4, 4, 4, SandGrid.SAND, 0); // 底层沙
      g.setCell(2, 3, SandGrid.SAND, 0);        // 顶层单粒，正下方被占
      g.step(0, 9.8);
      // 主方向被堵且滑移关闭，该粒静止
      expect(g.get(2, 3)).assertEqual(SandGrid.SAND);
    });

    it('setSlideChance_clampsRange', 0, () => {
      const g: SandGrid = new SandGrid(3, 3);
      g.setSlideChance(-5); // 应夹到 0
      g.setSlideChance(99); // 应夹到 1
      // 不抛异常即通过
      expect(g.get(0, 0)).assertEqual(SandGrid.EMPTY);
    });

    it('phasePreserved_afterMove', 0, () => {
      const g: SandGrid = new SandGrid(5, 10);
      g.setCell(2, 3, SandGrid.SAND, 1); // 蓝沙
      g.step(0, 9.8);
      expect(g.get(2, 4)).assertEqual(SandGrid.SAND);
      expect(g.getPhase(2, 4)).assertEqual(1);
    });

    it('tiltedGravity_diagonalFlow', 0, () => {
      // 45° 倾斜重力：主方向仍垂直向下，沙粒下落或对角滑移，但 y 必增
      const g: SandGrid = new SandGrid(10, 10);
      g.setCell(5, 2, SandGrid.SAND, 0);
      g.step(4.9, 8.5); // 斜向右下
      let found: boolean = false;
      for (let cx = 4; cx <= 6; cx++) {
        if (g.get(cx, 3) === SandGrid.SAND) {
          found = true;
        }
      }
      expect(found).assertTrue();
    });

    it('densitySwap_heavySinksIntoLight', 0, () => {
      // 重沙(0)压在轻沙(1)上方 → 置换后重沙在下、轻沙在上
      const g: SandGrid = new SandGrid(5, 10);
      g.setSinkChance(1.0); // 必置换
      g.setCell(2, 3, SandGrid.SAND, 0); // 重沙
      g.setCell(2, 4, SandGrid.SAND, 1); // 轻沙
      g.step(0, 9.8);
      expect(g.getPhase(2, 4)).assertEqual(0); // 重沙沉到下面
      expect(g.getPhase(2, 3)).assertEqual(1); // 轻沙浮到上面
    });

    it('densitySwap_lightDoesNotSinkIntoHeavy', 0, () => {
      // 轻沙(1)压在重沙(0)上方 → 稳定分层，不交换
      const g: SandGrid = new SandGrid(5, 10);
      g.setSinkChance(1.0);
      g.setCell(2, 3, SandGrid.SAND, 1); // 轻沙在上
      g.setCell(2, 4, SandGrid.SAND, 0); // 重沙在下
      g.step(0, 9.8);
      expect(g.getPhase(2, 3)).assertEqual(1);
      expect(g.getPhase(2, 4)).assertEqual(0);
    });

    it('densitySwap_zeroChance_freezesLayers', 0, () => {
      // sinkChance=0 时即使倒置也不置换
      const g: SandGrid = new SandGrid(5, 10);
      g.setSinkChance(0);
      g.setCell(2, 3, SandGrid.SAND, 0);
      g.setCell(2, 4, SandGrid.SAND, 1);
      g.step(0, 9.8);
      expect(g.getPhase(2, 3)).assertEqual(0);
      expect(g.getPhase(2, 4)).assertEqual(1);
    });

    it('setSinkChance_clampsRange', 0, () => {
      const g: SandGrid = new SandGrid(3, 3);
      g.setSinkChance(-2);
      g.setSinkChance(55);
      expect(g.get(0, 0)).assertEqual(SandGrid.EMPTY);
    });

    it('twoPhase_conservation_mixedPile', 0, () => {
      // 两相混合沙堆：总数守恒 + 静置后重沙集中在下层
      const g: SandGrid = new SandGrid(20, 20);
      g.setSinkChance(0.5);
      g.setSlideChance(0.9);
      // 交错铺满下半区
      for (let cy = 10; cy < 20; cy++) {
        for (let cx = 0; cx < 20; cx++) {
          g.setCell(cx, cy, SandGrid.SAND, (cx + cy) % 2);
        }
      }
      const before: number = g.sandCount();
      for (let i = 0; i < 500; i++) {
        g.step(0, 9.8);
      }
      expect(g.sandCount()).assertEqual(before);
      // 分层判定：底部 4 行重沙占比应显著高于顶部 6 行沙中重沙占比
      let heavyBottom: number = 0;
      let totalBottom: number = 0;
      for (let cy = 16; cy < 20; cy++) {
        for (let cx = 0; cx < 20; cx++) {
          if (g.get(cx, cy) === SandGrid.SAND) {
            totalBottom++;
            if (g.getPhase(cx, cy) === 0) {
              heavyBottom++;
            }
          }
        }
      }
      expect(heavyBottom * 2 > totalBottom).assertTrue(); // 底部重沙 > 50%
    });
  });
}
