import { describe, it, expect } from '@ohos/hypium';
import { P011InputModeState, transitionP011InputMode, acceptP011SensorSample,
  selectP011Input, canP011GestureOwnInput } from '../main/ets/physics/P011InputMode';
import { WaterSandField } from '../main/ets/physics/WaterSandField';

function initial(): P011InputModeState {
  return { testMode: false, useSensor: true, scriptTargetTilt: null, consumedSequence: 0,
    tiltX: 0, lastTiltX: 0, forceInvert: false, wasForceInvert: false, gestureHoldUntil: 0,
    sensorGx: 0, sensorGy: 0, hasFreshSensorSample: false,
    lastSensorCommandTime: 0, lastTiltCommandTime: 0 };
}

export default function p011InputModeTest() {
  describe('P011InputModeTest', () => {
    it('MODE01_testRejectsOneHundredSensorSamplesExactly', 0, () => {
      let clean = transitionP011InputMode(initial(), true, 0, 0).state;
      let noisy = transitionP011InputMode(initial(), true, 0, 0).state;
      const targets = [0, 15, -15, 0, 89, -89];
      for (let q = 1; q <= targets.length; q++) {
        for (let n = 0; n < 100; n++) {
          const before = JSON.stringify(noisy);
          noisy = acceptP011SensorSample(noisy, n - 50, 50 - n, q * 100 + n, 0.35);
          expect(JSON.stringify(noisy)).assertEqual(before);
        }
        const command = { sequence: q, targetTilt: targets[q - 1], commandTime: q * 100 };
        const a = selectP011Input(clean, q, command);
        const b = selectP011Input(noisy, q, command);
        expect(JSON.stringify(a)).assertEqual(JSON.stringify(b));
        clean = a.state;
        noisy = b.state;
        expect(selectP011Input(noisy, q, command).freshScriptCommand).assertEqual(null);
      }
    });

    it('MODE01_03_experienceRejectsScriptAndGestureAndForceInvert', 0, () => {
      let state = acceptP011SensorSample(initial(), -2, 8, 123, 0.5);
      const baseline = state;
      state.forceInvert = true;
      state.wasForceInvert = true;
      const selected = selectP011Input(state, 7, { sequence: 7, targetTilt: 40, commandTime: 100 });
      expect(selected.source).assertEqual('sensor');
      expect(selected.freshScriptCommand).assertEqual(null);
      expect(selected.state.forceInvert).assertFalse();
      expect(selected.state.wasForceInvert).assertFalse();
      expect(selected.state.sensorGx).assertEqual(-1);
      expect(selected.state.sensorGy).assertEqual(4);
      expect(selected.state.lastSensorCommandTime).assertEqual(123);
      expect(canP011GestureOwnInput(selected.state)).assertFalse();
      expect(Math.atan2(-selected.state.sensorGx, selected.state.sensorGy)).assertEqual(
        Math.atan2(-baseline.sensorGx, baseline.sensorGy));
      expect(acceptP011SensorSample(selected.state, NaN, 1, 124, 0.5)).assertEqual(selected.state);
      expect(acceptP011SensorSample(selected.state, 1, 1, 0, 0.5)).assertEqual(selected.state);
    });

    it('MODE02_06_tenRoundTripsClearAllInputOnlyState', 0, () => {
      let state = initial();
      for (let n = 1; n <= 20; n++) {
        state.scriptTargetTilt = 20;
        state.tiltX = state.lastTiltX = 20;
        state.forceInvert = state.wasForceInvert = true;
        state.gestureHoldUntil = 999999;
        state.sensorGx = state.sensorGy = 9;
        state.hasFreshSensorSample = true;
        state.lastTiltCommandTime = state.lastSensorCommandTime = 42;
        const changed = transitionP011InputMode(state, !state.testMode, n, 0);
        expect(changed.changed).assertTrue();
        state = changed.state;
        expect(state.useSensor).assertEqual(!state.testMode);
        expect(state.scriptTargetTilt).assertEqual(null);
        expect(state.consumedSequence).assertEqual(n);
        expect(state.tiltX + state.lastTiltX + state.sensorGx + state.sensorGy +
          state.gestureHoldUntil + state.lastSensorCommandTime + state.lastTiltCommandTime).assertEqual(0);
        expect(state.forceInvert || state.wasForceInvert || state.hasFreshSensorSample).assertFalse();
        expect(selectP011Input(state, n, { sequence: n, targetTilt: 20, commandTime: 42 })
          .freshScriptCommand).assertEqual(null);
      }
    });

    it('MODE05_coldHotFirstEnvelopeAndSameModePreserveCommands', 0, () => {
      let state = transitionP011InputMode(initial(), true, 1, 1).state;
      const targets = [0, 15, -15, 0];
      for (let q = 1; q <= targets.length; q++) {
        const previous = state;
        const mode = transitionP011InputMode(state, true, q, q);
        expect(mode.changed).assertFalse();
        expect(mode.state).assertEqual(previous);
        const selection = selectP011Input(mode.state, q,
          { sequence: q, targetTilt: targets[q - 1], commandTime: 100 * q });
        expect(selection.freshScriptCommand?.sequence).assertEqual(q);
        expect(selection.targetTilt).assertEqual(targets[q - 1]);
        state = selection.state;
        expect(canP011GestureOwnInput(state)).assertFalse();
        expect(selectP011Input(state, q, null).targetTilt).assertEqual(targets[q - 1]);
      }
      state = transitionP011InputMode(state, false, 6, 0).state;
      expect(selectP011Input(state, 6, null).source).assertEqual('idle');
      state = transitionP011InputMode(state, true, 7, 7).state;
      expect(state.consumedSequence).assertEqual(6);
      expect(selectP011Input(state, 7, { sequence: 7, targetTilt: 25, commandTime: 700 })
        .freshScriptCommand?.sequence).assertEqual(7);
    });

    it('MODE03_07_freshCallbackRecoversAndBothSignsMapThroughSameChain', 0, () => {
      let state = transitionP011InputMode(initial(), true, 0, 0).state;
      state = selectP011Input(state, 1, { sequence: 1, targetTilt: 20, commandTime: 10 }).state;
      state = transitionP011InputMode(state, false, 1, 0).state;
      expect(selectP011Input(state, 1, null).source).assertEqual('idle');
      state = acceptP011SensorSample(state, -2, 8, 125, 1);
      expect(selectP011Input(state, 1, null).source).assertEqual('sensor');
      expect(Math.atan2(-state.sensorGx, state.sensorGy) > 0).assertTrue();
      state = acceptP011SensorSample(state, 2, 8, 126, 1);
      expect(Math.atan2(-state.sensorGx, state.sensorGy) < 0).assertTrue();
      expect(state.lastSensorCommandTime).assertEqual(126);
    });

    it('MODE04_repeatedDeterministicCommandTraceAndInvalidWatermark', 0, () => {
      const traces: string[] = [];
      for (let run = 0; run < 2; run++) {
        let state = transitionP011InputMode(initial(), true, 0, 0).state;
        let trace = '';
        for (let q = 1; q <= 100; q++) {
          const selection = selectP011Input(state, q,
            { sequence: q, targetTilt: (q % 3 - 1) * 15, commandTime: q * 100 });
          state = selection.state;
          trace += JSON.stringify(selection);
        }
        traces.push(trace);
        state = selectP011Input(state, 101, { sequence: 101, targetTilt: NaN, commandTime: 100 }).state;
        expect(state.consumedSequence).assertEqual(101);
        state.forceInvert = true;
        expect(selectP011Input(state, 102, { sequence: 102, targetTilt: 30, commandTime: 200 })
          .freshScriptCommand).assertEqual(null);
        expect(canP011GestureOwnInput(state)).assertFalse();
      }
      expect(traces[0]).assertEqual(traces[1]);
    });

    it('MODE07_equivalentNormalizedInputsShareExactPhysicsTrajectory', 0, () => {
      let simulation = transitionP011InputMode(initial(), true, 0, 0).state;
      let experience = initial();
      const fromScript = new WaterSandField(16, 192);
      const fromSensor = new WaterSandField(16, 192);
      for (let q = 1; q <= 4; q++) {
        const x = q % 2 === 0 ? 2 : -2;
        experience = acceptP011SensorSample(experience, x, 8, q * 100, 1);
        // The normalized angle uses the same signed mapping as SandCanvas's sensor branch.
        const angle = Math.atan2(-experience.sensorGx, experience.sensorGy) * 180 / Math.PI;
        const simulated = selectP011Input(simulation, q,
          { sequence: q, targetTilt: angle, commandTime: q * 100 });
        simulation = simulated.state;
        const sensed = selectP011Input(experience, q, null);
        experience = sensed.state;
        expect(sensed.source).assertEqual('sensor');
        fromScript.setTilt(simulated.targetTilt === null ? 0 : simulated.targetTilt);
        fromSensor.setTilt(angle);
        for (let frame = 0; frame < 30; frame++) {
          fromScript.step(1 / 60);
          fromSensor.step(1 / 60);
          expect(JSON.stringify(fromScript.snapshot())).assertEqual(JSON.stringify(fromSensor.snapshot()));
          const a = fromScript.getP011Metric();
          const b = fromSensor.getP011Metric();
          expect(a.effectiveTilt).assertEqual(b.effectiveTilt);
          expect(a.bottomFlux).assertEqual(b.bottomFlux);
          expect(a.totalMotionBudgetUsed).assertEqual(b.totalMotionBudgetUsed);
        }
      }
    });
  });
}
