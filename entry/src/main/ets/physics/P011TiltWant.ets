import {
  P011NaturalObservationRunRole,
  P011_CORRECTION_ROUTE_ID,
  P011_SCHEDULE_NINE_PHASE_30_V2,
  P011_SCHEDULE_NINE_PHASE_30_V3,
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
  | 'MISSING_SCHEDULE_ID'
  // Observation-3 v3 additions (GAP-2/5).
  | 'INVALID_V3_LEG_ID'
  | 'INVALID_V3_PHASE_KIND'
  | 'INVALID_EXEC_SOURCE_COMMIT'
  | 'INVALID_EXEC_SOURCE_FILE_SHA256'
  | 'INVALID_EXEC_BUILD_SHA256'
  // correction route wiring (D-1 follow-up): the p011route want parameter
  // only accepts the explicitly frozen correction route id.
  | 'INVALID_P011_ROUTE';

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
  // Observation-3 v3 (GAP-2): schedule segment binding of THIS command —
  // legId (L1..L8, null on platform rows), segmentId (H0..H8 / L1..L8) and
  // phaseKind. Optional so v2-era payloads parse unchanged.
  v3LegId?: string | null;
  v3SegmentId?: string | null;
  v3PhaseKind?: 'platform' | 'ramp' | null;
  // Observation-3 v3 (GAP-5): execution identity for executedThresholds.
  // Optional; absent keeps whatever the field instance already holds.
  execSourceCommit?: string | null;
  execSourceFileSha256?: string | null;
  execBuildSha256?: string | null;
  // correction route wiring (D-1 follow-up): the explicitly bound physics
  // apply route. Only 'relax-correction-weighted-v1' (P011_CORRECTION_ROUTE
  // _ID) is accepted; null/absent keeps the default legacy apply. Wiring
  // only — binding the route does NOT by itself activate the correction
  // allocator (the dual-threshold state machine must also be armed).
  correctionRouteId?: string | null;
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
    scheduleIdText === P011_SCHEDULE_NINE_PHASE_30_V3 ||
    scheduleIdText === P011_SCHEDULE_SEVEN_PHASE_45_V1) {
    scheduleId = scheduleIdText;
  } else if (scheduleIdText !== '') {
    warnings.push('INVALID_SCHEDULE_ID');
  }
  // Observation-3 v3 segment binding (optional; validated only when the
  // v3 schedule is selected).
  const v3LegText = parameters === undefined || parameters.v3legid === undefined
    ? null : `${parameters.v3legid}`.trim();
  const v3SegmentText = parameters === undefined || parameters.v3segmentid === undefined
    ? null : `${parameters.v3segmentid}`.trim();
  const v3PhaseKindText = parameters === undefined ||
    parameters.v3phasekind === undefined
    ? null : `${parameters.v3phasekind}`.trim();
  let v3LegId: string | null = null;
  let v3SegmentId: string | null = null;
  let v3PhaseKind: 'platform' | 'ramp' | null = null;
  if (scheduleId === P011_SCHEDULE_NINE_PHASE_30_V3) {
    if (v3SegmentText !== null) {
      if (!/^[HL][0-9]$/.test(v3SegmentText)) {
        warnings.push('INVALID_V3_LEG_ID');
      } else {
        v3SegmentId = v3SegmentText;
      }
    }
    if (v3LegText !== null && v3LegText !== '') {
      if (!/^L[1-8]$/.test(v3LegText)) {
        warnings.push('INVALID_V3_LEG_ID');
      } else {
        v3LegId = v3LegText;
      }
    }
    if (v3PhaseKindText !== null) {
      if (v3PhaseKindText !== 'platform' && v3PhaseKindText !== 'ramp') {
        warnings.push('INVALID_V3_PHASE_KIND');
      } else {
        v3PhaseKind = v3PhaseKindText;
      }
    }
  }
  // Execution identity (optional, GAP-5).
  const execText = (key: string): string | null => {
    const raw = parameters === undefined || parameters[key] === undefined
      ? null : `${parameters[key]}`.trim();
    return raw === null || raw.length === 0 ? null : raw;
  };
  const execSourceCommit = execText('execsourcecommit');
  const execSourceFileSha256 = execText('execsourcefilesha256');
  const execBuildSha256 = execText('execbuildsha256');
  if (execSourceCommit !== null && !/^[0-9a-fA-F]{6,64}$/.test(execSourceCommit)) {
    warnings.push('INVALID_EXEC_SOURCE_COMMIT');
  }
  if (execSourceFileSha256 !== null && !/^[0-9a-fA-F]{64}$/.test(execSourceFileSha256)) {
    warnings.push('INVALID_EXEC_SOURCE_FILE_SHA256');
  }
  if (execBuildSha256 !== null && !/^[0-9a-fA-F]{64}$/.test(execBuildSha256)) {
    warnings.push('INVALID_EXEC_BUILD_SHA256');
  }
  // correction route wiring (optional; only the frozen correction route id
  // is a legal binding — anything non-empty that is not the correction id
  // is rejected so a typo can never silently run a different route).
  const routeText = parameters === undefined || parameters.p011route === undefined
    ? '' : `${parameters.p011route}`.trim();
  let correctionRouteId: string | null = null;
  if (routeText === P011_CORRECTION_ROUTE_ID) {
    correctionRouteId = routeText;
  } else if (routeText !== '') {
    warnings.push('INVALID_P011_ROUTE');
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
      scheduleId,
      v3LegId,
      v3SegmentId,
      v3PhaseKind,
      execSourceCommit,
      execSourceFileSha256,
      execBuildSha256,
      correctionRouteId
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
      command.scheduleId !== P011_SCHEDULE_NINE_PHASE_30_V3 &&
      command.scheduleId !== P011_SCHEDULE_SEVEN_PHASE_45_V1) {
      return null;
    }
    if (command.v3LegId === undefined) {
      command.v3LegId = null;
    }
    if (command.v3SegmentId === undefined) {
      command.v3SegmentId = null;
    }
    if (command.v3PhaseKind === undefined) {
      command.v3PhaseKind = null;
    } else if (command.v3PhaseKind !== 'platform' && command.v3PhaseKind !== 'ramp') {
      return null;
    }
    if (command.execSourceCommit === undefined) {
      command.execSourceCommit = null;
    }
    if (command.execSourceFileSha256 === undefined) {
      command.execSourceFileSha256 = null;
    }
    if (command.execBuildSha256 === undefined) {
      command.execBuildSha256 = null;
    }
    if (command.correctionRouteId === undefined) {
      command.correctionRouteId = null;
    } else if (command.correctionRouteId !== null &&
      command.correctionRouteId !== P011_CORRECTION_ROUTE_ID) {
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
