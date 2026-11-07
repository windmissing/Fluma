"""Compare the same fixed native read regions with lossless component pixels.

Each side is a JSON input containing manifest, hold, png, bounds [l,t,r,b],
and captureWallMs (the screenshot tool's recorded capture time in the hold
clock domain). Paths are relative to that input file. No resampling.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path

from PIL import Image


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def sha(data):
    return hashlib.sha256(data).hexdigest()


def raw_target(manifest_path, manifest, target, size):
    record = manifest[target]
    data = (manifest_path.parent / record["name"]).read_bytes()
    require(len(data) == record["bytes"] == size, target + "_SIZE")
    require(sha(data) == record["sha256"] == manifest["probe"][target]["sha256"], target + "_HASH")
    return data


def projected_masks(probe, sample, rois):
    """Independent CPU center/occlusion check from archived packet words."""
    radius = probe["configValues"][6] * probe["scale"]
    x, y, w, h = (probe[k] for k in ("boxX", "boxY", "boxWidth", "boxHeight"))
    base = probe["windowHeight"] - y - h
    inverted = probe["inverted"]
    xy, meta, bed = (sample[k] for k in ("projectionXY", "projectionMeta", "displayBed"))
    require(len(xy) == len(meta) and len(xy) % 2 == 0 and len(bed) == 128 and
            math.isfinite(radius) and radius > 0, "PROJECTED_SUPPORT_LAYOUT")
    evidence = []
    for i in range(0, len(meta), 2):
        identity, state = meta[i+1], meta[i] >> 8
        if identity not in (1, 2) or state not in (1, 2):
            continue
        col, height = xy[i:i+2]
        q = min(127, max(0, col))
        j = min(126, math.floor(q))
        center_bed = bed[j] + (bed[j+1]-bed[j])*(q-j)
        cx = x + (1-col/128 if inverted else col/128)*w
        cy = base + (1-height/256 if inverted else height/256)*h
        masks = []
        for rx, ry, rw, rh in rois:
            mask = bytes(int((state != 1 or height >= center_bed) and
                             (px+.5-cx)**2 + (py+.5-cy)**2 < radius**2)
                         for py in range(ry, ry+rh) for px in range(rx, rx+rw))
            masks.append({"pixels": sum(mask), "sha256": sha(mask)})
        evidence.append({"id": identity, "state": state, "center": [cx, cy], "regions": masks})
    return evidence


def load_side(path):
    spec = read_json(path)
    manifest_path = path.parent / spec["manifest"]
    manifest = read_json(manifest_path)
    manifest_sha = manifest_path.with_suffix(".sha256").read_text(encoding="utf-8-sig").split()
    require(manifest_sha == [sha(manifest_path.read_bytes()), manifest_path.name], "MANIFEST_SHA")
    probe, phase = manifest["probe"], manifest["phase"]
    hold = phase["hold"]
    seal = read_json(path.parent / spec["hold"])
    token = hold["token"]
    require(phase["revision"] == "T394_SINGLE_SAND_PHASE_V1" and
            seal["revision"] == "T394_DISPLAY_HOLD_V1" and seal["hold"] == hold, "HOLD_IDENTITY")
    require(probe["captureStatus"] == "CAPTURED" and probe["captureStage"] == "COMPLETE" and
            probe["presentationState"] == "CURRENT" and probe["swapSucceeded"] and probe["unbindSucceeded"],
            "NATIVE_CAPTURE")
    require(token and token == phase["sample"]["token"] == probe["requestedToken"] ==
            probe["actualToken"] == seal["endToken"] and
            hold["requestId"] == probe["requestId"] and
            hold["frame"] == probe["frameSerial"] == seal["endFrame"] and
            hold["sequence"] == phase["sequence"] and hold["name"] == phase["name"] and
            hold["geometry"] == phase["sample"]["geometry"], "TOKEN_IDENTITY")
    stages = [int(probe[key]) for key in ("candidateNs", "backNs", "captureEndNs",
                                         "swapStartNs", "swapEndNs", "unbindEndNs")]
    require(stages[0] > 0 and stages == sorted(stages), "NATIVE_TIME_ORDER")
    elapsed_wall = seal["endWallMs"] - hold["startWallMs"]
    elapsed_mono = seal["endMonoMs"] - hold["startMonoMs"]
    require(0 <= elapsed_wall and 0 <= elapsed_mono and abs(elapsed_wall-elapsed_mono) <= 250 and
            hold["startWallMs"] <= spec["captureWallMs"] <= seal["endWallMs"], "SCREENSHOT_OUTSIDE_HOLD")
    left, top, right, bottom = spec["bounds"]
    require(all(type(v) is int for v in (left, top, right, bottom)), "COMPONENT_BOUNDS")
    width, height = probe["windowWidth"], probe["windowHeight"]
    require((right-left, bottom-top) == (width, height) == (hold["width"], hold["height"]),
            "MAPPING_REQUIRES_EXACT_PIXELS")
    image_path = path.parent / spec["png"]
    with Image.open(image_path) as source:
        require(source.format == "PNG" and source.mode in ("RGB", "RGBA"), "LOSSLESS_RGB_REQUIRED")
        require(0 <= left < right <= source.width and 0 <= top < bottom <= source.height,
                "COMPONENT_BOUNDS")
        component = source.crop((left, top, right, bottom)).convert("RGB")
    rois, pixels, size = [], [], 0
    for roi in probe["rois"]:
        x, y, w, h = (roi[k] for k in ("x", "y", "width", "height"))
        require(all(type(v) is int for v in (x, y, w, h)) and 1 <= w <= 64 and 1 <= h <= 64 and
                0 <= x <= width-w and 0 <= y <= height-h and
                roi["stride"] == w*4 and roi["byteOffset"] == size, "ROI_LAYOUT")
        rows = [component.crop((x, height-y-i-1, x+w, height-y-i)).tobytes() for i in range(h)]
        pixels.append(b"".join(rows))
        rois.append((x, y, w, h))
        size += w*h*4
    require(len(rois) == 3 and size*2 <= 98304, "THREE_FIXED_ROIS_REQUIRED")
    target_rgb = {}
    for target in ("candidate", "back"):
        rgba = raw_target(manifest_path, manifest, target, size)
        rgb = bytes(v for i, v in enumerate(rgba) if i % 4 != 3)
        offset = 0
        regions = []
        for _, _, w, h in rois:
            end = offset+w*h*3
            regions.append(rgb[offset:end])
            offset = end
        target_rgb[target] = regions
    masks = projected_masks(probe, phase["sample"], rois)
    return phase, rois, pixels, target_rgb, sha(image_path.read_bytes()), masks


def pair(a_path, b_path):
    a, ar, ap, at, a_sha, a_masks = load_side(a_path)
    b, br, bp, bt, b_sha, b_masks = load_side(b_path)
    require(a["name"] in ("CONTROL_A", "LOCK_A") and b["name"] == a["name"][:-2]+"_B" and
            b["sequence"] == a["sequence"]+1 and a["sample"]["authority"]["run"] ==
            b["sample"]["authority"]["run"] and a["sample"]["geometry"] == b["sample"]["geometry"] and
            a["sample"]["digest"] == b["sample"]["digest"] and ar == br, "DYNAMIC_PAIR_IDENTITY")
    result = {"revision": "T394_DISPLAY_PAIR_V1", "aPngSha256": a_sha, "bPngSha256": b_sha,
              "aProjectedMasks": a_masks, "bProjectedMasks": b_masks,
              "displayStatus": "DISPLAY_NOT_JUDGABLE"}
    require(any(m["id"] == 1 and m["regions"][0]["pixels"] > 0 for m in a_masks) and
            not any(m["id"] == 2 for m in a_masks) and
            any(m["id"] == 2 and m["regions"][1]["pixels"] > 0 for m in b_masks) and
            all(m["regions"][2]["pixels"] == 0 for m in a_masks+b_masks), "PROJECTED_SUPPORT_MISMATCH")
    if at["candidate"][:2] == bt["candidate"][:2]:
        result["firstDifference"] = "CANDIDATE_UNCHANGED"
    elif at["back"][:2] == bt["back"][:2]:
        result["firstDifference"] = "DEFAULT_BACK_UNCHANGED"
    elif at["back"][2] != bt["back"][2] or at["candidate"][2] != bt["candidate"][2]:
        result["firstDifference"] = "BACKGROUND_CONTROL_CHANGED"
    elif ap == at["back"]:
        if bp == bt["back"]:
            result["displayStatus"] = "DISPLAY_A_AND_B_PAIRED"
        elif bp == at["back"]:
            result["displayStatus"] = "DISPLAY_REPEATS_A_IN_B_HOLD"
        else:
            result["firstDifference"] = "B_SCREENSHOT_DOES_NOT_MATCH_EITHER_BACK"
    else:
        result["firstDifference"] = "A_SCREENSHOT_BACK_MISMATCH"
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("a", type=Path)
    parser.add_argument("b", type=Path)
    args = parser.parse_args()
    try:
        result = pair(args.a, args.b)
    except (KeyError, TypeError, ValueError, OSError) as exc:
        result = {"revision": "T394_DISPLAY_PAIR_V1", "displayStatus": "DISPLAY_NOT_JUDGABLE",
                  "reason": str(exc)}
    print(json.dumps(result))
