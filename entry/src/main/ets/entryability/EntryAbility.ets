import UIAbility from '@ohos.app.ability.UIAbility';
import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import Want from '@ohos.app.ability.Want';
import window from '@ohos.window';
import hilog from '@ohos.hilog';
import preferences from '@ohos.data.preferences';
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
    // §27.6: load the persisted threshold pair. A corrupt/illegal pair is
    // dropped with a warning and the 26/20 session default stays. The
    // promise resolves before onWindowStageCreate loads the page content
    // (loadContent queues behind the ability event loop), so SandCanvas
    // sees the restored pair at initField time.
    preferences.getPreferences(this.context, {
      name: 'p011_threshold_pair'
    }).then((prefs: preferences.Preferences): void => {
      AppStorage.setOrCreate('p011Preferences', prefs);
      const stored = prefs.getSync('p011.thetaPair', '') as string;
      if (stored.length === 0) {
        return;
      }
      const parsed = JSON.parse(stored) as Record<string, Object>;
      const start = parsed.start as number;
      const stop = parsed.stop as number;
      const source = parsed.source as string;
      if (Number.isFinite(start) && Number.isFinite(stop) &&
        0 <= stop && stop < start && start <= 30) {
        AppStorage.setOrCreate('p011PersistedThetaStart', start);
        AppStorage.setOrCreate('p011PersistedThetaStop', stop);
        AppStorage.setOrCreate('p011PersistedThetaSource', source);
        hilog.info(this.hilogDomain, this.hilogTag,
          'P011_THETA_PAIR restored start=%{public}f stop=%{public}f',
          start, stop);
      } else {
        hilog.warn(this.hilogDomain, this.hilogTag,
          'P011_THETA_PAIR dropped illegal persisted pair');
      }
    }).catch((): void => {
      // First launch or storage failure — session default 26/20 stays.
      hilog.warn(this.hilogDomain, this.hilogTag,
        'P011_THETA_PAIR preferences unavailable');
    });
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
    //               spark=跳过亮粒 / strat=跳过 C 层理（§11.4）/
    //               tracer=跳过掉沙喷流层（B-route 锚点测量 OFF 臂）
    //   p012fillbox：tight=精确平铺边界（默认）/ full=旧整盒（v1 对照）
    // 非法值忽略用默认（与 p012tier 同容错风格）。
    if (params && params.p012ablate !== undefined) {
      const ablate = `${params.p012ablate}`;
      if (ablate === 'off' || ablate === 'grain' || ablate === 'fog' ||
        ablate === 'spark' || ablate === 'strat' || ablate === 'e3i2' ||
        ablate === 'e3n1' || ablate === 'dyn' ||
        ablate === 'toplight' || ablate === 'tracer') {
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
    // v3 schedule player entry (§26.4/§26.5): p011schedule=peakADeg:rate with
    // the optional four-parameter run-metadata group (fail-closed: a partial
    // or invalid group rejects the whole schedule want). Parsed here,
    // consumed by SandCanvas which owns the player instance. Warnings go to
    // hilog only; an illegal value never arms a player.
    // preregistration §1: p011cvar for the schedule path (the wstilt
    // path parses it in P011TiltWant). Validated; illegal rejects the
    // schedule want downstream (stored as '' and checked by SandCanvas).
    const rawCvar = parameters === undefined || parameters.p011cvar === undefined
      ? '' : `${parameters.p011cvar}`.trim().toUpperCase();
    const cvarValid = rawCvar === '' || rawCvar === 'A' || rawCvar === 'B' ||
      rawCvar === 'C' || rawCvar === 'D';
    if (!cvarValid) {
      hilog.warn(this.hilogDomain, this.hilogTag,
        'P011_SCHEDULE_WARNING INVALID_P011_CVAR');
    }
    AppStorage.setOrCreate('p011ScheduleCvar', cvarValid ? rawCvar : '');
    // preregistration §1: the correction route binding for schedule runs
    // (B/C/D must explicitly bind the route; A leaves it unbound).
    const rawRoute = parameters === undefined || parameters.p011route === undefined
      ? '' : `${parameters.p011route}`.trim();
    const routeValid = rawRoute === '' || rawRoute === 'relax-correction-weighted-v1';
    if (!routeValid) {
      hilog.warn(this.hilogDomain, this.hilogTag,
        'P011_SCHEDULE_WARNING INVALID_P011_ROUTE');
    }
    AppStorage.setOrCreate('p011ScheduleRoute', routeValid ? rawRoute : '');
    // preregistration §4: conditioning prefix for schedule-driven matrix
    // runs (G1/G2/G0 strata). 'none'/absent = no prefix.
    const rawConditioning = parameters === undefined ||
      parameters.p011conditioning === undefined
      ? 'none' : `${parameters.p011conditioning}`.trim().toUpperCase();
    const conditioningValid = rawConditioning === 'none' ||
      rawConditioning === 'G1' || rawConditioning === 'G2' ||
      rawConditioning === 'G0';
    if (!conditioningValid) {
      hilog.warn(this.hilogDomain, this.hilogTag,
        'P011_SCHEDULE_WARNING INVALID_P011_CONDITIONING');
    }
    AppStorage.setOrCreate('p011ScheduleConditioning',
      conditioningValid ? rawConditioning : 'none');
    const scheduleParsed = parseP011ScheduleWant(parameters);
    for (const scheduleWarning of scheduleParsed.warnings) {
      hilog.warn(this.hilogDomain, this.hilogTag,
        'P011_SCHEDULE_WARNING %{public}s', scheduleWarning);
    }
    if (scheduleParsed.peakADeg !== null && scheduleParsed.rateDegPerSec !== null &&
      scheduleParsed.warnings.length === 0) {
      AppStorage.setOrCreate('p011SchedulePeakADeg', scheduleParsed.peakADeg);
      AppStorage.setOrCreate('p011ScheduleRate', scheduleParsed.rateDegPerSec);
      // §26.5: carry the validated metadata group (null on pure smoke) so
      // SandCanvas can arm BEFORE player.start().
      if (scheduleParsed.metadata !== null) {
        AppStorage.setOrCreate('p011ScheduleRunRole', scheduleParsed.metadata.runRole);
        AppStorage.setOrCreate('p011ScheduleCalibrationId',
          scheduleParsed.metadata.thresholdCalibrationId);
        AppStorage.setOrCreate('p011ScheduleThetaStartDeg',
          scheduleParsed.metadata.thetaStartDeg);
        AppStorage.setOrCreate('p011ScheduleThetaStopDeg',
          scheduleParsed.metadata.thetaStopDeg);
        AppStorage.setOrCreate('p011ScheduleMetadataArmed', true);
      } else {
        AppStorage.setOrCreate('p011ScheduleRunRole', null);
        AppStorage.setOrCreate('p011ScheduleCalibrationId', null);
        AppStorage.setOrCreate('p011ScheduleThetaStartDeg', null);
        AppStorage.setOrCreate('p011ScheduleThetaStopDeg', null);
        AppStorage.setOrCreate('p011ScheduleMetadataArmed', false);
      }
      // §26.6: carry the execution-identity triple (non-null iff metadata
      // is armed — the parse-level XOR binding guarantees it).
      if (scheduleParsed.execIdentity !== null) {
        AppStorage.setOrCreate('p011ScheduleExecSourceCommit',
          scheduleParsed.execIdentity.execSourceCommit);
        AppStorage.setOrCreate('p011ScheduleExecSourceFileSha256',
          scheduleParsed.execIdentity.execSourceFileSha256);
        AppStorage.setOrCreate('p011ScheduleExecBuildSha256',
          scheduleParsed.execIdentity.execBuildSha256);
      } else {
        AppStorage.setOrCreate('p011ScheduleExecSourceCommit', null);
        AppStorage.setOrCreate('p011ScheduleExecSourceFileSha256', null);
        AppStorage.setOrCreate('p011ScheduleExecBuildSha256', null);
      }
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
