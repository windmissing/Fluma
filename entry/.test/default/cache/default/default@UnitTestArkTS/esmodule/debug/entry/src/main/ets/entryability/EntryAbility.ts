import UIAbility from "@ohos:app.ability.UIAbility";
import type AbilityConstant from "@ohos:app.ability.AbilityConstant";
import type Want from "@ohos:app.ability.Want";
import type window from "@ohos:window";
import hilog from "@ohos:hilog";
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
let bjccovmuyz06ur = new BjcCov({ version: "bjc v1.0.0", versionCode: 10000, path: "entry/src/main/ets/entryability/EntryAbility.ets", hash: "4adc304682c72e18b7fcdcdc0361b2811464b8784e1514ccccb19b113c961f35", lineCnt: 41, count: 0, projectPath: "", projectPathHash: "ceba5a5fd1f37ddf984966c405a592e2d3985b2f0267a1b8661240241877a6b9", functions: { 0: { name: "EntryAbility.onCreate", count: 0, regions: { 0: { startLoc: { line: 11, col: 3 }, endLoc: { line: 13, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 12, col: 5 }, endLoc: { line: 13, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 0 }, 1: { name: "EntryAbility.onDestroy", count: 0, regions: { 0: { startLoc: { line: 15, col: 3 }, endLoc: { line: 17, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 16, col: 5 }, endLoc: { line: 17, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 1 }, 2: { name: "EntryAbility.onWindowStageCreate", count: 0, regions: { 0: { startLoc: { line: 19, col: 3 }, endLoc: { line: 28, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 20, col: 5 }, endLoc: { line: 28, col: 4 }, count: 0, ignored: 0 }, 2: { startLoc: { line: 26, col: 7 }, endLoc: { line: 27, col: 6 }, count: 0, ignored: 0 }, 3: { startLoc: { line: 24, col: 9 }, endLoc: { line: 25, col: 8 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 2 }, 3: { name: "anonymous_0", count: 0, regions: { 0: { startLoc: { line: 21, col: 49 }, endLoc: { line: 27, col: 6 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 22, col: 21 }, endLoc: { line: 25, col: 8 }, count: 0, ignored: 0 } }, branches: { 0: { startLoc: { line: 22, col: 11 }, endLoc: { line: 22, col: 19 }, trueCount: 0, falseCount: 0, group: {}, ignored: 0 } }, ignored: 0, index: 3 }, 4: { name: "EntryAbility.onWindowStageDestroy", count: 0, regions: { 0: { startLoc: { line: 30, col: 3 }, endLoc: { line: 32, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 31, col: 5 }, endLoc: { line: 32, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 4 }, 5: { name: "EntryAbility.onForeground", count: 0, regions: { 0: { startLoc: { line: 34, col: 3 }, endLoc: { line: 36, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 35, col: 5 }, endLoc: { line: 36, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 5 }, 6: { name: "EntryAbility.onBackground", count: 0, regions: { 0: { startLoc: { line: 38, col: 3 }, endLoc: { line: 40, col: 4 }, count: 0, ignored: 0 }, 1: { startLoc: { line: 39, col: 5 }, endLoc: { line: 40, col: 4 }, count: 0, ignored: 0 } }, branches: {}, ignored: 0, index: 6 } }, exeLine: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 7, 6: 8, 7: 9, 8: 11, 9: 12, 10: 15, 11: 16, 12: 19, 13: 20, 14: 21, 15: 22, 16: 23, 17: 24, 18: 26, 19: 30, 20: 31, 21: 34, 22: 35, 23: 38, 24: 39 } });
export default class EntryAbility extends UIAbility {
    hilogTag: string = 'EntryAbility';
    hilogDomain: number = 0x0000;
    onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
        bjccovmuyz06ur.instrumentFunction(0);
        bjccovmuyz06ur.instrumentRegion(0, 1);
        hilog.info(this.hilogDomain, this.hilogTag, 'onCreate');
    }
    onDestroy(): void {
        bjccovmuyz06ur.instrumentFunction(1);
        bjccovmuyz06ur.instrumentRegion(1, 1);
        hilog.info(this.hilogDomain, this.hilogTag, 'onDestroy');
    }
    onWindowStageCreate(windowStage: window.WindowStage): void {
        bjccovmuyz06ur.instrumentFunction(2);
        bjccovmuyz06ur.instrumentRegion(2, 1);
        hilog.info(this.hilogDomain, this.hilogTag, 'onWindowStageCreate');
        windowStage.loadContent('pages/SandCanvas', (err) => {
            bjccovmuyz06ur.instrumentFunction(3);
            if (err.code) {
                bjccovmuyz06ur.instrumentBranch(3, 0, true);
                bjccovmuyz06ur.instrumentRegion(3, 1);
                hilog.error(this.hilogDomain, this.hilogTag, 'Failed to load the content. Cause: %{public}s', err.message);
                bjccovmuyz06ur.instrumentRegion(2, 3);
                return;
            }
            else {
                bjccovmuyz06ur.instrumentBranch(3, 0, false);
            }
            bjccovmuyz06ur.instrumentRegion(2, 2);
            hilog.info(this.hilogDomain, this.hilogTag, 'Succeeded in loading the content.');
        });
    }
    onWindowStageDestroy(): void {
        bjccovmuyz06ur.instrumentFunction(4);
        bjccovmuyz06ur.instrumentRegion(4, 1);
        hilog.info(this.hilogDomain, this.hilogTag, 'onWindowStageDestroy');
    }
    onForeground(): void {
        bjccovmuyz06ur.instrumentFunction(5);
        bjccovmuyz06ur.instrumentRegion(5, 1);
        hilog.info(this.hilogDomain, this.hilogTag, 'onForeground');
    }
    onBackground(): void {
        bjccovmuyz06ur.instrumentFunction(6);
        bjccovmuyz06ur.instrumentRegion(6, 1);
        hilog.info(this.hilogDomain, this.hilogTag, 'onBackground');
    }
}
