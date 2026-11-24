"""T912 interactive driver: run hello_world_pc with T909 flags and inject real input.

Scenarios (each writes its own directory with argv.json/demo.jsonl):
  1-gravity: empty-bed gravity arrows (R button, R/Home keys, arrow keys)
  2-sand:    long-hold sand admission + observation series + bottom ROI
  2-water:   water default vs kappa-zero observation series
"""
import argparse
import ctypes
import ctypes.wintypes as wt
import json
import os
import subprocess
import sys
import time

INPUT_MOUSE = 0
MOUSEEVENTF_MOVE = 0x0001
MOUSEEVENTF_LEFTDOWN = 0x0002
MOUSEEVENTF_LEFTUP = 0x0004
MOUSEEVENTF_ABSOLUTE = 0x8000
WM_CLOSE = 0x0010
WM_KEYDOWN = 0x0100
WM_KEYUP = 0x0101
WM_CHAR = 0x0102
WM_LBUTTONDOWN = 0x0201
WM_LBUTTONUP = 0x0202
MK_LBUTTON = 0x0001
VK_LEFT = 0x25
VK_RIGHT = 0x27
VK_HOME = 0x24
WM_APP = 0x8000
K_SET_GRAVITY = WM_APP + 3


class MOUSEINPUT(ctypes.Structure):
    _fields_ = [("dx", wt.LONG), ("dy", wt.LONG), ("mouseData", wt.DWORD),
                ("dwFlags", wt.DWORD), ("time", wt.DWORD),
                ("dwExtraInfo", ctypes.POINTER(wt.ULONG))]


class RECT(ctypes.Structure):
    _fields_ = [("left", wt.LONG), ("top", wt.LONG), ("right", wt.LONG), ("bottom", wt.LONG)]


class INPUT(ctypes.Structure):
    _fields_ = [("type", wt.DWORD), ("mi", MOUSEINPUT)]


def send_mouse(flags, x, y):
    item = INPUT(type=INPUT_MOUSE)
    item.mi.dx = x
    item.mi.dy = y
    item.mi.dwFlags = flags | MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE
    ctypes.windll.user32.SendInput(1, ctypes.byref(item), ctypes.sizeof(INPUT))


def client_to_screen(hwnd, cx, cy):
    pt = wt.POINT(cx, cy)
    ctypes.windll.user32.ClientToScreen(hwnd, ctypes.byref(pt))
    return pt.x, pt.y


def absolute_of(x, y):
    w = ctypes.windll.user32.GetSystemMetrics(0)
    h = ctypes.windll.user32.GetSystemMetrics(1)
    return (int(65535 * max(0, min(w - 1, x)) / max(1, w - 1)),
            int(65535 * max(0, min(h - 1, y)) / max(1, h - 1)))


def real_click_hold(hwnd, cx, cy, seconds):
    """Hold via PostMessage: robust to focus/z-order; sends MOVE ticks like a real drag."""
    lp = (cy & 0xFFFF) << 16 | (cx & 0xFFFF)
    ctypes.windll.user32.PostMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lp)
    steps = max(1, int(seconds / 0.2))
    for _ in range(steps):
        time.sleep(0.2)
        ctypes.windll.user32.PostMessageW(hwnd, 0x0200, MK_LBUTTON, lp)  # WM_MOUSEMOVE
    ctypes.windll.user32.PostMessageW(hwnd, WM_LBUTTONUP, 0, lp)


def post_key(hwnd, vk, char=None):
    ctypes.windll.user32.PostMessageW(hwnd, WM_KEYDOWN, vk, 0)
    if char is not None:
        ctypes.windll.user32.PostMessageW(hwnd, WM_CHAR, char, 0)
    ctypes.windll.user32.PostMessageW(hwnd, WM_KEYUP, vk, 0)


def post_button_click(hwnd, cx, cy):
    lp = (cy & 0xFFFF) << 16 | (cx & 0xFFFF)
    ctypes.windll.user32.PostMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lp)
    time.sleep(0.05)
    ctypes.windll.user32.PostMessageW(hwnd, WM_LBUTTONUP, 0, lp)


