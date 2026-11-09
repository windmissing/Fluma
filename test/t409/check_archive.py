"""Developer archive smoke check; T411 provides the independent verdict tool."""
import hashlib
import json
from pathlib import Path
import sys


def verify(path):
    data=path.read_bytes();m=json.loads(data);root=path.parent.resolve()
    assert m['revision']=='T409_EVIDENCE_ARCHIVE_V1' and m['state']=='COMPLETE','MANIFEST_INCOMPLETE'
    scope=('captureId','activeRevision','packageSha256','epoch','runLo','runHi','pageSession','startNs','endNs')
    pages=[];size=len(data)
    for f in m['files']:
        p=(root/f['name']).resolve()
        assert p.parent==root and p.is_file(),'MISSING_FRAGMENT'
        b=p.read_bytes();size+=len(b)
        assert len(b)==f['bytes'] and hashlib.sha256(b).hexdigest()==f['sha256'],'FRAGMENT_SHA_OR_BYTES'
        pages.append(json.loads(b))
    assert size==m['totalBytes']<=m['maxBytes']<=1048576,'TOTAL_BUDGET'
    assert len(pages)==m['pageCount'],'PAGE_COUNT'
    rows=[];tokens=[]
    for i,p in enumerate(pages):
        assert p['revision']=='T409_EVIDENCE_CAPTURE_PAGE_V1' and not p['incomplete'] and p['state']=='FROZEN','PAGE_INCOMPLETE'
        assert all(p[k]==m[k] for k in scope),'IDENTITY_MIX'
        assert p['cursor']==i and p['next']==i+1 and p['complete']==(i==len(pages)-1),'CURSOR'
        assert p['pageCount']==len(pages) and p['rowCount']==m['rowCount'] and p['tokenCount']==m['tokenCount'],'COUNTS'
        if i==0:assert p['type']=='SUMMARY' and p['summary']==m['summary'],'SUMMARY'
        elif i<=((m['rowCount']+3)//4):
            assert p['type']=='EVENTS' and 1<=len(p['rows'])<=4,'ROW_PAGE'
            rows+=p['rows']
        else:
            assert p['type']=='TOKEN','TOKEN_PAGE'
            tokens.append(p['token'])
    assert len(rows)==m['rowCount'] and len(tokens)==m['tokenCount']<=8,'MISSING_ROWS'
    for i,r in enumerate(rows):
        assert r['serial']==str(i+1) and r['captureId']==m['captureId'],'SERIAL'
        l=r['ledger']
        assert int(l['generatedQuanta'])==sum(int(l[k]) for k in ('acceptedQuantumCount','pendingQuanta','activePlanRemaining','overloadDroppedQuanta','revokedDroppedQuanta')),'QUOTA_BALANCE'
    required=['BEGIN','END']
    for flag,kind in [('readySeen','READY'),('sourceAttemptSeen','SOURCE_ATTEMPT'),('sourceOfferSeen','FIRST_OFFER'),('acceptedSeen','FIRST_ACCEPT'),('faultSeen','FIRST_FAILURE'),('revokeSeen','REVOKE')]:
        if m['summary'][flag]:required.append(kind)
    assert required==m['requiredEvents'] and all(k in [r['kind'] for r in rows] for k in required),'REQUIRED_EVENTS'
    assert rows[0]['kind']=='BEGIN' and rows[-1]['kind']=='END' and rows[-1]['ledger']==m['summary']['terminalLedger'],'TERMINAL'
    assert m['summary']['dropped']=='0','DROPPED'
    assert len(set(t['root'] for t in tokens))==len(tokens),'TOKEN_DUPLICATE'
    for t in tokens:
        assert t['rootToken'].startswith('GRS_ACCEPTED_ROOT_Q20_M1_V1|'),'ROOT_TOKEN'
        if t['presentationEvidence']=='NOT_ATTEMPTED_NO_SCENE_TOKEN':assert not t['requestedToken'] and not t['current'] and not t['swapSucceeded'],'FALSE_PRESENT'
        if t['current']:assert t['swapSucceeded'] and t['requestedToken']==t['renderedToken'] and t['requestedToken'],'PRESENT_TOKEN'
    return {'state':'DEVELOPER_ARCHIVE_CHECK_PASS','rows':len(rows),'tokens':len(tokens),'bytes':size,'device':'NOT_RUN'}


if __name__=='__main__':
    try:print(json.dumps(verify(Path(sys.argv[1]))))
    except (AssertionError,KeyError,ValueError,OSError) as e:
        print(json.dumps({'state':'NOT_JUDGABLE','reason':str(e)}));sys.exit(1)
