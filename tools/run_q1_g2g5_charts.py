#!/usr/bin/env python3
"""P-012 Q1 G2-G5 visualization preprocessing (tester, offline only).

Turns the already-measured q1-revision-run-v2.json data into designer/manager-
readable comparison charts for the Q1 three-way ruling record. PURE
VISUALIZATION: no new measurements, no JSON modification, descriptive
annotations only (never CI, never acceptance verdicts).

Charts (PNG, 200 dpi, into test/artifacts/p012-measurements/):
  g2_tier_comparison.png      - 200px vs 100px per source (reference + device),
                                ratio 100/200 annotated; tiers NOT mixed
  g3_encoding_sensitivity.png - hfH shift at Q75/Q90 re-encode per source
  g5_autocorr_curves.png      - lag 1..16 autocorrelation, reference vs device
  g5_spectrum_compare.png     - normalized spectrum, frequency in cycles per
                                container inner width

Every chart carries: data source file + sha256 prefix, caliber note
(hfH-display-v2-p1 @200/100px, descriptive only), and generation timestamp.
"""
import hashlib
import json
import time
from pathlib import Path

import textwrap

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ART = Path(r"D:/hmprojects/HelloWorld/test/artifacts/p012-measurements")
SRC = ART / "q1-revision-run-v2.json"
CALIBER = "hfH-display-v2-p1 (single-pass area resample, mask-first, FROZEN v2.0)"
NOTE = "descriptive only - NOT a confidence interval, NOT an acceptance verdict"