def post_gravity(hwnd, angle256):
    lp = int(angle256) & 0xFFFFFFFF
    ctypes.windll.user32.PostMessageW(hwnd, K_SET_GRAVITY, angle256 & 0xFFFFFFFF, lp)


def wait_line(path, want, timeout, state=True):
    deadline = time.time() + timeout
    seen = 0
    while time.time() < deadline:
        try:
            with open(path, 'rb') as handle:
                lines = handle.read().splitlines()
        except OSError:
            time.sleep(0.05)
            continue
        for line in lines[seen:]:
            try:
                row = json.loads(line)
            except json.JSONDecodeError:
                continue
            if (state and row.get('state') == want) or (not state and row.get('stage') == want):
                return True
        seen = len(lines)
        time.sleep(0.05)
    return False


def find_window(path, timeout=30):
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with open(path, 'rb') as handle:
                for line in handle.read().splitlines():
                    try:
                        row = json.loads(line)
                    except json.JSONDecodeError:
                        continue
                    if row.get('stage') == 'WINDOW_AVAILABLE':
                        return int(row['hwnd']), (row['width'], row['height'])
        except OSError:
            pass
        time.sleep(0.05)
    return None, None


def wait_pointer_ready(path, timeout=120):
    return wait_line(path, 'M1_POINTER_READY', timeout, state=False)


def gravity_scenario(hwnd, size, out_path):
    """Scenario 1: empty bed (fresh run), no sand: test arrow, reset paths.

    Real user paths: R key, Home key, arrow keys, on-screen R button click.
    Also verify the button click does not emit sand input (no WIN32_POINTER DOWN).
    """
    w, h = size
    time.sleep(1.5)  # let the empty preview present at least one frame
    # 1) keyboard R -> absolute 0 (already 0 on fresh run; verify receipt)
    post_key(hwnd, 0x52, char=0x52)  # 'R'
    time.sleep(1.0)
    # 2) arrow right -> relative +15 deg
    post_key(hwnd, VK_RIGHT)
    time.sleep(1.5)
    # 3) arrow right again -> +30 deg request
    post_key(hwnd, VK_RIGHT)
    time.sleep(1.5)
    # 4) R key resets to 0
    post_key(hwnd, 0x52, char=0x52)
    time.sleep(1.5)
    # 5) Home key also resets
    post_key(hwnd, VK_HOME)
    time.sleep(1.0)
    # 6) arrow left twice -> -30 deg
    post_key(hwnd, VK_LEFT)
    time.sleep(1.0)
    post_key(hwnd, VK_LEFT)
    time.sleep(1.5)
    # 7) on-screen R button: client coords (w-25, 23) center of [w-42,w-8)x[8,38)
    bx, by = w - 25, 23
    post_button_click(hwnd, bx, by)
    time.sleep(1.5)
    # 8) another button click to confirm repeated reset works
    post_button_click(hwnd, bx, by)
    time.sleep(1.0)


def hold_scenario(hwnd, size, out_path, seconds, center=True, cx=392, cy=380):
    """Long-hold sand/water admission at a fixed client point."""
    real_click_hold(hwnd, cx, cy, seconds)


def hold_gravity_scenario(hwnd, size, cx, cy):
    """Hold to build a bed, then send gravity inputs during the hold and after UP.

    Sequence: hold 6s -> +15 -> +30 (rel) -> R key reset -> UP -> button reset.
    Exercises constrained vs mobile bed and both reset paths with a real bed.
    """
    sx, sy = client_to_screen(hwnd, cx, cy)
    nx, ny = absolute_of(sx, sy)
    send_mouse(MOUSEEVENTF_LEFTDOWN, nx, ny)
    time.sleep(6.0)
    post_key(hwnd, VK_RIGHT)          # +15 request during hold
    time.sleep(3.0)
    post_key(hwnd, VK_RIGHT)          # +30 request during hold
    time.sleep(3.0)
    post_key(hwnd, 0x52, char=0x52)   # R reset during hold
    time.sleep(3.0)
    send_mouse(MOUSEEVENTF_LEFTUP, nx, ny)
    time.sleep(4.0)
    # after UP: try arrow again on the settled bed
    post_key(hwnd, VK_RIGHT)
    time.sleep(3.0)
    # on-screen R button reset
    w, h = size
    post_button_click(hwnd, w - 25, 23)
    time.sleep(2.0)


