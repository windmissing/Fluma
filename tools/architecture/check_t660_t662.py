"""Finite contract/original-file consistency checks; never executes product/test/native code."""
import json
from pathlib import Path
import subprocess
import sys
from analyze_t660_t662 import ROOT, record, write


def read(name):
    return json.loads((ROOT / name).read_text(encoding="utf-8-sig"))


subprocess.run([sys.executable, str(ROOT / "tools/architecture/freeze_t660_contract.py"), "--check"], check=True)
for n in [660, 661, 662]:
    c = read(f"docs/architecture-T{n}-contract-v1.json")
    assert c.get("deviceUsed") is False
    for x in [f"docs/architecture-T{n}-v1.md", f"docs/analysis-T{n}-evidence-v1.json"]:
        assert (ROOT / x).exists()

c = read("docs/architecture-T662-contract-v1.json")
fields = c["cursor"]["fields"]
assert fields[0]["offset"] == 0
assert all(a["offset"] + a["bytes"] == b["offset"] for a, b in zip(fields, fields[1:]))
assert fields[-1]["offset"] + fields[-1]["bytes"] == c["cursor"]["bytesPerAttempt"] == 128
assert 128 * c["cursor"]["attemptSlots"] == c["cursor"]["additionalNativePayloadBytes"] == 384
e = read("docs/analysis-T662-evidence-v1.json")
assert e["sequenceContiguous"] and e["identityStableAcross39AttemptSamples"]
assert e["diagnosticQueries"] == 40 and e["objectLogQueries"] == 24 and e["nativeQueryCount"] == 64
assert not e["actualTerminalRows"] and e["falseTerminalSeen"] == 38
assert e["original"] == record(e["original"]["path"])
assert e["package"]["signedHashMatches"]
assert all(len(x["embeddedSources"]) == 1 for x in e["plannedPackageProgramsNotRuntimeSubstitutes"])

c = read("docs/architecture-T661-contract-v1.json")
segs = c["segments"]
assert segs[0]["byteRange"][0] == 0 and segs[-1]["byteRange"][1] == 32768
assert all(a["byteRange"][1] == b["byteRange"][0] for a, b in zip(segs, segs[1:]))
assert all(x["count"] * x["stride"] == x["byteRange"][1] - x["byteRange"][0] for x in segs)
e = read("docs/analysis-T661-evidence-v1.json")
assert len(e["uiOriginals"]) == 24 and len(e["signedVersions"]) == 7
assert all(v["signedHashMatches"] for v in e["signedVersions"])
assert all(v["identical"] for v in e["currentToBuildTreeSourceComparison"])
assert all(s["chars"] == 600 and not s["completeJSON"] for cap in e["captures"] for s in cap["statuses"])
libs = [next(m for m in v["modules"] if m["entry"].endswith("libgrs_g13.so")) for v in e["signedVersions"]]
assert len(set(v["sha256"] for v in libs)) == len(set(v["buildId"] for v in libs)) == 1

m = read("docs/architecture-T660-g11-manifest-v2.json")
for item in m["inputs"] + [m["visualSeedRecipe"], m["amendment"], m["predecessor"]]:
    assert item == record(item["path"]), "STALE_INPUT " + item["path"]
c = read("docs/architecture-T660-contract-v1.json")
assert c["seed"]["expected224Words"] == [144, 152]
assert c["seed"]["completion256Words"] == [224, 232]
assert c["ghost"]["defaultBlendTicks"] == 256 and c["ghost"]["markerWord30"]["name"] == "phaseOldPacketMassQ20"
assert m["cold"]["capability"] == "G11_VIS_COLD_V2" and m["cold"]["page"] == "G11_VIS_SEGMENT_PAGE_V2"
for path, marker in [("docs/architecture-T563-g11-accepted-flux-visual-v1.md", "T660 正式修订"), ("docs/architecture-T634-v1.md", "T660 接续修订"), ("docs/architecture-T647-v1.md", "T660：actualVisualSeed"), ("docs/architecture-T649-v1.md", "T661 原件归因"), ("docs/architecture-T651-v1.md", "T662 采集终态")]:
    assert marker in (ROOT / path).read_text(encoding="utf-8-sig")

out = {"schema": "T660_T662_ARCHITECTURE_CHECK_V1", "result": "STATIC_CONTRACT_AND_ORIGINAL_BYTE_CHECK_PASS", "deviceUsed": False, "productTestsRun": False, "buildRun": False, "productPASS": False,
       "checks": ["8 resource rows and seed byte spans/witness capacity", "65536 control partition and T647928B ranges", "128word512B owner bookkeeping partition", "128B cursor/384B three-attempt payload", "T591103 rows/64calls/38false terminal and actual embedded source bytes", "T65424 UI originals/3 incomplete600char status strings/7 actual packages and same G13 ELF", "authoritative input hashes and revision2 cold/seed/gate ABI links"],
       "authorities": [record(f"docs/architecture-T{n}-contract-v1.json") for n in [660, 661, 662]]}
write("docs/analysis-T660-T662-check-v1.json", out)
print(json.dumps(out, ensure_ascii=False, indent=2))
