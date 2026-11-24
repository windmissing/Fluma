"""Run hello_world_pc for T912 g13 arms with live stdout drain (Start-Process pipes fill)."""
import json
import subprocess
import sys
import time

EXE = sys.argv[1]
OUT = sys.argv[2]
ARGS = sys.argv[3:]

with open(OUT, 'wb') as log:
    proc = subprocess.Popen([EXE, *ARGS], stdout=log, stderr=subprocess.STDOUT)
    rc = proc.wait()
print(json.dumps({'exit': rc, 'out': OUT, 'argv': [EXE, *ARGS]}))
