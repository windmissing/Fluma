import json
from pathlib import Path
from tempfile import TemporaryDirectory

from PIL import Image

from pair_display import pair, sha


def write(path, value):
    path.write_text(json.dumps(value), encoding="utf-8")


def side(root, tag, phase_name, sequence, capture_b=False, screenshot_b=False):
    width, height = 32, 32
    token = "token-" + tag
    rois = [(6, 14, 4, 4), (22, 14, 4, 4), (14, 23, 4, 4)]
    old, new, bg = (10, 40, 12), (60, 70, 20), (0, 0, 0)
    colors = [old, new if capture_b else bg, bg]
    data = b"".join(bytes([*color, 255])*16 for color in colors)
    files = {}
    for name in ("candidate", "back"):
        f = root / f"{tag}-{name}.rgba"
        f.write_bytes(data)
        files[name] = {"name": f.name, "bytes": len(data), "sha256": sha(data)}
    hold = {"token": token, "requestId": sequence, "frame": sequence, "sequence": sequence,
            "name": phase_name, "width": width, "height": height, "startWallMs": 1000,
            "startMonoMs": 500, "geometry": "g"}
    phase = {"revision": "T394_SINGLE_SAND_PHASE_V1", "sequence": sequence, "name": phase_name,
             "hold": hold, "sample": {"token": token, "authority": {"run": "fixture"},
                                     "geometry": "g", "digest": "d", "displayBed": [0]*128,
                                     "projectionXY": [32, 128, 96, 128] if capture_b else [32, 128],
                                     "projectionMeta": [256, 1, 256, 2] if capture_b else [256, 1]}}
    probe = {"captureStatus": "CAPTURED", "captureStage": "COMPLETE", "presentationState": "CURRENT",
             "swapSucceeded": True, "unbindSucceeded": True, "windowWidth": width, "windowHeight": height,
             "requestedToken": token, "actualToken": token, "requestId": sequence, "frameSerial": sequence,
             "boxX": 0, "boxY": 0, "boxWidth": width, "boxHeight": height, "scale": 1, "inverted": False,
             "configValues": [0]*6+[2], "candidate": files["candidate"], "back": files["back"],
             "rois": [{"x": x, "y": y, "width": w, "height": h, "stride": w*4, "byteOffset": i*64}
                      for i, (x, y, w, h) in enumerate(rois)]}
    for i, key in enumerate(("candidateNs", "backNs", "captureEndNs", "swapStartNs",
                             "swapEndNs", "unbindEndNs"), 1):
        probe[key] = str(i)
    manifest = root / (tag+"-manifest.json")
    write(manifest, {"probe": probe, "phase": phase, **files})
    manifest.with_suffix(".sha256").write_text(sha(manifest.read_bytes())+"  "+manifest.name+"\n", encoding="utf-8")
    seal = root / (tag+"-hold.json")
    write(seal, {"revision": "T394_DISPLAY_HOLD_V1", "hold": hold,
                 "endWallMs": 1100, "endMonoMs": 600, "endToken": token, "endFrame": sequence})
    png = root / (tag+".png")
    image = Image.new("RGB", (40, 40))
    for (x, y, w, h), color in zip(rois, [old, new if screenshot_b else bg, bg]):
        image.paste(color, (4+x, 4+height-y-h, 4+x+w, 4+height-y))
    image.save(png)
    spec = root / (tag+".json")
    write(spec, {"manifest": manifest.name, "hold": seal.name, "png": png.name,
                 "bounds": [4, 4, 36, 36], "captureWallMs": 1050})
    return spec


with TemporaryDirectory() as scratch:
    root = Path(scratch)
    a = side(root, "a", "LOCK_A", 10)
    b = side(root, "b", "LOCK_B", 11, True, True)
    assert pair(a, b)["displayStatus"] == "DISPLAY_A_AND_B_PAIRED"
    repeated = side(root, "repeat", "LOCK_B", 11, True, False)
    assert pair(a, repeated)["displayStatus"] == "DISPLAY_REPEATS_A_IN_B_HOLD"
    spec = json.loads(b.read_text())
    for patch, reason in (({"captureWallMs": 1200}, "SCREENSHOT_OUTSIDE_HOLD"),
                          ({"bounds": [4, 4, 35, 36]}, "MAPPING_REQUIRES_EXACT_PIXELS")):
        write(b, {**spec, **patch})
        try:
            pair(a, b)
        except ValueError as exc:
            assert str(exc) == reason
        else:
            raise AssertionError(reason)
    write(b, spec)
    (root / "b-back.rgba").write_bytes(bytes(192))
    try:
        pair(a, b)
    except ValueError as exc:
        assert str(exc) == "back_HASH"
    else:
        raise AssertionError("back_HASH")
print('T394 screenshot pairing: paired / repeated / clock / mapping / SHA checks PASS')
