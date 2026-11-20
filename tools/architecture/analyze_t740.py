"""Audit the original QUANTUM schedule and T738 evidence without using a device."""
from collections import Counter
from itertools import groupby, pairwise
import json
from pathlib import Path
import re
from zipfile import ZipFile

from analyze_t660_t662 import ROOT, build_id, record, sha, write
from analyze_t733 import COST_COMPARISON_GROUPS, GROUPS


SOURCE = "entry/src/main/cpp/gpu_resident/"
T738 = "test/reports/t491/t738/"
SCHEDULE = "docs/architecture-T554-g9-dm-dispatch-v2.json"
SCOPES = {"ROOT": 0, "DISCOVERY": 1, "AUTHORITATIVE": 2, "REJECTED_PREFIX_CHECK": 3, "REPLAY": 4}


def document(path):
    return json.loads((ROOT / path).read_bytes())


def rows_from_header(header, opcode):
    text = header.read_text(encoding="utf-8")
    start = re.search(r"kQuantumPasses\s*=\s*\{\{(.*?)\}\};", text, re.S)[1]
    return [[opcode[name], int(role), int(j), int(micro)] for name, role, j, micro in
            re.findall(r"\{Op::(\w+),\s*(\d+),\s*(\d+),\s*(-?\d+)\}", start)]


def profile_transfer(header):
    profiles = {}
    for line in header.read_text(encoding="utf-8").splitlines():
        if not line.startswith(("{192,", "{512,")):
            continue
        values, ranges = line[1:].split(",{{{", 1)
        k, _, _, state, work, arena, *_ = map(int, values.split(","))
        offsets = re.findall(r"\{(\d+),(\d+)\}", "{" + ranges)
        assert len(offsets) == 23
        end, capacity = map(int, offsets[8 + 8])
        assert capacity >= 1024 and end + 1024 <= work
        profiles[str(k)] = {"completionMapOffsetBytes": end, "completionMapBytes": 1024,
                            "rootCopyBytes": state + 2 * arena,
                            "fullCopyBytes": state + 8 * arena}
    assert len(profiles) == 2
    return profiles


def plan_cost(opcodes, groups):
    assignment = {opcode: role for role, ops in groups.items() for opcode in ops}
    assert len(assignment) == sum(len(group) for group in groups.values())
    roles = [assignment[op] for op in opcodes]
    transitions = Counter(f"{a}>{b}" for a, b in pairwise(roles) if a != b)
    return {"dispatchesByProgram": dict(Counter(roles)), "switches": sum(transitions.values()),
            "switchesByEdge": dict(sorted(transitions.items())),
            "firstRole": roles[0], "lastRole": roles[-1]}


def owner_bindings(quantum):
    rows = []
    copies = {"selectionBefore": [], "authorityAfter": [], "fullTrialAfter": []}
    for pass_row in quantum:
        ordinal, op, role = pass_row["index"], pass_row["op"], SCOPES[pass_row["role"]]
        read, write_bank, events = "BASE", "CANDIDATE", "ACTIVE_EVENTS"
        if op == "BASE_FREEZE":
            read, write_bank = "CURRENT", "BASE"
        if role in (3, 4):
            write_bank = "REPLAY"
        if op == "FINAL_ENERGY_CURSORS" and role in (2, 4):
            read, write_bank, events = "SELECTED_EVENTS", "ACTIVE_EVENTS", "BASE"
        if op == "SCOPE_SEAL_AND_CHOICE" and role == 4:
            read, write_bank = "CANDIDATE", "REPLAY"
        if op == "COMPARE_SELECTED_AUTHORITATIVE":
            read, write_bank = "SELECTED", "CANDIDATE"
        if op == "ROOT_SEAL":
            read, write_bank = "BASE", "CANDIDATE"
        slots = [read, write_bank, "WORK", events]
        rows.append(slots)
        if op == "DISCOVERY_COMMIT_SELECTION":
            copies["selectionBefore"].append(ordinal)
        if op == "FINAL_ENERGY_CURSORS" and role == 2:
            copies["authorityAfter"].append(ordinal)
        if op == "SCOPE_SEAL_AND_CHOICE" and role == 1:
            copies["fullTrialAfter"].append(ordinal)
    assert copies == {"selectionBefore": [808], "authorityAfter": [1200],
                      "fullTrialAfter": [23, 55, 111, 215, 415, 807]}
    changes = []
    for slots, run in groupby(enumerate(rows), key=lambda pair: pair[1]):
        ordinals = [index for index, _ in run]
        if slots != ["BASE", "CANDIDATE", "WORK", "ACTIVE_EVENTS"]:
            changes.append({"first": ordinals[0], "lastInclusive": ordinals[-1], "slots": slots})
    alias_changes = [sum(left[i] != right[i] for left, right in pairwise(rows)) for i in range(4)]
    return {"exceptions": changes, "copiesAt": copies,
            "staticBaseCallsOriginal": 4 * len(quantum),
            "adjacentLogicalBindingChanges": alias_changes,
            "minInstallCallsWithinUninterruptedRoot": 4 + sum(alias_changes)}


