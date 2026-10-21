#!/usr/bin/env python3
"""P-012 EAB perf ladder: FPS at matched fill phase (25s settle) for
  tier 0 (legacy OFF), tier 1 (E), tier 2 (EA), tier 3 (EAB).
Each: force-stop -> launch -> 25s settle -> 3 snapshots 2s apart (FPS text).
Outputs: test/artifacts/p012-measurements/perf-ladder-<TAG>-<tag>-w{i}.jpeg

s97 起渲染器已移除 s94 诊断消融开关（renderSeed 999/998/997），颗粒/雾/亮粒
的逐项成本隔离改由 perf-fix 任务里的新消融机制负责，本脚本不再保留旧变体。
"""
import subprocess, sys, time, os

HDC = r"C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe"
OUT = r"D:\hmprojects\HelloWorld\test\artifacts\p012-measurements"
TAG = os.environ.get("P012_TAG", "s96")
# 窗口数与间隔：DoD① 要求 ≥3 帧窗口并给出 P50/P95 帧时，默认 6 窗口 × 1.0s。
WINDOWS = int(os.environ.get("P012_WINDOWS", "6"))
GAP = float(os.environ.get("P012_GAP", "1.0"))

VARIANTS = [
    ("tier0", 0, 0), ("tier1", 1, 0), ("tier2", 2, 0), ("tier3", 3, 0),
]
# 诊断用：只跑指定档位（P012_TIERS="tier2,tier3"）
_only = os.environ.get("P012_TIERS", "")
if _only:
    _keep = set(x.strip() for x in _only.split(",") if x.strip())
    VARIANTS = [v for v in VARIANTS if v[0] in _keep]

def sh(cmd, timeout=120):
    r = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=timeout)
    return (r.stdout or "") + (r.stderr or "")

def snap(local):
    sh(f'"{HDC}" shell "snapshot_display -f /data/local/tmp/x.jpeg"', timeout=60)
    sh(f'"{HDC}" file recv /data/local/tmp/x.jpeg "{local}"', timeout=60)
    ok = os.path.exists(local) and os.path.getsize(local) > 1000
    print("  snap ->", os.path.basename(local), "OK" if ok else "FAIL", flush=True)
    return ok

def main():
    os.makedirs(OUT, exist_ok=True)
    sh(f'"{HDC}" shell power-shell timeout -o 600000')
    for tag, tier, seed in VARIANTS:
        print(f"=== {tag} (tier={tier} seed={seed}) ===", flush=True)
        sh(f'"{HDC}" shell aa force-stop com.example.helloworld')
        time.sleep(1.5)
        sh(f'"{HDC}" shell aa start -a EntryAbility -b com.example.helloworld '
           f'--ps rendermode 4 --ps p012tier {tier} --ps p012seed {seed} --ps testmode true')
        time.sleep(2.0)
        sh(f'"{HDC}" shell uitest uiInput click 240 620')   # dismiss USB dialog
        time.sleep(25.0)
        for i in range(WINDOWS):
            if not snap(os.path.join(OUT, f"perf-ladder-{TAG}-{tag}-w{i}.jpeg")):
                print(f"{tag} snapshot FAILED", flush=True)
            if i < WINDOWS - 1:
                time.sleep(GAP)
    sh(f'"{HDC}" shell power-shell timeout -r')
    print("DONE", flush=True)

if __name__ == "__main__":
    main()
