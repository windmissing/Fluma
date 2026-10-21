import { describe, it, expect } from '@ohos/hypium';
import { parseP011ScheduleWant, P011SchedulePlayer,
  P011_SCHEDULE_DRIFT_MARKER_MS
} from '../main/ets/physics/P011SchedulePlayer';
import { P011ScheduleV3, p011BuildScheduleV3, p011ScheduleV3ToCsv,
  P011_V3_COMMAND_STEP_MS, P011_V3_PLATFORM_HOLD_MS
} from '../main/ets/physics/P011ScheduleV3';

/**
 * P-011 observation-3 v3 schedule player (§26.4) unit cases:
 *  - want parameter parse: legal / illegal / absent (three-state style)
 *  - dispatch order on the monotonic clock (single + multi-command frames)
 *  - drift recording (recorded never chased; marker threshold)
 *  - lifecycle: finished state lifts mutual exclusion; forceStop recovers
 *    single commands after a crashed player (risk 1)
 *  - generated schedule matches the frozen CSV contract (sequence already
 *    covered by p011_scheduleV3_generationAndCsvContract; here only the
 *    player->CSV consistency of the replayed command stream)
 */

function wantParams(schedule: string | null): Record<string, Object> {
  const params: Record<string, Object> = {};
  if (schedule !== null) {
    params['p011schedule'] = schedule;
  }
  return params;
}

