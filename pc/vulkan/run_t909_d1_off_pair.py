"""Collect exactly two natural OFF runs and compare original owner clocks."""

import argparse
from hashlib import sha256
import json
from pathlib import Path
import subprocess
import sys


def fingerprint(path):
    data=path.read_bytes()
    return dict(path=str(path),bytes=len(data),sha256=sha256(data).hexdigest())


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ("exe","source","candidate","commit","out"):
        parser.add_argument(name,type=Path)
    parser.add_argument("--plan",action="store_true",help="freeze the invocation without running GPU work")
    args=parser.parse_args()
    out=args.out.resolve()
    paths=[p.resolve(strict=True) for p in (args.exe,args.source,args.candidate,args.commit)]
    root=Path(__file__).resolve().parents[2]
    argv=[str(paths[0]),"--m1-window",*map(str,paths[1:]),"--m1-d1","OFF","--m1-source-trace"]
    receipt=dict(mode="NATURAL_OFF_PAIR",argv=argv,cwd=str(root),inputs=list(map(fingerprint,paths)),attempts=[])
    if args.plan:
        print(json.dumps(receipt,indent=2))
        return
    out.mkdir(parents=True)
    logs=[]
    for i in (1,2):
        log=out/f"off-{i}.jsonl"
        error=out/f"off-{i}.stderr"
        with log.open("xb") as stdout,error.open("xb") as stderr:
            with subprocess.Popen(argv,cwd=root,stdout=stdout,stderr=stderr) as child:
                try:
                    exit_code=child.wait(timeout=120)
                    state="CHILD_EXIT"
                except subprocess.TimeoutExpired:
                    child.kill()
                    exit_code=child.wait()
                    state="WATCHDOG_TERMINATED"
                receipt["attempts"].append(dict(pid=child.pid,state=state,exitCode=exit_code))
        logs.append(log)
        receipt["attempts"][-1].update(stdout=fingerprint(log),stderr=fingerprint(error))
    result=subprocess.run([sys.executable,str(root/"test/t897/check.py"),"--off",*map(str,logs)],
                          cwd=root,capture_output=True)
    (out/"pair.json").write_bytes(result.stdout)
    (out/"pair.stderr").write_bytes(result.stderr)
    receipt["checkerExit"]=result.returncode
    (out/"receipt.json").write_text(json.dumps(receipt,indent=2)+"\n",encoding="utf8")
    raise SystemExit(result.returncode or int(any(row["state"]!="CHILD_EXIT" for row in receipt["attempts"])))


if __name__=="__main__":
    main()
