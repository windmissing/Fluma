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
let bjccovmuyz06vq = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "entry/src/main/ets/physics/SpatialHash.ets", hash: "746fe06223d3b27b152deb8ee8e91307c279f761e5c5a65a8170b00b0aaad80b", lineCnt: 54, count: 0, projectPath: "", projectPathHash: "ceba5a5fd1f37ddf984966c405a592e2d3985b2f0267a1b8661240241877a6b9", functions: { 0: { name: "SpatialHash.constructor", count: 0, regions: { 0: { startLoc: { line: 8, col: 3 }, endLoc: { line: 10, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 9, col: 5 }, endLoc: { line: 10, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "SpatialHash.setCellSize", count: 0, regions: { 0: { startLoc: { line: 12, col: 3 }, endLoc: { line: 14, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 13, col: 5 }, endLoc: { line: 14, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "SpatialHash.cellKey", count: 0, regions: { 0: { startLoc: { line: 16, col: 3 }, endLoc: { line: 18, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 17, col: 5 }, endLoc: { line: 18, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "SpatialHash.rebuild", count: 0, regions: { 0: { startLoc: { line: 20, col: 3 }, endLoc: { line: 35, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 23, col: 5 }, endLoc: { line: 34, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 29, col: 20 }, endLoc: { line: 32, col: 8 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 33, col: 7 }, endLoc: { line: 34, col: 6 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 29, col: 11 }, endLoc: { line: 29, col: 18 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 3 }, 4: { name: "SpatialHash.queryNearby", count: 0, regions: { 0: { startLoc: { line: 37, col: 3 }, endLoc: { line: 53, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 41, col: 5 }, endLoc: { line: 51, col: 6 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 42, col: 7 }, endLoc: { line: 50, col: 8 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 45, col: 21 }, endLoc: { line: 49, col: 10 }, count: 0, ignored: 0 }, 4: { startLoc: { line: 46, col: 11 }, endLoc: { line: 48, col: 12 }, count: 0, ignored: 0 }, 5: { startLoc: { line: 52, col: 5 }, endLoc: { line: 53, col: 4 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 45, col: 13 }, endLoc: { line: 45, col: 19 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 4 } }, exeLine: { 0: 1, 1: 3, 2: 4, 3: 5, 4: 6, 5: 8, 6: 9, 7: 12, 8: 13, 9: 16, 10: 17, 11: 20, 12: 21, 13: 22, 14: 23, 15: 24, 16: 25, 17: 26, 18: 27, 19: 28, 20: 29, 21: 30, 22: 31, 23: 33, 24: 37, 25: 38, 26: 39, 27: 40, 28: 41, 29: 42, 30: 43, 31: 44, 32: 45, 33: 46, 34: 47, 35: 52 } });
export class SpatialHash {
    private cellSize: number;
    private buckets: Map<number, number[]> = new Map();
    private particles: Particle[] = [];
    constructor(cellSize: number) {
        bjccovmuyz06vq.instrumentFunction(0);
        bjccovmuyz06vq.instrumentRegion(0, 1);
        this.cellSize = cellSize;
    }
    setCellSize(size: number): void {
        bjccovmuyz06vq.instrumentFunction(1);
        bjccovmuyz06vq.instrumentRegion(1, 1);
        this.cellSize = size;
    }
    private cellKey(cx: number, cy: number): number {
        bjccovmuyz06vq.instrumentFunction(2);
        bjccovmuyz06vq.instrumentRegion(2, 1);
        return (cx + 100000) * 200001 + (cy + 100000);
    }
    rebuild(particles: Particle[]): void {
        bjccovmuyz06vq.instrumentFunction(3);
        this.particles = particles;
        this.buckets.clear();
        for (let i = 0; i < particles.length; i++) {
            bjccovmuyz06vq.instrumentRegion(3, 1);
            const p = particles[i];
            const cx = Math.floor(p.x / this.cellSize);
            const cy = Math.floor(p.y / this.cellSize);
            const key = this.cellKey(cx, cy);
            let bucket = this.buckets.get(key);
            if (!bucket) {
                bjccovmuyz06vq.instrumentBranch(3, 0, true);
                bjccovmuyz06vq.instrumentRegion(3, 2);
                bucket = [];
                this.buckets.set(key, bucket);
            }
            else {
                bjccovmuyz06vq.instrumentBranch(3, 0, false);
            }
            bjccovmuyz06vq.instrumentRegion(3, 3);
            bucket.push(i);
        }
    }
    queryNearby(p: Particle): number[] {
        bjccovmuyz06vq.instrumentFunction(4);
        const result: number[] = [];
        const cx = Math.floor(p.x / this.cellSize);
        const cy = Math.floor(p.y / this.cellSize);
        for (let dx = -1; dx <= 1; dx++) {
            bjccovmuyz06vq.instrumentRegion(4, 1);
            for (let dy = -1; dy <= 1; dy++) {
                bjccovmuyz06vq.instrumentRegion(4, 2);
                const key = this.cellKey(cx + dx, cy + dy);
                const bucket = this.buckets.get(key);
                if (bucket) {
                    bjccovmuyz06vq.instrumentBranch(4, 0, true);
                    bjccovmuyz06vq.instrumentRegion(4, 3);
                    for (let i = 0; i < bucket.length; i++) {
                        bjccovmuyz06vq.instrumentRegion(4, 4);
                        result.push(bucket[i]);
                    }
                }
                else {
                    bjccovmuyz06vq.instrumentBranch(4, 0, false);
                }
            }
        }
        bjccovmuyz06vq.instrumentRegion(4, 5);
        return result;
    }
}
