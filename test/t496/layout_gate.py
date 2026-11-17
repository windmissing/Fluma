# T547 §1 nonempty + precise class layout gate.
# Extracts the class layout section (from "class <Ns>::<Name>\tsize(...):" to the
# section terminator) from both dumps and compares exactly; empty section = fail.
import re,sys
def section(path,name):
    txt=open(path,encoding='utf-8',errors='replace').read()
    m=re.search(r'class [^\n]*'+re.escape(name)+r'[^\n]*size\(\d+\):\n(.*?)(?:\n\n|\Z)',txt,re.S)
    if not m: return None
    body=m.group(1)
    return body if body.strip() else None
d=section(sys.argv[1],sys.argv[3]); i=section(sys.argv[2],sys.argv[3])
if d is None or i is None:
    print('LAYOUT_SECTION_EMPTY'); sys.exit(1)
if d!=i:
    import difflib
    for l in list(difflib.unified_diff(d.splitlines(),i.splitlines(),lineterm=''))[:20]:
        print(l[:160])
    sys.exit(1)
print('LAYOUT_SECTION_OK bytes=%d/%d'%(len(d),len(i)))
sys.exit(0)
