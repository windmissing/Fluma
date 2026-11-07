import json
from pathlib import Path
from tempfile import TemporaryDirectory

from PIL import Image

from screen import calibrate, plan, pair, single, pattern, mapping_point, digest


def write(path, value):
    path.write_text(json.dumps(value), encoding="utf-8")


def artifact(root, name, model, kind, seq, tiles, bad=None, registered=None):
    nw, nh = model["native"]
    left, top, width, height = model["coverage"]
    token = name
    def pixel(x, y):
        if kind == "CAL":
            return pattern(x, y)
        bg = (16, 16, 36)
        radius2=25 if bad=="nativeRadius" else 16
        old = (x+.5-nw*.25)**2+(y+.5-30)**2 <= radius2
        new = kind == "B" and bad!="missingB" and (x+.5-nw*.75)**2+(y+.5-30)**2 <= radius2
        return (145, 168, 111) if old or new else bg
    image = Image.new("RGB", (width+20, height+20), (255, 255, 255))
    for y in range(top, top+height):
        for x in range(left, left+width):
            nx, ny = mapping_point(model, x, y)
            image.putpixel((x, y), pixel(nx, ny))
    if bad == "shift":
        before = image.copy()
        for y in range(top, top+height):
            for x in range(left+1, left+width):
                image.putpixel((x, y), before.getpixel((x-1, y)))
    if bad == "color":
        for y in range(top, top+height):
            for x in range(left, left+width):
                if image.getpixel((x, y)) == (145, 168, 111):
                    image.putpixel((x, y), (144, 168, 111))
    png = root/(name+".png")
    image.save(png)
    rgba = bytearray()
    rois = []
    for x, y, w, h in tiles:
        rois.append({"x": x, "y": y, "width": w, "height": h, "stride": w*4, "byteOffset": len(rgba)})
        for yy in range(y, y+h):
            for xx in range(x, x+w):
                rgba.extend((*pixel(xx, yy), 255))
    targets = {}
    for target in ("candidate", "back"):
        raw = root/(name+"-"+target+".rgba")
        raw.write_bytes(rgba)
        targets[target] = {"name": raw.name, "bytes": len(rgba), "sha256": digest(rgba)}
    hold = {"token": token, "requestId": seq, "frame": seq, "sequence": seq,
            "startMonoMs": 10, "name": "CONTROL_"+kind}
    phase = {"hold": hold, "sequence": seq, "name": "CONTROL_"+kind,
             "sample": {"token": token, "digest": "fixed-optics", "authority": {"run": "test"},
                        "projectionXY": [32,30*256/nh]+([96,30*256/nh] if kind=="B" else []),
                        "projectionMeta":[512,1]+([512,2] if kind=="B" else []),"displayBed":[0]*128}}
    probe = {"captureStatus": "CAPTURED", "captureStage": "COMPLETE", "presentationState": "CURRENT",
             "swapSucceeded": True, "unbindSucceeded": True, "frameSerial": seq, "requestId": seq,
             "requestedToken": token, "actualToken": token, "session": 1,
             "surfaceEpoch": 2, "contextEpoch": 3, "geometryEpoch": 4,
             "windowWidth": nw, "windowHeight": nh, "boxX": 0, "boxY": 0, "boxWidth": float(nw),
             "boxHeight": float(nh), "scale": 1., "inverted": False, "rois": rois,
             "candidateNs":"1","backNs":"2","captureEndNs":"3","swapStartNs":"4","swapEndNs":"5","unbindEndNs":"6",
             "configValues":[0]*6+[4],
             "calibration": kind == "CAL", "calibrationShaderSha256": digest(Path(__file__).with_name("calibration.glsl").read_bytes()),
             **targets}
    manifest = root/(name+"-manifest.json")
    write(manifest, {"probe": probe, "phase": phase,
                     "calibrationId": registered["calibrationId"] if registered else "",
                     "roiPlanId": registered["planId"] if registered else "", **targets})
    manifest.with_suffix(".sha256").write_text(digest(manifest.read_bytes())+"  "+manifest.name)
    hold_path = root/(name+"-hold.json")
    write(hold_path, {"hold": hold, "endToken": token, "endFrame": seq, "endMonoMs": 100,
                     "geometry": json.dumps({"revision": "T406_GEOMETRY_V1", "globalX": left,
                                             "globalY": top, "widthVp": nw, "heightVp": nh, "vp2px": 1})})
    capture = root/(name+"-capture.json")
    write(capture, {"revision":"T406_CAPTURE_CALL_V1","clock":"PYTHON_MONOTONIC_NS","fileSha256": digest(png.read_bytes()), "exitCode": 0, "tool": "fixture",
                    "command": ["fixture"], "startMonoNs": 20000000, "endMonoNs": 30000000})
    spec = root/(name+".json")
    write(spec, {"manifest": manifest.name, "hold": hold_path.name, "capture": capture.name, "png": png.name,
                 "deviceId": "host-fixture", "toolConfiguration": "PNG_UNSCALED",
                 "screenshotSize": list(image.size), "bounds": [left, top, left+nw, top+nh],
                 "rotation": 0, "screenshotScale": 1,
                 "clockMap": {"source": "HOST_FIXTURE_SHARED", "offsetMs": 0, "uncertaintyMs": 0, "hostToDeviceRate": 1},
                 "registration": {"source": "INDEPENDENT_GEOMETRY_PREDECLARED", "models": [model]}})
    return spec


