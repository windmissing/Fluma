"""Summarize ordered event work from a sealed M1 root archive."""

import argparse
from collections import Counter
import json
from pathlib import Path

from read_t909_observation import archive


def work(rows):
    endpoints = set()
    types = Counter()
    conservative_depth = {}
    longest = 0
    for row in rows:
        kind, origin, target = row[6], row[20], row[21]
        if not 0 <= origin < 128 or not 0 <= target < 128:
            raise ValueError("Invalid event column")
        types[kind] += 1
        endpoints.update((origin, target))
        # Include nearest column guards and same-slot updates. This is an
        # upper bound for the ordered local dependencies, not GPU duration.
        keys = {("column", col) for end in (origin, target)
                for col in range(max(0,end-1), min(128,end+2))}
        if kind not in (9, 10):
            keys.add(("slot", (row[52]>>18)&3, row[16]))
        depth = 1+max((conservative_depth.get(key, 0) for key in keys), default=0)
        for key in keys:
            conservative_depth[key] = depth
        longest = max(longest, depth)
    return dict(rows=len(rows), kinds=dict(types), endpointColumns=sorted(endpoints),
                localConflictChainUpperBound=longest)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("roots", nargs="+", type=Path)
    analyses = []
    for path in parser.parse_args().roots:
        root = archive(path)
        if root["rootKind"] == 2:
            analyses.append(dict(nonce=root["nonce"], commit=root["commit"],
                                 archiveSHA=root["payloadSHA"], **work(root["events"])))
    print(json.dumps(analyses, indent=2))
