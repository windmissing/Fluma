"""T926/T927 compact demo driver: late-fence retirement recovery, long pour, candidate parity.

Scenarios (each spawns its own process/window, single window per run):
  recovery:  hold 12s -> trigger CLOCK_GAP retirement -> keep holding -> verify
             M1_HOLD_RECOVERY SUSPEND/RETIRED/NEW_BASELINE/HOLD_REVALIDATED chain,
             new session SOURCE admission, old-session offer rejection.
  gap-hold:  one Down spanning the whole window; no Up between retirement phases.
  longrun:   repeated hold/idle cycles >=60s after first pile; final probes must
             still admit (contrast with T920 pre-fix behaviour).
  parity:    same input script on t918 profile vs t926 fast candidate; compare
             events, inventory, replay bytes and dispatch counts.
"""
import argparse
import ctypes
import json
import os
import subprocess
import time

WM_KEYDOWN = 0x0100
WM_KEYUP = 0x0101
WM_CHAR = 0x0102
WM_LBUTTONDOWN = 0x0201
WM_LBUTTONUP = 0x0202
WM_MOUSEMOVE = 0x0200
WM_CLOSE = 0x0010
MK_LBUTTON = 1


def lp_of(x, y):
    return (y & 0xFFFF) << 16 | (x & 0xFFFF)


def post_msg(hwnd, msg, wp, lp):
    ctypes.windll.user32.PostMessageW(hwnd, msg, wp, lp)


def hold_move(hwnd, cx, cy, seconds):
    lp = lp_of(cx, cy)
    post_msg(hwnd, WM_MOUSEMOVE, MK_LBUTTON, lp)
    for _ in range(max(1, int(seconds / 0.2))):
        time.sleep(0.2)
        post_msg(hwnd, WM_MOUSEMOVE, MK_LBUTTON, lp)


def read_rows(path):
    try:
        with open(path, 'rb') as handle:
            lines = handle.read().splitlines()
    except OSError:
        return []
    rows = []
    for line in lines:
        try:
            rows.append(json.loads(line))
        except (json.JSONDecodeError, UnicodeDecodeError):
            continue
    return rows


def find_window(path, timeout=40):
    deadline = time.time() + timeout
    while time.time() < deadline:
        for row in read_rows(path):
            if row.get('stage') == 'WINDOW_AVAILABLE':
                return int(row['hwnd']), (row['width'], row['height'])
        time.sleep(0.2)
    return None, None


def wait_stage(path, stage, timeout):
    deadline = time.time() + timeout
    seen = 0
    while time.time() < deadline:
        rows = read_rows(path)
        for row in rows[seen:]:
            if row.get('stage') == stage:
                return True
        seen = len(rows)
        time.sleep(0.1)
    return False


def count_stage(rows, stage):
    return sum(1 for r in rows if r.get('stage') == stage)


def launch(cmd, out):
    os.makedirs(out, exist_ok=True)
    log = os.path.join(out, 'demo.jsonl')
    with open(os.path.join(out, 'argv.json'), 'w', encoding='utf-8') as f:
        json.dump({'command': cmd}, f, indent=2)
    with open(log, 'wb') as sink:
        return subprocess.Popen(cmd, stdout=sink, stderr=subprocess.STDOUT), log


def shutdown(hwnd, proc):
    post_msg(hwnd, WM_CLOSE, 0, 0)
    try:
        proc.wait(timeout=30)
    except subprocess.TimeoutExpired:
        proc.kill()


def base_cmd(exe, shaders, extra):
    cmd = [exe, '--m1-window',
           os.path.join(shaders, 'full_source.spv'),
           os.path.join(shaders, 'quantum_candidate.spv'),
           os.path.join(shaders, 'quantum_commit.spv'),
           '--m1-source-trace', '--m1-dual-source', '--m1-compact-demo']
    cmd += extra
    return cmd


def scenario_recovery(exe, shaders, out, extra, hold_total=14.0):
    cmd = base_cmd(exe, shaders, extra + ['--m1-one-quantum-grant'])
    proc, log = launch(cmd, out)
    try:
        hwnd, size = find_window(log)
        if not hwnd or not wait_stage(log, 'M1_POINTER_READY', 180):
            return {'ok': False, 'error': 'window/ready'}
        w, h = size
        cx, cy = w // 2, h // 2
        # single long Down spanning potential retirement gap
        post_msg(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lp_of(cx, cy))
        hold_move(hwnd, cx, cy, hold_total)
        post_msg(hwnd, WM_LBUTTONUP, 0, lp_of(cx, cy))
        time.sleep(4.0)
        rows = read_rows(log)
        result = {
            'ok': True,
            'holdRecovery': [r for r in rows if r.get('stage') == 'M1_HOLD_RECOVERY'],
            'retirePending': count_stage(rows, 'M1_RETIRE_PENDING'),
            'tombstoneRetired': count_stage(rows, 'M1_TOMBSTONE_RETIRED'),
            'retireFailed': [r for r in rows if r.get('stage') == 'M1_RETIRE_FAILED'],
            'retireWait': count_stage(rows, 'M1_RETIRE_WAIT'),
            'quantumAccepted': count_stage(rows, 'M1_QUANTUM'),
            'sourceAccepted': count_stage(rows, 'M1_SOURCE'),
            'newDown': count_stage(rows, 'M1_INPUT_WAIT_NEW_DOWN'),
            'rows': len(rows),
        }
        return result
    finally:
        shutdown(hwnd, proc) if hwnd else proc.kill()