def schedules(compile_report):
    formal = document(SCHEDULE)
    quantum = formal["quantum"]
    opcode = formal["opcodeByName"]
    assert [p["index"] for p in quantum] == list(range(2404))
    payload = [[opcode[p["op"]], SCOPES[p["role"]], p["Jslot"], p["micro"]] for p in quantum]
    disp_sha = sha(json.dumps(payload, separators=(",", ":")).encode())
    t733 = document("docs/architecture-T733-closure-v1.json")["families"]
    families, profiles = {}, {}
    parents = {}
    for family, row in compile_report.items():
        header = (ROOT / row["generatedBootPath"]).with_name("schema_generated.h")
        assert rows_from_header(header, opcode) == payload
        assert t733[family]["schedule"]["Quantum"]["passesSHA"] == disp_sha
        transfer = profile_transfer(header)
        assert not profiles or transfer == profiles
        profiles = transfer
        parents[family] = {"schema": record(header.relative_to(ROOT).as_posix()),
                           "bootRoleSHA": row["role"]["sourceSHA"]}
        families[family] = {}
        for arm, previous in [("nine", t733[family]), ("eight", t733[family]["costComparison"])]:
            roles = previous["roles"]
            assert all(item["staticCompileExit"] == 0 and item["sourceBytes"] <= 49152 for item in roles)
            families[family][arm] = [{"role": r["role"], "bytes": r["sourceBytes"], "sha": r["sourceSHA"],
                                     "opcodes": r["opcodes"]} for r in roles]
    main = plan_cost([row[0] for row in payload], GROUPS)
    alternative = plan_cost([row[0] for row in payload], COST_COMPARISON_GROUPS)
    assert (main["switches"], alternative["switches"]) == (802, 612)
    assert main["firstRole"] == "BOOT" and main["lastRole"] == "ROOT_FINAL"
    # Check that both implementations still have the same opcode-dependent Bind/Dispatch.
    bodies = []
    for family in ("g9rx", "g9dm"):
        path = ROOT / SOURCE / family / "transport.cpp"
        source = path.read_text(encoding="utf-8")
        body = source[source.index("void Transport::Bind("):source.index("Result Transport::Pump(")]
        bodies.append("".join(body.split()))
        assert "while (next_ < count && submitted < 10)" in source and ">= 8000000" in source
    assert bodies[0] == bodies[1]
    bridge = ROOT / SOURCE / "g9rx/bridge.cpp"
    source = bridge.read_text(encoding="utf-8")
    wait = re.search(r"wake_\.wait_for\(lock, std::chrono::milliseconds\(2\),(.*?)\}\);", source, re.S)[1]
    assert "RootLive" not in wait and "ownerRootLive" not in wait
    mins = (len(quantum) + 9) // 10
    return {"formal": record(SCHEDULE), "passSHA": disp_sha, "schemas": parents, "families": families,
            "scopes": dict(Counter(p["role"] for p in quantum)), "nineRoleCost": main, "eightRoleCost": alternative,
            "bindings": owner_bindings(quantum), "profiles": profiles,
            "cadence": {"minPumpBatches": mins, "rootDeadlineNs": 250000000,
                        "rxIdleWaitNs": 2000000, "betweenBatchesAtIdleWaitNs": (mins - 1) * 2000000,
                        "bridge": record(bridge.relative_to(ROOT).as_posix())}}


