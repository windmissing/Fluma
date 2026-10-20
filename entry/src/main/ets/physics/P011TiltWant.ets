import {
  P011NaturalObservationRunRole,
  P011_SCHEDULE_NINE_PHASE_30_V2,
  P011_SCHEDULE_SEVEN_PHASE_45_V1
} from './P011NaturalObservation';

export type P011TiltWantWarning =
  | 'INVALID_WSTILT'
  | 'P011_RENDER_MODE_REQUIRED'
  | 'P011_TEST_MODE_REQUIRED'
  | 'P011_FORCE_INVERT_CONFLICT'
  | 'INVALID_P011_RUN_ROLE'
  | 'MISSING_P011_RUN_ROLE'
  | 'MISSING_THRESHOLD_CALIBRATION_ID'
  | 'MISSING_THETA_START_DEG'
  | 'MISSING_THETA_STOP_DEG'
  | 'INVALID_THETA_START_DEG'
  | 'INVALID_THETA_STOP_DEG'
  | 'INVALID_SCHEDULE_ID'
  | 'MISSING_SCHEDULE_ID';

export interface P011TiltWantCommand {
  sequence: number;
  targetTilt: number;
  commandTime: number;
  observationRunRole: P011NaturalObservationRunRole | null;
  thresholdCalibrationId: string | null;
  thetaStartDeg: number | null;
  thetaStopDeg: number | null;
  // Observation-3 v2: command schedule identifier for the phase-name
  // mapping (nine-phase 30° v2 or legacy seven-phase 45° v1). Null keeps
  // the legacy mapping. Optional so existing v1 runner payloads parse
  // unchanged.
  scheduleId: string | null;
}

export interface P011TiltWantParseResult {
  command: P011TiltWantCommand | null;
  warnings: P011TiltWantWarning[];
}

function optionalFiniteNumber(parameters: Record<string, Object> | undefined,
  key: string): number | null {
  if (parameters === undefined || parameters[key] === undefined) {
    return null;
  }
  const text = `${parameters[key]}`.trim();
  if (!/^[+-]?(?:\d+(?:\.\d*)?|\.\d+)$/.test(text)) {
    return null;
  }
  const value = parseFloat(text);
  return Number.isFinite(value) ? value : null;
}

