export type P011TiltWantWarning =
  | 'INVALID_WSTILT'
  | 'P011_RENDER_MODE_REQUIRED'
  | 'P011_TEST_MODE_REQUIRED'
  | 'P011_FORCE_INVERT_CONFLICT';

export interface P011TiltWantCommand {
  sequence: number;
  targetTilt: number;
  commandTime: number;
}

export interface P011TiltWantParseResult {
  command: P011TiltWantCommand | null;
  warnings: P011TiltWantWarning[];
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
  if (warnings.length > 0) {
    return { command: null, warnings };
  }
  return {
    command: { sequence, targetTilt, commandTime },
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
    if (!Number.isInteger(command.sequence) || command.sequence !== sequence ||
      !Number.isFinite(command.targetTilt) || command.targetTilt < -89 ||
      command.targetTilt > 89 || !Number.isFinite(command.commandTime)) {
      return null;
    }
    return command;
  } catch (error) {
    return null;
  }
}
