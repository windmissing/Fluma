import type { Particle } from './Particle';
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
let bjccovmuyz06w2 = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "entry/src/main/ets/physics/Bounds.ets", hash: "0382af32ddf927baf9a8d44ea15edb184d7169b1b8141b534a66fffb701e5704", lineCnt: 24, count: 0, projectPath: "", projectPathHash: "ceba5a5fd1f37ddf984966c405a592e2d3985b2f0267a1b8661240241877a6b9", functions: { 0: { name: "Bounds.reflect", count: 0, regions: { 0: { startLoc: { line: 4, col: 3 }, endLoc: { line: 23, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 5, col: 24 }, endLoc: { line: 8, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 9, col: 24 }, endLoc: { line: 12, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 13, col: 24 }, endLoc: { line: 16, col: 6 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 17, col: 24 }, endLoc: { line: 20, col: 6 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 21, col: 29 }, endLoc: { line: 21, col: 38 }, count: 0, ignored: 0 }, 6: { startLoc: { line: 22, col: 29 }, endLoc: { line: 22, col: 38 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 5, col: 9 }, endLoc: { line: 5, col: 22 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 1: { startLoc: { line: 9, col: 9 }, endLoc: { line: 9, col: 22 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 2: { startLoc: { line: 13, col: 9 }, endLoc: { line: 13, col: 22 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 3: { startLoc: { line: 17, col: 9 }, endLoc: { line: 17, col: 22 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 4: { startLoc: { line: 21, col: 9 }, endLoc: { line: 21, col: 27 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 }, 5: { startLoc: { line: 22, col: 9 }, endLoc: { line: 22, col: 27 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 0 } }, exeLine: { 0: 1, 1: 3, 2: 4, 3: 5, 4: 6, 5: 7, 6: 9, 7: 10, 8: 11, 9: 13, 10: 14, 11: 15, 12: 17, 13: 18, 14: 19, 15: 21, 16: 22 } });
export class Bounds {
    static reflect(p: Particle, w: number, h: number, e: number): void {
        bjccovmuyz06w2.instrumentFunction(0);
        if (p.x - p.r < 0) {
            bjccovmuyz06w2.instrumentBranch(0, 0, true);
            bjccovmuyz06w2.instrumentRegion(0, 1);
            p.x = p.r;
            p.vx = -p.vx * e;
        }
        else {
            bjccovmuyz06w2.instrumentBranch(0, 0, false);
        }
        if (p.x + p.r > w) {
            bjccovmuyz06w2.instrumentBranch(0, 1, true);
            bjccovmuyz06w2.instrumentRegion(0, 2);
            p.x = w - p.r;
            p.vx = -p.vx * e;
        }
        else {
            bjccovmuyz06w2.instrumentBranch(0, 1, false);
        }
        if (p.y - p.r < 0) {
            bjccovmuyz06w2.instrumentBranch(0, 2, true);
            bjccovmuyz06w2.instrumentRegion(0, 3);
            p.y = p.r;
            p.vy = -p.vy * e;
        }
        else {
            bjccovmuyz06w2.instrumentBranch(0, 2, false);
        }
        if (p.y + p.r > h) {
            bjccovmuyz06w2.instrumentBranch(0, 3, true);
            bjccovmuyz06w2.instrumentRegion(0, 4);
            p.y = h - p.r;
            p.vy = -p.vy * e;
        }
        else {
            bjccovmuyz06w2.instrumentBranch(0, 3, false);
        }
        if (Math.abs(p.vx) < 1) {
            bjccovmuyz06w2.instrumentBranch(0, 4, true);
            bjccovmuyz06w2.instrumentRegion(0, 5);
            p.vx = 0;
        }
        else {
            bjccovmuyz06w2.instrumentBranch(0, 4, false);
        }
        if (Math.abs(p.vy) < 1) {
            bjccovmuyz06w2.instrumentBranch(0, 5, true);
            bjccovmuyz06w2.instrumentRegion(0, 6);
            p.vy = 0;
        }
        else {
            bjccovmuyz06w2.instrumentBranch(0, 5, false);
        }
    }
}
