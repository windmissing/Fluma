"""Bound T960 offer observations and empty boundaries from T966's archival trace."""

from collections import Counter
import gzip
import hashlib
import json
from pathlib import Path


def audit(data):
    rows = list(map(json.loads, data.splitlines()))
    pointer = [r for r in rows if r.get("stage") == "WIN32_POINTER"]
    down = [int(r["qpc"]) for r in pointer if r["kind"] == "DOWN"]
    up = [int(r["qpc"]) for r in pointer if r["kind"] == "UP"]
    assert len(down) == len(up) == 2 and down[0] < up[0] < down[1] < up[1]
    assert int(next(r["qpcFrequency"] for r in rows if "qpcFrequency" in r)) == 10**7
    first_ns = down[0] * 100
    deadline_ns = first_ns + 4 * 10**9
    cuts = (down[0], up[0], down[1], up[1])
    labels = ("before_hold1", "hold1", "between_holds", "hold2", "after_hold2")

    def region(tick):
        return labels[sum(tick >= cut for cut in cuts)]

    counts, empty_intervals, nonempty = Counter(), [], []
    previous_q_fence_qpc = None
    for line, row in enumerate(rows, 1):
        boundary = row.get("m1SourceBoundary")
        q_authority = row.get("m1QuantumAuthority", {})
        if boundary is not None:
            offer = int(boundary["offer"])
            counts["nonempty" if offer else "empty"] += 1
            if offer:
                pre_take_ns = int(row["m1PulseNs"]) + int(boundary["ageNs"])
                nonempty.append(dict(line=line, offer=offer, hold=int(boundary["hold"]),
                                     preTakeNs=pre_take_ns, sourceFenceNs=int(row["m1FenceNs"])))
            else:
                following_record_qpc = int(q_authority["recordBeginQpc"])
                assert previous_q_fence_qpc is not None and previous_q_fence_qpc <= following_record_qpc
                before, after = region(previous_q_fence_qpc), region(following_record_qpc)
                empty_intervals.append(dict(line=line, lo=previous_q_fence_qpc, hi=following_record_qpc,
                                            region=before if before == after else "crosses_"+before+"_"+after))
        if (row.get("m1QuantumNonce") and q_authority.get("acceptedRoot") == row["m1QuantumNonce"] and
                row.get("m1QuantumFenceVkResult") == 0):
            fence_qpc = int(row["m1QuantumFenceNs"]) // 100
            assert previous_q_fence_qpc is None or fence_qpc >= previous_q_fence_qpc
            previous_q_fence_qpc = fence_qpc
    first = next((line, row) for line, row in enumerate(rows, 1)
                 if row.get("m1AdmittedQ", 0) > 0 and row.get("m1Hold") == "1")
    first_line, first_row = first
    assert int(first_row["m1ProducerWords"][8]) == 1 and "m1SourceBoundary" not in first_row
    before = [b for b in nonempty if b["hold"] == 1 and b["preTakeNs"] < deadline_ns]
    next_hold = next(b for b in nonempty if b["hold"] != 1)
    latest = max(before, key=lambda b: b["offer"])
    # The associated SOURCE command starts recording strictly after TakeLatest.
    # Thus the offer was taken before four seconds even though the SOURCE fence was later.
    last_source_record_qpc = int(rows[latest["line"]-1]["m1SourceAuthority"]["recordBeginQpc"])
    assert last_source_record_qpc*100 < deadline_ns < latest["sourceFenceNs"]
    claim_count = 1 + len(before)
    source_count = 1 + sum(b["sourceFenceNs"] < deadline_ns for b in before)
    assert claim_count == 13 and source_count == 12 and (latest["offer"],next_hold["offer"]) == (252,261)
    bounds = [latest["offer"], next_hold["offer"]-1]
    return dict(sha256=hashlib.sha256(data).hexdigest(), qpcFrequency=10**7,
                pointerWindows=[dict(downQpc=d, upQpc=u) for d, u in zip(down, up)],
                loggedBoundaryCount=dict(counts),
                emptyBoundaryRegions=dict(Counter(b["region"] for b in empty_intervals)),
                emptyBoundaryIntervals=empty_intervals,
                firstPreviewAcceptedLine=first_line, firstFourSeconds=dict(
                    steadyBoundaryOffers=[b["offer"] for b in before], ingressTakes=claim_count,
                    sourceFences=source_count, generatedOfferIdCountBounds=bounds,
                    generatedIdsWithoutKnownFourSecondTakeBounds=[x-claim_count for x in bounds],
                    lastOfferPreTakeNsAfterDown=latest["preTakeNs"]-first_ns,
                    lastOfferSourceRecordQpcAfterDown=last_source_record_qpc-down[0],
                    lastOfferSourceFenceNsAfterDown=latest["sourceFenceNs"]-first_ns,
                    followingObservedOffer=next_hold))


if __name__ == "__main__":
    archive = Path(__file__).resolve().parents[1] / "t966/demo-r4.jsonl.gz"
    result = audit(gzip.decompress(archive.read_bytes()))
    print(json.dumps(result, indent=2))