def hold_material_scenario(hwnd, cx, cy, material_key, seconds):
    """Select material (1=sand, 2=water), then hold to admit it."""
    post_key(hwnd, material_key, char=material_key)
    time.sleep(0.5)
    real_click_hold(hwnd, cx, cy, seconds)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True)
    parser.add_argument('--modules', required=True)
    parser.add_argument('--candidate', default=None,
                        help='override quantum candidate spv (e.g. inventory v1)')
    parser.add_argument('--out', required=True)
    parser.add_argument('--scenario', required=True,
                        choices=('gravity', 'hold', 'hold-gravity', 'hold-material'))
    parser.add_argument('--hold-seconds', type=float, default=8.0)
    parser.add_argument('--hold-x', type=int, default=392)
    parser.add_argument('--hold-y', type=int, default=380)
    parser.add_argument('--observe-request', default=None)
    parser.add_argument('--observe-prefix', default=None)
    parser.add_argument('--kappa-zero', action='store_true')
    parser.add_argument('--stage-timing', action='store_true')
    parser.add_argument('--inventory-v1', action='store_true')
    parser.add_argument('--extra', nargs='*', default=[])
    parser.add_argument('--post-idle', type=float, default=6.0)
    parser.add_argument('--material', default='2', help='1=sand, 2=water')
    parser.add_argument('--startup-timeout', type=float, default=180.0)
    args = parser.parse_args()

    os.makedirs(args.out, exist_ok=True)
    log_path = os.path.join(args.out, 'demo.jsonl')
    command = [args.exe, '--m1-window',
               os.path.join(args.modules, 'full_source.spv'),
               args.candidate or os.path.join(args.modules, 'quantum_candidate.spv'),
               os.path.join(args.modules, 'quantum_commit.spv'),
               '--m1-empty-preview', '--m1-manual-gravity', '--m1-one-quantum-grant',
               '--m1-dual-source']
    if args.observe_request:
        command += ['--m1-observe-series', args.observe_request, args.observe_prefix]
    if args.kappa_zero:
        command += ['--m1-observe-kappa-zero']
    if args.stage_timing:
        command += ['--m1-stage-timing']
    if args.inventory_v1:
        command += ['--m1-inventory-v1']
    command += args.extra
    with open(os.path.join(args.out, 'argv.json'), 'w', encoding='utf-8') as manifest:
        json.dump({'command': command, 'scenario': args.scenario}, manifest, indent=2)
    with open(log_path, 'wb') as out:
        process = subprocess.Popen(command, stdout=out, stderr=subprocess.STDOUT)
    try:
        hwnd, size = find_window(log_path, 30)
        if hwnd is None:
            print('no window', file=sys.stderr)
            return 4
        if not wait_pointer_ready(log_path, args.startup_timeout):
            print('pointer not ready', file=sys.stderr)
            return 5
        if args.scenario == 'gravity':
            gravity_scenario(hwnd, size, log_path)
        elif args.scenario == 'hold-gravity':
            hold_gravity_scenario(hwnd, size, args.hold_x, args.hold_y)
            time.sleep(args.post_idle)
        elif args.scenario == 'hold-material':
            hold_material_scenario(hwnd, args.hold_x, args.hold_y,
                                   ord(args.material or '2'), args.hold_seconds)
            time.sleep(args.post_idle)
        else:
            hold_scenario(hwnd, size, log_path, args.hold_seconds,
                          cx=args.hold_x, cy=args.hold_y)
            time.sleep(args.post_idle)
        # graceful close after terminal states settle
        time.sleep(1.0)
        ctypes.windll.user32.PostMessageW(hwnd, WM_CLOSE, 0, 0)
        try:
            process.wait(timeout=30)
        except subprocess.TimeoutExpired:
            process.kill()
        return 0
    finally:
        if process.poll() is None:
            process.kill()


if __name__ == '__main__':
    raise SystemExit(main())
