"""Offline T592 v3 evidence/source check; no device, product, generator or test run."""
import json
from pathlib import Path
import re
import zipfile
from analyze_t660_t662 import ROOT, record, sha, ui_texts, write

product = ".codex/local-test/t659-delivery-source/.codex/local-test/product/"
ets = product + "entry/src/main/ets/"
paths = ["docs/test-report-T592-v3.md", "docs/developer-T659-v1.md", "test/reports/t659/compile-receipt.json", "test/reports/t491/t592b/v3-fix9.json", "test/reports/t257/T592-fix9.json", "test/t540/RasterGeometry.ets", "test/t540/page_raster.inc", "test/t540/overlay.py", "tools/singlesand/prepare_owner_build.py"]
paths += [ets + p for p in ["pages/P051DerivedV2Demo.ets", "render/RasterGeometry.ets", "render/PrepareReadiness.ets", "render/PrepareReadinessV4.ets", "render/P013SingleSandController.ets", "render/T413Capture.ets"]]
sources = [record(p) for p in paths]
raster = (ROOT / (ets + "render/RasterGeometry.ets")).read_text(encoding="utf-8-sig")
predicate = raster[raster.index("export function rasterPredicate"):raster.index("export function rasterNodeReason")]
assert "n.geometry!==geometry" in predicate and "n.packetWidth!==p.rasterW" in predicate
assert "p.revision!==p.generation" not in predicate
ui = []
for p in sorted((ROOT / "test/reports/t491/t592b").glob("fix9_l*.json")):
    texts = list(ui_texts(json.loads(p.read_text(encoding="utf-8-sig"))))
    relevant = [s for s in texts if any(k in s for k in ["RASTER_NATIVE", "PREPARE_TIMEOUT", "planRevision", "T532", "FAILED", "准备"])]
    ui.append({**record(p.relative_to(ROOT).as_posix()), "applicationEvidenceTexts": relevant, "allTextCount": len(texts)})
signature = json.loads((ROOT / "test/reports/t257/T592-fix9.json").read_text(encoding="utf-8-sig"))
signed = Path(signature["signed"]["path"])
package = {"path": str(signed), "bytes": signed.stat().st_size, "sha256": sha(signed.read_bytes()), "receiptMatched": sha(signed.read_bytes()) == signature["signed"]["sha256"]}
with zipfile.ZipFile(signed) as z:
    package["payloads"] = [{"entry": p, "bytes": len(z.read(p)), "sha256": sha(z.read(p))} for p in ["ets/modules.abc", "libs/arm64-v8a/libp013renderer.so"]]
