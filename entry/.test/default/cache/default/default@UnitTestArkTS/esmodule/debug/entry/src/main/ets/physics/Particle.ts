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
let bjccovmuyz06vf = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "entry/src/main/ets/physics/Particle.ets", hash: "bbe01e9e83253c274af988a077f75b6442770450e071d757a1fff1aba58cda62", lineCnt: 27, count: 0, projectPath: "", projectPathHash: "ceba5a5fd1f37ddf984966c405a592e2d3985b2f0267a1b8661240241877a6b9", functions: { 0: { name: "Particle.constructor", count: 0, regions: { 0: { startLoc: { line: 11, col: 3 }, endLoc: { line: 18, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 12, col: 5 }, endLoc: { line: 18, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "Particle.integrate", count: 0, regions: { 0: { startLoc: { line: 20, col: 3 }, endLoc: { line: 25, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 21, col: 5 }, endLoc: { line: 25, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 6, 6: 7, 7: 8, 8: 9, 9: 11, 10: 12, 11: 13, 12: 14, 13: 15, 14: 16, 15: 17, 16: 20, 17: 21, 18: 22, 19: 23, 20: 24 } });
export class Particle {
    x: number = 0;
    y: number = 0;
    vx: number = 0;
    vy: number = 0;
    r: number;
    mass: number = 1;
    color: string;
    phase: number = 0;
    constructor(x: number, y: number, r: number, color: string, mass: number = 1, phase: number = 0) {
        bjccovmuyz06vf.instrumentFunction(0);
        bjccovmuyz06vf.instrumentRegion(0, 1);
        this.x = x;
        this.y = y;
        this.r = r;
        this.color = color;
        this.mass = mass;
        this.phase = phase;
    }
    integrate(dt: number, gx: number, gy: number): void {
        bjccovmuyz06vf.instrumentFunction(1);
        bjccovmuyz06vf.instrumentRegion(1, 1);
        this.vx += gx * dt;
        this.vy += gy * dt;
        this.x += this.vx * dt;
        this.y += this.vy * dt;
    }
}