def boot_evidence(compile_report):
    receipts = {}
    for family, info in compile_report.items():
        receipt_path = f"test/reports/t257/T738-{family.lower()}.json"
        receipt = document(receipt_path)
        signed = Path(receipt["signed"]["path"])
        assert sha(signed.read_bytes()) == receipt["signed"]["sha256"] and receipt["verifyApp"] == "PASS"
        lib = {"RX": "libgrs_g9rx.so", "DM": "libgrs_dm_t647.so"}[family]
        with ZipFile(signed) as archive:
            raw = archive.read("libs/arm64-v8a/" + lib)
        assert sha(raw) == info["unsignedHap"]["packagedElfSHA256"] and build_id(raw) == info["linkedElf"]["buildId"]
        receipts[family] = {"receipt": record(receipt_path), "signedSHA": receipt["signed"]["sha256"],
                            "packagedDsoSHA": sha(raw), "dsoBuildId": build_id(raw)}
    path = T738 + "h738rx2.txt"
    log = (ROOT / path).read_text(encoding="utf-8-sig")
    rows = [json.loads(line.split("G9Boot: ", 1)[1]) for line in log.splitlines()
            if "G9Boot: {" in line]
    stages = {row["stage"]: row for row in rows}
    a = stages["UNMAP_EXIT"]
    assert (a["gl"], a["egl"], a["error"], a["seal"], a["raw"]) == (0, 12288, 0, 0x641647, 1)
    assert a["mappedBuildId"] == receipts["RX"]["dsoBuildId"]
    assert a["sourceSHA"] == compile_report["RX"]["role"]["sourceSHA"]
    assert int(stages["FINISH_EXIT"]["ns"]) < int(a["ns"]) < int(stages["BOOT_AUDIT_OK"]["ns"])
    assert len({(r["pid"], r["tid"], r["run"], r["owner"], r["context"],
                 r["sourceSHA"], r["mappedBuildId"]) for r in rows}) == 1
    assert stages["FINISH_EXIT"]["raw"] in (37146, 37148)
    assert "ticket=1 ns=7269532242656 phase=BOOT_AUDIT_OK run=1 context=1 result=1638" in log
    receipts["RX"].update(runtimeOriginal=record(path), stages=stages,
                          observedRawTail=[a["error"], a["seal"]], processStart="UNOBSERVED",
                          freshProcessCacheHit="UNOBSERVED")
    dm_path = T738 + "dm20-boot.jsonl"
    dm = [json.loads(line) for line in (ROOT / dm_path).read_bytes().splitlines() if line.strip()]
    assert dm[1]["sourceSHA"] == compile_report["DM"]["role"]["sourceSHA"]
    assert dm[-1]["result"] == 1638 and dm[-1]["rootsAuthorized"] is False
    binary = compile_report["DM"]["standalone"]
    elf = (ROOT / binary["path"]).read_bytes()
    assert sha(elf) == binary["sha256"] and build_id(elf) == binary["buildId"]
    receipts["DM"].update(runtimeOriginal=record(dm_path), status=dm[-1],
                          standaloneExpectedBuildId=binary["buildId"], observedRawTail="UNOBSERVED",
                          loadedDso="UNOBSERVED_IN_ATTEMPT", processStart="UNOBSERVED")
    return receipts


if __name__ == "__main__":
    compile_report = document("test/reports/t734/compile.json")["families"]
    evidence = {"schema": "T740_RX_DM_QUANTUM_CONTINUATION_V1",
                "deviceUsedForThisAudit": False, "quantumExecution": "NOT_RUN",
                "firstQuantumSwap": "NOT_RUN", "sourceContinuationMeasured": False,
                "schedule": schedules(compile_report), "historicalBoot": boot_evidence(compile_report)}
    write("docs/architecture-T740-quantum-contract-v1.json", evidence)
    print("T740 checked actual generated 2,404 passes, role transitions, ownership cadence and T738 originals")