def rejected(action, label):
    try:
        value=action()
    except (ValueError,KeyError,TypeError,OSError) as exc:
        return str(exc)
    assert value.get("displayStatus")=="DISPLAY_NOT_JUDGABLE",label+" accepted"
    return value.get("reason",label)


def mutate_manifest(spec_path, callback):
    spec=json.loads(spec_path.read_text());path=spec_path.parent/spec["manifest"]
    record=json.loads(path.read_text());callback(record);write(path,record)
    path.with_suffix(".sha256").write_text(digest(path.read_bytes())+"  "+path.name)


def replace_png(spec_path, source):
    spec=json.loads(spec_path.read_text());png=spec_path.parent/spec["png"]
    png.write_bytes(source.read_bytes())
    receipt=spec_path.parent/spec["capture"];data=json.loads(receipt.read_text())
    data["fileSha256"]=digest(png.read_bytes());write(receipt,data)


with TemporaryDirectory() as directory:
    root = Path(directory)
    cases = []
    for nw, sw in ((118,118),(1178,1177)):
        model = {"native": [nw, 68], "coverage": [4, 4, sw, 68],
                 "rule": "PIXEL_CENTER_NEAREST", "mirrorX": False, "mirrorY": False}
        calibration_spec = artifact(root, "cal"+str(sw), model, "CAL", 1,
                                    [[0, 0, 16, 16], [nw//2-8, 25, 16, 16], [nw-16, 52, 16, 16]])
        calibration_path = root/("calibration"+str(sw)+".json")
        cal = calibrate(calibration_spec, calibration_path)
        assert cal["validationPoints"] == sw+68
        roi_path = root/("plan"+str(sw)+".json")
        p = plan(calibration_path, [[4+sw//4-10,32,20,20],[4+3*sw//4-10,32,20,20],[4+sw//2-10,7,20,12]], roi_path)
        a = artifact(root, "a"+str(sw), model, "A", 2, p["nativeTiles"], registered=p)
        b = artifact(root, "b"+str(sw), model, "B", 3, p["nativeTiles"], registered=p)
        assert pair(a, b, calibration_path, roi_path)["displayStatus"] == "DISPLAY_CALIBRATED_A_AND_B_PAIRED"
        for fault in ("shift", "color"):
            negative = artifact(root, fault+str(sw), model, "B", 3, p["nativeTiles"], fault, p)
            result = pair(a, negative, calibration_path, roi_path)
            assert result["displayStatus"] == "DISPLAY_NOT_JUDGABLE"
            assert result["b"]["regions"][1]["differenceCount"] > 0
        controls={}
        for fault in ("nativeRadius","missingB"):
            bad=artifact(root,fault+str(sw),model,"B",3,p["nativeTiles"],fault,p)
            controls[fault]=rejected(lambda:pair(a,bad,calibration_path,roi_path),fault)
        both_a=artifact(root,"bothA"+str(sw),model,"A",2,p["nativeTiles"],"shift",p)
        both_b=artifact(root,"bothB"+str(sw),model,"B",3,p["nativeTiles"],"shift",p)
        controls["bothShift"]=rejected(lambda:pair(both_a,both_b,calibration_path,roi_path),"both shift")
        repeat=artifact(root,"repeat"+str(sw),model,"B",3,p["nativeTiles"],registered=p)
        aspec=json.loads(a.read_text());replace_png(repeat,root/aspec["png"])
        assert pair(a,repeat,calibration_path,roi_path)["displayStatus"]=="DISPLAY_REPEATS_A_IN_B_HOLD"
        controls["repeatA"]="DISPLAY_REPEATS_A_IN_B_HOLD"
        tiles=[t[:] for t in p["nativeTiles"]];tiles[1][2]-=1
        missing=artifact(root,"missing"+str(sw),model,"B",3,tiles,registered=p)
        controls["missingColumn"]=rejected(lambda:pair(a,missing,calibration_path,roi_path),"missing column")
        for field in ("contextEpoch","geometryEpoch","windowWidth"):
            bad=artifact(root,field+str(sw),model,"B",3,p["nativeTiles"],registered=p)
            mutate_manifest(bad,lambda m:m["probe"].update({field:m["probe"][field]+1}))
            controls[field]=rejected(lambda:pair(a,bad,calibration_path,roi_path),field)
        for fault in ("mirrorX","mirrorY","width","offset","halfPixel","ratio"):
            cspec=json.loads(calibration_spec.read_text());wrong=dict(model)
            wrong["coverage"]=model["coverage"][:]
            if fault in ("mirrorX","mirrorY"):wrong[fault]=True
            elif fault=="width":wrong["coverage"][2]-=1
            elif fault=="offset":wrong["coverage"][0]+=1
            elif fault=="halfPixel":wrong["coverage"][0]+=.5
            else:wrong["coverage"][3]-=1
            cspec["registration"]["models"]=[wrong]
            bad_cal=root/("badCal"+fault+str(sw)+".json");write(bad_cal,cspec)
            controls[fault]=rejected(lambda:calibrate(bad_cal,root/"invalid-cal.json"),fault)
        sha_bad=artifact(root,"shaBad"+str(sw),model,"B",3,p["nativeTiles"],registered=p)
        sha_spec=json.loads(sha_bad.read_text());manifest=json.loads((root/sha_spec["manifest"]).read_text())
        raw=root/manifest["back"]["name"];raw.write_bytes(bytes([0])+raw.read_bytes()[1:])
        controls["rawSha"]=rejected(lambda:pair(a,sha_bad,calibration_path,roi_path),"raw SHA")
        static=artifact(root,"static"+str(sw),model,"A",2,p["nativeTiles"],registered=p)
        mutate_manifest(static,lambda m:m["phase"].update({"name":"STATIC_NEUTRAL"}))
        assert single(static,calibration_path,roi_path)["displayStatus"]=="DISPLAY_CALIBRATED_SINGLE_MATCH"
        spec = json.loads(b.read_text())
        changed = {**spec, "deviceId": "other"}
        write(b, changed)
        try:
            pair(a, b, calibration_path, roi_path)
        except ValueError as exc:
            assert str(exc) == "CALIBRATION_EXPIRED_IDENTITY"
        else:
            raise AssertionError("identity accepted")
        write(b, spec)
        cap = root/spec["capture"]
        receipt = json.loads(cap.read_text())
        write(cap, {**receipt, "endMonoNs": 200000000})
        try:
            pair(a, b, calibration_path, roi_path)
        except ValueError as exc:
            assert str(exc) == "CAPTURE_INTERVAL_OUTSIDE_HOLD"
        else:
            raise AssertionError("late capture accepted")
        cases.append({"screenWidth": sw, "paired": True, "shiftRejected": True, "code1Rejected": True,
                      "expiredCalibrationRejected": True, "captureIntervalRejected": True, "negativeControls":controls,
                      "staticSingle":True})
    print(json.dumps({"revision": "T406_TOOL_UNITS_V2", "cases": cases}))
