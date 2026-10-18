import { P011TiltWantCommand } from './P011TiltWant';

// P-011 dual-input-v1, architecture §15: input-only, fixed-size pure state machine.
export type P011ModeRequestSource = 'startup' | 'want' | 'panel' | 'longPress';

export interface P011InputModeState {
  testMode: boolean;
  useSensor: boolean;
  scriptTargetTilt: number | null;
  consumedSequence: number;
  tiltX: number;
  lastTiltX: number;
  forceInvert: boolean;
  wasForceInvert: boolean;
  gestureHoldUntil: number;
  sensorGx: number;
  sensorGy: number;
  hasFreshSensorSample: boolean;
  lastSensorCommandTime: number;
  lastTiltCommandTime: number;
}

export interface P011ModeTransitionResult {
  state: P011InputModeState;
  changed: boolean;
}

function copyInputState(state: P011InputModeState): P011InputModeState {
  return { testMode: state.testMode, useSensor: state.useSensor,
    scriptTargetTilt: state.scriptTargetTilt, consumedSequence: state.consumedSequence,
    tiltX: state.tiltX, lastTiltX: state.lastTiltX, forceInvert: state.forceInvert,
    wasForceInvert: state.wasForceInvert, gestureHoldUntil: state.gestureHoldUntil,
    sensorGx: state.sensorGx, sensorGy: state.sensorGy, hasFreshSensorSample: state.hasFreshSensorSample,
    lastSensorCommandTime: state.lastSensorCommandTime, lastTiltCommandTime: state.lastTiltCommandTime };
}

export function transitionP011InputMode(state: P011InputModeState, nextTestMode: boolean,
  pendingSequence: number, currentWantSequence: number): P011ModeTransitionResult {
  if (state.testMode === nextTestMode) {
    return { state, changed: false };
  }
  const discardThrough = nextTestMode && currentWantSequence > state.consumedSequence
    ? currentWantSequence - 1 : pendingSequence;
  return { changed: true, state: {
    testMode: nextTestMode, useSensor: !nextTestMode, scriptTargetTilt: null,
    consumedSequence: Math.max(state.consumedSequence, discardThrough),
    tiltX: 0, lastTiltX: 0, forceInvert: false, wasForceInvert: false,
    gestureHoldUntil: 0, sensorGx: 0, sensorGy: 0, hasFreshSensorSample: false,
    lastSensorCommandTime: 0, lastTiltCommandTime: 0
  } };
}

export function acceptP011SensorSample(state: P011InputModeState, x: number, y: number,
  sampleTime: number, smoothingAlpha: number): P011InputModeState {
  if (state.testMode || !Number.isFinite(x) || !Number.isFinite(y) ||
    !Number.isFinite(sampleTime) || sampleTime <= 0) {
    return state;
  }
  const next = copyInputState(state);
  next.sensorGx = state.sensorGx * (1 - smoothingAlpha) + x * smoothingAlpha;
  next.sensorGy = state.sensorGy * (1 - smoothingAlpha) + y * smoothingAlpha;
  next.lastSensorCommandTime = sampleTime;
  next.hasFreshSensorSample = true;
  return next;
}

export interface P011InputSelection {
  state: P011InputModeState;
  source: 'sensor' | 'script' | 'gesture' | 'idle';
  targetTilt: number | null;
  freshScriptCommand: P011TiltWantCommand | null;
}

export function selectP011Input(state: P011InputModeState, pendingSequence: number,
  command: P011TiltWantCommand | null): P011InputSelection {
  if (!state.testMode) {
    const next = copyInputState(state);
    next.useSensor = true;
    next.forceInvert = next.wasForceInvert = false;
    next.scriptTargetTilt = null;
    next.consumedSequence = Math.max(state.consumedSequence, pendingSequence);
    return { state: next, source: next.hasFreshSensorSample ? 'sensor' : 'idle',
      targetTilt: next.hasFreshSensorSample ? null : 0, freshScriptCommand: null };
  }
  if (!state.forceInvert && pendingSequence > state.consumedSequence) {
    if (command !== null && command.sequence === pendingSequence &&
      Number.isFinite(command.targetTilt) && Math.abs(command.targetTilt) <= 89 &&
      Number.isFinite(command.commandTime) && command.commandTime > 0) {
      const next = copyInputState(state);
      next.useSensor = false;
      next.consumedSequence = pendingSequence;
      next.scriptTargetTilt = next.tiltX = command.targetTilt;
      return { state: next,
        source: 'script', targetTilt: command.targetTilt, freshScriptCommand: command };
    }
    state = copyInputState(state);
    state.consumedSequence = pendingSequence;
  }
  const held = !state.forceInvert ? state.scriptTargetTilt : null;
  return { state, source: held !== null ? 'script' : 'gesture',
    targetTilt: held, freshScriptCommand: null };
}

export function canP011GestureOwnInput(state: P011InputModeState): boolean {
  return state.testMode && state.scriptTargetTilt === null && !state.forceInvert;
}
