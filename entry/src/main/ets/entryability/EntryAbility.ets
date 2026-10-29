import { p011ShakeOnlyWant } from '../physics/P011DeviceRunAssembly';
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
    if(isNewWant && parameters?.p011finish !== undefined) {
      for(const key of Object.keys(parameters)) {
        if(key!=='p011finish'&&key!=='p011finishrun'&&key!=='p011finishgeneration'&&!key.startsWith('ohos.')) {
          hilog.warn(this.hilogDomain,this.hilogTag,'INVALID_CAPTURE_FINISH_ENVELOPE');return;
        }
      }
      const requestId=`${parameters.p011finish}`,runId=`${parameters.p011finishrun ?? ''}`,generation=Number(`${parameters.p011finishgeneration ?? ''}`);
      if(!requestId||!runId||!Number.isSafeInteger(generation)||generation<1){hilog.warn(this.hilogDomain,this.hilogTag,'INVALID_CAPTURE_FINISH');return;}
      const command: LiquidCAWantCommand={finishCaptureRequestId:requestId,finishCaptureRunId:runId,finishCaptureGeneration:generation,
        renderMode:4,testMode:false,forceInvert:false,seed:0,cellPx:4,caseId:'',warnings:[],shake:0};
      AppStorage.setOrCreate('liquidWantCommand',JSON.stringify(command));AppStorage.setOrCreate('liquidWantCommandSeq',++this.wantCommandSequence);return;
    }
    // Partial stop/update must not inherit parse defaults for mode/seed/test identity.
    const shakeOnly = isNewWant ? p011ShakeOnlyWant(parameters) : null;
    if (shakeOnly !== null) {
      if (!Number.isFinite(shakeOnly)) { hilog.warn(this.hilogDomain, this.hilogTag, 'INVALID_SHAKE'); return; }
      this.wantCommandSequence++;
      const command: LiquidCAWantCommand = { shakeOnly: true, shake: shakeOnly,
        renderMode: 4, testMode: false, forceInvert: false, seed: 0, cellPx: 4, caseId: '', warnings: [] };
      AppStorage.setOrCreate('liquidWantCommand', JSON.stringify(command));
      AppStorage.setOrCreate('liquidWantCommandSeq', this.wantCommandSequence);
      return;
    }
    if (!isNewWant) {
      AppStorage.setOrCreate('p011ParticleRequested', false);
      AppStorage.setOrCreate('p011DeviceCapture', false);
    }
    if (parameters !== undefined && parameters.p011particle !== undefined) {
      const requested = `${parameters.p011particle}` === '1';
      if (requested || `${parameters.p011particle}` === '0') AppStorage.setOrCreate('p011ParticleRequested', requested);
      else hilog.warn(this.hilogDomain, this.hilogTag, 'INVALID_P011_PARTICLE');
    }
    if (parameters !== undefined && parameters.p011capture !== undefined)
      AppStorage.setOrCreate('p011DeviceCapture', `${parameters.p011capture}` === '1');
    AppStorage.setOrCreate('p011ShakeExplicit', parameters !== undefined && parameters.shake !== undefined);
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
    // S4 §1.4: deep-creep arming via p011s4 = 'deep:<k_deep>:<mu_y>:<hMin>'
    // (three-segment, independent from p011route; illegal values reject).
    if (parameters && parameters.p011s4 !== undefined) {
      const s4 = `${parameters.p011s4}`.trim();
      const parts = s4.split(':');
      if (parts.length === 4 && parts[0] === 'deep') {
        const kDeep = Number(parts[1]);
        const muY = Number(parts[2]);
        const hMin = Number(parts[3]);
        if (Number.isFinite(kDeep) && Number.isFinite(muY) &&
          Number.isFinite(hMin) && kDeep > 0 && kDeep <= 0.2 &&
          muY >= 0 && muY <= 0.5 && hMin >= 0.5 && hMin <= 2) {
          AppStorage.setOrCreate('p011S4KDeep', kDeep);
          AppStorage.setOrCreate('p011S4MuY', muY);
          AppStorage.setOrCreate('p011S4HMin', hMin);
          hilog.info(this.hilogDomain, this.hilogTag,
            'want param p011s4 k=%{public}f mu=%{public}f hMin=%{public}f',
            kDeep, muY, hMin);
        } else {
          hilog.warn(this.hilogDomain, this.hilogTag,
            'P011_S4_WARNING INVALID_P011_S4_PARAMS');
        }
      } else if (s4 !== '') {
        hilog.warn(this.hilogDomain, this.hilogTag,
          'P011_S4_WARNING INVALID_P011_S4_FORMAT');
      }
    }
    // S3 §M-6: moisture arming via p011s3 = 'moisture:<k>:<q_ref>'
    // (three-segment; k in {8,16}, qRef in (0,1)).
    if (parameters && parameters.p011s3 !== undefined) {
      const s3 = `${parameters.p011s3}`.trim();
      const parts = s3.split(':');
      if (parts.length === 3 && parts[0] === 'moisture') {
        const kTier = Number(parts[1]);
        const qRef = Number(parts[2]);
        if ((kTier === 8 || kTier === 16) && Number.isFinite(qRef) &&
          qRef > 0 && qRef < 1) {
          AppStorage.setOrCreate('p011S3K', kTier);
          AppStorage.setOrCreate('p011S3QRef', qRef);
          hilog.info(this.hilogDomain, this.hilogTag,
            'want param p011s3 k=%{public}d qRef=%{public}f', kTier, qRef);
        } else {
          hilog.warn(this.hilogDomain, this.hilogTag,
            'P011_S3_WARNING INVALID_P011_S3_PARAMS');
        }
      } else if (s3 !== '') {
        hilog.warn(this.hilogDomain, this.hilogTag,
          'P011_S3_WARNING INVALID_P011_S3_FORMAT');
      }
    }
    // S3 §M-5: runtime water ops via p011water = 'add:<amount>' or
    // 'drain:<amount>' (staged once; the page executes on arming).
    if (parameters && parameters.p011water !== undefined) {
      const w = `${parameters.p011water}`.trim();
      const parts = w.split(':');
      if (parts.length === 2 && (parts[0] === 'add' || parts[0] === 'drain')) {
        const amount = Number(parts[1]);
        if (Number.isFinite(amount) && amount > 0) {
          AppStorage.setOrCreate('p011WaterOp', parts[0]);
          AppStorage.setOrCreate('p011WaterAmount', amount);
        } else {
          hilog.warn(this.hilogDomain, this.hilogTag,
            'P011_S3_WARNING INVALID_P011_WATER_AMOUNT');
        }
      } else if (w !== '') {
        hilog.warn(this.hilogDomain, this.hilogTag,
          'P011_S3_WARNING INVALID_P011_WATER_FORMAT');
      }
    }
    // Plan-3 stage a (advection §6.1 A-3): arming via
    // p011adv = 'adv:<gain>:<kDrag>:<uCap>:<cap>' (five segments; gain in
    // [0,1], kDrag/uCap/cap > 0; illegal values reject the whole want).
    if (parameters && parameters.p011adv !== undefined) {
      const adv = `${parameters.p011adv}`.trim();
      const parts = adv.split(':');
      if (parts.length === 5 && parts[0] === 'adv') {
        const gain = Number(parts[1]);
        const kDrag = Number(parts[2]);
        const uCap = Number(parts[3]);
        const cap = Number(parts[4]);
        if (Number.isFinite(gain) && Number.isFinite(kDrag) &&
          Number.isFinite(uCap) && Number.isFinite(cap) &&
          gain >= 0 && gain <= 1 && kDrag > 0 && uCap > 0 && cap > 0) {
          AppStorage.setOrCreate('p011AdvGain', gain);
          AppStorage.setOrCreate('p011AdvKDrag', kDrag);
          AppStorage.setOrCreate('p011AdvUCap', uCap);
          AppStorage.setOrCreate('p011AdvCap', cap);
          hilog.info(this.hilogDomain, this.hilogTag,
            'want param p011adv gain=%{public}f k=%{public}f cap=%{public}f',
            gain, kDrag, cap);
        } else {
          hilog.warn(this.hilogDomain, this.hilogTag,
            'P011_ADV_WARNING INVALID_P011_ADV_PARAMS');
        }
      } else if (adv !== '') {
        hilog.warn(this.hilogDomain, this.hilogTag,
          'P011_ADV_WARNING INVALID_P011_ADV_FORMAT');
      }
    }
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
    if (parameters !== undefined && parameters.emptysanddemo !== undefined) {
      AppStorage.setOrCreate('emptySandDemo', `${parameters.emptysanddemo}` === '1');
    }
    if (parameters !== undefined && parameters.p051v2 !== undefined) {
      AppStorage.setOrCreate('p051v2Demo', `${parameters.p051v2}` === '1');
    }
    if (parameters !== undefined && parameters.cwdemo !== undefined) {
      AppStorage.setOrCreate('cwDemo', `${parameters.cwdemo}` === '1');
    }
    // P-013 P1-G0 平台探针门（PROBE-ONLY：仅供真机取证，勿并入发布分支）
    // p013g0=1 → 加载 pages/P013P1G0Probe；p013g0hit=default|transparent|none|block；
    // p013g0diag=true|false → GLES 诊断 pass 开/关（关时 native 不绘制任何像素）。
    // 必须置于 isNewWant 提前返回之前：onCreate 以 isNewWant=false 调用本方法。
    if (parameters !== undefined && parameters.p013g0 !== undefined) {
      AppStorage.setOrCreate('p013g0', `${parameters.p013g0}` === '1');
      const hitRaw = parameters.p013g0hit === undefined ?
        'default' : `${parameters.p013g0hit}`.trim();
      const hit = hitRaw === 'transparent' || hitRaw === 'none' || hitRaw === 'block' ?
        hitRaw : 'default';
      AppStorage.setOrCreate('p013g0hit', hit);
      AppStorage.setOrCreate('p013g0diag',
        parameters.p013g0diag === undefined ? true : `${parameters.p013g0diag}` === 'true');
      const libRaw = parameters.p013g0lib === undefined ?
        'hello' : `${parameters.p013g0lib}`.trim();
      AppStorage.setOrCreate('p013g0profile', `${parameters.p013g0profile ?? 'texture'}`.trim() === 'surface' ? 'surface' : 'texture');
      AppStorage.setOrCreate('p013g0lib', libRaw === 'renderer' ? 'renderer' : 'hello');
      hilog.info(this.hilogDomain, this.hilogTag,
        'P013_G0 want enabled hit=%{public}s diag=%{public}d lib=%{public}s', hit,
        AppStorage.get<boolean>('p013g0diag') ? 1 : 0,
        AppStorage.get<string>('p013g0lib'));
    }
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
    // P-013 P1-G0 平台探针页（PROBE-ONLY：勿并入发布分支）
    const probePage: string = AppStorage.get<boolean>('cwDemo') === true ? 'pages/ClearWaterDemo' : AppStorage.get<boolean>('p051v2Demo') === true ? 'pages/P051DerivedV2Demo' : AppStorage.get<boolean>('emptySandDemo') === true ? 'pages/EmptySandDemo' : AppStorage.get<boolean>('p013g0') === true ?
      'pages/P013P1G0Probe' : 'pages/SandCanvas';
    windowStage.loadContent(probePage, (err) => {
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
    AppStorage.setOrCreate('emptySandDemoForeground', true);
    hilog.info(this.hilogDomain, this.hilogTag, 'onForeground');
  }

  onBackground(): void {
    AppStorage.setOrCreate('emptySandDemoForeground', false);
    hilog.info(this.hilogDomain, this.hilogTag, 'onBackground');
  }
}