assert package["receiptMatched"]
assert package["payloads"][0]["sha256"] == "28077347107671a7013380238b16adff0ce23ac91700be3b6cfc0d684b2dc6fe"
assert package["payloads"][1]["sha256"] == "4a18eb40342e63d229828c190d296bbd5a980154f983758766337b55530920f7"
raw = {"schema": "T592_V3_RASTER_FIRST_ATTRIBUTION_V1", "deviceUsed": False, "buildRun": False, "productTestsRun": False,
       "sourceFiles": sources, "actualSignedArtifact": package, "uiOriginals": ui,
       "screenshots": [record(p.relative_to(ROOT).as_posix()) for p in sorted((ROOT / "test/reports/t491/t592b").glob("fix9-screen*.jpeg"))],
       "visuallyInspected": {"path": "test/reports/t491/t592b/fix9-screen4.jpeg", "visible": {"state": "FAILED PREPARE_TIMEOUT:RASTER_NATIVE_MISMATCH", "captureState": "IDLE", "rasterRevision": "T532_WINDOW_HOST_RASTER_V3", "planRevision": 9, "rasterGeneration": 9, "geometry": 11, "rasterWidth": 1154, "rasterHeight": 1644, "reserve": "visibly disabled"}, "fullNativeFieldsVisible": False, "notStrictPNGCalibration": True},
       "predicateOriginal": predicate,
       "predicateMeaning": {"revision": "protocol string compared only with fixed T532_WINDOW_HOST_RASTER_V3", "generation": "per-plan monotonic generation, screenshot planRevision9 == raster.generation9", "nativeGate": ["nonzero controller", "nonzero session", "nonzero surface", "nonzero context", "nonempty run", "native.geometry == current geometry", "native.packetWidth == rasterW", "native.packetHeight == rasterH", "native.viewportWidth == rasterW", "native.viewportHeight == rasterH"]},
       "reportInterpretationRejected": "protocol name T532_WINDOW_HOST_RASTER_V3 vs generation9 is not a revision mismatch predicate; screenshot shows generation pairing9=9",
       "oldCrashClaimLimit": "this new prepare no same() crash reported; does not prove every owner/ticket negative control or lifetime passed",
       "exactFailingField": "NOT_CAPTURED; current screenshots truncate before raster.native; fix9_l1..l8 expose system UI texts, not full application readiness payload",
       "existingColdExport": {"producer": "P051DerivedV2Demo.prepareUpdate calls prepareCold on state transition", "filePattern": "t540-prepare-<session>-<pageGeneration>-<prepareId>-<state>-<observationSerial>.json", "maxUTF8Bytes": 16384, "snapshotPath": "readiness.authority.snapshot", "rasterNativePath": "readiness.authority.snapshot.raster.native", "APIPath": "readiness.authority.api", "truncationFlags": ["readiness.detailTruncated", "observationTruncated", "readiness.reason=PREPARE_COLD_UTF8_LIMIT"], "contains": ["readiness.authority snapshot when not detailTruncated", "raster.native", "frame observation", "owner/API first-last error"], "newRunRawFileInSavedEvidence": False, "historicalFile": "test/reports/t491/t408vA/t540-prepare-s1794916671861-1-1-1-PREPARING-38.json", "historicalFileCannotSubstitute": True},
       "sourceCandidateNotProvenDeviceCause": "publishRaster advances geometry on actual layout/native identity/size changes; native.geometry excluded from observe change-key explicitly avoids geometry-only feedback. Do not blindly change this policy or claim a proven loop from one screenshot",
       "verdict": "NATIVE_PAIRING_GATE_PRECISE_SUBFIELD_NOT_YET_CAPTURED; NOT_PROTOCOL_NAME_VS_GENERATION_MISMATCH",
       "handoff": "effector original T592 retrieves existing same-attempt FAILED prepare cold first, tester T671 parses/offline prepares; exact code mismatch only then architect->developer; no guessed version bump/designer/relaxed gate"}
write("docs/analysis-T592-raster-v1.json", raw)
contract = {"schema": "T592_RASTER_NATIVE_FIRST_ATTRIBUTION_V1", "date": "2026-11-19", "deviceUsed": False, "buildRun": False, "productTestsRun": False, "productPASS": False,
            "verdict": raw["verdict"], "evidence": "docs/analysis-T592-raster-v1.json", "predicate": raw["predicateMeaning"], "visible": raw["visuallyInspected"]["visible"],
            "fieldProducers": {"controller": "P013SingleSandController.session", "session": "preparation.packet.session", "surface/context": "native.getState surfaceEpoch/contextEpoch", "run/geometry/packetDimensions": "actual preparation.packet", "viewport": "native.getState widthPx/heightPx"},
            "cold": raw["existingColdExport"], "roles": {"architect": "first attribution only; no T592 state takeover", "effector": "original T592 same-attempt device cold-file retrieval/execution", "tester": "T671 offline parser/fixture/build/signature", "developer": "only concrete engineering gap after exact raw evidence and architect revision"},
            "noUnsupportedFix": ["compare protocol string to generation", "force revision/generation values", "copy latest geometry to stale packet", "relax size/identity predicates", "increase PREPARING3000ms or add READY TTL", "JPEG as strict PNG"],
            "boundaries": {"queueTouched": False, "CPU": "STOPPED", "goal10": "PAUSED", "waterOrder": [13, 14, 12], "render": "XComponent+shader foreground/background only; FAULT/explicitly STALE; no Canvas/softwareGL product fallback", "managerGateAdded": False},
            "executionRequirement": "要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。"}
write("docs/architecture-T592-raster-contract-v1.json", contract)
print(json.dumps({"verdict": raw["verdict"], "signedHashMatched": package["receiptMatched"], "UIFiles": len(ui), "nativeSubconditions": len(raw["predicateMeaning"]["nativeGate"]), "productTestsRun": False}, ensure_ascii=True))