// P-011 bottom-creep-v2.8-heavy-interface: single owner for test-only wstilt Want parsing.
export function parseP011TiltWant(parameters: Record<string, Object> | undefined,
  renderMode: number, testMode: boolean, forceInvert: boolean,
  sequence: number, commandTime: number): P011TiltWantParseResult {
  const warnings: P011TiltWantWarning[] = [];
  if (renderMode !== 4) {
    warnings.push('P011_RENDER_MODE_REQUIRED');
  }
  if (!testMode) {
    warnings.push('P011_TEST_MODE_REQUIRED');
  }
  const rawForceInvert = parameters !== undefined &&
    `${parameters.forceinvert}`.toLowerCase() === 'true';
  if (forceInvert || rawForceInvert) {
    warnings.push('P011_FORCE_INVERT_CONFLICT');
  }
  const angleText = parameters === undefined ? '' : `${parameters.wstilt}`.trim();
  const targetTilt = parseFloat(angleText);
  if (!/^[+-]?(?:\d+(?:\.\d*)?|\.\d+)$/.test(angleText) ||
    !Number.isFinite(targetTilt) || targetTilt < -89 || targetTilt > 89) {
    warnings.push('INVALID_WSTILT');
  }
  const runRoleText = parameters === undefined ? '' : `${parameters.p011runrole}`.trim();
  let observationRunRole: P011NaturalObservationRunRole | null = null;
  if (runRoleText === 'calibration' || runRoleText === 'holdout') {
    observationRunRole = runRoleText;
  } else if (runRoleText === '') {
    warnings.push('MISSING_P011_RUN_ROLE');
  } else {
    warnings.push('INVALID_P011_RUN_ROLE');
  }
  const thresholdCalibrationId = parameters === undefined ||
    parameters.thresholdcalibrationid === undefined
    ? null : `${parameters.thresholdcalibrationid}`.trim();
  if (thresholdCalibrationId === null || thresholdCalibrationId.length === 0) {
    warnings.push('MISSING_THRESHOLD_CALIBRATION_ID');
  }
  const thetaStartProvided = parameters !== undefined &&
    parameters.thetastartdeg !== undefined;
  const thetaStartDeg = optionalFiniteNumber(parameters, 'thetastartdeg');
  if (thetaStartProvided && thetaStartDeg === null) {
    warnings.push('INVALID_THETA_START_DEG');
  } else if (!thetaStartProvided) {
    warnings.push('MISSING_THETA_START_DEG');
  }
  const thetaStopProvided = parameters !== undefined &&
    parameters.thetastopdeg !== undefined;
  const thetaStopDeg = optionalFiniteNumber(parameters, 'thetastopdeg');
  if (thetaStopProvided && thetaStopDeg === null) {
    warnings.push('INVALID_THETA_STOP_DEG');
  } else if (!thetaStopProvided) {
    warnings.push('MISSING_THETA_STOP_DEG');
  }
  const scheduleIdText = parameters === undefined ||
    parameters.scheduleid === undefined
    ? '' : `${parameters.scheduleid}`.trim();
  let scheduleId: string | null = null;
  if (scheduleIdText === P011_SCHEDULE_NINE_PHASE_30_V2 ||
    scheduleIdText === P011_SCHEDULE_SEVEN_PHASE_45_V1) {
    scheduleId = scheduleIdText;
  } else if (scheduleIdText !== '') {
    warnings.push('INVALID_SCHEDULE_ID');
  }
  if (warnings.some((warning) => warning !== 'MISSING_P011_RUN_ROLE' &&
    warning !== 'MISSING_THRESHOLD_CALIBRATION_ID' &&
    warning !== 'MISSING_THETA_START_DEG' &&
    warning !== 'MISSING_THETA_STOP_DEG' &&
    warning !== 'MISSING_SCHEDULE_ID')) {
    return { command: null, warnings };
  }
  return {
    command: {
      sequence,
      targetTilt,
      commandTime,
      observationRunRole,
      thresholdCalibrationId,
      thetaStartDeg: optionalFiniteNumber(parameters, 'thetastartdeg'),
      thetaStopDeg: optionalFiniteNumber(parameters, 'thetastopdeg'),
      scheduleId
    },
    warnings
  };
}

export function decodeP011TiltWant(encoded: string | undefined,
  sequence: number, consumedSequence: number): P011TiltWantCommand | null {
  if (encoded === undefined || !Number.isInteger(sequence) || sequence <= consumedSequence) {
    return null;
  }
  try {
    const command = JSON.parse(encoded) as P011TiltWantCommand;
    if (command.observationRunRole === undefined) {
      command.observationRunRole = null;
    }
    if (command.thresholdCalibrationId === undefined) {
      command.thresholdCalibrationId = null;
    }
    if (command.thetaStartDeg === undefined) {
      command.thetaStartDeg = null;
    }
    if (command.thetaStopDeg === undefined) {
      command.thetaStopDeg = null;
    }
    if (command.scheduleId === undefined) {
      command.scheduleId = null;
    } else if (command.scheduleId !== P011_SCHEDULE_NINE_PHASE_30_V2 &&
      command.scheduleId !== P011_SCHEDULE_SEVEN_PHASE_45_V1) {
      return null;
    }
    if (!Number.isInteger(command.sequence) || command.sequence !== sequence ||
      !Number.isFinite(command.targetTilt) || command.targetTilt < -89 ||
      command.targetTilt > 89 || !Number.isFinite(command.commandTime) ||
      (command.observationRunRole !== null &&
        command.observationRunRole !== 'calibration' &&
        command.observationRunRole !== 'holdout')) {
      return null;
    }
    return command;
  } catch (error) {
    return null;
  }
}
