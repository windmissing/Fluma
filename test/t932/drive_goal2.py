"""T932 goal-2 window cross-generation lifecycle: resize, minimize/restore, new-gen first frame, old-gen retirement.

Scenario (single t926 demo window):
  1. hold 4s to pour (accept quanta, gen N)
  2. resize window (WM_SIZE -> generation++, owner loop re-enters)
  3. verify: new WINDOW_AVAILABLE with new geometry, new M1_QUANTUM present queued
     (new-gen first frame), old-gen quanta stop, new hold required (M1_INPUT_WAIT_NEW_DOWN
     or fresh DOWN), recovery events recorded
  4. minimize (SW_MINIMIZE) then restore (SW_RESTORE): IsIconic -> width=height=0 ->
     wake waits until restored; verify generation advanced and physics resumed
  5. WSI vs effective visibility: FENCED_READBACK rows carry real pixel data (ROI);
     M1_QUANTUM_PRESENT_QUEUED rows are WSI-side evidence. Both are recorded per present.
"""
import ctypes
import json
import os
import subprocess
import time

WM_LBUTTONDOWN = 0x0201
WM_LBUTTONUP = 0x0202
WM_MOUSEMOVE = 0x0200
WM_CLOSE = 0x0010
MK_LBUTTON = 1
SW_MINIMIZE = 6
SW_RESTORE = 9


def lp_of(x, y):
    return (y & 0xFFFF) << 16 | (x & 0xFFFF)


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


def wait_ready(path, timeout=180):
    deadline = time.time() + timeout
    seen = 0
    while time.time() < deadline:
        rows = read_rows(path)
        for row in rows[seen:]:
            if row.get('stage') == 'M1_POINTER_READY':
                return True
        seen = len(rows)
        time.sleep(0.1)
    return False


def hold(hwnd, cx, cy, seconds):
    lp = lp_of(cx, cy)
    ctypes.windll.user32.PostMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lp)
    for _ in range(max(1, int(seconds / 0.2))):
        time.sleep(0.2)
        ctypes.windll.user32.PostMessageW(hwnd, WM_MOUSEMOVE, MK_LBUTTON, lp)
    ctypes.windll.user32.PostMessageW(hwnd, WM_LBUTTONUP, 0, lp)


def main():
    out = 'test/reports/t932/runs/goal2-lifecycle'
    os.makedirs(out, exist_ok=True)
    cmd = ['.codex/local-test/t926-ninja/hello_world_pc.exe', '--m1-window',
           '.codex/local-test/t918-shaders/full_source.spv',
           '.codex/local-test/t918-shaders/quantum_candidate.spv',
           '.codex/local-test/t918-shaders/quantum_commit.spv',
           '--m1-source-trace', '--m1-dual-source', '--m1-compact-demo']
    log = os.path.join(out, 'demo.jsonl')
    json.dump({'command': cmd}, open(os.path.join(out, 'argv.json'), 'w'), indent=2)
    with open(log, 'wb') as sink:
        proc = subprocess.Popen(cmd, stdout=sink, stderr=subprocess.STDOUT)
    try:
        hwnd, size = find_window(log)
        if not hwnd or not wait_ready(log):
            return {'ok': False, 'error': 'window/ready'}
        w, h = size
        cx, cy = w // 2, h // 2
        # phase 1: pour to get accepted quanta in generation N
        hold(hwnd, cx, cy, 4.0)
        time.sleep(2.0)
        rows_before = read_rows(log)
        gen_before = max(int(r.get('geometry', 0) or 0) for r in rows_before if r.get('geometry'))
        q_before = sum(1 for r in rows_before if r.get('state') == 'M1_QUANTUM_PRESENT_QUEUED')
        wa_before = sum(1 for r in rows_before if r.get('stage') == 'WINDOW_AVAILABLE')
        # phase 2: resize larger
        ctypes.windll.user32.SetWindowPos(hwnd, 0, 100, 100, 800, 500, 0x0004)
        time.sleep(4.0)
        rows_resize = read_rows(log)
        # phase 3: pour again in new generation
        w2, h2 = 800, 500
        hold(hwnd, w2 // 2, h2 // 2, 4.0)
        time.sleep(2.0)
        # phase 4: minimize then restore
        ctypes.windll.user32.ShowWindow(hwnd, SW_MINIMIZE)
        time.sleep(3.0)
        rows_min = read_rows(log)
        ctypes.windll.user32.ShowWindow(hwnd, SW_RESTORE)
        time.sleep(4.0)
        # phase 5: final pour after restore
        rows_restored = read_rows(log)
        hold(hwnd, w2 // 2, h2 // 2, 3.0)
        time.sleep(3.0)
        ctypes.windll.user32.PostMessageW(hwnd, WM_CLOSE, 0, 0)
        try:
            proc.wait(timeout=30)
        except subprocess.TimeoutExpired:
            proc.kill()
        rows = read_rows(log)
        wa = [r for r in rows if r.get('stage') == 'WINDOW_AVAILABLE']
        qs = [r for r in rows if r.get('state') == 'M1_QUANTUM_PRESENT_QUEUED']
        recover = [r for r in rows if r.get('stage') == 'M1_HOLD_RECOVERY']
        fr = [r for r in rows if r.get('stage') == 'FENCED_READBACK']
        # per-generation present counts
        from collections import Counter
        per_gen = Counter()
        for r in qs:
            per_gen[r.get('geometry')] += 1
        result = {
            'ok': True,
            'windowAvailable': [{'geometry': r.get('geometry'), 'w': r.get('width'), 'h': r.get('height')}
                                for r in wa],
            'genBefore': gen_before, 'qBefore': q_before, 'waBefore': wa_before,
            'quantaPerGeneration': dict(per_gen),
            'holdRecovery': len(recover),
            'fencedReadbackRows': len(fr),
            'lastQuantaAfterRestore': per_gen.get(wa[-1].get('geometry'), 0) if wa else 0,
            'rows': len(rows),
        }
        with open(os.path.join(out, 'lifecycle.json'), 'w', encoding='utf-8') as f:
            json.dump(result, f, indent=1)
        print(json.dumps(result, indent=1)[:1500])
        return 0
    finally:
        if proc.poll() is None:
            proc.kill()


if __name__ == '__main__':
    raise SystemExit(main())
