"""T66 offline raw export manifest; never deduplicates records or repairs a prefix."""
import argparse
import hashlib
import json
from pathlib import Path


def manifest(ledger, admissions, fence):
    def stream(path):
        data = Path(path).read_bytes()
        rows = [json.loads(line) for line in data.splitlines() if line.strip()]
        return rows, {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest(), "rawRows": len(rows)}
    rows, left = stream(ledger)
    events, right = stream(admissions)
    closed = (fence.get("state") == "CLOSED" and
              fence.get("sentRecords") == fence.get("writtenRecords") == len(rows) and
              fence.get("sentAdmissions") == fence.get("writtenAdmissions") == len(events))
    for row in rows:
        context = row.get("context", {})
        if context.get("runId") != fence.get("runId") or context.get("identityRevision") != fence.get("identityRevision"):
            closed = False
    if rows:
        last = rows[-1].get("context", {})
        for raw, frozen in [("sequence", "lastSequence"), ("frameId", "lastFrameId"),
                            ("substepId", "lastSubstepId"), ("attemptId", "lastAttemptId"),
                            ("sourceEpoch", "lastSourceEpoch"), ("observationRouteId", "routeId")]:
            if raw not in last or frozen not in fence or last[raw] != fence[frozen]:
                closed = False
    for event in events:
        if event.get("runId") != fence.get("runId"):
            closed = False
    return {"revision": "P011_EXPORT_MANIFEST_V2", "status": "COMPLETE" if closed else "PREFIX_OR_MISMATCH",
            "fence": fence, "ledger": left, "admissions": right,
            "lastRawContext": rows[-1].get("context") if rows else None}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ledger")
    parser.add_argument("admissions")
    parser.add_argument("fence", help="Exact P011_OBS_STORE_FENCE JSON payload")
    parser.add_argument("output")
    args = parser.parse_args()
    result = manifest(args.ledger, args.admissions, json.loads(Path(args.fence).read_text(encoding="utf-8")))
    Path(args.output).write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    raise SystemExit(0 if result["status"] == "COMPLETE" else 1)
