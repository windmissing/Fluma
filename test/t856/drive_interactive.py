"""T856 evidence collector: run the interactive PC PE and inject a real hold.

Starts hello_world_pc with --m1-window + --m1-cold, waits for M1_POINTER_READY,
sends WM_LBUTTONDOWN / mouse moves / WM_LBUTTONUP through SendInput, and saves
stdout to a JSONL file. A keyboard shortcut closes the window when finished.
"""
import argparse
import ctypes
import ctypes.wintypes as wt
import json
import math
import subprocess
import sys
import threading
import time

INPUT_MOUSE = 0
MOUSEEVENTF_MOVE = 0x0001
MOUSEEVENTF_LEFTDOWN = 0x0002
MOUSEEVENTF_LEFTUP = 0x0004
WM_CLOSE = 0x0010
INPUT_KEYBOARD = 1
KEYEVENTF_KEYUP = 0x0002
VK_ESCAPE = 0x1B
MOUSEEVENTF_ABSOLUTE = 0x8000


class MOUSEINPUT(ctypes.Structure):
    _fields_ = [("dx", wt.LONG), ("dy", wt.LONG), ("mouseData", wt.DWORD),
                ("dwFlags", wt.DWORD), ("time", wt.DWORD), ("dwExtraInfo", ctypes.POINTER(wt.ULONG))]


class _I(ctypes.Union):
    _fields_ = [("mi", MOUSEINPUT)]


class INPUT(ctypes.Structure):
    _fields_ = [("type", wt.DWORD), ("union", _I)]


class RECT(ctypes.Structure):
    _fields_ = [("left", wt.LONG), ("top", wt.LONG), ("right", wt.LONG), ("bottom", wt.LONG)]


def send_mouse(flags, x, y):
    item = INPUT(type=INPUT_MOUSE)
    item.union.mi.dx = x
    item.union.mi.dy = y
    item.union.mi.dwFlags = flags | MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE
    item.union.mi.mouseData = 0
    ctypes.windll.user32.SendInput(1, ctypes.byref(item), ctypes.sizeof(INPUT))


def client_center(hwnd):
    rect = RECT()
    ctypes.windll.user32.GetClientRect(hwnd, ctypes.byref(rect))
    pt = wt.POINT((rect.right - rect.left) // 2, (rect.bottom - rect.top) // 2)
    ctypes.windll.user32.ClientToScreen(hwnd, ctypes.byref(pt))
    screen_w = ctypes.windll.user32.GetSystemMetrics(0)
    screen_h = ctypes.windll.user32.GetSystemMetrics(1)
    nx = int(65535 * max(0, min(screen_w - 1, pt.x)) / max(1, screen_w - 1))
    ny = int(65535 * max(0, min(screen_h - 1, pt.y)) / max(1, screen_h - 1))
    return pt, nx, ny


def click_at(hwnd, hold_seconds):
    _, nx, ny = client_center(hwnd)
    send_mouse(MOUSEEVENTF_LEFTDOWN, nx, ny)
    time.sleep(hold_seconds)
    send_mouse(MOUSEEVENTF_LEFTUP, nx, ny)


def post_close(hwnd):
    ctypes.windll.user32.PostMessageW(hwnd, WM_CLOSE, 0, 0)


def wait_stage(path, stage, timeout):
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
            if row.get('stage') == stage:
                return True
        seen = len(lines)
        time.sleep(0.05)
    return False


def find_hwnd(path, timeout):
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True)
    parser.add_argument('--spv-dir', required=True)
    parser.add_argument('--cold-file', required=True)
    parser.add_argument('--out', required=True)
    parser.add_argument('--hold-seconds', type=float, default=6.0)
    parser.add_argument('--startup-timeout', type=float, default=120.0)
    args = parser.parse_args()

    command = [args.exe, '--m1-window',
               f'{args.spv_dir}/full_source.spv',
               f'{args.spv_dir}/quantum_candidate.spv',
               f'{args.spv_dir}/quantum_commit.spv',
               '--m1-cold', args.cold_file]
    with open(args.out, 'wb') as out:
        process = subprocess.Popen(command, stdout=out, stderr=subprocess.STDOUT)
    try:
        hwnd, _ = find_hwnd(args.out, 30)
        if hwnd is None:
            print('no window found', file=sys.stderr)
            return 4
        if not wait_stage(args.out, 'M1_POINTER_READY', args.startup_timeout):
            print('pointer never ready', file=sys.stderr)
            return 5
        click_at(hwnd, args.hold_seconds)
        # After the up event keep the window alive briefly so the terminal
        # receipt (retired / present) can be emitted, then close the window.
        ok = wait_stage(args.out, 'M1_QUANTUM_COLD_CAPTURE', 60)
        print('cold capture seen:', ok, file=sys.stderr)
        # Continue to wait for present retirement; the window stays interactive.
        retired = wait_stage_state(args.out, 'M1_QUANTUM_PRESENT_RETIRED', 120)
        print('retired seen:', retired, file=sys.stderr)
        post_close(hwnd)
        try:
            process.wait(timeout=60)
        except subprocess.TimeoutExpired:
            process.kill()
        return 0
    finally:
        if process.poll() is None:
            process.kill()


def wait_stage_state(path, state, timeout):
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
            if row.get('state') == state:
                return True
        seen = len(lines)
        time.sleep(0.05)
    return False


if __name__ == '__main__':
    raise SystemExit(main())