def sha16(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()[:16]


def stamp(fig):
    fig.text(0.99, 0.01,
             f"source: {SRC.name} (sha16 {sha16(SRC)}) | {CALIBER} | {NOTE} | "
             f"generated {time.strftime('%Y-%m-%d %H:%M')}",
             ha="right", va="bottom", fontsize=6, color="#555555")


def main():
    data = json.loads(SRC.read_text(encoding="utf-8"))
    groups = data["groups"]
    ref = data["reference"]
    comp = data["comparison"]

    # ---------------- G2: tier comparison ----------------
    fig, ax = plt.subplots(figsize=(10, 5.5))
    labels, v200, v100, colors = [], [], [], []
    for f in ("video1_frame0.jpg", "video1_frame5.jpg",
              "video1_frame2.jpg", "video1_frame6.jpg"):
        g = groups["G2"]["reference"][f]
        labels.append(f.replace("video1_", "").replace(".jpg", "")
                      + ("\n(static)" if g["state"] == "static" else "\n(dynamic)"))
        v200.append(g["primary200"])
        v100.append(g["sensitivity100"])
        colors.append("#4477aa")
    for key, arm_color in (("s99w-pair1-off", "#6688aa"), ("s99w-pair1-eab", "#aa4444")):
        c = comp[key]
        labels.append(key.replace("s99w-", "") + "\n(device)")
        v200.append(c["displayPrimary"]["hfH"])
        v100.append(c["displaySensitivity"]["hfH"])
        colors.append(arm_color)
    x = range(len(labels))
    w = 0.38
    ax.bar([i - w / 2 for i in x], v200, w, label="primary 200px", color="#4477aa")
    ax.bar([i + w / 2 for i in x], v100, w, label="sensitivity 100px (direct)",
           color="#ee7733")
    ymax = max(max(v200), max(v100))
    ax.set_ylim(0, ymax * 1.18)
    for i in x:
        ratio = v100[i] / v200[i] if v200[i] else 0
        ax.text(i, max(v200[i], v100[i]) + ymax * 0.02, f"×{ratio:.2f}",
                ha="center", fontsize=9, color="#333333")
    ax.set_xticks(list(x))
    ax.set_xticklabels(labels, fontsize=9)
    ax.set_ylabel("hfH")
    ax.set_title("G2 scale sensitivity: 200px primary vs direct-100px "
                 "(tiers reported separately, never mixed)")
    ax.legend()
    ax.grid(axis="y", alpha=0.3)
    note = ("reference direct-to-100px ratio ×1.48–1.75 vs device ×0.61–0.63: "
            "opposite direction = support-structure difference (69-70 ref rows "
            "vs 3 device rows), NOT a scale-robustness conclusion")
    ax.text(0.02, 0.97, "\n".join(textwrap.wrap(note, 56)),
           transform=ax.transAxes, fontsize=7,
            verticalalignment="top", bbox=dict(boxstyle="round", fc="#fffbe6"))
    stamp(fig)
    fig.tight_layout(rect=(0, 0.04, 1, 1))
    fig.savefig(ART / "g2_tier_comparison.png", dpi=200)
    plt.close(fig)

    # ---------------- G3: encoding sensitivity ----------------
    g3 = groups["G3"]["results"]
    fig, ax = plt.subplots(figsize=(10, 5.5))
    labels, orig, q75, q90 = [], [], [], []
    for f in ("video1_frame0.jpg", "video1_frame5.jpg",
              "video1_frame2.jpg", "video1_frame6.jpg"):
        labels.append(f.replace("video1_", "").replace(".jpg", ""))
        orig.append(ref[f]["displayPrimary"]["hfH"])
        q75.append(g3[f]["q75"]["hfH"])
        q90.append(g3[f]["q90"]["hfH"])
    for key in ("s99w-pair1-off", "s99w-pair1-eab"):
        labels.append(key.replace("s99w-", ""))
        base = comp[key]["displayPrimary"]["hfH"]
        orig.append(base)
        q75.append(g3[key]["q75"]["hfH"])
        q90.append(g3[key]["q90"]["hfH"])
    x = range(len(labels))
    ax.bar([i - w for i in x], orig, w, label="original (as-captured)",
           color="#4477aa")
    ax.bar(list(x), q75, w, label="re-encoded Q75 (4:2:0)", color="#ee7733")
    ax.bar([i + w for i in x], q90, w, label="re-encoded Q90 (4:2:0)",
           color="#228833")
    ax.set_xticks(list(x))
    ax.set_xticklabels(labels, fontsize=9, rotation=20)
    ax.set_ylabel("hfH @200px")
    ax.set_title("G3 encoding sensitivity: JPEG Q75/Q90 re-encode, same "
                 "encoder version (sensitivity only)")
    ax.legend()
    ax.grid(axis="y", alpha=0.3)
    note = ("reference shifts +8% Q75→Q90; device arms ~±0.3-1%: original JPEG "
            "loss is irreversible; re-encode never feeds band formation")
    ax.text(0.02, 0.97, "\n".join(textwrap.wrap(note, 56)),
           transform=ax.transAxes, fontsize=7,
            verticalalignment="top", bbox=dict(boxstyle="round", fc="#fffbe6"))
    stamp(fig)
    fig.tight_layout(rect=(0, 0.04, 1, 1))
    fig.savefig(ART / "g3_encoding_sensitivity.png", dpi=200)
    plt.close(fig)

    # ---------------- G5: autocorrelation curves ----------------
    g5 = groups["G5"]["results"]
    fig, ax = plt.subplots(figsize=(10, 5.5))
    series = [
        ("reference f0 (static)", g5["video1_frame0.jpg"]["autocorrLag1to16"],
         "#4477aa", "-"),
        ("reference f5 (static)", g5["video1_frame5.jpg"]["autocorrLag1to16"],
         "#66aaff", "--"),
        ("device OFF pair1", g5["s99w-pair1-off"]["autocorrLag1to16"],
         "#228833", "-"),
        ("device EAB pair1", g5["s99w-pair1-eab"]["autocorrLag1to16"],
         "#aa4444", "-"),
    ]
    for label, ac, color, ls in series:
        lags = sorted(int(k) for k in ac)
        ax.plot(lags, [ac[str(l)] for l in lags], marker="o", ms=3,
                label=label, color=color, linestyle=ls)
    ax.axhline(0, color="#888888", lw=0.8)
    ax.set_xlabel("lag (px @200px scale)")
    ax.set_ylabel("autocorrelation (mean-removed, normalized)")
    ax.set_title("G5 diagnostic: horizontal autocorrelation lag 1..16 "
                 "(thin device band = 3 supported rows, diagnostic only)")
    ax.legend(fontsize=9)
    ax.grid(alpha=0.3)
    note = ("reference ≈0.99 at lag1 (smooth continuous texture); OFF ≈-0.10 "
            "(whitened grid); EAB ≈0.20 (intermediate). No gates attached.")
    ax.text(0.02, 0.97, "\n".join(textwrap.wrap(note, 56)),
           transform=ax.transAxes, fontsize=7,
            verticalalignment="top", bbox=dict(boxstyle="round", fc="#fffbe6"))
    stamp(fig)
    fig.tight_layout(rect=(0, 0.04, 1, 1))
    fig.savefig(ART / "g5_autocorr_curves.png", dpi=200)
    plt.close(fig)

    # ---------------- G5: normalized spectrum ----------------
    fig, ax = plt.subplots(figsize=(10, 5.5))
    for label, key, color in (
            ("reference f0 (static)", "video1_frame0.jpg", "#4477aa"),
            ("reference f5 (static)", "video1_frame5.jpg", "#66aaff"),
            ("device OFF pair1", "s99w-pair1-off", "#228833"),
            ("device EAB pair1", "s99w-pair1-eab", "#aa4444")):
        spec = g5[key]["spectrum"]["cyclesPerContainerWidth"]
        cyc = [p["cycles"] for p in spec]
        pw = [p["relPower"] for p in spec]
        ax.plot(cyc, pw, marker="s", ms=4, label=label, color=color,
                linestyle="-" if "device" in label else "--")
    ax.set_xlabel("cycles per container inner width (200px)")
    ax.set_ylabel("relative power (peak-normalized)")
    ax.set_title("G5 diagnostic: dominant spatial frequencies "
                 "(descriptive; wide spectrum ≠ single grain-size claim)")
    ax.set_yscale("log")
    ax.legend(fontsize=9)
    ax.grid(alpha=0.3, which="both")
    note = ("reference: low-frequency 1-3 cycles/width; OFF: peaks at 24-25+72 "
            "(~8px/~3px grid); EAB: peaks at 10-11+32 (~19px/~6px) - EAB "
            "differs from BOTH reference and OFF in frequency distribution")
    ax.text(0.02, 0.97, "\n".join(textwrap.wrap(note, 56)),
           transform=ax.transAxes, fontsize=7,
            verticalalignment="top", bbox=dict(boxstyle="round", fc="#fffbe6"))
    stamp(fig)
    fig.tight_layout(rect=(0, 0.04, 1, 1))
    fig.savefig(ART / "g5_spectrum_compare.png", dpi=200)
    plt.close(fig)

    # G4 chart: s96 tilt sequence values
    g4 = groups["G4"]["results"]
    fig, ax = plt.subplots(figsize=(10, 4.5))
    idx = sorted(g4, key=lambda s: int(s.split("-")[1]))
    vals = [g4[k]["displayPrimary"]["hfH"] for k in idx]
    ax.plot(range(len(idx)), vals, marker="o", color="#4477aa")
    ax.set_xticks(range(len(idx)))
    ax.set_xticklabels([k.replace("tilt-", "") for k in idx], fontsize=8)
    ax.set_xlabel("s96 tilt frame index (0.15s intervals)")
    ax.set_ylabel("hfH @200px (bottom deposited band)")
    ax.set_title("G4 dynamic: s96 tilt 0..15, bottom-band hfH (group kept "
                 "separate from s99w static; band static during tilt)")
    ax.grid(alpha=0.3)
    note = ("bottom deposited band barely changes during tilt flow (flow "
            "happens in upper sand) - a property of THIS measurement position, "
            "not evidence of no flow; s96 build ≠ s99w build, never merged")
    ax.text(0.02, 0.97, "\n".join(textwrap.wrap(note, 56)),
           transform=ax.transAxes, fontsize=7,
            verticalalignment="top", bbox=dict(boxstyle="round", fc="#fffbe6"))
    stamp(fig)
    fig.tight_layout(rect=(0, 0.04, 1, 1))
    fig.savefig(ART / "g4_s96_tilt_series.png", dpi=200)
    plt.close(fig)

    print("charts written:")
    for p in ("g2_tier_comparison.png", "g3_encoding_sensitivity.png",
              "g4_s96_tilt_series.png", "g5_autocorr_curves.png",
              "g5_spectrum_compare.png"):
        f = ART / p
        print(" ", p, f.stat().st_size, "bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
