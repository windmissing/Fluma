"""Reproduce T677 historical-log contamination using an independent build."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

build = Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='T677 receipt replay ') as directory:
    output = Path(directory)
    argv = [sys.executable, 'tools/build/build_receipt.py', str(build), str(output)]
    run = subprocess.run(argv, capture_output=True, text=True, check=True)
    historical = Path('.codex/local-test')
    comparisons = {}
    for name, old in [('native-build.log','t666-delivery.log'), ('incremental.log','t666-final-incremental.log')]:
        comparisons[name] = {'receiptSHA':digest(output/name),'historicalSHA':digest(historical/old)}
    print(json.dumps({'argv':argv,'exit':run.returncode,'comparisons':comparisons,
                      'result':'HISTORICAL_LOG_CONTAMINATION' if all(
                        v['receiptSHA']==v['historicalSHA'] for v in comparisons.values()) else 'RECHECK_REQUIRED',
                      'productPASS':False,'device':'NOT_RUN'},indent=2))
