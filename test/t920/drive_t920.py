"""T920 compact demo driver: hold to pour, verify 2-4s visible pile, rotate box, reset.

Scenarios on t918 PE (--m1-compact-demo) and Baseline profile:
  pile:   hold 4s inside box -> settle 6s (admission + ROI + LAND evidence)
  rotate: after pile, LEFT x6 -> RIGHT x8 (boundary reverse) -> R key -> button
"""
import argparse
import ctypes
import ctypes.wintypes as wt
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
MK_LBUTTON = 0x0001
VK_LEFT = 0x25
VK_RIGHT = 0x27
VK_HOME = 0x24
WM_APP = 0x8000


def post_key(hwnd, vk, char=None):
    ctypes.windll.user32.PostMessageW(hwnd, WM_KEYDOWN, vk, 0)
    if char is not None:
        ctypes.windll.user32.PostMessageW(hwnd, WM_CHAR, char, 0)
    ctypes.windll.user32.PostMessageW(hwnd, WM_KEYUP, vk, 0)


def lp_of(x, y):
    return (y & 0xFFFF) << 16 | (x & 0xFFFF)


def hold(hwnd, cx, cy, seconds):
    lp = lp_of(cx, cy)
    ctypes.windll.user32.PostMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lp)
    steps = max(1, int(seconds / 0.2))
    for _ in range(steps):
        time.sleep(0.2)
        ctypes.windll.user32.PostMessageW(hwnd, WM_MOUSEMOVE, MK_LBUTTON, lp)
    ctypes.windll.user32.PostMessageW(hwnd, WM_LBUTTONUP, 0, lp)


def button_click(hwnd, w):
    lp = lp_of(w - 25, 23)
    ctypes.windll.user32.PostMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lp)
    time.sleep(0.05)
    ctypes.windll.user32.PostMessageW(hwnd, WM_LBUTTONUP, 0, lp)


def find_window(path, timeout=40):
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
        time.sleep(0.2)
    return None, None


def wait_stage(path, stage, timeout):
    deadline = time.time() + timeout
    seen = 0
    while time.time() < deadline:
        try:
            with open(path, 'rb') as handle:
                lines = handle.read().splitlines()
        except OSError:
            time.sleep(0.1)
            continue
        for line in lines[seen:]:
            try:
                row = json.loads(line)
            except json.JSONDecodeError:
                continue
            if row.get('stage') == stage:
                return True
        seen = len(lines)
        time.sleep(0.1)
    return False


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True)
    parser.add_argument('--shaders', required=True)
    parser.add_argument('--out', required=True)
    parser.add_argument('--scenario', choices=('pile', 'rotate'), required=True)
    parser.add_argument('--extra', nargs='*', default=[])
    parser.add_argument('--observe-request')
    parser.add_argument('--observe-prefix')
    parser.add_argument('--observe-single')
    parser.add_argument('--post-idle', type=float, default=6.0)
    args = parser.parse_args()

    os.makedirs(args.out, exist_ok=True)
    log = os.path.join(args.out, 'demo.jsonl')
    cmd = [args.exe, '--m1-window',
           os.path.join(args.shaders, 'full_source.spv'),
           os.path.join(args.shaders, 'quantum_candidate.spv'),
           os.path.join(args.shaders, 'quantum_commit.spv'),
           '--m1-one-quantum-grant', '--m1-source-trace', '--m1-dual-source']
    if args.observe_request:
        cmd += ['--m1-observe-series', args.observe_request, args.observe_prefix]
    if args.observe_single:
        cmd += ['--m1-observe', args.observe_single, args.observe_prefix]
    cmd += args.extra
    with open(os.path.join(args.out, 'argv.json'), 'w', encoding='utf-8') as f:
        json.dump({'command': cmd, 'scenario': args.scenario}, f, indent=2)
    with open(log, 'wb') as sink:
        proc = subprocess.Popen(cmd, stdout=sink, stderr=subprocess.STDOUT)
    try:
        hwnd, size = find_window(log, 40)
        if not hwnd:
            print('no window')
            return 4
        if not wait_stage(log, 'M1_POINTER_READY', 180):
            print('pointer not ready')
            return 5
        w, h = size
        if args.scenario == 'pile':
            hold(hwnd, w // 2, h // 2, 4.0)
            time.sleep(args.post_idle)
        else:
            hold(hwnd, w // 2, h // 2, 4.0)
            time.sleep(3.0)
            for _ in range(6):
                post_key(hwnd, VK_LEFT)
                time.sleep(0.8)
            for _ in range(8):
                post_key(hwnd, VK_RIGHT)
                time.sleep(0.8)
            post_key(hwnd, 0x52, char=0x52)
            time.sleep(1.5)
            button_click(hwnd, w)
            time.sleep(2.0)
        time.sleep(1.0)
        ctypes.windll.user32.PostMessageW(hwnd, 0x0010, 0, 0)
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
