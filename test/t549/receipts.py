"""Hash original inputs/outputs; never execute GPU or interpret a physics verdict."""
import argparse,hashlib,json
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('directory');p.add_argument('receipt');a=p.parse_args()
root=Path(a.directory).resolve();dest=Path(a.receipt).resolve()
if dest.exists():raise RuntimeError('FRESH_RECEIPT_REQUIRED')
files=[]
for f in sorted(root.iterdir()):
 if f.is_file() and f.resolve()!=dest:
  h=hashlib.sha256()
  with f.open('rb') as stream:
   for block in iter(lambda:stream.read(65536),b''):h.update(block)
  files.append({'name':f.name,'bytes':f.stat().st_size,'sha256':h.hexdigest()})
dest.write_text(json.dumps({'revision':'T549_RAW_FILE_RECEIPT_V1','physicsVerdict':'TESTER_OWNED','files':files},indent=2)+'\n',encoding='utf8')
