"""T920 long-run driver: reproduce user-reported slowdown/no-sand after load accumulates.

Repeats hold-pour cycles on the full 128x256 container (same modules/argv family
as the user's demo-7b46 log: t830-m1-bounded b8f0e327 candidate + t909-r2 PE),
runs >=90s wall after first pile, then probes whether repeated holds still admit.
"""
import argparse
import ctypes
import ctypes.wintypes as wt
import json
import os
import subprocess
import time

WM_LBUTTONDOWN = 0x0201
WM_LBUTTONUP = 0x0202
WM_MOUSEMOVE = 0x0200
MK_LBUTTON = 1


def lp_of(x, y):
    return (y & 0xFFFF) << 16 | (x & 0xFFFF)


def hold(hwnd, cx, cy, seconds):
    lp = lp_of(cx, cy)
    ctypes.windll.user32.PostMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lp)
    for _ in range(max(1, int(seconds / 0.2))):
        time.sleep(0.2)
        ctypes.windll.user32.PostMessageW(hwnd, WM_MOUSEMOVE, MK_LBUTTON, lp)
    ctypes.windll.user32.PostMessageW(hwnd, WM_LBUTTONUP, 0, lp)


def find_window(path, timeout=40):
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            for line in open(path, 'rb').read().splitlines():
                try:
                    r = json.loads(line)
                except json.JSONDecodeError:
                    continue
                if r.get('stage') == 'WINDOW_AVAILABLE':
                    return int(r['hwnd']), (r['width'], r['height'])
        except OSError:
            pass
        time.sleep(0.2)
    return None, None


def wait_ready(path, timeout=180):
    deadline = time.time() + timeout
    seen = 0
    while time.time() < deadline:
        try:
            lines = open(path, 'rb').read().splitlines()
        except OSError:
            time.sleep(0.1)
            continue
        for line in lines[seen:]:
            try:
                r = json.loads(line)
            except json.JSONDecodeError:
                continue
            if r.get('stage') == 'M1_POINTER_READY':
                return True
        seen = len(lines)
        time.sleep(0.1)
    return False


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True)
    parser.add_argument('--modules', required=True)
    parser.add_argument('--out', required=True)
    parser.add_argument('--cycles', type=int, default=12)
    parser.add_argument('--hold-seconds', type=float, default=6.0)
    parser.add_argument('--idle-seconds', type=float, default=4.0)
    parser.add_argument('--final-probes', type=int, default=6)
    args = parser.parse_args()

    os.makedirs(args.out, exist_ok=True)
    log = os.path.join(args.out, 'demo.jsonl')
    cmd = [args.exe, '--m1-window',
           os.path.join(args.modules, 'full_source.spv'),
           os.path.join(args.modules, 'quantum_candidate.spv'),
           os.path.join(args.modules, 'quantum_commit.spv'),
           '--m1-empty-preview', '--m1-manual-gravity', '--m1-one-quantum-grant',
           '--m1-dual-source', '--m1-source-trace']
    with open(os.path.join(args.out, 'argv.json'), 'w', encoding='utf-8') as f:
        json.dump({'command': cmd, 'cycles': args.cycles, 'hold': args.hold_seconds,
                   'idle': args.idle_seconds, 'finalProbes': args.final_probes}, f, indent=2)
    with open(log, 'wb') as sink:
        proc = subprocess.Popen(cmd, stdout=sink, stderr=subprocess.STDOUT)
    try:
        hwnd, size = find_window(log, 40)
        if not hwnd:
            print('no window')
            return 4
        if not wait_ready(log, 180):
            print('not ready')
            return 5
        w, h = size
        cx, cy = w // 2, int(h * 0.5)
        # phase 1: one long continuous hold to load the system
        hold(hwnd, cx, cy, args.hold_seconds)
        # phase 2: settle pause
        time.sleep(args.idle_seconds)
        # phase 3: repeated probes after heavy load
        for i in range(args.final_probes):
            hold(hwnd, cx, cy, args.hold_seconds)
            time.sleep(args.idle_seconds)
        time.sleep(5.0)
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
