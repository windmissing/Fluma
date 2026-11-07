"""Run the tester's lossless screenshot tool and record the entire call."""
import argparse
import hashlib
import json
import subprocess
import time
from pathlib import Path

cli = argparse.ArgumentParser(description=__doc__)
cli.add_argument("png", type=Path)
cli.add_argument("receipt", type=Path)
cli.add_argument("command", nargs=argparse.REMAINDER)
args = cli.parse_args()
if not args.command:
    cli.error("provide screenshot command after output paths")
start = time.monotonic_ns()
result = subprocess.run(args.command, capture_output=True)
end = time.monotonic_ns()
record = {"revision": "T406_CAPTURE_CALL_V1", "tool": args.command[0], "command": args.command,
          "startMonoNs": start, "endMonoNs": end, "clock": "PYTHON_MONOTONIC_NS",
          "exitCode": result.returncode, "stdout": result.stdout.decode(errors="replace"),
          "stderr": result.stderr.decode(errors="replace"),
          "fileSha256": hashlib.sha256(args.png.read_bytes()).hexdigest() if args.png.is_file() else ""}
args.receipt.write_text(json.dumps(record, indent=2), encoding="utf-8")
print(json.dumps(record))
