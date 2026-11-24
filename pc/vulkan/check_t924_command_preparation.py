"""Check the measured command preparation boundary of one SOURCE root."""

import argparse
import copy
import json
from pathlib import Path

CALLS=("vkResetCommandPool","vkResetFences","vkBeginCommandBuffer")


def require(condition,reason):
    if not condition:
        raise ValueError(reason)


def compare(rows):
    timeline,=[r for r in rows if r.get("stage")=="ROOT_TIMELINE"]
    receipt=timeline["commandPreparation"]
    grant,deadline=map(int,(timeline["grantNs"],timeline["deadlineNs"]))
    record,submit=map(int,(timeline["recordEnterNs"],timeline["submitEnterNs"]))
    require(deadline-grant==250_000_000 and grant<=record<=submit,"BUSINESS_WINDOW")
    stage=[receipt[k] for k in CALLS]
    edges=[int(r[k]) for r in stage for k in ("enterNs","exitNs")]
    require(0<edges[0] and edges==sorted(edges) and all(s["vkResult"]==0 for s in stage),"COMMAND_CALL_SEQUENCE")
    require(int(receipt["pool"])>0 and int(receipt["fence"])>0 and int(receipt["priorGeneration"])>0 and
            receipt["fence"]==timeline["fence"] and receipt["useGeneration"]==timeline["fenceGeneration"] and
            int(receipt["useGeneration"])==int(receipt["priorGeneration"])+1,"COMMAND_OBJECT_GENERATION")
    early=[r for r in rows if r.get("stage")=="SOURCE_COMMAND_PREPARED"]
    if receipt["outsideGrant"]:
        proof=receipt["retireProof"]
        require(len(early)==1 and early[0]["run"]==timeline["run"] and early[0]["vkResult"]==0 and
                early[0]["commandPreparation"]==receipt and proof["vkResult"]==0 and
                0<int(proof["enterNs"])<=int(proof["exitNs"])<=edges[0]<=edges[-1]<=grant,
                "RETIRED_PREPARATION_SCOPE")
    else:
        require(not early and record<=edges[0]<=edges[-1]<=submit and receipt["retireProof"]["vkResult"] is None,
                "IN_GRANT_PREPARATION_SCOPE")
    return dict(outsideGrant=receipt["outsideGrant"],grantNs=grant,prepareToSubmitNs=submit-edges[-1],
                durations=dict(zip(CALLS,(b-a for a,b in zip(edges[::2],edges[1::2])))))


def selftest():
    call=lambda a,b,status=0:dict(enterNs=str(a),exitNs=str(b),vkResult=status)
    receipt=dict(outsideGrant=False,pool="5",fence="7",priorGeneration="1",useGeneration="2",
                 retireProof=call(0,0,None),**dict(zip(CALLS,(call(120,131),call(132,140),call(141,152)))))
    root=dict(stage="ROOT_TIMELINE",run="1",grantNs="100",deadlineNs="250000100",recordEnterNs="110",
              submitEnterNs="170",fence="7",fenceGeneration="2",commandPreparation=receipt)
    require(compare([root])["durations"][CALLS[0]]==11,"INSIDE_SMOKE")
    early=copy.deepcopy(root)
    r=early["commandPreparation"]
    r.update(outsideGrant=True,retireProof=call(10,20),
             **dict(zip(CALLS,(call(21,31),call(32,39),call(40,52)))))
    pre=dict(stage="SOURCE_COMMAND_PREPARED",run="1",vkResult=0,commandPreparation=r)
    require(compare([pre,early])["outsideGrant"],"OUTSIDE_SMOKE")
    for key,value,reason in (("useGeneration","3","COMMAND_OBJECT_GENERATION"),
                             ("retireProof",call(22,23),"RETIRED_PREPARATION_SCOPE"),
                             ("vkResetFences",call(32,39,-4),"COMMAND_CALL_SEQUENCE")):
        clone=copy.deepcopy([pre,early])
        clone[-1]["commandPreparation"][key]=value
        try:
            compare(clone)
        except ValueError as exc:
            require(str(exc)==reason,"UNEXPECTED_NEGATIVE")
        else:
            raise AssertionError(reason)
    return dict(mode="SYNTHETIC",positives=2,negatives=3)


if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input",nargs="?",type=Path)
    args=parser.parse_args()
    print(json.dumps(compare([json.loads(line) for line in args.input.read_text(encoding="utf-8-sig").splitlines()])
                     if args.input else selftest(),indent=2))
