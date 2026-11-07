"""Independent, frozen nearest-sampling calibration and complete RGB pairing."""
import argparse
import hashlib
import json
import math
from pathlib import Path

from PIL import Image


def check(ok, reason):
    if not ok:
        raise ValueError(reason)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def seal(value):
    return digest(json.dumps(value, sort_keys=True, separators=(",", ":")).encode())


def pattern(x, y):
    h = x*73+y*151+(x//17)*31+(y//23)*47
    return (32+h % 192, 32+(x*37+y*19) % 192, 32+(x*11+y*97) % 192)


def calibration_color(code, rule):
    if rule == "IDENTITY_RGB8":
        return code
    check(rule == "SRGB_DECODE_RGB8", "COLOR_MODEL_UNSUPPORTED")
    return tuple(round((v/255/12.92 if v/255 <= .04045 else ((v/255+.055)/1.055)**2.4)*255)
                 for v in code)


def mapping_point(model, x, y):
    # Screen pixel centers to bottom-left native integer texels. Rational
    # arithmetic avoids introducing an independent floating-point rounding.
    left, top, width, height = model["coverage"]
    nw, nh = model["native"]
    dx, dy = x-left, y-top
    check(0 <= dx < width and 0 <= dy < height, "SCREEN_CORE_OUTSIDE_MAPPING")
    u = (2*dx+1)*nw//(2*width)
    v = (2*dy+1)*nh//(2*height)
    if model.get("mirrorX", False):
        u = nw-1-u
    if model.get("mirrorY", False):
        v = nh-1-v
    return u, nh-1-v


def context(probe):
    return {k: probe[k] for k in ("session", "surfaceEpoch", "contextEpoch", "geometryEpoch",
                                  "windowWidth", "windowHeight", "boxX", "boxY", "boxWidth",
                                  "boxHeight", "scale", "inverted")}


def load(spec_path):
    spec = read(spec_path)
    path = spec_path.parent / spec["manifest"]
    manifest = read(path)
    expected = path.with_suffix(".sha256").read_text(encoding="utf-8-sig").split()
    check(expected == [digest(path.read_bytes()), path.name], "MANIFEST_SHA")
    p, phase = manifest["probe"], manifest["phase"]
    hold = phase["hold"]
    times = [int(p[k]) for k in ("candidateNs", "backNs", "captureEndNs", "swapStartNs", "swapEndNs", "unbindEndNs")]
    check(times[0] > 0 and times == sorted(times), "NATIVE_TIME_ORDER")
    end = read(spec_path.parent/spec["hold"])
    check(end["hold"] == hold and end["endToken"] == hold["token"] and
          end["endFrame"] == hold["frame"] == p["frameSerial"] and
          hold["token"] == phase["sample"]["token"] == p["requestedToken"] == p["actualToken"] and
          phase["sequence"] == hold["sequence"] and hold["requestId"] == p["requestId"], "HOLD_IDENTITY")
    check(p["captureStatus"] == "CAPTURED" and p["captureStage"] == "COMPLETE" and
          p["presentationState"] == "CURRENT" and p["swapSucceeded"] and p["unbindSucceeded"],
          "NATIVE_EXCHANGE_UNVERIFIED")
    check(spec["rotation"] == 0 and spec["screenshotScale"] == 1, "SCREENSHOT_CONFIGURATION_UNSUPPORTED")
    receipt_path = spec_path.parent/spec["capture"]
    captured = read(receipt_path)
    png = spec_path.parent/spec["png"]
    check(captured["revision"]=="T406_CAPTURE_CALL_V1" and captured["clock"]=="PYTHON_MONOTONIC_NS", "CAPTURE_TOOL_RAW_CLOCK_REQUIRED")
    check(captured["fileSha256"] == digest(png.read_bytes()) and captured["exitCode"] == 0 and
          captured["tool"] and captured["command"] and captured["startMonoNs"] <= captured["endMonoNs"],
          "CAPTURE_TOOL_RECEIPT")
    # Map the entire tool call interval with an independently registered
    # offset/error bound. Operator click times cannot substitute for this.
    clock = spec["clockMap"]
    check(clock["source"] and clock["uncertaintyMs"] >= 0 and clock["hostToDeviceRate"] > 0,
          "CLOCK_MAPPING_UNKNOWN")
    lower = captured["startMonoNs"]/1e6*clock["hostToDeviceRate"]+clock["offsetMs"]-clock["uncertaintyMs"]
    upper = captured["endMonoNs"]/1e6*clock["hostToDeviceRate"]+clock["offsetMs"]+clock["uncertaintyMs"]
    check(hold["startMonoMs"] <= lower <= upper <= end["endMonoMs"], "CAPTURE_INTERVAL_OUTSIDE_HOLD")
    image = Image.open(png)
    check(image.format == "PNG" and image.mode in ("RGB", "RGBA"), "LOSSLESS_RGB_REQUIRED")
    check(list(image.size) == spec["screenshotSize"], "SCREENSHOT_SIZE")
    image = image.convert("RGB")
    check(spec["deviceId"] and spec["toolConfiguration"], "DEVICE_CONFIGURATION_MISSING")
    geometry = json.loads(end["geometry"])
    check(geometry["revision"] == "T406_GEOMETRY_V1" and
          all(type(geometry[k]) in (float,int) and math.isfinite(geometry[k])
              for k in ("globalX","globalY","widthVp","heightVp","vp2px")) and
          all(geometry[k]>0 for k in ("widthVp","heightVp","vp2px")), "FLOAT_GEOMETRY_MISSING")
    key = {"native": context(p), "layout": spec["bounds"], "floating": geometry,
           "screen": spec["screenshotSize"], "rotation": spec["rotation"], "scale": spec["screenshotScale"],
           "device": spec["deviceId"], "tool": spec["toolConfiguration"]}
    targets = {}
    total = 0
    for i, r in enumerate(p["rois"]):
        x, y, w, h = (r[k] for k in ("x", "y", "width", "height"))
        check(all(type(v) is int for v in (x, y, w, h)) and 0 <= x <= p["windowWidth"]-w and
              0 <= y <= p["windowHeight"]-h and 1 <= w <= 64 and 1 <= h <= 64 and
              r["byteOffset"] == total and r["stride"] == w*4, "NATIVE_TILE_LAYOUT")
        total += w*h*4
    check(len(p["rois"]) == 3 and 2*total <= 98304, "READBACK_BUDGET")
    for target in ("candidate", "back"):
        record = manifest[target]
        raw = (path.parent/record["name"]).read_bytes()
        check(len(raw) == record["bytes"] == total and digest(raw) == record["sha256"] == p[target]["sha256"],
              target.upper()+"_SHA")
        pixels = {}
        for r in p["rois"]:
            for yy in range(r["height"]):
                for xx in range(r["width"]):
                    pos = r["byteOffset"]+(yy*r["width"]+xx)*4
                    pixels[(r["x"]+xx, r["y"]+yy)] = tuple(raw[pos:pos+4])
        targets[target] = pixels
    return spec, manifest, image, key, targets


def calibration_points(coverage):
    left, top, width, height = coverage
    # Registered before observing content. Odd grid points are holdouts.
    return [(left+(width-1)*i//8, top+(height-1)*j//8, (i+j) % 2)
            for j in (1, 2, 4, 6, 7) for i in (1, 2, 4, 6, 7)]


def calibrate(spec_path, output):
    spec, manifest, image, key, targets = load(spec_path)
    probe = manifest["probe"]
    check(probe.get("calibration") is True, "INDEPENDENT_PATTERN_REQUIRED")
    shader = Path(__file__).with_name("calibration.glsl").read_bytes()
    check(probe["calibrationShaderSha256"] == digest(shader), "CALIBRATION_SHADER")
    check(all(rgba == (*pattern(x, y), 255) for (x, y), rgba in targets["candidate"].items()),
          "NATIVE_CALIBRATION_PATTERN_FAILED")
    back_rules = [rule for rule in ("IDENTITY_RGB8", "SRGB_DECODE_RGB8")
                  if all(rgba == (*calibration_color(pattern(x, y), rule), 255)
                         for (x, y), rgba in targets["back"].items())]
    check(len(back_rules) == 1, "CALIBRATION_BACK_COLOR_MODEL_UNKNOWN")
    back_rule = back_rules[0]
    registration = spec["registration"]
    # Complete model hypotheses are predeclared in the capture spec. They
    # may be based on measured borders/float geometry, never target grains.
    check(registration["source"] == "INDEPENDENT_GEOMETRY_PREDECLARED" and
          1 <= len(registration["models"]) <= 64, "MODEL_REGISTRATION")
    matches = []
    native = [probe["windowWidth"], probe["windowHeight"]]
    for model in registration["models"]:
        check(model["native"] == native and model["rule"] == "PIXEL_CENTER_NEAREST" and
              len(model["coverage"]) == 4 and all(type(v) is int for v in model["coverage"]) and
              model["coverage"][2]>0 and model["coverage"][3]>0,
              "MODEL_UNSUPPORTED")
        points = calibration_points(model["coverage"])
        errors = [0, 0]
        for x, y, holdout in points:
            if not (0 <= x < image.width and 0 <= y < image.height):
                errors[holdout] += 1
            elif image.getpixel((x, y)) != calibration_color(pattern(*mapping_point(model, x, y)), back_rule):
                errors[holdout] += 1
        if errors == [0, 0]:
            matches.append(model)
    check(len(matches) == 1, "CALIBRATION_AMBIGUOUS_OR_FAILED")
    model = matches[0]
    # Verify an additional asymmetric dense cross. All points independent
    # of material geometry and covering the registered component.
    left, top, width, height = model["coverage"]
    points = [(left+i, top+height//3) for i in range(width)]
    points += [(left+width*2//3, top+j) for j in range(height)]
    check(all(image.getpixel((x, y)) == calibration_color(pattern(*mapping_point(model, x, y)), back_rule)
              for x, y in points),
          "VALIDATION_POINTS_FAILED")
    calibration = {"revision": "T402_FROZEN_MAPPING_V2", "identity": key, "model": model,
                   "shaderSha256": digest(shader), "captureSpecSha256": digest(spec_path.read_bytes()),
                   "validationPoints": len(points), "registeredGridPoints": 25,
                   "trainingPoints": sum(p[2]==0 for p in calibration_points(model["coverage"])),
                   "holdoutPoints": sum(p[2]==1 for p in calibration_points(model["coverage"])),
                   "colorRule": "RGB8_CODE_EXACT", "alphaRule": "NATIVE_ONLY",
                   "calibrationBackEncoding": back_rule,
                   "samplingEvidence": "INDEPENDENT_ASYMMETRIC_SHADER"}
    calibration["calibrationId"] = seal(calibration)
    output.write_text(json.dumps(calibration, indent=2), encoding="utf-8")
    return calibration


def frozen(path):
    record = read(path)
    identity = record.pop("calibrationId")
    check(record["revision"] == "T402_FROZEN_MAPPING_V2" and seal(record) == identity, "CALIBRATION_ID")
    record["calibrationId"] = identity
    return record


def plan(calibration_path, cores, output):
    calibration = frozen(calibration_path)
    check(len(cores) == 3, "THREE_SCREEN_CORES_REQUIRED")
    tiles = []
    for x, y, w, h in cores:
        check(all(type(v) is int for v in (x, y, w, h)) and 1 <= w <= 64 and 1 <= h <= 64, "SCREEN_CORE")
        indices = [mapping_point(calibration["model"], px, py)
                   for py in range(y, y+h) for px in range(x, x+w)]
        left, right = min(p[0] for p in indices), max(p[0] for p in indices)
        bottom, top = min(p[1] for p in indices), max(p[1] for p in indices)
        # No unobserved interpolation or inferred background in padding.
        tile = [left, bottom, right-left+1, top-bottom+1]
        check(tile[2] <= 64 and tile[3] <= 64, "NATIVE_TILE_TOO_LARGE")
        tiles.append(tile)
    check(sum(w*h*8 for _, _, w, h in tiles) <= 98304, "READBACK_BUDGET")
    record = {"revision": "T402_SCREEN_ROI_PLAN_V2", "calibrationId": calibration["calibrationId"],
              "screenCores": cores, "nativeTiles": tiles, "arithmeticUncertaintyPx": 0.0003,
              "supportRule": "F32_PACKET_CENTERS_BINARY_DISC"}
    record["planId"] = seal(record)
    output.write_text(json.dumps(record, indent=2), encoding="utf-8")
    return record



def support(probe, sample, model, roi_plan):
    """Independent packet projection; candidate 0.0003px arithmetic envelope.

    Uncertain boundary points are reported, never removed from RGB pairing.
    This is a projection-domain candidate, not an attribution to GPU AA.
    """
    xy, meta, bed = (sample[k] for k in ("projectionXY", "projectionMeta", "displayBed"))
    check(len(xy)==len(meta) and len(xy)%2==0 and len(bed)==128 and
          all(math.isfinite(v) for v in xy+bed), "PROJECTED_SUPPORT_LAYOUT")
    radius = probe["configValues"][6]*probe["scale"]
    x,y,w,h = (probe[k] for k in ("boxX","boxY","boxWidth","boxHeight"))
    check(math.isfinite(radius) and radius>0 and w>0 and h>0, "PROJECTED_SUPPORT_RADIUS")
    base=probe["windowHeight"]-y-h
    epsilon=roi_plan["arithmeticUncertaintyPx"]
    check(epsilon==0.0003, "ARITHMETIC_DOMAIN_UNREGISTERED")
    result=[]
    for i in range(0,len(meta),2):
        state, identity=meta[i]>>8, meta[i+1]
        if state not in (1,2):
            continue
        col,height=xy[i:i+2]
        q=min(127,max(0,col));j=min(126,math.floor(q))
        center_bed=bed[j]+(bed[j+1]-bed[j])*(q-j)
        cx=x+(1-col/128 if probe["inverted"] else col/128)*w
        cy=base+(1-height/256 if probe["inverted"] else height/256)*h
        rows=[]
        for rx,ry,rw,rh in roi_plan["screenCores"]:
            binary=[];uncertain=[]
            for py in range(ry,ry+rh):
                for px in range(rx,rx+rw):
                    nx,ny=mapping_point(model,px,py)
                    distance=math.hypot(nx+.5-cx,ny+.5-cy)
                    visible=state!=1 or height>=center_bed
                    binary.append(int(visible and distance<radius))
                    if abs(distance-radius)<=epsilon or (state==1 and abs(height-center_bed)*h/256<=epsilon):
                        uncertain.append([px,py])
            rows.append({"_mask":binary,"pixels":sum(binary),"maskSha256":digest(bytes(binary)),
                         "uncertainCoordinates":uncertain})
        result.append({"id":identity,"state":state,"center":[cx,cy],"radius":radius,"regions":rows})
    return result


def checked_plan(path, calibration):
    record=read(path)
    check(seal({k:v for k,v in record.items() if k!="planId"})==record["planId"] and
          record["calibrationId"]==calibration["calibrationId"], "ROI_PLAN_ID")
    return record


def single(spec_path, calibration_path, plan_path):
    calibration=frozen(calibration_path)
    roi_plan=checked_plan(plan_path,calibration)
    result=compare(spec_path,calibration,roi_plan)
    ok=result["screen"]==result["back"] and result["nativeDifferences"]==0
    return {"revision":"T402_DISPLAY_SINGLE_V2",
            "displayStatus":"DISPLAY_CALIBRATED_SINGLE_MATCH" if ok else "DISPLAY_NOT_JUDGABLE",
            "reason":"" if ok else "SCREEN_OR_NATIVE_MISMATCH",
            "calibrationId":calibration["calibrationId"],"planId":roi_plan["planId"],
            "nativeSameTokenExchange":True,"holdIdentity":True,"mappingIdentity":True,
            **{k:result[k] for k in ("regions","nativeDifferences","nativeAlphaValues","projectedMasks")}}


def compare(spec_path, calibration, roi_plan):
    spec, manifest, image, key, targets = load(spec_path)
    check(key == calibration["identity"], "CALIBRATION_EXPIRED_IDENTITY")
    probe, phase = manifest["probe"], manifest["phase"]
    check(not probe.get("calibration", False), "MATERIAL_SHADER_REQUIRED")
    check([[r[k] for k in ("x", "y", "width", "height")] for r in probe["rois"]] ==
          roi_plan["nativeTiles"], "PREDECLARED_NATIVE_TILE_MISMATCH")
    check(manifest.get("calibrationId") == calibration["calibrationId"] and
          manifest.get("roiPlanId") == roi_plan.get("planId", manifest.get("roiPlanId")),
          "RESERVATION_MAPPING_IDENTITY")
    rows, screen, back = [], [], []
    for core in roi_plan["screenCores"]:
        x, y, w, h = core
        differences, old = [], []
        for py in range(y, y+h):
            for px in range(x, x+w):
                nx, ny = mapping_point(calibration["model"], px, py)
                check((nx, ny) in targets["back"], "ROI_MISSING_NATIVE_COLUMN")
                observed = image.getpixel((px, py))
                expected = targets["back"][(nx, ny)][:3]
                old.append((observed, expected))
                if observed != expected:
                    differences.append({"screen": [px, py], "native": [nx, ny],
                                        "actual": list(observed), "expected": list(expected)})
        screen.append([p[0] for p in old])
        back.append([p[1] for p in old])
        rows.append({"samples": w*h, "differences": differences, "differenceCount": len(differences)})
    native_diff = sum(targets["candidate"][p] != rgba for p, rgba in targets["back"].items())
    alpha = sorted(set(rgba[3] for target in targets.values() for rgba in target.values()))
    masks=support(probe,phase["sample"],calibration["model"],roi_plan)
    # Dynamic fixture's two AIR supports are independently projected.
    # Exact RGB pairing still includes arithmetic-boundary samples.
    occupancy=[]
    background=set(back[2])
    if phase["name"] in ("CONTROL_A","CONTROL_B","LOCK_A","LOCK_B"):
        check(len(background)==1,"BACKGROUND_ORACLE_NOT_UNIFORM")
        bg=next(iter(background))
        for region in range(2):
            core=roi_plan["screenCores"][region]
            rx,ry,rw,rh=core
            uncertain={tuple(p) for m in masks for p in m["regions"][region]["uncertainCoordinates"]}
            bad=[]
            for i,color in enumerate(back[region]):
                px,py=rx+i%rw,ry+i//rw
                covered=any(m["regions"][region]["_mask"][i] for m in masks)
                if (px,py) not in uncertain and (color!=bg)!=covered:
                    bad.append([px,py])
            occupancy.append({"mismatchCoordinates":bad,"mismatchCount":len(bad),
                              "uncertainCount":len(uncertain)})
    for m in masks:
        for r in m["regions"]:
            del r["_mask"]
    return {"phase": phase, "regions": rows, "screen": screen, "back": back, "projectedMasks":masks, "supportOccupancy":occupancy,
            "nativeDifferences": native_diff, "nativeAlphaValues": alpha}


def pair(a, b, calibration_path, plan_path):
    calibration = frozen(calibration_path)
    roi_plan = checked_plan(plan_path,calibration)
    plan_id = roi_plan["planId"]
    ca, cb = compare(a, calibration, roi_plan), compare(b, calibration, roi_plan)
    pa, pb = ca["phase"], cb["phase"]
    check(pa["name"] in ("CONTROL_A", "LOCK_A") and pb["name"] == pa["name"][:-2]+"_B" and
          pb["sequence"] == pa["sequence"]+1 and pa["sample"]["authority"]["run"] ==
          pb["sample"]["authority"]["run"] and pa["sample"]["digest"] == pb["sample"]["digest"],
          "PAIR_IDENTITY")
    am,bm=ca["projectedMasks"],cb["projectedMasks"]
    check(any(m["id"]==1 and m["regions"][0]["pixels"]>0 for m in am) and
          not any(m["id"]==2 for m in am) and
          any(m["id"]==2 and m["regions"][1]["pixels"]>0 for m in bm) and
          all(m["regions"][2]["pixels"]==0 for m in am+bm), "PROJECTED_SUPPORT_MISMATCH")
    status = "DISPLAY_NOT_JUDGABLE"
    if any(r["mismatchCount"] for c in (ca,cb) for r in c["supportOccupancy"]):
        reason="NATIVE_PROJECTED_SUPPORT_MISMATCH"
    elif ca["nativeDifferences"] or cb["nativeDifferences"]:
        reason = "CANDIDATE_BACK_DIFFERENCE"
    elif ca["back"][:2] == cb["back"][:2]:
        reason = "B_CONTENT_MISSING"
    elif ca["back"][2] != cb["back"][2]:
        reason = "BACKGROUND_CHANGED"
    elif ca["screen"] != ca["back"]:
        reason = "A_SCREEN_MISMATCH"
    elif cb["screen"] == cb["back"]:
        reason = "";status = "DISPLAY_CALIBRATED_A_AND_B_PAIRED"
    elif cb["screen"] == ca["back"]:
        reason = "";status = "DISPLAY_REPEATS_A_IN_B_HOLD"
    else:
        reason = "B_SCREEN_MISMATCH"
    return {"revision": "T402_DISPLAY_PAIR_V2", "displayStatus": status, "reason": reason,
            "calibrationId": calibration["calibrationId"], "planId": plan_id,
            "nativeSameTokenExchange": True, "holdIdentity": True, "mappingIdentity": True,
            "a": {k: ca[k] for k in ("regions", "nativeDifferences", "nativeAlphaValues", "projectedMasks", "supportOccupancy")},
            "b": {k: cb[k] for k in ("regions", "nativeDifferences", "nativeAlphaValues", "projectedMasks", "supportOccupancy")}}


if __name__ == "__main__":
    cli = argparse.ArgumentParser(description=__doc__)
    sub = cli.add_subparsers(dest="action", required=True)
    cal = sub.add_parser("calibrate")
    cal.add_argument("spec", type=Path);cal.add_argument("output", type=Path)
    planner = sub.add_parser("plan")
    planner.add_argument("calibration", type=Path);planner.add_argument("cores", type=Path)
    planner.add_argument("output", type=Path)
    judge = sub.add_parser("pair")
    for key in ("a", "b", "calibration", "plan"):
        judge.add_argument(key, type=Path)
    inspect = sub.add_parser("single")
    for key in ("spec", "calibration", "plan"):
        inspect.add_argument(key,type=Path)
    args = cli.parse_args()
    try:
        if args.action == "calibrate":
            result = calibrate(args.spec, args.output)
        elif args.action == "plan":
            result = plan(args.calibration, read(args.cores), args.output)
        elif args.action == "single":
            result=single(args.spec,args.calibration,args.plan)
        else:
            result = pair(args.a, args.b, args.calibration, args.plan)
    except (ValueError, KeyError, OSError, TypeError, ZeroDivisionError, IndexError) as exc:
        result = {"revision": "T402_DISPLAY_PAIR_V2", "displayStatus": "DISPLAY_NOT_JUDGABLE", "reason": str(exc)}
    print(json.dumps(result))
