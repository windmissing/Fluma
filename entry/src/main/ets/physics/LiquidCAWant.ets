export interface LiquidCAWantParams {
  renderMode: number;
  testMode: boolean;
  forceInvert: boolean;
  shake: number;
  seed: number;
  cellPx: number;
  caseId: string;
  warnings: string[];
}

export interface LiquidCAWantCommand {
  shakeOnly?: boolean;
  p011TiltWantSequence?: number;
  renderMode: number;
  testMode: boolean;
  forceInvert: boolean;
  shake: number;
  seed: number;
  cellPx: number;
  caseId: string;
  warnings: string[];
}

export function parseLiquidCAWant(parameters: Record<string, Object> | undefined,
  allowForceInvert: boolean = false): LiquidCAWantParams {
  const result: LiquidCAWantParams = {
    renderMode: 1,
    testMode: false,
    forceInvert: false,
    shake: 0,
    seed: 0,
    cellPx: 4,
    caseId: '',
    warnings: []
  };
  if (parameters === undefined) {
    return result;
  }
  const renderMode = parseInt(`${parameters.rendermode}`);
  if (Number.isInteger(renderMode) && renderMode >= 0 && renderMode <= 4) {
    result.renderMode = renderMode;
  } else if (parameters.rendermode !== undefined) {
    result.warnings.push('INVALID_RENDERMODE');
  }
  result.testMode = parameters.testmode !== undefined && `${parameters.testmode}`.toLowerCase() === 'true';
  const forceInvert = `${parameters.forceinvert}`.toLowerCase() === 'true';
  result.forceInvert = allowForceInvert && forceInvert;
  if (forceInvert && !allowForceInvert) {
    result.warnings.push('INIT_FORCE_INVERT_INVALID');
  }
  const shake = parseFloat(`${parameters.shake}`);
  if (Number.isFinite(shake) && shake >= 0 && shake <= 1) {
    result.shake = shake;
  } else if (parameters.shake !== undefined) {
    result.warnings.push('INVALID_SHAKE');
  }
  const seed = parseInt(`${parameters.seed}`);
  if (Number.isInteger(seed) && seed >= 0) {
    result.seed = seed;
  } else if (parameters.seed !== undefined) {
    result.warnings.push('INVALID_SEED');
  }
  const cellPx = parseInt(`${parameters.cellpx}`);
  if (Number.isInteger(cellPx) && cellPx > 0) {
    result.cellPx = cellPx;
  } else if (parameters.cellpx !== undefined) {
    result.warnings.push('INVALID_CELL_PX');
  }
  const caseId = `${parameters.caseid}`;
  if (caseId === 'TC-11' || caseId === 'TC-12' || caseId === 'TC-13') {
    result.caseId = caseId;
  } else if (parameters.caseid !== undefined) {
    result.warnings.push('INVALID_CASE_ID');
  }
  return result;
}