def scenario_longrun(exe, shaders, out, extra, cycles=10, hold=6.0, idle=4.0, probes=6):
    cmd = base_cmd(exe, shaders, extra + ['--m1-one-quantum-grant'])
    proc, log = launch(cmd, out)
    try:
        hwnd, size = find_window(log)
        if not hwnd or not wait_stage(log, 'M1_POINTER_READY', 180):
            return {'ok': False, 'error': 'window/ready'}
        w, h = size
        cx, cy = w // 2, h // 2
        t0 = time.time()
        admitted = []
        for i in range(cycles):
            post_msg(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lp_of(cx, cy))
            hold_move(hwnd, cx, cy, hold)
            post_msg(hwnd, WM_LBUTTONUP, 0, lp_of(cx, cy))
            time.sleep(idle)
            rows = read_rows(log)
            admitted.append(count_stage(rows, 'M1_QUANTUM'))
        time.sleep(10.0)
        for i in range(probes):
            post_msg(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lp_of(cx, cy))
            hold_move(hwnd, cx, cy, hold)
            post_msg(hwnd, WM_LBUTTONUP, 0, lp_of(cx, cy))
            time.sleep(idle)
        time.sleep(4.0)
        elapsed = time.time() - t0
        rows = read_rows(log)
        quanta = [r for r in rows if r.get('stage') == 'M1_QUANTUM']
        return {
            'ok': True,
            'wallSeconds': round(elapsed, 1),
            'quantumTotal': len(quanta),
            'quantumAfterFirstPile': admitted[-1] if admitted else 0,
            'finalProbesStillAdmit': count_stage(rows, 'M1_QUANTUM') > (admitted[-1] if admitted else 0),
            'retirePending': count_stage(rows, 'M1_RETIRE_PENDING'),
            'tombstone': count_stage(rows, 'M1_TOMBSTONE_RETIRED'),
            'retireFailed': [r for r in rows if r.get('stage') == 'M1_RETIRE_FAILED'],
            'sources': count_stage(rows, 'M1_SOURCE'),
            'rows': len(rows),
        }
    finally:
        shutdown(hwnd, proc) if hwnd else proc.kill()


def scenario_parity(exe, shaders, out, extra, hold=6.0):
    cmd = base_cmd(exe, shaders, extra)
    proc, log = launch(cmd, out)
    try:
        hwnd, size = find_window(log)
        if not hwnd or not wait_stage(log, 'M1_POINTER_READY', 180):
            return {'ok': False, 'error': 'window/ready'}
        w, h = size
        cx, cy = w // 2, h // 2
        post_msg(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lp_of(cx, cy))
        hold_move(hwnd, cx, cy, hold)
        post_msg(hwnd, WM_LBUTTONUP, 0, lp_of(cx, cy))
        time.sleep(6.0)
        rows = read_rows(log)
        quanta = [r for r in rows if r.get('stage') == 'M1_QUANTUM']
        dispatches = []
        for q in quanta:
            work = q.get('m1QuantumWork') or {}
            if 'dispatches' in work:
                dispatches.append(work['dispatches'])
        completion = [q.get('m1QuantumCompletion') for q in quanta if q.get('m1QuantumCompletion')]
        return {
            'ok': True,
            'quantumTotal': len(quanta),
            'dispatches': dispatches,
            'completionTails': completion,
            'sources': count_stage(rows, 'M1_SOURCE'),
            'rows': len(rows),
        }
    finally:
        shutdown(hwnd, proc) if hwnd else proc.kill()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True)
    parser.add_argument('--shaders', required=True)
    parser.add_argument('--out', required=True)
    parser.add_argument('--scenario', choices=('recovery', 'longrun', 'parity'), required=True)
    parser.add_argument('--extra', nargs='*', default=[])
    args = parser.parse_args()
    if args.scenario == 'recovery':
        result = scenario_recovery(args.exe, args.shaders, args.out, args.extra)
    elif args.scenario == 'longrun':
        result = scenario_longrun(args.exe, args.shaders, args.out, args.extra)
    else:
        result = scenario_parity(args.exe, args.shaders, args.out, args.extra)
    result_path = os.path.join(args.out, 'scenario-result.json')
    with open(result_path, 'w', encoding='utf-8') as f:
        json.dump(result, f, indent=1)
    print(json.dumps(result, indent=1)[:2000])
    return 0 if result.get('ok') else 3


if __name__ == '__main__':
    raise SystemExit(main())
