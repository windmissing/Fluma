"""Offline original owner/witness plus unchanged T413 strict HOLD/PNG/time/ROI verifier."""
from pathlib import Path
import argparse,json,sys
sys.path.insert(0,str(Path(__file__).parents[1]/'t413'))
from capture_hold import verify_bundle
from check_prepare import check
p=argparse.ArgumentParser();p.add_argument('prepare',type=Path);p.add_argument('witness',type=Path);p.add_argument('bundle',type=Path);a=p.parse_args()
print(json.dumps({'owner':check(json.loads(a.prepare.read_text(encoding='utf8')),json.loads(a.witness.read_text(encoding='utf8'))),'hold':verify_bundle(a.bundle),'deviceCommandsRun':False},indent=2))
