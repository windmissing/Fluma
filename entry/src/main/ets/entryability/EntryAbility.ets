import UIAbility from '@ohos.app.ability.UIAbility';
import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import Want from '@ohos.app.ability.Want';
import window from '@ohos.window';
import hilog from '@ohos.hilog';
import { LiquidCAWantCommand, parseLiquidCAWant } from '../physics/LiquidCAWant';
import { LiquidCAEventPayload, LiquidInputSource } from '../physics/LiquidCATypes';
import { LiquidCATelemetry } from '../physics/LiquidCATelemetry';
import { nextP011TimeMs } from '../physics/WaterSandField';
import { P011TiltWantCommand, parseP011TiltWant } from '../physics/P011TiltWant';
import { parseP011ScheduleWant } from '../physics/P011SchedulePlayer';

export default class EntryAbility extends UIAbility {
  hilogTag: string = 'EntryAbility';
  hilogDomain: number = 0x0000;
  private liquidTelemetry: LiquidCATelemetry = new LiquidCATelemetry();
  private wantCommandSequence: number = 0;
  private p011TiltWantSequence: number = 0;
  private p011ScheduleWantSequence: number = 0;

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    hilog.info(this.hilogDomain, this.hilogTag, 'onCreate');
    this.applyWant(want.parameters, false);
    const params = want.parameters;
    if (params && params.wspreset !== undefined) {
      AppStorage.setOrCreate('wspreset', `${params.wspreset}`);
      hilog.info(this.hilogDomain, this.hilogTag, 'want param wspreset=%{public}s', `${params.wspreset}`);
    }
    if (params && params.wscols !== undefined) {
      const cols = parseInt(`${params.wscols}`);
      if (!Number.isNaN(cols) && cols >= 2) {
        AppStorage.setOrCreate('wscols', cols);
      }
    }
    if (params && params.wscapacity !== undefined) {
      const capacity = parseInt(`${params.wscapacity}`);
      if (!Number.isNaN(capacity) && capacity >= 2) {
        AppStorage.setOrCreate('wscapacity', capacity);
      }
    }
    if (params && params.wsmixband !== undefined) {
      const mixBand = parseFloat(`${params.wsmixband}`);
      if (!Number.isNaN(mixBand) && mixBand >= 1 && mixBand <= 2) {
        AppStorage.setOrCreate('wsmixband', mixBand);
      }
    }
    if (params && params.wssnapshot !== undefined) {
      AppStorage.setOrCreate('wssnapshot', `${params.wssnapshot}` === 'true');
    }
    if (params && params.wssandtopmax !== undefined) {
      const sandTopMax = parseFloat(`${params.wssandtopmax}`);
      if (Number.isFinite(sandTopMax) && sandTopMax > 0 && sandTopMax <= 30) {
        AppStorage.setOrCreate('wssandtopmax', sandTopMax);
        hilog.info(this.hilogDomain, this.hilogTag, 'want param wssandtopmax=%{public}f', sandTopMax);
      }
    }
    // P-012 E-batch：外观 want 参数（p012tier 0..3；p012seed >=0）
    if (params && params.p012tier !== undefined) {
      const tier = parseInt(`${params.p012tier}`);
      if (Number.isInteger(tier) && tier >= 0 && tier <= 3) {
        AppStorage.setOrCreate('p012tier', tier);
        hilog.info(this.hilogDomain, this.hilogTag, 'want param p012tier=%{public}d', tier);
      }
    }
    if (params && params.p012seed !== undefined) {
      const p012Seed = parseInt(`${params.p012seed}`);
      if (Number.isInteger(p012Seed) && p012Seed >= 0) {
        AppStorage.setOrCreate('p012seed', p012Seed);
      }
    }
    // P-012 §10.4 消融 want 参数（外观诊断域，不进物理 checker/遥测）：
    //   p012ablate：off=全部启用（默认）/ grain=跳过颗粒层 / fog=跳过雾层 /
    //               spark=跳过亮粒
    //   p012fillbox：tight=精确平铺边界（默认）/ full=旧整盒（v1 对照）
    // 非法值忽略用默认（与 p012tier 同容错风格）。
    if (params && params.p012ablate !== undefined) {
      const ablate = `${params.p012ablate}`;
      if (ablate === 'off' || ablate === 'grain' || ablate === 'fog' || ablate === 'spark') {
        AppStorage.setOrCreate('p012ablate', ablate);
        hilog.info(this.hilogDomain, this.hilogTag, 'want param p012ablate=%{public}s', ablate);
      }
    }
    if (params && params.p012fillbox !== undefined) {
      const fillbox = `${params.p012fillbox}`;
      if (fillbox === 'full' || fillbox === 'tight') {
        AppStorage.setOrCreate('p012fillbox', fillbox);
        hilog.info(this.hilogDomain, this.hilogTag, 'want param p012fillbox=%{public}s', fillbox);
      }
    }
  }

  onNewWant(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    hilog.info(this.hilogDomain, this.hilogTag, 'onNewWant');
    this.applyWant(want.parameters, true);
  }

  private applyWant(parameters: Record<string, Object> | undefined, isNewWant: boolean): void {
    const parsed = parseLiquidCAWant(parameters, isNewWant);
    // P-011 dual-input-v1: identity travels with mode; publish mode before this Want's tilt.
    const tiltCommand = this.parseP011Want(parameters, parsed.renderMode, parsed.testMode, parsed.forceInvert);
    const tiltSequence = tiltCommand === null ? 0 : tiltCommand.sequence;
    // v3 schedule player entry (§26.4): p011schedule=peakADeg:rate. Parsed
    // here, consumed by SandCanvas which owns the player instance. Warnings
    // go to hilog only; an illegal value never arms a player.
    const scheduleParsed = parseP011ScheduleWant(parameters);
    for (const scheduleWarning of scheduleParsed.warnings) {
      hilog.warn(this.hilogDomain, this.hilogTag,
        'P011_SCHEDULE_WARNING %{public}s', scheduleWarning);
    }
    if (scheduleParsed.peakADeg !== null && scheduleParsed.rateDegPerSec !== null) {
      AppStorage.setOrCreate('p011SchedulePeakADeg', scheduleParsed.peakADeg);
      AppStorage.setOrCreate('p011ScheduleRate', scheduleParsed.rateDegPerSec);
      this.p011ScheduleWantSequence++;
      AppStorage.setOrCreate('p011ScheduleWantSeq', this.p011ScheduleWantSequence);
      hilog.info(this.hilogDomain, this.hilogTag,
        'P011_SCHEDULE want peak=%{public}f rate=%{public}f',
        scheduleParsed.peakADeg, scheduleParsed.rateDegPerSec);
    }
    AppStorage.setOrCreate('rendermode', parsed.renderMode);
    AppStorage.setOrCreate('testmode', parsed.testMode);
    AppStorage.setOrCreate('forceinvert', false);
    AppStorage.setOrCreate('liquidSeed', parsed.seed);
    AppStorage.setOrCreate('liquidCellPx', parsed.cellPx);
    AppStorage.setOrCreate('liquidStartShake', parsed.shake);
    AppStorage.setOrCreate('liquidCaseId', parsed.caseId);
    AppStorage.setOrCreate('p011TiltWantModeSequence', tiltSequence);
    this.liquidTelemetry.setTestMode(parsed.testMode);
    for (const warning of parsed.warnings) {
      const event: LiquidCAEventPayload = {
        type: 'model_error', sequence: 0, eventTimeMs: Date.now(), caseId: parsed.caseId, boundary: true,
        source: LiquidInputSource.WANT, forceInvert: false, shakeIntensity: parsed.shake,
        errorCode: warning, cols: 0, rows: 0, cellPx: parsed.cellPx,
        initialHeavy: 0, initialLight: 0, seed: parsed.seed, scaled: false
      };
      this.liquidTelemetry.onEvent(event);
    }
    hilog.info(this.hilogDomain, this.hilogTag,
      'LiquidCA want rendermode=%{public}d testmode=%{public}d seed=%{public}d shake=%{public}f',
      parsed.renderMode, parsed.testMode ? 1 : 0, parsed.seed, parsed.shake);
    if (!isNewWant) {
      this.publishP011TiltWant(tiltCommand);
      return;
    }
    this.wantCommandSequence++;
    const command: LiquidCAWantCommand = {
      p011TiltWantSequence: tiltSequence,
      renderMode: parsed.renderMode,
      testMode: parsed.testMode,
      forceInvert: parsed.forceInvert,
      shake: parsed.shake,
      seed: parsed.seed,
      cellPx: parsed.cellPx,
      caseId: parsed.caseId,
      warnings: parsed.warnings
    };
    AppStorage.setOrCreate('liquidWantCommand', JSON.stringify(command));
    AppStorage.setOrCreate('liquidWantCommandSeq', this.wantCommandSequence);
    this.publishP011TiltWant(tiltCommand);
  }

  private parseP011Want(parameters: Record<string, Object> | undefined,
    renderMode: number, testMode: boolean, forceInvert: boolean): P011TiltWantCommand | null {
    if (parameters === undefined || parameters.wstilt === undefined) {
      return null;
    }
    this.p011TiltWantSequence++;
    const parsed = parseP011TiltWant(parameters, renderMode, testMode, forceInvert,
      this.p011TiltWantSequence, nextP011TimeMs());
    for (const warning of parsed.warnings) {
      hilog.warn(this.hilogDomain, this.hilogTag, 'P011_WSTILT_WARNING %{public}s', warning);
    }
    return parsed.command;
  }

  private publishP011TiltWant(command: P011TiltWantCommand | null): void {
    if (command === null) {
      return;
    }
    AppStorage.setOrCreate('p011TiltWantCommand', JSON.stringify(command));
    AppStorage.setOrCreate('p011TiltWantCommandSeq', command.sequence);
    hilog.info(this.hilogDomain, this.hilogTag,
      'P011_WSTILT seq=%{public}d target=%{public}f', command.sequence, command.targetTilt);
  }

  onDestroy(): void {
    hilog.info(this.hilogDomain, this.hilogTag, 'onDestroy');
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    hilog.info(this.hilogDomain, this.hilogTag, 'onWindowStageCreate');
    windowStage.loadContent('pages/SandCanvas', (err) => {
      if (err.code) {
        hilog.error(this.hilogDomain, this.hilogTag, 'Failed to load the content. Cause: %{public}s', err.message);
        return;
      }
      hilog.info(this.hilogDomain, this.hilogTag, 'Succeeded in loading the content.');
    });
  }

  onWindowStageDestroy(): void {
    hilog.info(this.hilogDomain, this.hilogTag, 'onWindowStageDestroy');
  }

  onForeground(): void {
    hilog.info(this.hilogDomain, this.hilogTag, 'onForeground');
  }

  onBackground(): void {
    hilog.info(this.hilogDomain, this.hilogTag, 'onBackground');
  }
}
