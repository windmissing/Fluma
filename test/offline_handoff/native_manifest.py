"""Bound executable + raw build/source/object closure for offline handoff."""
from pathlib import Path
import json,hashlib,subprocess,sys
sys.path.insert(0,str(Path(__file__).parent))
from package import file,build_id,ROOT,READELF
p=Path(sys.argv[1]).resolve();dest=Path(sys.argv[2]);out={'schema':'OFFLINE_NATIVE_EXECUTABLE_V1','device':'NOT_RUN','productPASS':False,'executable':file(p),'buildId':build_id(p.read_bytes())}
r=subprocess.run([str(READELF),'-d',str(p)],capture_output=True,check=True);out['dynamic']=r.stdout.decode('utf8');out['inspectionArgv']=[str(READELF),'-d',str(p)];out['inspectionTool']=file(READELF)
if len(sys.argv)>3:
 project=Path(sys.argv[3]).resolve();out['closure']={str(x.relative_to(project)):file(x) for x in project.rglob('*') if x.is_file() and not any(v in x.parts for v in ['oh_modules','node_modules','.hvigor']) and (x.suffix in ('.h','.cpp','.inc','.o','.rsp','.ninja','.comp','.glsl') or x.name=='compile_commands.json')}
out['sourceBinding']='exact actual snapshot/objects, not a claim about runtime-loaded maps or product PASS';dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(json.dumps(out,indent=2)+'\n')
