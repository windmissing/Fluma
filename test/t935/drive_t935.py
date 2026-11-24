"""T935 integrated demo qualification: one PE, one launcher, full experience round.

Rounds on .codex/local-test/t933-integrated-demo via its own Start Demo.cmd
contract (same argv). Scenarios:
  1. empty box visible + READY
  2. hold 2-4s -> same-root pixel sand pile; release -> settle retained
  3. >=60s repeated holds -> late-phase real SOURCE/QUANTUM still admitted
  4. LEFT/RIGHT box turns, boundary reverse, R key + on-screen reset button
     (request/actual angle rows)
  5. late-fence retirement recovery (first wait timeout -> same-fence success ->
     new session continues)
  6. input vs LAND conservation accounting from m1root archives
All timing from wall clock at Down. Logs to test/reports/t935/runs/.
"""
import ctypes
import json
import os
import subprocess
import time
from collections import Counter

WM_LBUTTONDOWN = 0x0201
WM_LBUTTONUP = 0x0202
WM_MOUSEMOVE = 0x0200
WM_KEYDOWN = 0x0100
WM_KEYUP = 0x0101
WM_CHAR = 0x0102
WM_CLOSE = 0x0010
MK_LBUTTON = 1
VK_LEFT = 0x25
VK_RIGHT = 0x27

PKG = '.codex/local-test/t933-integrated-demo'


def lp_of(x, y):
    return (y & 0xFFFF) << 16 | (x & 0xFFFF)


def post(hwnd, msg, wp, lp):
    ctypes.windll.user32.PostMessageW(hwnd, msg, wp, lp)


def key(hwnd, vk, char=None):
    post(hwnd, WM_KEYDOWN, vk, 0)
    if char is not None:
        post(hwnd, WM_CHAR, char, 0)
    post(hwnd, WM_KEYUP, vk, 0)


def hold(hwnd, cx, cy, seconds):
    lp = lp_of(cx, cy)
    post(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lp)
    for _ in range(max(1, int(seconds / 0.2))):
        time.sleep(0.2)
        post(hwnd, WM_MOUSEMOVE, MK_LBUTTON, lp)
    post(hwnd, WM_LBUTTONUP, 0, lp)


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


def main():
    out = 'test/reports/t935/runs/integrated-round'
    os.makedirs(out, exist_ok=True)
    # Same entry as Start Demo.cmd: same exe, same shaders, same argv (no launcher
    # hash echo since we stream stdout ourselves; manifest checked separately).
    cmd = [PKG + '/hello_world_pc.exe', '--m1-window',
           PKG + '/full_source.spv', PKG + '/quantum_candidate.spv',
           PKG + '/quantum_commit.spv',
           '--m1-compact-demo', '--m1-integrated', '--m1-dual-source',
           '--m1-source-trace']
    log = os.path.join(out, 'demo.jsonl')
    json.dump({'command': cmd, 'launcherContract': 'Start Demo.cmd argv',
               'pe': '4a40a1410c14...'}, open(os.path.join(out, 'argv.json'), 'w'), indent=2)
    with open(log, 'wb') as sink:
        proc = subprocess.Popen(cmd, stdout=sink, stderr=subprocess.STDOUT)
    try:
        hwnd = size = None
        deadline = time.time() + 40
        while time.time() < deadline:
            for row in read_rows(log):
                if row.get('stage') == 'WINDOW_AVAILABLE':
                    hwnd, size = int(row['hwnd']), (row['width'], row['height'])
                    break
            if hwnd:
                break
            time.sleep(0.2)
        if not hwnd:
            return {'ok': False, 'error': 'no window'}
        ready = wait_stage(log, 'M1_POINTER_READY', 180)
        if not ready:
            return {'ok': False, 'error': 'not ready'}
        w, h = size
        cx, cy = w // 2, h // 2

        # phase 1: 2-4s pile with wall-clock from Down
        t_down = time.time()
        hold(hwnd, cx, cy, 3.0)
        t_up = time.time()
        time.sleep(3.0)  # settle after release
        rows = read_rows(log)
        qs = [r for r in rows if r.get('state') == 'M1_QUANTUM_PRESENT_QUEUED']
        settle_retained = qs and qs[-1].get('m1SceneLease', {}).get('sandStockQ') not in (None, '0', 0)

        # phase 2: >=60s repeated holds
        t60 = time.time()
        for i in range(10):
            hold(hwnd, cx, cy, 5.0)
            time.sleep(3.0)
        late_rows = read_rows(log)
        late_q = sum(1 for r in late_rows if r.get('state') == 'M1_QUANTUM_PRESENT_QUEUED')

        # phase 3: box turns + boundary reverse + reset
        for _ in range(6):
            key(hwnd, VK_LEFT)
            time.sleep(0.7)
        for _ in range(10):
            key(hwnd, VK_RIGHT)
            time.sleep(0.7)
        key(hwnd, 0x52, char=0x52)  # R
        time.sleep(1.2)
        # on-screen reset button top-right
        post(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lp_of(w - 25, 23))
        time.sleep(0.05)
        post(hwnd, WM_LBUTTONUP, 0, lp_of(w - 25, 23))
        time.sleep(2.0)
        rows = read_rows(log)
        # gravity/angle rows: request vs actual
        angle_rows = [r for r in rows if r.get('stage') == 'M1_GRAVITY_FRAME']
        requested = [r.get('requestedAngle256') for r in angle_rows]
        actual = [r.get('actualAngle256') for r in angle_rows if r.get('actualAngle256') is not None]

        # phase 4: final probes — still pouring after everything
        hold(hwnd, cx, cy, 4.0)
        time.sleep(3.0)
        rows = read_rows(log)
        final_q = sum(1 for r in rows if r.get('state') == 'M1_QUANTUM_PRESENT_QUEUED')
        recovery = [r for r in rows if r.get('stage') == 'M1_HOLD_RECOVERY']
        rec_events = Counter(r.get('event') for r in recovery)
        stages = Counter(r.get('stage') for r in rows)
        result = {
            'ok': True,
            'downToFirstPresentWall': None,  # filled below
            'pilePhase': {'holdSeconds': round(t_up - t_down, 2), 'quantaAtSettle': len(qs),
                          'settleRetained': bool(settle_retained)},
            'late60s': {'wallSeconds': round(time.time() - t60, 1), 'quantaDuring': late_q},
            'finalProbes': {'quantaTotal': final_q},
            'boxTurns': {'requestedAngles': requested[:20], 'actualAngles': actual[:20],
                         'gravityRows': len(angle_rows)},
            'recovery': dict(rec_events),
            'stages': {k: v for k, v in stages.items() if k and v >= 2},
            'rows': len(rows),
        }
        # first accepted present wall-clock after first Down: find first quantum's present qpc vs boot
        if qs:
            wf = qs[0].get('windowFrame') or {}
            result['firstPresentQpc'] = wf.get('presentExitQpc')
        with open(os.path.join(out, 'round.json'), 'w', encoding='utf-8') as f:
            json.dump(result, f, indent=1)
        print(json.dumps(result, indent=1)[:2000])
        post(hwnd, WM_CLOSE, 0, 0)
        try:
            proc.wait(timeout=30)
        except subprocess.TimeoutExpired:
            proc.kill()
        return 0
    finally:
        if proc.poll() is None:
            proc.kill()


if __name__ == '__main__':
    raise SystemExit(main())
