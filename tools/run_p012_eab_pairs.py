#!/usr/bin/env python3
"""P-012 EAB-batch: 3 OFF/ON paired blocks + bootstrap replicates (s96).

DoD (EAB-batch §9.3):
  (2) 3 paired OFF/ON blocks, each with the SAME physical step sequence
      (fresh launch, seed 0, testmode, frozen 25s settle) -- only the
      appearance tier differs (0 vs 3).
  (3) same-physical-step on/off consistency = within-block OFF/ON snapshot
      comparison (read-only).
  Bootstrap replicates: the 3 blocks are the replicate set for the
  lumStd/hfH interval (tools/bootstrap_p012_anchors.py).

Per block:
  force-stop -> launch (rendermode 4, p012tier 0, seed 0, testmode true)
  -> dismiss USB dialog (240,620) -> settle 25s -> snap OFF
  -> open sheet (1100,230) -> appearance x3 (640,2163) -> dismiss (640,700)
  -> settle 8s -> snap ON -> dump hilog tail.

Outputs to test/artifacts/p012-measurements/:
  eab-<TAG>-pair{N}-off-w{0,1,2}.jpeg  (FPS window, same physical step sequence)
  eab-<TAG>-pair{N}-eab-w{0,1,2}.jpeg
  eab-<TAG>-hilog.txt (last block)
"""
import subprocess, sys, time, os

HDC = r"C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe"
OUT = r"D:\hmprojects\HelloWorld\test\artifacts\p012-measurements"
PAIRS = int(os.environ.get("P012_PAIRS", "3"))
TAG = os.environ.get("P012_TAG", "s96")

def sh(cmd, timeout=120):
    r = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=timeout)
    return (r.stdout or "") + (r.stderr or "")

def snap(local):
    sh(f'"{HDC}" shell "snapshot_display -f /data/local/tmp/x.jpeg"', timeout=60)
    sh(f'"{HDC}" file recv /data/local/tmp/x.jpeg "{local}"', timeout=60)
    ok = os.path.exists(local) and os.path.getsize(local) > 1000
    print("  snap ->", os.path.basename(local), os.path.getsize(local) if os.path.exists(local) else "MISSING",
          "bytes", "OK" if ok else "FAIL", flush=True)
    return ok

def snap_window(tag, n=3, gap=2.0):
    """FPS window: n snapshots `gap` seconds apart (FPS text read offline)."""
    oks = []
    for i in range(n):
        oks.append(snap(os.path.join(OUT, f"eab-{TAG}-{tag}-w{i}.jpeg")))
        if i < n - 1:
            time.sleep(gap)
    return all(oks)

def tap(x, y, settle=1.2):
    sh(f'"{HDC}" shell uitest uiInput click {x} {y}')
    time.sleep(settle)

def one_pair(n):
    print(f"=== pair {n}: launch OFF ===", flush=True)
    sh(f'"{HDC}" shell aa force-stop com.example.helloworld')
    time.sleep(1.5)
    out = sh(f'"{HDC}" shell aa start -a EntryAbility -b com.example.helloworld '
             f'--ps rendermode 4 --ps p012tier 0 --ps p012seed 0 --ps testmode true')
    print("  launch:", out.strip()[:160], flush=True)
    time.sleep(2.0)
    tap(240, 620, settle=0.8)            # dismiss USB dialog if present
    print("  settling 25s (frozen off-ws0 settle) ...", flush=True)
    time.sleep(25.0)
    if not snap_window(f"pair{n}-off"):
        return False
    print(f"=== pair {n}: toggle to EAB ===", flush=True)
    tap(1100, 230, settle=1.5)           # open tune sheet
    for _ in range(3):
        tap(640, 2163, settle=1.0)       # appearance: 0->1->2->3
    tap(640, 700, settle=1.0)            # dismiss sheet
    print("  settling 8s ...", flush=True)
    time.sleep(8.0)
    if not snap_window(f"pair{n}-eab"):
        return False
    return True

def main():
    os.makedirs(OUT, exist_ok=True)
    sh(f'"{HDC}" shell power-shell timeout -o 600000')
    for n in range(1, PAIRS + 1):
        if not one_pair(n):
            print(f"pair {n} FAILED (snapshot)", flush=True)
            sys.exit(1)
    log = sh(f'"{HDC}" shell "hilog -x -T P012"', timeout=60)
    with open(os.path.join(OUT, f"eab-{TAG}-hilog.txt"), "w", encoding="utf-8") as f:
        f.write(log)
    print("hilog lines:", len(log.splitlines()), flush=True)
    sh(f'"{HDC}" shell power-shell timeout -r')
    print("DONE", flush=True)

if __name__ == "__main__":
    main()
