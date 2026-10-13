import type { Particle } from './Particle';
import type { SpatialHash } from './SpatialHash';
class BjcCov {
    coverage: {
        [key: string]: string | number;
    };
    constructor(covData: object) {
        const gcv = "__coverage__";
        let coverage = globalThis[gcv] || (globalThis[gcv] = {});
        if (!coverage[covData.path] && true) {
            coverage[covData.path] = covData;
        }
        this.coverage = coverage[covData.path];
    }
    instrumentFunction(func: number) {
        this.coverage.functions[func].count++;
        this.coverage.functions[func].regions[0].count++;
    }
    instrumentRegion(func: number, region: number) {
        this.coverage.functions[func].regions[region].count++;
    }
    instrumentReturn(func: number, retIdx: number) {
        this.coverage.functions[func].returnes[retIdx].count++;
    }
    instrumentBranch(func: number, branch: number, trueOrFalse: boolean) {
        if (trueOrFalse) {
            this.coverage.functions[func].branches[branch].trueCount++;
            for (let r of Object.values(this.coverage.functions[func].branches[branch].group)) {
                if (r !== branch) {
                    this.coverage.functions[func].branches[r as number].falseCount++;
                }
            }
        }
        else {
            this.coverage.functions[func].branches[branch].falseCount++;
        }
    }
}
let bjccovmuyz06wd = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "entry/src/main/ets/physics/CollisionSolver.ets", hash: "a0bc66089677f3a51a8bd926f2c2fcc54812dc294fdde2d0b1103dafdfd800d8", lineCnt: 42, count: 0, projectPath: "", projectPathHash: "ceba5a5fd1f37ddf984966c405a592e2d3985b2f0267a1b8661240241877a6b9", functions: { 0: { name: "CollisionSolver.solve", count: 0, regions: { 0: { startLoc: { line: 5, col: 3 }, endLoc: { line: 41, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 9, col: 5 }, endLoc: { line: 40, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 12, col: 7 }, endLoc: { line: 39, col: 8 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 14, col: 21 }, endLoc: { line: 14, col: 30 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 19, col: 47 }, endLoc: { line: 19, col: 56 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 33, col: 24 }, endLoc: { line: 33, col: 33 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 34, col: 9 }, endLoc: { line: 39, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 14, col: 13 }, endLoc: { line: 14, col: 19 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 19, col: 13 }, endLoc: { line: 19, col: 45 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 20, col: 19 }, endLoc: { line: 20, col: 57 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 33, col: 13 }, endLoc: { line: 33, col: 22 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 0 } }, exeLine: { 0: 1, 1: 2, 2: 4, 3: 5, 4: 6, 5: 7, 6: 9, 7: 10, 8: 11, 9: 12, 10: 13, 11: 14, 12: 15, 13: 16, 14: 17, 15: 18, 16: 19, 17: 20, 18: 21, 19: 22, 20: 23, 21: 24, 22: 25, 23: 26, 24: 27, 25: 28, 26: 29, 27: 30, 28: 31, 29: 32, 30: 33, 31: 34, 32: 35, 33: 36, 34: 37, 35: 38 } });
export class CollisionSolver {
    static solve(particles: Particle[], hash: SpatialHash, r: number, eSame: number, eCross: number): void {
        bjccovmuyz06wd.instrumentFunction(0);
        const minDist = 2 * r;
        const minDist2 = minDist * minDist;
        for (let i = 0; i < particles.length; i++) {
            bjccovmuyz06wd.instrumentRegion(0, 1);
            const pi = particles[i];
            const neighbors = hash.queryNearby(pi);
            for (let k = 0; k < neighbors.length; k++) {
                bjccovmuyz06wd.instrumentRegion(0, 2);
                const j = neighbors[k];
                if (j <= i) {
                    bjccovmuyz06wd.instrumentBranch(0, 0, true);
                    bjccovmuyz06wd.instrumentRegion(0, 3);
                    continue;
                }
                else {
                    bjccovmuyz06wd.instrumentBranch(0, 0, false);
                }
                const pj = particles[j];
                const dx = pj.x - pi.x;
                const dy = pj.y - pi.y;
                const dist2 = dx * dx + dy * dy;
                if (dist2 >= minDist2 || dist2 === 0) {
                    bjccovmuyz06wd.instrumentBranch(0, 1, true);
                    bjccovmuyz06wd.instrumentRegion(0, 4);
                    continue;
                }
                else {
                    bjccovmuyz06wd.instrumentBranch(0, 1, false);
                }
                const e = pi.phase === pj.phase ? (bjccovmuyz06wd.instrumentBranch(0, 2, true), eSame) : (bjccovmuyz06wd.instrumentBranch(0, 2, false), eCross);
                const dist = Math.sqrt(dist2);
                const nx = dx / dist;
                const ny = dy / dist;
                const overlap = minDist - dist;
                const mi = pi.mass;
                const mj = pj.mass;
                const invSum = 1 / (mi + mj);
                pi.x -= mj * invSum * overlap * nx;
                pi.y -= mj * invSum * overlap * ny;
                pj.x += mi * invSum * overlap * nx;
                pj.y += mi * invSum * overlap * ny;
                const vrel = (pi.vx - pj.vx) * nx + (pi.vy - pj.vy) * ny;
                if (vrel <= 0) {
                    bjccovmuyz06wd.instrumentBranch(0, 3, true);
                    bjccovmuyz06wd.instrumentRegion(0, 5);
                    continue;
                }
                else {
                    bjccovmuyz06wd.instrumentBranch(0, 3, false);
                }
                bjccovmuyz06wd.instrumentRegion(0, 6);
                const J = -(1 + e) * vrel / (1 / mi + 1 / mj);
                pi.vx += J / mi * nx;
                pi.vy += J / mi * ny;
                pj.vx -= J / mj * nx;
                pj.vy -= J / mj * ny;
            }
        }
    }
}
