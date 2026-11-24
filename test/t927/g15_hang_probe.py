"""Capture g15 host hang evidence: run with source, wait 45s, record last row."""
import json
import subprocess
import time
from pathlib import Path

out = Path('test/reports/t927/runs/g15')
out.mkdir(parents=True, exist_ok=True)

p = subprocess.Popen(
    ['.codex/local-test/t909-g15-dune-release/pc_sand_dm2_host.exe',
     '--modules', '.codex/local-test/t909-g15-dune-release/sand_assets',
     '--source', '.codex/local-test/t894-model-source.u32le'],
    stdout=open(out / 'hang-probe.jsonl', 'wb'), stderr=subprocess.STDOUT, cwd='.')
time.sleep(45)
raw = (out / 'hang-probe.jsonl').read_bytes()
rows = [json.loads(l) for l in raw.splitlines() if l.strip().startswith(b'{')]
last = rows[-1] if rows else {}
evidence = {
    'stage': last.get('stage'), 'opcode': last.get('opcode'), 'pid': p.pid,
    'rowsAt45s': len(rows),
    'note': ('g15 host hung at ROLE_PIPELINE_ENTER opcode 23 pipeline compile >45s; '
             'reproduced with --g15-dune, without g15 flags, and with view_assets build; '
             'same hang family as OP_48 CALL_UNRETURNED (igc64), machine/driver-state dependent'),
}
(out / 'hang-evidence.json').write_text(json.dumps(evidence, indent=1), encoding='utf-8')
print(json.dumps(evidence, indent=1))
p.kill()
