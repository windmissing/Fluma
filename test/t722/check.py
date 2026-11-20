"""Independent finite G11 FLOW and presentation receipt expectations."""
from fractions import Fraction as F
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]

def locus(column,rank,mass):
    return column+F(2*rank+1,2*mass)

def expected_after(mass,edges):
    # Enumerate origins in each receiver. Positive face mass travels right.
    if len(edges)+1!=len(mass):
        raise ValueError('EDGE_COUNT')
    recv=[[] for _ in mass]
    for c,m in enumerate(mass):
        left=max(0,-edges[c-1]) if c else 0
        right=max(0,edges[c]) if c<len(edges) else 0
        if m<left+right:
            raise ValueError('UNSEALED_DONOR')
        for rank in range(m):
            dest=c-1 if rank<left else c+1 if rank>=m-right else c
            recv[dest].append((c,rank))
    return {key:locus(dest,rank,len(column)) for dest,column in enumerate(recv)
            for rank,key in enumerate(column)}

def presentation(physics,material,scene,current,swap):
    scope=('token','run','context','generation')
    if not swap or any(tuple(part[k] for k in scope)!=tuple(current[k] for k in scope)
                       for part in (physics,material,scene)):
        return 'RETAIN_STALE'
    return 'PUBLISH'

def main():
    forward=expected_after([8]*3,[2,2])
    reverse=expected_after([8]*3,[-2,-2])
    still=expected_after([8]*3,[0,0])
    assert len(forward)==len(reverse)==len(still)==24
    assert forward[1,0]-still[1,0]==F(1,4)
    assert reverse[1,7]-still[1,7]==-F(1,4)
    assert all(reverse[2-c,7-r]==3-x for (c,r),x in forward.items())
    assert all(x==locus(c,r,8) for (c,r),x in still.items())
    try:
        expected_after([2,2,2],[-2,2])
    except ValueError as error:
        assert str(error)=='UNSEALED_DONOR'
    else:
        raise AssertionError('OVERSPENT')
    frame=dict(token='accepted:9',run=2,context=3,generation=4)
    results={'matched':presentation(frame,frame,frame,frame,True)}
    for field in frame:
        wrong=dict(frame,**{field:'wrong'})
        results['material-'+field]=presentation(frame,wrong,frame,frame,True)
        results['scene-'+field]=presentation(frame,frame,wrong,frame,True)
    results['failed-swap']=presentation(frame,frame,frame,frame,False)
    assert list(results.values())==['PUBLISH']+['RETAIN_STALE']*9
    recipe=json.loads((ROOT/'docs/architecture-T660-visual-recipe-v2.json').read_bytes())
    assert 'sealed final per-origin accepted shares' in recipe['physicalSource']
    artifact=json.loads((ROOT/'test/reports/t722/artifact.json').read_bytes())
    assert len(artifact['checks']['shaders'])==8
    product=ROOT/'.codex/local-test/t639-delivery-v2'
    import hashlib
    static={}
    for name in ('g11vis_r1.comp','g11vis_dm.comp'):
        raw=Path(artifact['checks']['shaders'][name]['argv'][-1]).read_bytes()
        assert hashlib.sha256(raw).hexdigest()==artifact['checks']['shaders'][name]['sha256']
        move=raw.split(b'bool g11MoveFlow(',1)[1].split(b'uint g11LaunchFor(',1)[0]
        assert all(key in move for key in (b'g11SideShare(0u,c,key)',b'g11SideShare(1u,c,key)',
                                         b'b>mass-a',b'rank<a',b'rank>=mass-b',b'g11FindMap(4u,dst,key)'))
        static[name]=hashlib.sha256(move).hexdigest()
    scene=product/'entry/src/main/cpp/gpu_resident/g11vis/scene.cpp'
    raw=scene.read_bytes()
    assert all(token in raw for token in (b'owner.AcquireScene()',b'owner.CanPresent(*lease) && swap && swap(user)',
                                          b'owner.Presented(*lease)',b'owner.ReleaseScene(*lease, fence)'))
    print(json.dumps({'fixtureOnly':True,'device':'NOT_RUN','gpuPixels':'NOT_RUN',
                      'acceptedFaceMassScenarios':{'forward':['1/4','2/8'],
                          'reverse':['-1/4','-2/8'],'zeroWithMomentumChange':'stationary',
                          'twoSidedOverspend':'REJECT','mirror':'EXACT'},
                      'positions':{key:{f'{c},{r}':str(x) for (c,r),x in data.items()}
                                   for key,data in [('forward',forward),('reverse',reverse),('zero',still)]},
                      'presentationCases':results,'physicalSource':recipe['physicalSource'],
                      'artifactBuildId':artifact['checks']['BuildId'],'staticFlowFunctions':static,
                      'staticSceneSHA256':hashlib.sha256(raw).hexdigest()},indent=2))

if __name__=='__main__':
    main()
