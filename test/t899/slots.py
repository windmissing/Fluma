"""Compare A/B shader build products without running Windows Vulkan."""

import json
from pathlib import Path
import re
import sys


def assets(directory):
    return {row["role"]: row for row in json.loads((directory / "pc_steady_assets.json").read_bytes())}


def compare(a, b):
    ledger = "OP17_PRIVATE_LEDGER"
    snapshot = ledger + "_SNAPSHOT"
    baseline = assets(a)
    candidate = assets(b)
    assert candidate.pop(snapshot)["moduleSHA"] != baseline[ledger]["moduleSHA"]
    assert candidate == baseline, "A/B changed an existing steady role"

    src = (b / f"PC_{snapshot}.comp").read_text(encoding="utf-8")
    match = re.search(r"\nvoid t899Snapshot\(\)\{.*?\n\}\n", src, re.S)
    assert match, "snapshot implementation missing"
    slot_body = match.group()
    dispatch = Path(__file__).resolve().parents[2] / "docs" / "architecture-T607-g13-dispatch-v1.json"
    plan = json.loads(dispatch.read_bytes())
    ordinals = [i for i, row in enumerate(plan["quantum"]) if row["opcode"] == 17]
    assert re.findall(r"case (\d+)u:slot=(\d+)u;", slot_body) == [
        (str(ordinal), str(slot)) for slot, ordinal in enumerate(ordinals)
    ]
    assert len(ordinals) == 14 and all(row["opcode"] != 17 for row in plan["source"])
    assert "uint dst=uPrivateWord+32u+slot*42u;" in slot_body
    assert "for(uint i=0u;i<32u;++i)w[dst+i]=w[uPrivateWord+i];" in slot_body
    assert "for(uint i=0u;i<10u;++i)w[dst+32u+i]=c[513u+i];" in slot_body
    unchanged = src[:match.start()] + src[match.end():]
    unchanged = unchanged.replace("if(!t854Previous(2u)){t899Snapshot();return;}",
                                  "if(!t854Previous(2u))return;")
    unchanged = unchanged.replace("if(lane==0u){t854Stage(3u);memoryBarrierBuffer();t899Snapshot();}return;",
                                  "if(lane==0u)t854Stage(3u);return;")
    assert unchanged == (a / f"PC_{ledger}.comp").read_text(encoding="utf-8")
    print("T899 B: existing modules unchanged; 14 exact slot ordinals; both 42-word capture paths")


if __name__ == "__main__":
    compare(*(Path(arg) for arg in sys.argv[1:]))
