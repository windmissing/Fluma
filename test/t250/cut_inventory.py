"""Record original cut sites, without inventing an AFTER failure predicate."""
from pathlib import Path
import re,json
root=Path(__file__).resolve().parents[2];base=root/'entry/src/main/ets/physics'
names=['SOURCE_IDENTITY','SOURCE_CAPACITY_ETA','BEGIN_HOT','COLLECT_HEADER','COLLECT_AB','REQUEST_VALIDATE','SINK_RESERVE_GUARD','PATCH_PREPARE','FIND','PATCH_PUBLISH','AUDIT_SINK','TICKET_CLOSE_EVENTS','MOTION_APPLY','AUDIT_TAIL','AUDIT_FINISH','PREVIEW_RESTORE','AUTHORITATIVE','FIELD_AUDIT','SOURCE_PUBLISH','OBSERVE_SEAL']
sites={i:[] for i in range(1,21)}
for p in base.rglob('*.ets'):
    s=p.read_text(encoding='utf-8-sig')
    for m in re.finditer(r'(?:txTrace|trace)\??\.(?:enter|cut)\((\d+)\)',s):
        n=int(m[1])
        if n in sites:
            sites[n].append({'file':str(p.relative_to(root)).replace('\\','/'),'line':s.count('\n',0,m.start())+1,'context':s[max(0,m.start()-180):m.start()+100]})
rows=[]
for n,name in enumerate(names,1):
    rows.append({'cutId':n,'name':name,'originalSites':sites[n],
        'BEFORE':'IMPLEMENTED' if n in (1,6) else 'NOT_IMPLEMENTED',
        'AFTER':'NO_ORIGINAL_FAILURE_PREDICATE_DECLARED',
        'beforeGap':None if n in (1,6) else ('original check exists; isolated injector/rollback-readset mapping remains to implement' if sites[n] else 'no direct enter/cut match; requires binding-specific original checkpoint mapping'),
        'afterGap':'T256 names boundary but does not identify a mutable input plus next original rejecting consumer; cannot throw a synthetic business error after a successful check. Need per-cut cancellation/revalidation semantics for AFTER, or record it as observation only.'})
out=root/'test/reports/t250/cut-inventory.json';out.write_text(json.dumps({'revision':'T250_CUT_SITES_V1','cuts':rows,'implementedEdges':2,'unsupportedEdges':38},indent=2)+'\n');print('wrote original cut inventory')