export default function p011SchedulePlayerTest() {
  describe('p011SchedulePlayerTest', () => {
    // --- want parameter parse ------------------------------------------

    it('p011_schedulePlayer_wantParseThreeStates', 0, () => {
      // (a) legal: 20:0.5
      const legal = parseP011ScheduleWant(wantParams('20:0.5'));
      expect(legal.peakADeg !== null &&
        Math.abs((legal.peakADeg as number) - 20) < 1e-12).assertTrue();
      expect(legal.rateDegPerSec !== null &&
        (legal.rateDegPerSec as number) === 0.5).assertTrue();
      expect(legal.warnings.length === 0).assertTrue();
      // (b) illegal format (missing rate)
      const badFormat = parseP011ScheduleWant(wantParams('20'));
      expect(badFormat.peakADeg === null).assertTrue();
      expect(badFormat.warnings.includes('INVALID_P011SCHEDULE_FORMAT')).assertTrue();
      // (c) illegal peak (>= 30, outside the 0 < A < 30 bound)
      const badPeak = parseP011ScheduleWant(wantParams('30:0.5'));
      expect(badPeak.warnings.includes('INVALID_P011SCHEDULE_PEAK')).assertTrue();
      // (d) non-frozen rate
      const badRate = parseP011ScheduleWant(wantParams('20:0.7'));
      expect(badRate.warnings.includes('INVALID_P011SCHEDULE_RATE')).assertTrue();
      // (e) absent parameter -> nulls, no warnings (no schedule run)
      const absent = parseP011ScheduleWant(wantParams(null));
      expect(absent.peakADeg === null).assertTrue();
      expect(absent.rateDegPerSec === null).assertTrue();
      expect(absent.warnings.length === 0).assertTrue();
    });

    // --- dispatch order and monotonic clock ----------------------------

    it('p011_schedulePlayer_dispatchesInOrderOnClock', 0, () => {
      const player = new P011SchedulePlayer();
      expect(player.start(20, 0.5, 1000)).assertTrue();
      expect(player.isActive()).assertTrue();
      // t0: nothing due yet (H0 is at offset 0 -> due at t=1000).
      const t0 = player.tick(999);
      expect(t0.length === 0).assertTrue();
      // t=1000: H0 due.
      const h0 = player.tick(1000);
      expect(h0.length === 1).assertTrue();
      expect(h0[0].command.segmentId).assertEqual('H0');
      expect(h0[0].driftMs === 0).assertTrue();
      // Inside L1 the step is 100 ms: one command per tick.
      const l1Start = player.tick(1000 + P011_V3_PLATFORM_HOLD_MS);
      expect(l1Start.length === 1).assertTrue();
      expect(l1Start[0].command.phaseKind).assertEqual('ramp');
      expect(l1Start[0].command.legId).assertEqual('L1');
      const next = player.tick(1000 + P011_V3_PLATFORM_HOLD_MS +
        P011_V3_COMMAND_STEP_MS);
      expect(next.length === 1).assertTrue();
      expect(next[0].command.commandSequence)
        .assertEqual(l1Start[0].command.commandSequence + 1);
    });

    it('p011_schedulePlayer_slowFrameDispatchesMultipleInOrder', 0, () => {
      // §26.4 risk 3: a slow frame may dispatch several commands; setTilt is
      // last-write-wins target semantics and the drift records the window.
      const player = new P011SchedulePlayer();
      expect(player.start(20, 0.5, 0)).assertTrue();
      // Jump far ahead: everything from offset 0 to 15000 ms is due.
      const dispatches = player.tick(15000);
      expect(dispatches.length > 50).assertTrue();
      let lastSeq = 0;
      let lastOffset = -1;
      for (const dispatch of dispatches) {
        expect(dispatch.command.commandSequence > lastSeq).assertTrue();
        expect(dispatch.scheduledOffsetMs >= lastOffset).assertTrue();
        lastSeq = dispatch.command.commandSequence;
        lastOffset = dispatch.scheduledOffsetMs;
        // Late dispatch: drift = actual - scheduled >= 0.
        expect(dispatch.driftMs >= 0).assertTrue();
      }
      // currentCommand is the LAST dispatched (last-write-wins target).
      const current = player.currentCommand();
      expect(current !== null &&
        current.commandSequence === lastSeq).assertTrue();
    });

    // --- drift recording (decision 2) ----------------------------------

    it('p011_schedulePlayer_driftRecordedNeverChased', 0, () => {
      const player = new P011SchedulePlayer();
      expect(player.start(20, 0.5, 5000)).assertTrue();
      // H0 due at 5000; tick arrives 70 ms late -> drift 70 > 50 marker.
      const late = player.tick(5070);
      expect(late.length === 1).assertTrue();
      expect(Math.abs(late[0].driftMs - 70) < 1e-9).assertTrue();
      expect(late[0].driftExceeded).assertTrue();
      expect(late[0].driftMs > P011_SCHEDULE_DRIFT_MARKER_MS).assertTrue();
      // The NEXT command keeps its own schedule (no catch-up): L1 start is
      // due at 5000 + 5100 = 10100; a tick at 10110 drifts 10 ms (no marker).
      const nextLate = player.tick(10110);
      expect(nextLate.length === 1).assertTrue();
      expect(Math.abs(nextLate[0].driftMs - 10) < 1e-9).assertTrue();
      expect(nextLate[0].driftExceeded).assertFalse();
      // scheduledOffsetMs is the schedule's own offset (not the wall clock).
      expect(nextLate[0].scheduledOffsetMs)
        .assertEqual(P011_V3_PLATFORM_HOLD_MS);
    });

    // --- lifecycle (risk 1) ----------------------------------------------

    it('p011_schedulePlayer_finishLiftsMutualExclusion', 0, () => {
      const player = new P011SchedulePlayer();
      expect(player.start(20, 0.5, 0)).assertTrue();
      // Replay the whole schedule in one far-future tick.
      const all = player.tick(9 * P011_V3_PLATFORM_HOLD_MS +
        4 * 20 / 0.5 * 1000 + 1000);
      expect(all.length > 100).assertTrue();
      expect(player.getState()).assertEqual('finished');
      expect(player.isActive()).assertFalse();
      // finished -> a further tick dispatches nothing, single commands are
      // free again (isActive false is the mutual-exclusion predicate).
      expect(player.tick(Number.MAX_SAFE_INTEGER).length === 0).assertTrue();
    });

    it('p011_schedulePlayer_forceStopRecoversSingleCommands', 0, () => {
      // Risk 1: a crashed player (never finished naturally) must not
      // permanently reject single commands — forceStop clears the flag.
      const player = new P011SchedulePlayer();
      expect(player.start(20, 0.5, 0)).assertTrue();
      expect(player.isActive()).assertTrue();
      // Simulate the crash: no more ticks. The teardown path calls
      // forceStop; afterwards single commands are accepted again.
      player.forceStop();
      expect(player.isActive()).assertFalse();
      expect(player.getState()).assertEqual('inactive');
      expect(player.tick(100000).length === 0).assertTrue();
    });

    it('p011_schedulePlayer_invalidStartStaysInactive', 0, () => {
      const player = new P011SchedulePlayer();
      // A=30 violates 0 < A < 30 (P011RelaxHysteresisParamError) -> the
      // player stays inactive and start returns false.
      expect(player.start(30, 0.5, 0)).assertFalse();
      expect(player.isActive()).assertFalse();
      expect(player.getState()).assertEqual('inactive');
      // rate 0.7 is not frozen either.
      expect(player.start(20, 0.7, 0)).assertFalse();
      expect(player.isActive()).assertFalse();
    });

    // --- replayed stream matches the frozen CSV --------------------------

    it('p011_schedulePlayer_replayMatchesFrozenCsv', 0, () => {
      // The player replays the SAME command list the CSV contract froze:
      // drive the whole schedule at exact offsets and compare the stream
      // (sequence, offset, segment, phase, leg, target, rate) with
      // p011BuildScheduleV3/p011ScheduleV3ToCsv ground truth.
      const schedule: P011ScheduleV3 = p011BuildScheduleV3(20, 0.5);
      const player = new P011SchedulePlayer();
      expect(player.start(20, 0.5, 0)).assertTrue();
      const replayed: string[] = [];
      for (const command of schedule.commands) {
        const dispatches = player.tick(command.offsetMs);
        for (const dispatch of dispatches) {
          const rate = dispatch.command.rateDegPerSec === 0 ? '0'
            : (Math.round(dispatch.command.rateDegPerSec * 100) / 100).toFixed(2);
          replayed.push(`${dispatch.command.commandSequence},` +
            `${dispatch.command.offsetMs},${dispatch.command.segmentId},` +
            `${dispatch.command.phaseKind},${dispatch.command.legId},` +
            `${(Math.round(dispatch.command.targetTiltDeg * 100) / 100).toFixed(2)},` +
            `${rate}`);
        }
      }
      // CSV body rows (skip header, drop trailing empty from split).
      const csv = p011ScheduleV3ToCsv(schedule);
      const rows = csv.split('\n');
      const body = rows.slice(1, rows.length - 1);
      expect(replayed.length).assertEqual(body.length);
      for (let index = 0; index < body.length; index++) {
        expect(replayed[index]).assertEqual(body[index]);
      }
    });
  });
}
